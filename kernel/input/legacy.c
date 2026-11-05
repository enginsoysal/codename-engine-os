#include <engine/m4.h>
static unsigned ready,head,tail,shift,extended;static uint8_t queue[256];static uint64_t dropped;
static const char plain[58]={0,0,'1','2','3','4','5','6','7','8','9','0','-','=',8,9,'q','w','e','r','t','y','u','i','o','p','[',']',10,0,'a','s','d','f','g','h','j','k','l',';',39,'`',0,92,'z','x','c','v','b','n','m',',','.','/',0,'*',0,' '};
static const char upper[58]={0,0,'!','@','#','$','%','^','&','*','(',')','_','+',8,9,'Q','W','E','R','T','Y','U','I','O','P','{','}',10,0,'A','S','D','F','G','H','J','K','L',':',34,'~',0,'|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,' '};
static void enqueue(uint8_t c){unsigned h=__atomic_load_n(&head,__ATOMIC_RELAXED),next=(h+1)&255;if(next==__atomic_load_n(&tail,__ATOMIC_ACQUIRE)){__atomic_fetch_add(&dropped,1,__ATOMIC_RELAXED);return;}queue[h]=c;__atomic_store_n(&head,next,__ATOMIC_RELEASE);}
static int writable(void){uint64_t end=m3_clock_ticks()+20;unsigned n=0;while(in8(0x64)&2)if(m3_clock_ticks()>=end||++n==1000000)return 0;return 1;}
void input_init(void){
    __asm__ volatile("cli");out8(0x21,255);for(unsigned i=0;i<64&&(in8(0x64)&1);++i)(void)in8(0x60);
    if(writable()){out8(0x64,0x20);uint64_t end=m3_clock_ticks()+20;unsigned n=0;while(!(in8(0x64)&1)&&m3_clock_ticks()<end&&++n<1000000){}
        if(in8(0x64)&1){uint8_t config=(in8(0x60)|0x41)&~0x10u;if(writable()){out8(0x64,0x60);if(writable())out8(0x60,config);}}
        if(writable())out8(0x64,0xae);if(writable())out8(0x60,0xf4);
    }
    out8(0x3f9,1);ready=1;out8(0x21,0xed);out8(0xa1,255);
}
unsigned input_irq(uint64_t *frame){
    if(!ready||(frame[18]&3))return 0;unsigned vector=(unsigned)frame[15];if(vector!=33&&vector!=36)return 0;
    if(vector==36){for(unsigned i=0;i<64&&(in8(0x3fd)&1);++i){uint8_t c=in8(0x3f8);enqueue(c==13?10:c);}}
    else for(unsigned i=0;i<32&&(in8(0x64)&1);++i){uint8_t status=in8(0x64),code=in8(0x60);if(status&32)continue;
        if(code==0xe0){extended=1;continue;}if(code==0xfa)continue;
        if(code==42){shift|=1;continue;}if(code==54){shift|=2;continue;}if(code==170){shift&=~1u;continue;}if(code==182){shift&=~2u;continue;}
        if(code&128){extended=0;continue;}if(extended){extended=0;continue;}if(code<58){uint8_t c=(uint8_t)(shift?upper[code]:plain[code]);if(c)enqueue(c);}
    }
    out8(0x20,0x20);return 1;
}
int input_pop(void){unsigned t=__atomic_load_n(&tail,__ATOMIC_RELAXED);if(t==__atomic_load_n(&head,__ATOMIC_ACQUIRE))return -1;uint8_t c=queue[t];__atomic_store_n(&tail,(t+1)&255,__ATOMIC_RELEASE);return c;}
uint64_t input_dropped(void){return __atomic_load_n(&dropped,__ATOMIC_RELAXED);}
