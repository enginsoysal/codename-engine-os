#include <engine/network/clock.h>
#include <engine/base.h>
static uint8_t rtc(unsigned index){out8(0x70,(uint8_t)(0x80|index));return in8(0x71);}
#ifdef ENGINE_M74
static volatile unsigned changed74;
static volatile uint64_t elapsed74;
void clock74_start(void){
    uint64_t flags;__asm__ volatile("pushfq;popq %0;cli":"=r"(flags)::"memory");
    /* Update-ended IRQ, once per RTC second; no periodic PIT polling. */
    unsigned status=rtc(0x0b);out8(0x70,0x8b);out8(0x71,(uint8_t)(status|0x10));
    (void)rtc(0x0c);out8(0x70,0);out8(0xa1,in8(0xa1)&~1u);out8(0x21,in8(0x21)&~4u);
    if(flags&512)__asm__ volatile("sti":::"memory");
}
unsigned clock74_irq(uint64_t *frame){
    if(frame[15]!=40)return 0;
    unsigned status=rtc(0x0c);out8(0x70,0);
    if(status&0x10){++elapsed74;changed74=1;}
    out8(0xa0,0x20);out8(0x20,0x20);return 1;
}
unsigned clock74_pending(void){return changed74;}
uint64_t clock74_elapsed(void){return elapsed74;}
void clock74_consume(void){changed74=0;}
#endif
static unsigned number(unsigned value,unsigned binary){
    if(binary)return value;
    if((value&15)>9||(value>>4)>9)return 255;return (value>>4)*10+(value&15);
}
unsigned clock7_read(clock7_time *out){
    if(!out)return 1;
    uint64_t flags;__asm__ volatile("pushfq;popq %0;cli":"=r"(flags)::"memory");
    uint8_t a[9]={0},b[9]={0};unsigned valid=0;
    const unsigned registers[]={0,2,4,7,8,9,0x32,0x0b,0x0d};
    /* Bound UIP/stability retries; never wait forever on missing/broken RTC. */
    for(unsigned attempt=0;attempt<64;++attempt){
        if(rtc(0x0a)&0x80)continue;
        for(unsigned i=0;i<9;++i)a[i]=rtc(registers[i]);
        if(rtc(0x0a)&0x80)continue;
        for(unsigned i=0;i<9;++i)b[i]=rtc(registers[i]);
        if(!(rtc(0x0a)&0x80)&&!memcmp(a,b,sizeof(a))){valid=1;break;}
    }
    /* Engine OS currently owns this PC RTC/NMI selector; restore NMI enabled. */
    out8(0x70,0);
    if(flags&512)__asm__ volatile("sti":::"memory");
    if(!valid||!(a[8]&0x80)||(a[7]&0x80))return 1;
    unsigned binary=a[7]&4,pm=a[2]&0x80;
    unsigned second=number(a[0],binary),minute=number(a[1],binary),hour=number(a[2]&127,binary);
    if(!(a[7]&2)){if(hour<1||hour>12)return 1;hour=hour%12+(pm?12:0);}else if(pm)return 1;
    unsigned century=number(a[6],binary);
    if(a[6]!=0&&a[6]!=255&&century!=20)return 1;
    return clock7_calendar(2000+number(a[5],binary),number(a[4],binary),number(a[3],binary),hour,minute,second,out);
}
