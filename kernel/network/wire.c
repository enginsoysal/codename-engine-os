#include <engine/network/wire.h>

uint16_t net7_checksum(const void *buffer,unsigned bytes,uint32_t sum) {
    const uint8_t *p=buffer;
    while(bytes>=2){sum+=net7_get16(p);p+=2;bytes-=2;}
    if(bytes)sum+=(unsigned)*p<<8;
    while(sum>>16)sum=(sum&65535)+(sum>>16);
    return (uint16_t)~sum;
}
uint32_t net7_pseudo(uint32_t source,uint32_t destination,unsigned protocol,unsigned bytes) {
    return (source>>16)+(source&65535)+(destination>>16)+(destination&65535)+protocol+bytes;
}
static unsigned same(const uint8_t *a,const uint8_t *b,unsigned bytes) {
    for(unsigned i=0;i<bytes;++i)if(a[i]!=b[i])return 0;return 1;
}
static unsigned broadcast(const uint8_t *mac) {
    for(unsigned i=0;i<6;++i)if(mac[i]!=255)return 0;return 1;
}
unsigned net7_read_ipv4(const void *frame,unsigned bytes,const uint8_t mac[6],net7_ipv4 *out) {
    if(!frame||!mac||!out||bytes<34||bytes>1536)return NET7_INVALID;
    const uint8_t *packet=frame,*ip=packet+14;
    if((!same(packet,mac,6)&&!broadcast(packet))||net7_get16(packet+12)!=0x0800||(ip[0]>>4)!=4)return NET7_INVALID;
    unsigned header=(ip[0]&15u)*4,total=net7_get16(ip+2);
    if(header<20||header>60||header>bytes-14||total<header||total>bytes-14||
       !ip[8]||(net7_get16(ip+6)&0xbfff)||net7_checksum(ip,header,0))return NET7_INVALID;
    *out=(net7_ipv4){net7_get32(ip+12),net7_get32(ip+16),ip+header,total-header,ip[9]};
    return NET7_OK;
}
unsigned net7_read_udp(const net7_ipv4 *ip,net7_udp *out) {
    if(!ip||!out||ip->protocol!=17||ip->bytes<8||ip->bytes>1500||!ip->data)return NET7_INVALID;
    const uint8_t *p=ip->data;unsigned bytes=net7_get16(p+4);
    if(bytes<8||bytes>ip->bytes)return NET7_INVALID;
    if(net7_get16(p+6)&&net7_checksum(p,bytes,net7_pseudo(ip->source,ip->destination,17,bytes)))return NET7_INVALID;
    *out=(net7_udp){net7_get16(p),net7_get16(p+2),p+8,bytes-8};return NET7_OK;
}
unsigned net7_read_tcp(const net7_ipv4 *ip,net7_tcp *out) {
    if(!ip||!out||ip->protocol!=6||ip->bytes<20||ip->bytes>1500||!ip->data)return NET7_INVALID;
    const uint8_t *p=ip->data;unsigned header=(p[12]>>4)*4;
    if(header<20||header>ip->bytes||net7_checksum(p,ip->bytes,net7_pseudo(ip->source,ip->destination,6,ip->bytes)))return NET7_INVALID;
    *out=(net7_tcp){net7_get16(p),net7_get16(p+2),net7_get16(p+14),net7_get32(p+4),net7_get32(p+8),p+header,ip->bytes-header,p[13]};
    return NET7_OK;
}

unsigned net7_read_dhcp(const void *buffer,unsigned bytes,uint32_t transaction,const uint8_t mac[6],net7_lease *out) {
    if(!buffer||!mac||!out||bytes<240||bytes>1500)return NET7_INVALID;
    const uint8_t *p=buffer;
    if(p[0]!=2||p[1]!=1||p[2]!=6||net7_get32(p+4)!=transaction||!same(p+28,mac,6)||net7_get32(p+236)!=0x63825363)return NET7_INVALID;
    net7_lease lease={0};lease.address=net7_get32(p+16);
    unsigned seen=0,ended=0;
    for(unsigned pos=240;pos<bytes;){
        unsigned option=p[pos++];if(!option)continue;if(option==255){ended=1;break;}
        if(pos==bytes)return NET7_INVALID;
        unsigned size=p[pos++];if(size>bytes-pos)return NET7_INVALID;
        unsigned flag=option==53?1:option==54?2:option==1?4:option==3?8:option==6?16:option==51?32:0;
        if(flag&&(seen&flag))return NET7_INVALID;seen|=flag;
        if(option==53){if(size!=1)return NET7_INVALID;lease.type=p[pos];}
        if(option==54){if(size!=4)return NET7_INVALID;lease.server=net7_get32(p+pos);}
        if(option==1){if(size!=4)return NET7_INVALID;lease.mask=net7_get32(p+pos);}
        if(option==3||option==6){if(!size||size%4)return NET7_INVALID;if(option==3)lease.router=net7_get32(p+pos);else lease.dns=net7_get32(p+pos);}
        if(option==51){if(size!=4)return NET7_INVALID;lease.seconds=net7_get32(p+pos);}
        pos+=size;
    }
    if(!ended||!(seen&1)||!(seen&2)||!lease.server||(lease.type!=2&&lease.type!=5&&lease.type!=6))return NET7_INVALID;
    if(lease.type!=6){
        uint32_t inverse=~lease.mask;
        if(!lease.address||lease.address==UINT32_MAX||!lease.mask||(inverse&(inverse+1))||
           !lease.router||!lease.dns||!lease.seconds)return NET7_INVALID;
    }
    *out=lease;return NET7_OK;
}

