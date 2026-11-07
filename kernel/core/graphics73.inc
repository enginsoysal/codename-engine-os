/* Original clipped CPU compositor. Font coverage is generated from OFL Inter. */
#include <engine/ui/graphics73.h>
#include "font73.h"
static unsigned scale73=2,active73;
static unsigned choice73;
unsigned verbose73;
static os_boot_info_v0 view73;
unsigned ui73_scale(void){return scale73;}
unsigned ui73_scale_choice(void){return choice73;}
const char *ui73_font_license(void){return font73_license;}
const os_boot_info_v0 *ui73_set_scale(unsigned choice){
 unsigned wanted=choice?choice:screen->framebuffer_width>=3840&&screen->framebuffer_height>=2000?4:screen->framebuffer_width>=2560&&screen->framebuffer_height>=1400?3:2;
 if(wanted<2||wanted>4||screen->framebuffer_width*2/wanted<1024||screen->framebuffer_height*2/wanted<768)return 0;
 scale73=wanted;choice73=choice;view73.framebuffer_width=screen->framebuffer_width*2/wanted;view73.framebuffer_height=screen->framebuffer_height*2/wanted;gui6_repaint=1;
 serial_text("M73_SCALE_APPLIED choice=");serial_hex(choice);serial_text(" actual=");serial_hex(wanted);serial_text("\n");return &view73;
}
unsigned ui73_width(void){return screen->framebuffer_width;}
unsigned ui73_height(void){return screen->framebuffer_height;}
uint64_t ui73_framebuffer_bytes(void){return (uint64_t)screen->framebuffer_stride_bytes*screen->framebuffer_height;}
const os_boot_info_v0 *ui73_begin(const os_boot_info_v0*boot){
 scale73=boot->framebuffer_width>=3840&&boot->framebuffer_height>=2000?4:boot->framebuffer_width>=2560&&boot->framebuffer_height>=1400?3:2;
 view73=*boot;view73.framebuffer_width=boot->framebuffer_width*2/scale73;view73.framebuffer_height=boot->framebuffer_height*2/scale73;active73=1;
 serial_text("M73_DISPLAY physical=");serial_hex(boot->framebuffer_width);serial_text("x");serial_hex(boot->framebuffer_height);serial_text(" scale_half=");serial_hex(scale73);serial_text(" logical=");serial_hex(view73.framebuffer_width);serial_text("x");serial_hex(view73.framebuffer_height);serial_text("\n");
 return &view73;
}
static unsigned phys73(unsigned n){return n*scale73/2;}
static void blend73(unsigned x,unsigned y,uint32_t rgb,unsigned a,unsigned clipped){
 if(!a||x>=screen->framebuffer_width||y>=screen->framebuffer_height)return;
 if(clipped&&clip7_active&&(x<phys73(clip7_left)||x>=phys73(clip7_right)||y<phys73(clip7_top)||y>=phys73(clip7_bottom)))return;
 volatile uint32_t*p=(volatile uint32_t*)(uintptr_t)(screen->framebuffer.physical_address+(uint64_t)y*screen->framebuffer_stride_bytes+x*4);
 uint32_t fg=color(rgb);if(a==255){*p=fg;return;}uint32_t bg=*p;
 unsigned r=(((fg>>16)&255)*a+((bg>>16)&255)*(255-a)+127)/255;
 unsigned g=(((fg>>8)&255)*a+((bg>>8)&255)*(255-a)+127)/255;
 unsigned b=((fg&255)*a+(bg&255)*(255-a)+127)/255;*p=(r<<16)|(g<<8)|b;
}
static void rect73(unsigned x,unsigned y,unsigned w,unsigned h,uint32_t rgb){
 if(!x&&!y)gui6_repaint=1;
 if(x>=view73.framebuffer_width||y>=view73.framebuffer_height)return;
 if(w>view73.framebuffer_width-x)w=view73.framebuffer_width-x;if(h>view73.framebuffer_height-y)h=view73.framebuffer_height-y;
 unsigned right=x+w,bottom=y+h;
 if(clip7_active){if(x<clip7_left)x=clip7_left;if(y<clip7_top)y=clip7_top;if(right>clip7_right)right=clip7_right;if(bottom>clip7_bottom)bottom=clip7_bottom;}
 if(x>=right||y>=bottom)return;
 unsigned px=phys73(x),py=phys73(y),pr=phys73(right),pb=phys73(bottom);uint32_t value=color(rgb);
 for(unsigned j=py;j<pb;++j){volatile uint32_t*p=(volatile uint32_t*)(uintptr_t)(screen->framebuffer.physical_address+(uint64_t)j*screen->framebuffer_stride_bytes+px*4);for(unsigned i=px;i<pr;++i)*p++=value;}
}
static void round_alpha73(unsigned x,unsigned y,unsigned w,unsigned h,unsigned radius,uint32_t rgb,unsigned alpha){
 if(!w||!h)return;unsigned px=phys73(x),py=phys73(y),pw=phys73(x+w)-px,ph=phys73(y+h)-py,r=phys73(radius);
 if(r>pw/2)r=pw/2;if(r>ph/2)r=ph/2;
 unsigned l=px,t=py,right=px+pw,bottom=py+ph;
 if(clip7_active){if(l<phys73(clip7_left))l=phys73(clip7_left);if(t<phys73(clip7_top))t=phys73(clip7_top);if(right>phys73(clip7_right))right=phys73(clip7_right);if(bottom>phys73(clip7_bottom))bottom=phys73(clip7_bottom);}
 if(right>screen->framebuffer_width)right=screen->framebuffer_width;if(bottom>screen->framebuffer_height)bottom=screen->framebuffer_height;
 for(unsigned yy=t;yy<bottom;++yy)for(unsigned xx=l;xx<right;++xx){
 unsigned dx=xx-px,dy=yy-py;
 /* Shadow interiors are covered by the opaque surface: never blend them. */
 if(alpha<255&&dx>=r&&dx<pw-r&&dy>=r&&dy<ph-r)continue;
 if(!r||(dx>=r&&dx<pw-r)||(dy>=r&&dy<ph-r)){blend73(xx,yy,rgb,alpha,0);continue;}
 int cx=(int)(dx<r?r:pw-r),cy=(int)(dy<r?r:ph-r);unsigned covered=0;
 for(unsigned sy=0;sy<4;++sy)for(unsigned sx=0;sx<4;++sx){int vx=(int)dx*8+(int)sx*2+1-cx*8,vy=(int)dy*8+(int)sy*2+1-cy*8;if(vx*vx+vy*vy<=(int)(r*r*64))++covered;}
 blend73(xx,yy,rgb,(covered*alpha+8)/16,0);}
}
void ui73_round(unsigned x,unsigned y,unsigned w,unsigned h,unsigned r,uint32_t c){round_alpha73(x,y,w,h,r,c,255);}
void ui73_shadow(unsigned x,unsigned y,unsigned w,unsigned h){
 for(unsigned i=8;i;--i)if(x>=i&&y>=i)round_alpha73(x-i,y-i+4,w+2*i,h+2*i,12+i,0x081824,3);
}
static const font73_face *face73(unsigned role){
 unsigned logical=role>=4?32:role==3?24:16,actual=logical*scale73/2;
 unsigned index=actual==16?0:actual==24?1:actual==32?2:actual==36?3:actual==48?4:5;
 return &font73_faces[index+(role>=3?6:0)];
}
unsigned ui73_measure(const char*s,unsigned role){const font73_face*f=face73(role);unsigned width=0;for(unsigned n=0;s[n]&&n<1024;++n){unsigned c=(uint8_t)s[n];if(c<32||c>126)c='?';width+=font73_glyphs[f->first+c-32].advance;}return (width*2+scale73-1)/scale73;}
static void text73(unsigned x,unsigned y,const char*s,unsigned role,uint32_t rgb){
 const font73_face*f=face73(role);unsigned pen=phys73(x),baseline=phys73(y)+f->ascent;
 for(unsigned n=0;s[n]&&n<1024;++n){unsigned c=(uint8_t)s[n];if(c<32||c>126)c='?';const font73_glyph*g=&font73_glyphs[f->first+c-32];unsigned pixel_index=0;
 for(unsigned at=0;at<g->bytes;at+=2){unsigned count=font73_data[g->offset+at],alpha=font73_data[g->offset+at+1];
 for(unsigned i=0;i<count;++i,++pixel_index)if(alpha&&g->width){int dx=(int)pen+g->x+(int)(pixel_index%g->width),dy=(int)baseline+g->y+(int)(pixel_index/g->width);if(dx>=0&&dy>=0)blend73((unsigned)dx,(unsigned)dy,rgb,alpha,1);}}
 pen+=g->advance;}
}
void ui73_fit(unsigned x,unsigned y,unsigned width,const char*s,unsigned role,uint32_t rgb){
 char out[256];unsigned n=0;while(s[n]&&n<252){out[n]=s[n];out[n+1]=0;if(ui73_measure(out,role)>width)break;++n;}
 if(s[n]){while(n){out[n]=0;if(ui73_measure(out,role)+ui73_measure("...",role)<=width)break;--n;}out[n++]='.';out[n++]='.';out[n++]='.';}out[n]=0;text73(x,y,out,role,rgb);
}
unsigned ui73_paragraph(unsigned x,unsigned y,unsigned width,unsigned max_lines,const char*s,uint32_t rgb){
 unsigned at=0,line=0;while(s[at]&&line<max_lines){char out[256];unsigned n=0,last_space=0,start=at;
 while(s[at]&&s[at]!='\n'&&n<252){out[n++]=s[at++];out[n]=0;if(out[n-1]==' ')last_space=n;if(ui73_measure(out,1)>width){if(last_space){at=start+last_space;n=last_space-1;}else{--at;--n;}break;}}
 if(!n&&s[at]&&s[at]!='\n'){++at;continue;}out[n]=0;text73(x,y+line*24,out,1,rgb);++line;
 if(s[at]=='\n')++at;while(s[at]==' ')++at;}
 if(s[at]){serial_text("M73_TEXT_OVERFLOW\n");return 1;}return 0;
}
void ui73_folder(unsigned x,unsigned y,unsigned size,uint32_t tint){
 ui73_round(x,y,size/2,11,4,0xc3974e);ui73_round(x,y+6,size,size*3/4,5,tint);ui73_round(x+2,y+8,size-4,3,1,0xffe7b2);
}
static void cursor73(unsigned x,unsigned y){
 static uint32_t saved[32*48];static unsigned ox,oy,visible;
 unsigned w=phys73(15),h=phys73(24);if(gui6_repaint){visible=0;gui6_repaint=0;}
 if(visible)for(unsigned j=0;j<h;++j)for(unsigned i=0;i<w;++i){volatile uint32_t*p=(volatile uint32_t*)(uintptr_t)(screen->framebuffer.physical_address+(uint64_t)(oy+j)*screen->framebuffer_stride_bytes+(ox+i)*4);*p=saved[j*w+i];}
 if(x+w>screen->framebuffer_width)x=screen->framebuffer_width-w;if(y+h>screen->framebuffer_height)y=screen->framebuffer_height-h;
 for(unsigned j=0;j<h;++j)for(unsigned i=0;i<w;++i){volatile uint32_t*p=(volatile uint32_t*)(uintptr_t)(screen->framebuffer.physical_address+(uint64_t)(y+j)*screen->framebuffer_stride_bytes+(x+i)*4);saved[j*w+i]=*p;
 unsigned covered=0,edge=0;for(unsigned sy=0;sy<4;++sy)for(unsigned sx=0;sx<4;++sx){int lx=(int)(i*8+sx*2+1)*2/(int)scale73,ly=(int)(j*8+sy*2+1)*2/(int)scale73;
 if(lx>=8&&ly>=8&&lx<=ly/2+8&&ly<152){++covered;if(lx<16||lx>ly/2||ly>136)++edge;}}
 if(covered)blend73(x+i,y+j,edge*2>=covered?0x173442:0xffffff,covered*255/16,0);}
 ox=x;oy=y;visible=1;
}
