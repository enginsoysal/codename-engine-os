#include <engine/network/network.h>
#include <engine/network/tcp.h>
#include <engine/network/tls.h>
#include <engine/network/download.h>
#include <engine/m3.h>
typedef struct {uint32_t ip;uint8_t mac[6];uint64_t expires;} arp_entry;
static arp_entry arp[4];
static net7_configuration configuration;
static net7_lease offer;
static uint8_t mac[6],transmit[1536];
static unsigned clock_active,old_timer_mask,dhcp_state,retries;
static unsigned work;
static uint32_t transaction;
static uint64_t deadline,lease_expires,arp_deadline;
static uint32_t arp_target;
static unsigned dns_state,dns_retries,dns_sent;
static uint16_t dns_id,dns_port,packet_id;
static uint32_t dns_address;
static uint64_t dns_deadline;
static char dns_host[254];

static void state(unsigned value,const char *message) {
    configuration.state=value;configuration.message=message;++configuration.revision;
}
static void timer(unsigned active) {
    uint64_t flags;__asm__ volatile("pushfq;popq %0;cli":"=r"(flags)::"memory");
    if(active&&!clock_active){old_timer_mask=in8(0x21)&1;clock_active=1;out8(0x21,in8(0x21)&~1u);}
    else if(!active&&clock_active){clock_active=0;out8(0x21,(uint8_t)((in8(0x21)&~1u)|old_timer_mask));}
    if(flags&512)__asm__ volatile("sti":::"memory");
}
unsigned net7_clock_event(void) {
    if(!clock_active)return 0;__atomic_store_n(&work,1,__ATOMIC_RELEASE);return 1;
}
unsigned net7_work_pending(void) {
    return __atomic_load_n(&work,__ATOMIC_ACQUIRE)||net7_nic_pending();
}
void net7_schedule(void){timer(1);__atomic_store_n(&work,1,__ATOMIC_RELEASE);}
unsigned net7_random(void *output,unsigned bytes) {
    uint32_t a,b,c,d;__asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(1u),"c"(0));
    if(!(c&(1u<<30)))return 0;
    uint8_t *out=output;
    while(bytes){uint64_t value=0;unsigned char valid=0;
        for(unsigned attempt=0;attempt<16&&!valid;++attempt)__asm__ volatile("rdrand %0;setc %1":"=r"(value),"=qm"(valid));
        if(!valid)return 0;unsigned count=bytes>8?8:bytes;memcpy(out,&value,count);out+=count;bytes-=count;
    }
    return 1;
}
static void ethernet(uint8_t *packet,const uint8_t *destination,uint16_t type) {
    memcpy(packet,destination,6);memcpy(packet+6,mac,6);net7_put16(packet+12,type);
}
static void ip_header(uint8_t *packet,uint32_t source,uint32_t destination,unsigned protocol,unsigned bytes) {
    uint8_t *ip=packet+14;memset(ip,0,20);ip[0]=0x45;net7_put16(ip+2,(uint16_t)(20+bytes));
    net7_put16(ip+4,++packet_id);net7_put16(ip+6,0x4000);ip[8]=64;ip[9]=(uint8_t)protocol;
    net7_put32(ip+12,source);net7_put32(ip+16,destination);net7_put16(ip+10,net7_checksum(ip,20,0));
}
static unsigned send_udp(uint32_t source,uint32_t destination,uint16_t from,uint16_t to,const uint8_t *destination_mac,const void *data,unsigned bytes) {
    if(bytes>1472)return 3;
    ethernet(transmit,destination_mac,0x0800);ip_header(transmit,source,destination,17,bytes+8);
    uint8_t *udp=transmit+34;net7_put16(udp,from);net7_put16(udp+2,to);net7_put16(udp+4,(uint16_t)(bytes+8));net7_put16(udp+6,0);
    memcpy(udp+8,data,bytes);uint16_t sum=net7_checksum(udp,bytes+8,net7_pseudo(source,destination,17,bytes+8));
    net7_put16(udp+6,sum?sum:65535);return net7_nic_send(transmit,42+bytes);
}
static void send_arp(uint32_t target,const uint8_t *destination,unsigned reply,uint32_t target_ip) {
    uint8_t packet[42]={0};ethernet(packet,destination,0x0806);
    net7_put16(packet+14,1);net7_put16(packet+16,0x0800);packet[18]=6;packet[19]=4;net7_put16(packet+20,reply?2:1);
    memcpy(packet+22,mac,6);net7_put32(packet+28,configuration.address);
    if(reply)memcpy(packet+32,destination,6);net7_put32(packet+38,reply?target_ip:target);
    (void)net7_nic_send(packet,sizeof(packet));
}
static void request_arp(uint32_t target) {
    uint64_t now=m3_clock_ticks();if(target==arp_target&&now<arp_deadline)return;
    const uint8_t broadcast[6]={255,255,255,255,255,255};arp_target=target;arp_deadline=now+100;
    send_arp(target,broadcast,0,0);
}
static unsigned resolve_mac(uint32_t destination,uint8_t result[6]) {
    uint32_t target=(destination&configuration.mask)==(configuration.address&configuration.mask)?destination:configuration.router;
    uint64_t now=m3_clock_ticks();
    for(unsigned i=0;i<4;++i)if(arp[i].ip==target&&arp[i].expires>now){memcpy(result,arp[i].mac,6);return 1;}
    request_arp(target);return 0;
}
unsigned net7_send_ipv4(uint32_t destination,unsigned protocol,const void *data,unsigned bytes){
    if(configuration.state!=NET7_CONNECTED||!data||bytes>1480)return 3;
    uint8_t destination_mac[6];if(!resolve_mac(destination,destination_mac))return 1;
    ethernet(transmit,destination_mac,0x0800);ip_header(transmit,configuration.address,destination,protocol,bytes);
    memcpy(transmit+34,data,bytes);return net7_nic_send(transmit,34+bytes);
}
static void receive_arp(const uint8_t *packet,unsigned bytes) {
    if(bytes<42||net7_get16(packet+14)!=1||net7_get16(packet+16)!=0x0800||packet[18]!=6||packet[19]!=4)return;
    unsigned type=net7_get16(packet+20);if(type!=1&&type!=2)return;
    uint32_t sender=net7_get32(packet+28),target=net7_get32(packet+38);
    if(packet[22]&1||memcmp(packet+6,packet+22,6))return;
    if(sender&&sender==configuration.address&&memcmp(packet+22,mac,6)){
        configuration.address=0;dhcp_state=0;dns_state=NET7_DNS_FAILED;state(NET7_FAILED,"Address conflict detected. Retry network setup.");return;
    }
    if(sender&&target==configuration.address&&(type==1||sender==arp_target)){
        unsigned slot=0;for(unsigned i=0;i<4;++i){if(arp[i].ip==sender){slot=i;break;}if(arp[i].expires<arp[slot].expires)slot=i;}
        arp[slot].ip=sender;memcpy(arp[slot].mac,packet+22,6);arp[slot].expires=m3_clock_ticks()+6000;
    }
    if(type==1&&configuration.state==NET7_CONNECTED&&target==configuration.address)send_arp(0,packet+22,1,sender);
}
static unsigned dhcp_send(unsigned type) {
    uint8_t data[320]={0},broadcast[6]={255,255,255,255,255,255};
    data[0]=1;data[1]=1;data[2]=6;net7_put32(data+4,transaction);net7_put16(data+10,0x8000);memcpy(data+28,mac,6);net7_put32(data+236,0x63825363);
    unsigned pos=240;data[pos++]=53;data[pos++]=1;data[pos++]=(uint8_t)type;
    data[pos++]=61;data[pos++]=7;data[pos++]=1;memcpy(data+pos,mac,6);pos+=6;
    if(type==3){data[pos++]=50;data[pos++]=4;net7_put32(data+pos,offer.address);pos+=4;data[pos++]=54;data[pos++]=4;net7_put32(data+pos,offer.server);pos+=4;}
    data[pos++]=55;data[pos++]=6;const uint8_t wanted[]={1,3,6,51,54,58};memcpy(data+pos,wanted,sizeof(wanted));pos+=sizeof(wanted);
    data[pos++]=12;data[pos++]=8;memcpy(data+pos,"EngineOS",8);pos+=8;
    data[pos++]=57;data[pos++]=2;net7_put16(data+pos,576);pos+=2;data[pos++]=255;
    if(pos<300)pos=300;
    return send_udp(0,UINT32_MAX,68,67,broadcast,data,pos);
}
void net7_retry(void) {
    net7_nic_info nic=net7_nic_status();dns_state=NET7_DNS_IDLE;configuration.address=0;
    memset(arp,0,sizeof(arp));arp_target=0;
    if(!nic.present){state(NET7_NO_INTERFACE,"No supported Ethernet interface was found.");dhcp_state=0;timer(0);return;}
    if(nic.status){state(NET7_FAILED,nic.status==7?"Ethernet interrupt routing is unsupported. Check the supported VM adapter guide.":"Ethernet initialization failed. Open System details for its status.");dhcp_state=0;timer(0);return;}
    if(!nic.online){state(NET7_DISCONNECTED,"Ethernet is disconnected. Check the virtual cable.");dhcp_state=0;timer(0);return;}
    if(!net7_random(&transaction,sizeof(transaction)))transaction=(uint32_t)m3_tsc()^0x374e4554u;
    dhcp_state=1;retries=0;deadline=m3_clock_ticks()+100;state(NET7_CONFIGURING,"Connecting automatically...");
    timer(1);(void)dhcp_send(1);__atomic_store_n(&work,1,__ATOMIC_RELEASE);
}
static void receive_dhcp(const net7_udp *udp) {
    net7_lease lease;
    if(!dhcp_state||net7_read_dhcp(udp->data,udp->bytes,transaction,mac,&lease))return;
    if(dhcp_state==1&&lease.type==2){offer=lease;dhcp_state=2;retries=0;deadline=m3_clock_ticks()+100;(void)dhcp_send(3);return;}
    if(dhcp_state!=2||lease.server!=offer.server)return;
    if(lease.type==6){net7_retry();return;}
    if(lease.type!=5||lease.address!=offer.address)return;
    configuration.address=lease.address;configuration.mask=lease.mask;configuration.router=lease.router;configuration.dns=lease.dns;
    lease_expires=m3_clock_ticks()+(uint64_t)lease.seconds*100;dhcp_state=0;
    state(NET7_CONNECTED,"Ethernet connected. Address and DNS were configured automatically.");
    serial_text("M7_DHCP_CONFIGURED address=");serial_hex(configuration.address);serial_text(" dns=");serial_hex(configuration.dns);serial_text("\n");
}
static unsigned dns_send(void) {
    uint8_t destination[6];if(!resolve_mac(configuration.dns,destination))return 1;
    uint8_t query[512]={0};net7_put16(query,dns_id);net7_put16(query+2,0x0100);net7_put16(query+4,1);
    unsigned offset=12,pos=0;
    while(dns_host[pos]){
        unsigned begin=pos;while(dns_host[pos]&&dns_host[pos]!='.')++pos;
        unsigned count=pos-begin;if(!count||count>63||offset+count+1>sizeof(query)-5)return 3;
        query[offset++]=(uint8_t)count;memcpy(query+offset,dns_host+begin,count);offset+=count;if(dns_host[pos])++pos;
    }
    query[offset++]=0;net7_put16(query+offset,1);net7_put16(query+offset+2,1);offset+=4;
    unsigned status=send_udp(configuration.address,configuration.dns,dns_port,53,destination,query,offset);if(!status)dns_sent=1;return status;
}
unsigned net7_resolve(const char *host) {
    if(configuration.state!=NET7_CONNECTED||m3_clock_ticks()>=lease_expires)return 1;
    if(!host)return 1;unsigned n=0;while(n<253&&host[n]){dns_host[n]=host[n];++n;}if(!n||host[n])return 1;dns_host[n]=0;
    uint32_t random;if(!net7_random(&random,sizeof(random))){dns_state=NET7_DNS_FAILED;return 1;}
    dns_id=(uint16_t)random;dns_port=(uint16_t)(49152+(random>>16)%16384);dns_state=NET7_DNS_PENDING;dns_retries=0;dns_sent=0;dns_address=0;dns_deadline=m3_clock_ticks()+100;
    timer(1);(void)dns_send();__atomic_store_n(&work,1,__ATOMIC_RELEASE);return 0;
}
unsigned net7_dns_status(uint32_t *address) {if(address&&dns_state==NET7_DNS_READY)*address=dns_address;return dns_state;}
static void receive(const void *frame,unsigned bytes) {
    const uint8_t *packet=frame;
    if(net7_get16(packet+12)==0x0806){receive_arp(packet,bytes);return;}
    net7_ipv4 ip;if(net7_read_ipv4(frame,bytes,mac,&ip))return;
    if(ip.protocol==6){tcp7_receive(&ip);return;}
    net7_udp udp;if(net7_read_udp(&ip,&udp))return;
    if(udp.source==67&&udp.destination==68){receive_dhcp(&udp);return;}
    if(ip.destination!=configuration.address&&ip.destination!=UINT32_MAX)return;
    if(dns_state==NET7_DNS_PENDING&&ip.source==configuration.dns&&udp.source==53&&udp.destination==dns_port){
        uint32_t address=0;unsigned status=net7_dns_answer(udp.data,udp.bytes,dns_id,dns_host,&address);
        if(status==NET7_OK){dns_address=address;dns_state=NET7_DNS_READY;serial_text("M7_DNS_RESOLVED address=");serial_hex(address);serial_text("\n");++configuration.revision;}
        else if(status==NET7_NOT_FOUND||status==NET7_SERVER_ERROR){dns_state=NET7_DNS_FAILED;++configuration.revision;}
    }
}
void net7_init(void) {
    (void)net7_nic_init();net7_nic_info nic=net7_nic_status();memcpy(mac,nic.mac,6);net7_retry();
}
void net7_pump(void) {
    __atomic_exchange_n(&work,0,__ATOMIC_ACQ_REL);
    uint64_t now=m3_clock_ticks();net7_nic_info nic=net7_nic_status();
    if(nic.present&&nic.status){
        if(configuration.state!=NET7_FAILED){dhcp_state=0;dns_state=NET7_DNS_FAILED;configuration.address=0;
            state(NET7_FAILED,"Ethernet initialization failed. Open System details for its status.");}
        (void)net7_nic_poll(receive);tcp7_pump();tls7_pump();download7_pump();timer(0);return;
    }
    if(!nic.online||nic.status){
        if(configuration.state!=NET7_DISCONNECTED&&configuration.state!=NET7_NO_INTERFACE){dhcp_state=0;dns_state=NET7_DNS_FAILED;configuration.address=0;state(NET7_DISCONNECTED,"Ethernet disconnected. Your files are unchanged.");}
        (void)net7_nic_poll(receive);tcp7_pump();tls7_pump();download7_pump();timer(0);return;
    }
    if(configuration.state==NET7_DISCONNECTED||configuration.state==NET7_NO_INTERFACE||
       (configuration.state==NET7_CONNECTED&&now>=lease_expires))net7_retry();
    (void)net7_nic_poll(receive);
    if(dhcp_state&&now>=deadline){
        if(++retries>4){dhcp_state=0;state(NET7_FAILED,"Automatic address setup timed out. Retry or check the network.");}
        else {deadline=now+(100u<<(retries<3?retries:3));(void)dhcp_send(dhcp_state==1?1:3);}
    }
    if(dns_state==NET7_DNS_PENDING){
        if(!dns_sent)(void)dns_send();
        if(now>=dns_deadline){
            if(++dns_retries>4){dns_state=NET7_DNS_FAILED;++configuration.revision;}
            else {dns_deadline=now+200;dns_sent=0;(void)dns_send();}
        }
    }
    tcp7_pump();tls7_pump();download7_pump();timer(dhcp_state||dns_state==NET7_DNS_PENDING||tcp7_active());
}
net7_configuration net7_current(void) {return configuration;}
