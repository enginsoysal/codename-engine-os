#include <engine/base.h>
#include "boot_v0.h"
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
static const uint8_t digits[10][7]={
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
 {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
 {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14}};
static const os_boot_info_v0 *screen;
static uint32_t color(uint32_t rgb) {
    return screen->framebuffer_format==2?rgb:((rgb&255)<<16)|(rgb&0xff00)|((rgb>>16)&255);
}
static void pixel(unsigned x,unsigned y,uint32_t rgb) {
    if(x>=screen->framebuffer_width||y>=screen->framebuffer_height)return;
    volatile uint32_t *p=(volatile uint32_t*)(uintptr_t)(screen->framebuffer.physical_address+(uint64_t)y*screen->framebuffer_stride_bytes+x*4);
    *p=color(rgb);
}
static void text(unsigned x,unsigned y,const char *s,unsigned scale,uint32_t rgb) {
    for(;*s;++s,x+=6*scale) {
        char c=*s;if(c>='a'&&c<='z')c=(char)(c-'a'+'A');
        const uint8_t *glyph=c>='A'&&c<='Z'?letters[c-'A']:c>='0'&&c<='9'?digits[c-'0']:0;
        for(unsigned row=0;row<7;++row) {
            unsigned bits=glyph?glyph[row]:c=='-'&&row==3?31:c==':'&&(row==2||row==5)?4:0;
            for(unsigned col=0;col<5;++col)if(bits&(1u<<(4-col)))
                for(unsigned dy=0;dy<scale;++dy)for(unsigned dx=0;dx<scale;++dx)pixel(x+col*scale+dx,y+row*scale+dy,rgb);
        }
    }
}
void display_boot(const os_boot_info_v0 *boot) {
    screen=boot;if(!boot->framebuffer.byte_length)return;
    for(unsigned y=0;y<boot->framebuffer_height;++y)for(unsigned x=0;x<boot->framebuffer_width;++x)pixel(x,y,y<8?0x39d5bb:0x101d2b);
    text(40,48,"CODENAME: ENGINE OS",3,0xffffff);
    text(40,110,"M1 KERNEL ONLINE",2,0x39d5bb);
    text(40,158,"BUILD",2,0x93aabe);text(112,158,BUILD_ID,2,0x93aabe);
    text(40,210,"OWN KERNEL - UEFI SERVICES STOPPED",2,0xffffff);
    text(40,254,"WINDOWS 95-11: PLANNED",2,0x93aabe);
    text(40,296,"NO INSTALLER YET",2,0x93aabe);
}
void kernel_panic_screen(void) {if(screen&&screen->framebuffer.byte_length)text(40,350,"KERNEL PANIC - SEE SERIAL LOG",2,0xff7070);}