static char lower(char value) { return value>='A'&&value<='Z'?(char)(value-'A'+'a'):value; }
static unsigned letter(char value) {
    value=lower(value);return (value>='a'&&value<='z')||(value>='0'&&value<='9')||value=='-';
}
static unsigned name_equal(const char *a,const char *b) {
    unsigned n=0;for(;n<254&&a[n]&&lower(a[n])==lower(b[n]);++n){}
    return n<254&&!a[n]&&!b[n];
}
static unsigned dns_name(const uint8_t *message,unsigned bytes,unsigned start,char out[254],unsigned *next) {
    unsigned pos=start,written=0,jumped=0;
    for(unsigned hops=0;hops<128;++hops){
        if(pos>=bytes)return NET7_INVALID;
        unsigned count=message[pos++];
        if(!count){out[written]=0;if(!jumped)*next=pos;return NET7_OK;}
        if((count&0xc0)==0xc0){
            if(pos>=bytes)return NET7_INVALID;
            unsigned pointer=((count&63)<<8)|message[pos++];
            if(pointer>=pos-2)return NET7_INVALID; /* Compression refers to an earlier occurrence. */
            if(!jumped)*next=pos;jumped=1;pos=pointer;continue;
        }
        if(count>63||count>bytes-pos||written+count+(written?1:0)>253)return NET7_INVALID;
        if(written)out[written++]='.';
        for(unsigned i=0;i<count;++i){char value=(char)message[pos++];if(!letter(value))return NET7_INVALID;out[written++]=lower(value);}
    }
    return NET7_INVALID;
}
unsigned net7_dns_answer(const void *buffer,unsigned bytes,uint16_t transaction,const char *wanted,uint32_t *out) {
    if(!buffer||!wanted||!out||bytes<12||bytes>4096)return NET7_INVALID;
    const uint8_t *p=buffer;unsigned flags=net7_get16(p+2),answers=net7_get16(p+6);
    if(net7_get16(p)!=transaction||(flags&0xf800)!=0x8000||(flags&0x0240)||net7_get16(p+4)!=1||answers>64)return NET7_INVALID;
    char query[254],current[254];unsigned begin=0;
    if(dns_name(p,bytes,12,query,&begin)||!name_equal(query,wanted)||begin>bytes||bytes-begin<4||
       net7_get16(p+begin)!=1||net7_get16(p+begin+2)!=1)return NET7_INVALID;
    begin+=4;
    if((flags&15)==3)return NET7_NOT_FOUND;
    if(flags&15)return NET7_SERVER_ERROR;
    for(unsigned i=0;i<254;++i){current[i]=query[i];if(!query[i])break;}
    for(unsigned chain=0;chain<8;++chain){
        unsigned pos=begin,changed=0;uint32_t address=0;
        char next_name[254]={0};
        for(unsigned record=0;record<answers;++record){
            char owner[254];unsigned after;
            if(dns_name(p,bytes,pos,owner,&after)||after>bytes||bytes-after<10)return NET7_INVALID;
            unsigned type=net7_get16(p+after),class=net7_get16(p+after+2),size=net7_get16(p+after+8);
            pos=after+10;if(size>bytes-pos)return NET7_INVALID;
            if(class==1&&name_equal(owner,current)){
                if(type==1){if(size!=4)return NET7_INVALID;address=net7_get32(p+pos);}
                if(type==5){unsigned end;if(dns_name(p,bytes,pos,next_name,&end)||end!=pos+size||!next_name[0])return NET7_INVALID;changed=1;}
            }
            pos+=size;
        }
        if(address&&address!=UINT32_MAX){*out=address;return NET7_OK;}
        if(!changed)return NET7_NOT_FOUND;
        for(unsigned i=0;i<254;++i){current[i]=next_name[i];if(!next_name[i])break;}
    }
    return NET7_INVALID;
}

unsigned net7_parse_url(const char *input,net7_url *out) {
    if(!input||!out)return NET7_INVALID;
    unsigned size=0;while(size<768&&input[size])++size;
    if(size<9||size>=768)return NET7_INVALID;
    const char prefix[]="https://";for(unsigned i=0;i<8;++i)if(lower(input[i])!=prefix[i])return NET7_INVALID;
    net7_url url={0};unsigned pos=8,host=0,label=0;url.port=443;
    while(pos<size&&input[pos]!='/'&&input[pos]!='?'&&input[pos]!=':'){
        char value=lower(input[pos++]);
        if(value=='.'){if(!label||url.host[host-1]=='-')return NET7_INVALID;label=0;}
        else {if(!letter(value)||(!label&&value=='-')||++label>63)return NET7_INVALID;}
        if(host==253)return NET7_INVALID;url.host[host++]=value;
    }
    if(!host||!label||url.host[host-1]=='-')return NET7_INVALID;
    if(pos<size&&input[pos]==':'){
        ++pos;unsigned number=0,digits=0;
        while(pos<size&&input[pos]>='0'&&input[pos]<='9'){
            if(++digits>5)return NET7_INVALID;number=number*10+(unsigned)(input[pos++]-'0');
        }
        if(!digits||!number||number>65535||(pos<size&&input[pos]!='/'&&input[pos]!='?'))return NET7_INVALID;
        url.port=(uint16_t)number;
    }
    unsigned path=0;if(pos==size||input[pos]=='?')url.path[path++]='/';
    while(pos<size){unsigned char value=(unsigned char)input[pos++];if(value<33||value>126||value=='#'||path==255)return NET7_INVALID;url.path[path++]=(char)value;}
    url.path[path]=0;*out=url;return NET7_OK;
}
