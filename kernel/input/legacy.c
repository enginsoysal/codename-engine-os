#include <engine/m4.h>
#ifdef ENGINE_M6
#include <engine/install/m6.h>
#endif
static unsigned ready,head,tail,shift,extended;static uint8_t queue[256];static uint64_t dropped;
#ifdef ENGINE_M7
static unsigned control_modifier,alt_modifier;
#endif
static const char plain[58]={0,0,'1','2','3','4','5','6','7','8','9','0','-','=',8,9,'q','w','e','r','t','y','u','i','o','p','[',']',10,0,'a','s','d','f','g','h','j','k','l',';',39,'`',0,92,'z','x','c','v','b','n','m',',','.','/',0,'*',0,' '};
static const char upper[58]={0,0,'!','@','#','$','%','^','&','*','(',')','_','+',8,9,'Q','W','E','R','T','Y','U','I','O','P','{','}',10,0,'A','S','D','F','G','H','J','K','L',':',34,'~',0,'|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,' '};
static void enqueue(uint8_t c){unsigned h=__atomic_load_n(&head,__ATOMIC_RELAXED),next=(h+1)&255;if(next==__atomic_load_n(&tail,__ATOMIC_ACQUIRE)){__atomic_fetch_add(&dropped,1,__ATOMIC_RELAXED);return;}queue[h]=c;__atomic_store_n(&head,next,__ATOMIC_RELEASE);}
void input_feed(uint8_t c){enqueue(c);}
static int writable(void){uint64_t end=m3_clock_ticks()+20;unsigned n=0;while(in8(0x64)&2)if(m3_clock_ticks()>=end||++n==1000000)return 0;return 1;}
#ifdef ENGINE_M6
static uint8_t mouse_packet[4];static unsigned mouse_index,mouse_bytes=3;
static int mouse_reply(void){
    uint64_t limit=m3_clock_ticks()+20;for(unsigned n=0;n<1000000&&m3_clock_ticks()<limit;++n){uint8_t status=in8(0x64);if(!(status&1))continue;uint8_t reply=in8(0x60);if(status&32)return reply;}
    return -1;
}
static unsigned mouse_command(uint8_t value){if(!writable())return 0;out8(0x64,0xd4);if(!writable())return 0;out8(0x60,value);return mouse_reply()==0xfa;}
#ifdef ENGINE_M9
#include "vmmouse9.inc"
#endif
static void mouse_byte(uint8_t value){
    if(!mouse_index&&(!(value&8)||value==0xfa))return;mouse_packet[mouse_index++]=value;if(mouse_index!=mouse_bytes)return;mouse_index=0;
#ifdef ENGINE_M9
    if(vm9_input())return;
#endif
    if(mouse_packet[0]&0xc0)return;
    int dx=(int)mouse_packet[1]-((mouse_packet[0]&0x10)?256:0),dy=(int)mouse_packet[2]-((mouse_packet[0]&0x20)?256:0);m6_pointer_report(dx,-dy,mouse_packet[0]&7);
}
#endif
void input_init(void){
    __asm__ volatile("cli");out8(0x21,255);for(unsigned i=0;i<64&&(in8(0x64)&1);++i)(void)in8(0x60);
    if(writable()){out8(0x64,0x20);uint64_t end=m3_clock_ticks()+20;unsigned n=0;while(!(in8(0x64)&1)&&m3_clock_ticks()<end&&++n<1000000){}
        if(in8(0x64)&1){uint8_t config=(in8(0x60)|0x41)&~0x10u;
#ifdef ENGINE_M6
            config=(config|2)&~0x20u;
#endif
if(writable()){out8(0x64,0x60);if(writable())out8(0x60,config);}}
        if(writable())out8(0x64,0xae);if(writable())out8(0x60,0xf4);
    }
    #ifdef ENGINE_M6
    if(writable())out8(0x64,0xa8);
    unsigned mouse=0;if(mouse_command(0xf6)&&mouse_command(0xf2)){int id=mouse_reply();if(id==0||id==3||id==4){mouse_bytes=id?4:3;mouse=mouse_command(0xf4);}}serial_text(mouse?"M6_PS2_MOUSE_ONLINE\n":"M6_PS2_MOUSE_UNAVAILABLE\n");
#ifdef ENGINE_M9
    if(mouse)vm9_start();
#endif
    out8(0x3f9,1);ready=1;out8(0x21,0xe9);out8(0xa1,0xef);
#else
    out8(0x3f9,1);ready=1;out8(0x21,0xed);out8(0xa1,255);
#endif
}
#ifdef ENGINE_M92
static unsigned user_irqs92;
#endif
unsigned input_irq(uint64_t *frame){
    if(!ready
#ifndef ENGINE_M92
       ||(frame[18]&3)
#endif
    ){return 0;}
    unsigned vector=(unsigned)frame[15];if(vector!=33&&vector!=36
#ifdef ENGINE_M6
       &&vector!=44
#endif
    )return 0;
#ifdef ENGINE_M92
    /* A hardware interrupt belongs to its device even when it interrupts CPL3.
       Queue bounded input and return to the same protected process frame. */
    if((frame[18]&3)&&!user_irqs92++){serial_text("M92_INPUT_IRQ_CPL3_SERVICED\n");}
#endif
    if(vector==36){for(unsigned i=0;i<64&&(in8(0x3fd)&1);++i){uint8_t c=in8(0x3f8);enqueue(c==13?10:c);}}
    else for(unsigned i=0;i<32&&(in8(0x64)&1);++i){uint8_t status=in8(0x64),code=in8(0x60);
#ifdef ENGINE_M6
        if(status&0xc0){mouse_index=0;continue;}
#endif
        if(status&32){
#ifdef ENGINE_M6
            mouse_byte(code);
#endif
            continue;
        }
        if(code==0xe0){extended=1;continue;}if(code==0xfa)continue;
        if(code==42){shift|=1;continue;}if(code==54){shift|=2;continue;}if(code==170){shift&=~1u;continue;}if(code==182){shift&=~2u;continue;}
#ifdef ENGINE_M7
        if(code==29){control_modifier=1;extended=0;continue;}if(code==157){control_modifier=0;extended=0;continue;}
        if(code==56){alt_modifier=1;extended=0;continue;}if(code==184){alt_modifier=0;extended=0;continue;}
        if(code==68||(extended&&code==91)){enqueue(137);extended=0;continue;}
        if(!(code&128)&&extended){unsigned c=code==75?128:code==77?129:code==71?130:code==79?131:code==83?132:code==72?133:code==80?134:0;if(c)enqueue((uint8_t)c);extended=0;continue;}
        if(code==1){enqueue(27);continue;}
#endif
        if(code&128){extended=0;continue;}if(extended){extended=0;continue;}if(code<58){uint8_t c=(uint8_t)(shift?upper[code]:plain[code]);
#ifdef ENGINE_M7
        if(c==9)c=(uint8_t)(alt_modifier?135:shift?136:9);
        if(control_modifier&&c>='a'&&c<='z')c=(uint8_t)(c-'a'+1);
        if(control_modifier&&c>='A'&&c<='Z')c=(uint8_t)(c-'A'+1);
#endif
        if(c)enqueue(c);}
    }
    #ifdef ENGINE_M6
    if(vector==44)out8(0xa0,0x20);
#endif
    out8(0x20,0x20);return 1;
}
int input_pop(void){unsigned t=__atomic_load_n(&tail,__ATOMIC_RELAXED);if(t==__atomic_load_n(&head,__ATOMIC_ACQUIRE))return usb_mouse_pending()?256:-1;uint8_t c=queue[t];__atomic_store_n(&tail,(t+1)&255,__ATOMIC_RELEASE);return c;}
uint64_t input_dropped(void){return __atomic_load_n(&dropped,__ATOMIC_RELAXED);}
