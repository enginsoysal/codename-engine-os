#ifdef ENGINE_M51
#include "distro.h"
#endif
#include <engine/base.h>
#include <engine/boot/boot_v0.h>
#include "build.h"
/* Original 5x7 uppercase bitmap alphabet for this minimal boot console. */
static const uint8_t letters[26][7]={
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
 {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
 {14,17,16,23,17,17,14},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
 {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
 {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
 {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}};
#ifdef ENGINE_M4
static const uint8_t lower[26][7]={
 {0,0,14,1,15,17,15},{16,16,30,17,17,17,30},{0,0,14,16,16,17,14},
 {1,1,15,17,17,17,15},{0,0,14,17,31,16,14},{6,8,8,28,8,8,8},
 {0,15,17,17,15,1,14},{16,16,30,17,17,17,17},{4,0,12,4,4,4,14},
 {2,0,6,2,2,18,12},{16,16,18,20,24,20,18},{12,4,4,4,4,4,14},
 {0,0,26,21,21,21,21},{0,0,30,17,17,17,17},{0,0,14,17,17,17,14},
 {0,30,17,17,30,16,16},{0,15,17,17,15,1,1},{0,0,22,25,16,16,16},
 {0,0,15,16,14,1,30},{8,8,28,8,8,9,6},{0,0,17,17,17,19,13},
 {0,0,17,17,17,10,4},{0,0,17,17,21,21,10},{0,0,17,10,4,10,17},
 {0,17,17,17,15,1,14},{0,0,31,2,4,8,31}};
#endif
static const uint8_t digits[10][7]={
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
 {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
 {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14}};
static const os_boot_info_v0 *screen;
#ifdef ENGINE_M6
static unsigned gui6_repaint;
#endif
#ifdef ENGINE_M7
static unsigned clip7_active,clip7_left,clip7_top,clip7_right,clip7_bottom;
void gui7_clip(unsigned x,unsigned y,unsigned w,unsigned h){clip7_left=x;clip7_top=y;clip7_right=x+w;clip7_bottom=y+h;clip7_active=1;gui6_repaint=1;}
void gui7_clip_reset(void){clip7_active=0;}
#endif
static uint32_t color(uint32_t rgb) {
    return screen->framebuffer_format==2?rgb:((rgb&255)<<16)|(rgb&0xff00)|((rgb>>16)&255);
}
static void pixel(unsigned x,unsigned y,uint32_t rgb) {
#ifdef ENGINE_M7
    if(clip7_active&&(x<clip7_left||x>=clip7_right||y<clip7_top||y>=clip7_bottom))return;
#endif
    if(x>=screen->framebuffer_width||y>=screen->framebuffer_height)return;
    volatile uint32_t *p=(volatile uint32_t*)(uintptr_t)(screen->framebuffer.physical_address+(uint64_t)y*screen->framebuffer_stride_bytes+x*4);
    *p=color(rgb);
}
static void text(unsigned x,unsigned y,const char *s,unsigned scale,uint32_t rgb) {
    for(;*s;++s,x+=6*scale) {
        char c=*s;
#ifndef ENGINE_M4
        if(c>='a'&&c<='z')c=(char)(c-'a'+'A');
#endif
        const uint8_t *glyph=
#ifdef ENGINE_M4
            c>='a'&&c<='z'?lower[c-'a']:
#endif
            c>='A'&&c<='Z'?letters[c-'A']:c>='0'&&c<='9'?digits[c-'0']:0;
        for(unsigned row=0;row<7;++row) {
            unsigned bits=glyph?glyph[row]:c=='-'&&row==3?31:c=='='&&(row==2||row==4)?31:c==':'&&(row==2||row==5)?4:c==';'&&(row==2||row==5||row==6)?4:c=='/'?(1u<<(row*4/6)):c=='.'&&row==6?4:c=='_'&&row==6?31:c=='>'&&row>0&&row<6?(1u<<(2+(row>3?row-3:3-row))):0;
            for(unsigned col=0;col<5;++col)if(bits&(1u<<(4-col)))
                for(unsigned dy=0;dy<scale;++dy)for(unsigned dx=0;dx<scale;++dx)pixel(x+col*scale+dx,y+row*scale+dy,rgb);
        }
    }
}
void display_boot(const os_boot_info_v0 *boot) {
    screen=boot;if(!boot->framebuffer.byte_length)return;
    uint32_t accent=color(0x39d5bb),background=color(0x101d2b);
    for(unsigned y=0;y<boot->framebuffer_height;++y) {
        volatile uint32_t *row=(volatile uint32_t*)(uintptr_t)(boot->framebuffer.physical_address+(uint64_t)y*boot->framebuffer_stride_bytes);
        for(unsigned x=0;x<boot->framebuffer_width;++x)row[x]=y<8?accent:background;
    }
#ifdef ENGINE_M51
    text(40,48,DISTRO_NAME,3,0xffffff);text(40,110,"Starting. Please wait...",2,DISTRO_ACCENT);return;
#endif
    text(40,48,"CODENAME: ENGINE OS",3,0xffffff);
#ifdef ENGINE_M3
    text(40,110,"M3 KERNEL ONLINE",2,0x39d5bb);
#elif defined(ENGINE_M2)
    text(40,110,"M2 KERNEL ONLINE",2,0x39d5bb);
#else
    text(40,110,"M1 KERNEL ONLINE",2,0x39d5bb);
#endif
    text(40,158,"BUILD",2,0x93aabe);text(112,158,BUILD_ID,2,0x93aabe);
#ifdef ENGINE_M45
    text(40,210,"OWN KERNEL - FIRMWARE HANDOFF COMPLETE",2,0xffffff);
#else
    text(40,210,"OWN KERNEL - UEFI SERVICES STOPPED",2,0xffffff);
#endif
    text(40,254,"DOS / WINDOWS 1-11: PLANNED",2,0x93aabe);
#ifdef ENGINE_M5
    text(40,296,"LIVE INSTALL RECOVERY: STARTING",2,0x93aabe);
#else
    text(40,296,"NO INSTALLER YET",2,0x93aabe);
#endif
}
void kernel_panic_screen(void) {if(screen&&screen->framebuffer.byte_length)text(40,350,"KERNEL PANIC - SEE SERIAL LOG",2,0xff7070);}
#ifdef ENGINE_M2
static void number(unsigned x,unsigned y,uint64_t value) {
    char out[24];unsigned n=0;do {out[n++]=(char)('0'+value%10);value/=10;}while(value&&n<23);
    for(unsigned i=0;i<n/2;++i){char c=out[i];out[i]=out[n-i-1];out[n-i-1]=c;}out[n]=0;text(x,y,out,2,0x39d5bb);
}
void display_m2_result(unsigned failures,uint64_t peak,uint64_t retained,unsigned starts,unsigned faults) {
#ifdef ENGINE_M51
    if(!failures)return;
#endif
    if(!screen||!screen->framebuffer.byte_length)return;
    uint32_t background=color(0x101d2b);
    for(unsigned y=200;y<screen->framebuffer_height;++y) {
        volatile uint32_t *row=(volatile uint32_t*)(uintptr_t)(screen->framebuffer.physical_address+(uint64_t)y*screen->framebuffer_stride_bytes);
        for(unsigned x=0;x<screen->framebuffer_width;++x)row[x]=background;
    }
    text(40,210,failures?"USERMODE ISOLATION: FAIL":"USERMODE ISOLATION: PASS",2,failures?0xff7070:0x39d5bb);
#ifdef ENGINE_M3
    text(40,250,failures?"THREADS WAITS VM SMP: FAIL":"THREADS WAITS VM SMP: PASS",2,failures?0xff7070:0x39d5bb);
#else
    text(40,250,"PRIVATE MEMORY AND SHARED IPC: CHECKED",2,0xffffff);
#endif
#ifdef ENGINE_M3
    extern void m3_summary(unsigned*,unsigned*);unsigned cpus,threads;m3_summary(&cpus,&threads);(void)faults;(void)starts;
    text(40,290,"ACTIVE CPUS:",2,0x93aabe);number(340,290,cpus);
    text(40,330,"NATIVE THREAD STARTS:",2,0x93aabe);number(340,330,threads);
#else
    text(40,290,"CONTAINED USER FAULTS:",2,0x93aabe);number(340,290,faults);
    text(40,330,"PROCESS STARTS:",2,0x93aabe);number(340,330,starts);
#endif
    text(40,370,"PEAK MANAGED RAM KIB:",2,0x93aabe);number(340,370,peak/1024);
    text(40,410,"RETAINED MANAGED RAM KIB:",2,0x93aabe);number(360,410,retained/1024);
    text(40,450,"IDLE: CPU HALTED - NO POLLING",2,0x39d5bb);
    text(40,490,"DOS / WINDOWS 1-11: PLANNED",2,0x93aabe);
#ifdef ENGINE_M5
    text(40,530,"STARTING M5 LIVE / INSTALL / RECOVERY",2,0x93aabe);
#else
    text(40,530,"NO INSTALLER YET",2,0x93aabe);
#endif
}
#endif

#ifdef ENGINE_M4
void display_console_setup(void){
    if(!screen||!screen->framebuffer.byte_length)return;
    uint32_t bg=color(0x101d2b),accent=color(
#ifdef ENGINE_M51
DISTRO_ACCENT
#else
0x39d5bb
#endif
);
    for(unsigned y=0;y<screen->framebuffer_height;++y){volatile uint32_t *p=(volatile uint32_t*)(uintptr_t)(screen->framebuffer.physical_address+(uint64_t)y*screen->framebuffer_stride_bytes);for(unsigned x=0;x<screen->framebuffer_width;++x)p[x]=y<8?accent:bg;}
    #ifdef ENGINE_M51
    text(16,26,DISTRO_NAME,3,0xffffff);
#else
    text(16,26,"CODENAME: ENGINE OS",3,0xffffff);
#endif
#if defined(ENGINE_M51)
    text(16,76,"WELCOME / INSTALL / RECOVERY",2,DISTRO_ACCENT);text(16,112,"CHOOSE A NUMBER AND PRESS ENTER",2,0x93aabe);
#elif defined(ENGINE_M5)
    text(16,76,"M5 LIVE / INSTALL / RECOVERY",2,0x39d5bb);text(16,112,"HELP / PLAN N / VOLUME - TEXT INSTALLER",2,0x93aabe);
#elif defined(ENGINE_M45)
    text(16,76,"M4.5 DEVICE AND FILE CONSOLE",2,0x39d5bb);text(16,112,"DISKS / PARTS N / HELP - NO INSTALLER",2,0x93aabe);
#else
    text(16,76,"M4 FILESYSTEM AND USB CONSOLE",2,0x39d5bb);text(16,112,"HELP FOR COMMANDS - NO INSTALLER YET",2,0x93aabe);
#endif
}
void display_console_cell(unsigned col,unsigned row,char c){
    if(!screen||!screen->framebuffer.byte_length)return;unsigned x=16+col*12,y=152+row*18;
    for(unsigned dy=0;dy<16;++dy)for(unsigned dx=0;dx<12;++dx)pixel(x+dx,y+dy,0x101d2b);
    char s[2]={c,0};text(x,y,s,2,0xffffff);
}
void display_mouse(unsigned x,unsigned y){
    #ifdef ENGINE_M6
    static uint32_t saved[135];const unsigned cw=9,ch=15;
#else
    static uint32_t saved[25];const unsigned cw=5,ch=5;
#endif
    static unsigned oldx,oldy,visible;
    if(!screen||!screen->framebuffer.byte_length||screen->framebuffer_width<cw||screen->framebuffer_height<ch)return;
    #ifdef ENGINE_M6
    if(gui6_repaint){visible=0;gui6_repaint=0;}
#endif
    if(visible)for(unsigned dy=0;dy<ch;++dy)for(unsigned dx=0;dx<cw;++dx)*(volatile uint32_t*)(uintptr_t)(screen->framebuffer.physical_address+(uint64_t)(oldy+dy)*screen->framebuffer_stride_bytes+(oldx+dx)*4)=saved[dy*cw+dx];
    if(x>screen->framebuffer_width-cw)x=screen->framebuffer_width-cw;if(y>screen->framebuffer_height-ch)y=screen->framebuffer_height-ch;
    for(unsigned dy=0;dy<ch;++dy)for(unsigned dx=0;dx<cw;++dx){volatile uint32_t *p=(volatile uint32_t*)(uintptr_t)(screen->framebuffer.physical_address+(uint64_t)(y+dy)*screen->framebuffer_stride_bytes+(x+dx)*4);saved[dy*cw+dx]=*p;
#ifdef ENGINE_M6
    if(dx<=dy/2)*p=color(dx==0||dx==dy/2?0x14252c:0xffffff);
#else
    *p=color(0x39d5bb);
#endif
    }
    oldx=x;oldy=y;visible=1;
}
#endif

#ifdef ENGINE_M6
void gui6_rect(unsigned x,unsigned y,unsigned w,unsigned h,uint32_t rgb){
 if(!screen||x>=screen->framebuffer_width||y>=screen->framebuffer_height)return;
 if(!x&&!y)gui6_repaint=1;
 if(w>screen->framebuffer_width-x)w=screen->framebuffer_width-x;if(h>screen->framebuffer_height-y)h=screen->framebuffer_height-y;
#ifdef ENGINE_M7
 if(clip7_active){unsigned right=x+w,bottom=y+h;if(x<clip7_left)x=clip7_left;if(y<clip7_top)y=clip7_top;if(right>clip7_right)right=clip7_right;if(bottom>clip7_bottom)bottom=clip7_bottom;if(x>=right||y>=bottom)return;w=right-x;h=bottom-y;}
#endif
 uint32_t value=color(rgb);for(unsigned j=0;j<h;++j){volatile uint32_t *p=(volatile uint32_t*)(uintptr_t)(screen->framebuffer.physical_address+(uint64_t)(y+j)*screen->framebuffer_stride_bytes+x*4);for(unsigned i=0;i<w;++i)p[i]=value;}
}
void gui6_text(unsigned x,unsigned y,const char*s,unsigned scale,uint32_t rgb){if(screen)text(x,y,s,scale,rgb);}
#endif
