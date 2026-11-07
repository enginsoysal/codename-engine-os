#include <engine/network/tcp.h>
#include <engine/network/network.h>
#include <engine/m3.h>

/* One bounded, stop-and-wait client stream. No hidden socket allocator. */
enum { FIN=1,SYN=2,RST=4,ACK=16,RX_CAPACITY=16384,MSS=1460 };
static struct {
    unsigned state,queued,sent,retries,ack_pending,closing,peer_fin;
    uint32_t remote,local,una,next,receive;
    uint16_t port,source,peer_window,mss;
    uint8_t outgoing[MSS],incoming[RX_CAPACITY];
    unsigned out_bytes,out_flags,head,used;
    uint64_t deadline,progress;
} stream;

static int before(uint32_t a,uint32_t b){return (int32_t)(a-b)<0;}
static unsigned emit(unsigned flags,uint32_t sequence,const void *data,unsigned bytes) {
    uint8_t packet[24+MSS]={0};unsigned header=(flags&SYN)?24:20;
    net7_put16(packet,stream.source);net7_put16(packet+2,stream.port);
    net7_put32(packet+4,sequence);net7_put32(packet+8,(flags&ACK)?stream.receive:0);
    packet[12]=(uint8_t)((header/4)<<4);packet[13]=(uint8_t)flags;
    net7_put16(packet+14,(uint16_t)(RX_CAPACITY-stream.used));
    if(flags&SYN){packet[20]=2;packet[21]=4;net7_put16(packet+22,MSS);}
    if(bytes)memcpy(packet+header,data,bytes);
    net7_put16(packet+16,net7_checksum(packet,header+bytes,net7_pseudo(stream.local,stream.remote,6,header+bytes)));
    return net7_send_ipv4(stream.remote,6,packet,header+bytes);
}
static void fail(void){stream.state=TCP7_FAILED;stream.queued=stream.ack_pending=0;}
unsigned tcp7_status(void){return stream.state;}
unsigned tcp7_active(void){return stream.state>=TCP7_CONNECTING&&stream.state<=TCP7_CLOSING;}
void tcp7_abort(void){
    if(tcp7_active())(void)emit(RST|ACK,stream.next,0,0);
    memset(&stream,0,sizeof(stream));
}
unsigned tcp7_connect(uint32_t address,unsigned port){
    if(tcp7_active()||!address||!port||port>65535)return 1;
    net7_configuration config=net7_current();uint32_t random[2];
    if(config.state!=NET7_CONNECTED||!net7_random(random,sizeof(random)))return 1;
    memset(&stream,0,sizeof(stream));stream.remote=address;stream.local=config.address;
    stream.port=(uint16_t)port;stream.source=(uint16_t)(49152+(random[1]%16384));
    stream.una=random[0];stream.next=stream.una+1;stream.mss=536;
    stream.state=TCP7_CONNECTING;stream.queued=1;stream.out_flags=SYN;
    stream.progress=m3_clock_ticks();net7_schedule();return 0;
}
unsigned tcp7_write(const void *data,unsigned bytes){
    if((stream.state!=TCP7_OPEN&&stream.state!=TCP7_PEER_CLOSED)||stream.closing||stream.queued||!data||!bytes)return 0;
    unsigned count=bytes;if(count>stream.mss)count=stream.mss;
    if(count>stream.peer_window)count=stream.peer_window;if(!count)return 0;
    memcpy(stream.outgoing,data,count);stream.out_bytes=count;stream.out_flags=ACK;
    stream.queued=1;stream.sent=stream.retries=0;stream.next=stream.una+count;
    net7_schedule();return count;
}
unsigned tcp7_read(void *output,unsigned bytes){
    if(!output)return 0;unsigned count=bytes>stream.used?stream.used:bytes;
    uint8_t *out=output;for(unsigned i=0;i<count;++i)out[i]=stream.incoming[(stream.head+i)%RX_CAPACITY];
    stream.head=(stream.head+count)%RX_CAPACITY;stream.used-=count;
    if(count){stream.ack_pending=1;net7_schedule();}return count;
}
void tcp7_close(void){if(tcp7_active()){stream.closing=1;net7_schedule();}}
static unsigned peer_mss(const net7_ipv4 *ip,unsigned *result){
    unsigned header=(ip->data[12]>>4)*4;*result=536;
    for(unsigned pos=20;pos<header;){
        unsigned kind=ip->data[pos++];if(!kind)return 1;if(kind==1)continue;
        if(pos>=header)return 0;unsigned size=ip->data[pos++];
        if(size<2||size-2>header-pos)return 0;
        if(kind==2){if(size!=4)return 0;unsigned value=net7_get16(ip->data+pos);if(!value)return 0;*result=value>MSS?MSS:value;}
        pos+=size-2;
    }
    return 1;
}
void tcp7_receive(const net7_ipv4 *ip){
    net7_tcp packet;
    if(!tcp7_active()||ip->source!=stream.remote||ip->destination!=stream.local||net7_read_tcp(ip,&packet)||
       packet.source!=stream.port||packet.destination!=stream.source)return;
    uint64_t now=m3_clock_ticks();
    if(stream.state==TCP7_CONNECTING){
        if((packet.flags&(RST|ACK))==(RST|ACK)&&packet.acknowledgement==stream.next){fail();return;}
        unsigned mss;
        if((packet.flags&(SYN|ACK|RST|FIN))!=(SYN|ACK)||packet.acknowledgement!=stream.next||packet.bytes||!peer_mss(ip,&mss))return;
        stream.receive=packet.sequence+1;stream.una=stream.next;stream.queued=stream.sent=0;
        stream.mss=(uint16_t)mss;stream.peer_window=packet.window;stream.state=TCP7_OPEN;
        stream.ack_pending=1;stream.progress=now;net7_schedule();return;
    }
    if(packet.flags&RST){if(packet.sequence==stream.receive)fail();else stream.ack_pending=1;net7_schedule();return;}
    if(packet.flags&SYN){stream.ack_pending=1;net7_schedule();return;}
    /* Out-of-window segments cannot change send state or the advertised window. */
    uint32_t end=packet.sequence+packet.bytes+!!(packet.flags&FIN);
    if(before(stream.receive,packet.sequence)||before(end,stream.receive)){
        stream.ack_pending=1;net7_schedule();return;
    }
    if(!(packet.flags&ACK)||before(packet.acknowledgement,stream.una)||before(stream.next,packet.acknowledgement))return;
    stream.peer_window=packet.window;
    if(stream.queued&&stream.sent&&packet.acknowledgement==stream.next){
        unsigned was_fin=stream.out_flags&FIN;
        stream.una=stream.next;stream.queued=stream.sent=stream.out_bytes=0;stream.progress=now;
        if(was_fin&&stream.peer_fin){stream.state=TCP7_CLOSED;return;}
    }
    unsigned skip=stream.receive-packet.sequence;
    if(skip>packet.bytes)skip=packet.bytes;
    unsigned count=packet.bytes-skip;
    if(stream.peer_fin&&count){stream.ack_pending=1;net7_schedule();return;}
    if(count>RX_CAPACITY-stream.used){stream.ack_pending=1;net7_schedule();return;}
    for(unsigned n=0;n<count;++n)stream.incoming[(stream.head+stream.used+n)%RX_CAPACITY]=packet.data[skip+n];
    stream.used+=count;stream.receive+=count;
    if(count){stream.progress=now;stream.ack_pending=1;}
    if((packet.flags&FIN)&&packet.sequence+packet.bytes==stream.receive){
        if(!stream.peer_fin){++stream.receive;stream.peer_fin=1;stream.progress=now;
            if(stream.state==TCP7_CLOSING&&!stream.queued){(void)emit(ACK,stream.next,0,0);stream.state=TCP7_CLOSED;return;}
            if(stream.state==TCP7_OPEN)stream.state=TCP7_PEER_CLOSED;}
        stream.ack_pending=1;
    } else if(packet.flags&FIN)stream.ack_pending=1;
    if(count||stream.ack_pending||stream.closing)net7_schedule();
}
void tcp7_pump(void){
    if(!tcp7_active())return;uint64_t now=m3_clock_ticks();
    net7_configuration config=net7_current();
    if(config.state!=NET7_CONNECTED||config.address!=stream.local||now-stream.progress>=3000){fail();return;}
    if(stream.ack_pending&&!emit(ACK,stream.next,0,0))stream.ack_pending=0;
    if(stream.closing&&!stream.queued&&stream.state!=TCP7_CLOSING){
        stream.state=TCP7_CLOSING;stream.out_flags=FIN|ACK;stream.out_bytes=0;
        stream.next=stream.una+1;stream.queued=1;stream.sent=stream.retries=0;
    }
    if(stream.queued&&(!stream.sent||now>=stream.deadline)){
        if(stream.sent&&stream.retries>=4){fail();return;}
        if(!emit(stream.out_flags,stream.una,stream.outgoing,stream.out_bytes)){
            if(stream.sent)++stream.retries;stream.sent=1;stream.deadline=now+(100u<<(stream.retries<3?stream.retries:3));
        }
    }
}
