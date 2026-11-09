/* Original buffered 2D backend. VMware register semantics are documented in the
   upstream vmwgfx device interface; no host integration service is required. */
#include <engine/process.h>
#include <engine/m3.h>
#include <engine/ui/graphics9.h>
extern void *memmove(void*,const void*,size_t);
static graphics9_info graphics9;
static uint32_t *shadow9;
static uint16_t svga9_io;
static volatile uint32_t *fifo9;
static unsigned fifo9_limit;
static unsigned cursor9_visible,cursor9_x,cursor9_y,cursor9_w,cursor9_h;
static unsigned dirty9_left,dirty9_top,dirty9_right,dirty9_bottom,dirty9_active;
static unsigned batch9;
static void graphics9_dirty(unsigned x,unsigned y,unsigned w,unsigned h){
 if(!shadow9||!w||!h||x>=screen->framebuffer_width||y>=screen->framebuffer_height)return;
 if(w>screen->framebuffer_width-x)w=screen->framebuffer_width-x;if(h>screen->framebuffer_height-y)h=screen->framebuffer_height-y;
 if(!dirty9_active){dirty9_left=x;dirty9_top=y;dirty9_right=x+w;dirty9_bottom=y+h;dirty9_active=1;}
 else{if(x<dirty9_left)dirty9_left=x;if(y<dirty9_top)dirty9_top=y;if(x+w>dirty9_right)dirty9_right=x+w;if(y+h>dirty9_bottom)dirty9_bottom=y+h;}
}
static inline void out9(uint16_t port,uint32_t value){__asm__ volatile("outl %0,%1"::"a"(value),"Nd"(port));}
static inline uint32_t in9(uint16_t port){uint32_t value;__asm__ volatile("inl %1,%0":"=a"(value):"Nd"(port));return value;}
static uint32_t pci9(unsigned bdf,unsigned reg){out9(0xcf8,0x80000000u|(bdf<<8)|(reg&252));return in9(0xcfc);}
static uint32_t svga9_read(unsigned reg){out9(svga9_io,reg);return in9(svga9_io+1);}
static void svga9_write(unsigned reg,uint32_t value){out9(svga9_io,reg);out9(svga9_io+1,value);}
static unsigned svga9_sync(void){
 if(!fifo9)return 0;svga9_write(21,1);uint64_t start=m3_clock_ticks();
 for(unsigned i=0;i<2048;++i){if(!svga9_read(22))return 1;if(m3_clock_ticks()-start>=5)break;__asm__ volatile("pause");}
 ++graphics9.fifo_failures;fifo9=0;graphics9.backend=1;serial_text("M9_SVGA_QUARANTINED timeout\n");return 0;
}
static unsigned svga9_command(const uint32_t*words,unsigned count){
 if(!fifo9||count>8)return 0;unsigned min=fifo9[0],max=fifo9[1],next=fifo9[2],stop=fifo9[3];
 if(min!=4096||max!=fifo9_limit||max<=min||((min|max|next|stop)&3)||next<min||next>=max||stop<min||stop>=max)goto bad;
 unsigned free=stop>next?stop-next:max-next+stop-min;if(free<=count*4)goto bad;
 for(unsigned i=0;i<count;++i){fifo9[next/4]=words[i];next+=4;if(next==max)next=min;}
 __asm__ volatile("mfence":::"memory");fifo9[2]=next;return svga9_sync();
 bad:++graphics9.fifo_failures;fifo9=0;graphics9.backend=1;serial_text("M9_SVGA_QUARANTINED ring\n");return 0;
}
static void svga9_begin(void){
 unsigned found=0;
 for(unsigned bus=0;bus<256&&!found;++bus)for(unsigned dev=0;dev<32&&!found;++dev){
  unsigned first=(bus<<8)|(dev<<3);if((pci9(first,0)&65535)==65535)continue;unsigned functions=(pci9(first,12)&0x800000)?8:1;
  for(unsigned f=0;f<functions;++f){unsigned bdf=first|f,id=pci9(bdf,0);if((pci9(bdf,8)>>24)!=3)continue;
   if(!graphics9.adapter_id){graphics9.adapter_bdf=bdf;graphics9.adapter_id=id;}
   if(id==0x040515ad){found=bdf+1;graphics9.adapter_bdf=bdf;graphics9.adapter_id=id;break;}
  }
 }
 if(!found)return;unsigned bdf=found-1,bar=pci9(bdf,16),fb=pci9(bdf,20),mem=pci9(bdf,24);
 if(!(bar&1)||(bar&~3u)>0xfff0||(fb&7)||(mem&7)||!(bar&~3u))return;
 svga9_io=(uint16_t)(bar&~3u);unsigned id=svga9_read(0);if(id!=0x90000002u&&id!=0x90000001u)return;
 uint64_t physical=(uint64_t)(fb&~15u)+svga9_read(14);unsigned size=svga9_read(19),base=mem&~15u;
 if(physical!=screen->framebuffer.physical_address||svga9_read(2)!=screen->framebuffer_width||svga9_read(3)!=screen->framebuffer_height||svga9_read(7)!=32||svga9_read(12)!=screen->framebuffer_stride_bytes||base<0x10000000||size<16384||size>16*1024*1024||(uint64_t)base+size>UINT64_C(0x100000000))return;
 graphics9.capabilities=svga9_read(17);fifo9=(volatile uint32_t*)(uintptr_t)base;
 if(!svga9_sync())return;
 svga9_write(20,0);unsigned limit=size&~3u;fifo9_limit=limit;
 fifo9[0]=4096;fifo9[1]=limit;fifo9[2]=fifo9[3]=4096;__asm__ volatile("mfence":::"memory");svga9_write(20,1);
 graphics9.backend=2;serial_text("M9_VMWARE_SVGA_FIFO_ACTIVE\n");
}
static void graphics9_begin(void){
 if(shadow9)return;uint64_t bytes=(uint64_t)screen->framebuffer_stride_bytes*screen->framebuffer_height;
 page_stats pages=pages_stats();uint64_t free=pages.total_pages-pages.current_pages;
 if(!pages.total_pages)return;
 unsigned count=(unsigned)((bytes+4095)/4096);
 if(bytes&&bytes<=32*1024*1024&&free>count+6144){
  uint64_t address=pages_alloc_run(count);if(address){shadow9=(uint32_t*)(uintptr_t)address;graphics9.shadow_bytes=(uint64_t)count*4096;graphics9.backend=1;
   memcpy(shadow9,(const void*)(uintptr_t)screen->framebuffer.physical_address,(size_t)bytes);svga9_begin();}
 }
 serial_text("M9_GRAPHICS backend=");serial_hex(graphics9.backend);serial_text(" adapter=");serial_hex(graphics9.adapter_id);serial_text(" shadow_bytes=");serial_hex(graphics9.shadow_bytes);serial_text("\n");
}
const graphics9_info *graphics9_current(void){return &graphics9;}
static volatile uint32_t *pixel9(unsigned x,unsigned y){
 return (volatile uint32_t*)(uintptr_t)((shadow9?(uint64_t)(uintptr_t)shadow9:screen->framebuffer.physical_address)+(uint64_t)y*screen->framebuffer_stride_bytes+(uint64_t)x*4);
}
void graphics9_present(unsigned x,unsigned y,unsigned w,unsigned h){
 if(!shadow9||x>=screen->framebuffer_width||y>=screen->framebuffer_height||!w||!h)return;
 if(w>screen->framebuffer_width-x)w=screen->framebuffer_width-x;if(h>screen->framebuffer_height-y)h=screen->framebuffer_height-y;
 for(unsigned row=0;row<h;++row){uint64_t offset=(uint64_t)(y+row)*screen->framebuffer_stride_bytes+(uint64_t)x*4;
  void*destination=(void*)(uintptr_t)(screen->framebuffer.physical_address+offset);const void*source=(const uint8_t*)shadow9+offset;size_t qwords=w/2;
  __asm__ volatile("cld; rep movsq":"+D"(destination),"+S"(source),"+c"(qwords)::"memory");
  if(w&1)*(volatile uint32_t*)destination=*(const uint32_t*)source;
 }
 __asm__ volatile("sfence":::"memory");++graphics9.presents;graphics9.presented_pixels+=(uint64_t)w*h;
 uint32_t words[]={1,x,y,w,h};if(svga9_command(words,5))++graphics9.fifo_updates;
}
void graphics9_hide_cursor(void){
 if(cursor9_visible){cursor9_visible=0;graphics9_present(cursor9_x,cursor9_y,cursor9_w,cursor9_h);}
}
void graphics9_flush(void){
 if(batch9)return;
 if(dirty9_active){dirty9_active=0;graphics9_present(dirty9_left,dirty9_top,dirty9_right-dirty9_left,dirty9_bottom-dirty9_top);}
}
void graphics9_begin_batch(void){graphics9_hide_cursor();++batch9;}
void graphics9_end_batch(void){if(batch9&&!--batch9)graphics9_flush();}
static void graphics9_cursor(unsigned x,unsigned y,unsigned scale){
 if(batch9)return;
 graphics9_hide_cursor();graphics9_flush();unsigned w=15*scale/2,h=24*scale/2;
 if(x+w>screen->framebuffer_width)x=screen->framebuffer_width-w;if(y+h>screen->framebuffer_height)y=screen->framebuffer_height-h;
 for(unsigned j=0;j<h;++j)for(unsigned i=0;i<w;++i){unsigned covered=0,edge=0;
  for(unsigned sy=0;sy<4;++sy)for(unsigned sx=0;sx<4;++sx){int lx=(int)(i*8+sx*2+1)*2/(int)scale,ly=(int)(j*8+sy*2+1)*2/(int)scale;
   if(lx>=8&&ly>=8&&lx<=ly/2+8&&ly<152){++covered;if(lx<16||lx>ly/2||ly>136)++edge;}}
  if(covered){unsigned alpha=covered*255/16;uint32_t fg=color(edge*2>=covered?0x173442:0xffffff),bg=*pixel9(x+i,y+j),value=0;
   for(unsigned shift=0;shift<24;shift+=8)value|=(((((fg>>shift)&255)*alpha+((bg>>shift)&255)*(255-alpha)+127)/255)<<shift);
   *(volatile uint32_t*)(uintptr_t)(screen->framebuffer.physical_address+(uint64_t)(y+j)*screen->framebuffer_stride_bytes+(x+i)*4)=value;
  }
 }
 __asm__ volatile("sfence":::"memory");uint32_t words[]={1,x,y,w,h};if(svga9_command(words,5))++graphics9.fifo_updates;
 cursor9_x=x;cursor9_y=y;cursor9_w=w;cursor9_h=h;cursor9_visible=1;
}
unsigned graphics9_move(unsigned sx,unsigned sy,unsigned dx,unsigned dy,unsigned w,unsigned h){
 if(!shadow9||!w||!h||sx>=screen->framebuffer_width||dx>=screen->framebuffer_width||sy>=screen->framebuffer_height||dy>=screen->framebuffer_height||w>screen->framebuffer_width-sx||w>screen->framebuffer_width-dx||h>screen->framebuffer_height-sy||h>screen->framebuffer_height-dy)return 0;
 graphics9_hide_cursor();
 for(unsigned row=0;row<h;++row){unsigned at=dy>sy?h-1-row:row;
  void*destination=(void*)pixel9(dx,dy+at);const void*source=(const void*)pixel9(sx,sy+at);size_t count=w;
  if((uintptr_t)destination>(uintptr_t)source&&(uintptr_t)destination<(uintptr_t)source+(uint64_t)w*4){
   destination=(uint8_t*)destination+(uint64_t)(w-1)*4;source=(const uint8_t*)source+(uint64_t)(w-1)*4;
   __asm__ volatile("std; rep movsl; cld":"+D"(destination),"+S"(source),"+c"(count)::"memory","cc");
  }else __asm__ volatile("cld; rep movsl":"+D"(destination),"+S"(source),"+c"(count)::"memory","cc");
 }
 if(batch9){graphics9_dirty(dx,dy,w,h);graphics9.copy_pixels+=(uint64_t)w*h;++graphics9.copy_frames;return 1;}
 unsigned copied=0;if(graphics9.capabilities&2){uint32_t words[]={3,sx,sy,dx,dy,w,h};copied=svga9_command(words,7);if(copied)++graphics9.fifo_copies;}
 if(!copied)graphics9_present(dx,dy,w,h);graphics9.copy_pixels+=(uint64_t)w*h;++graphics9.copy_frames;return 1;
}
