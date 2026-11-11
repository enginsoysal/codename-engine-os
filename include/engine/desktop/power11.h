#ifndef ENGACER_POWER11_H
#define ENGACER_POWER11_H
#include <stdint.h>
/* Static _S5 package reader for qualified virtual firmware; not an AML interpreter. */
static inline unsigned p11_integer(const uint8_t*p,unsigned end,unsigned*at,uint64_t*out){if(*at>=end)return 0;unsigned op=p[(*at)++],n=op==0x0a?1:op==0x0b?2:op==0x0c?4:op==0x0e?8:0;if(op==0||op==1){*out=op;return 1;}if(!n||n>end-*at)return 0;uint64_t value=0;for(unsigned i=0;i<n;++i)value|=(uint64_t)p[(*at)++]<<(8*i);*out=value;return 1;}
static inline unsigned p11_s5(const uint8_t*p,unsigned bytes,unsigned*a,unsigned*b){unsigned found=0;
 for(unsigned i=0;i+8<bytes;++i){if(p[i]!=8)continue;unsigned at=i+1;if(p[at]==0x5c)++at;if(at+6>=bytes||p[at]!='_'||p[at+1]!='S'||p[at+2]!='5'||p[at+3]!='_'||p[at+4]!=0x12)continue;at+=5;unsigned begin=at,follow=p[at]>>6,length=p[at++]&(follow?15:63);if(follow>bytes-at)return 0;for(unsigned k=0;k<follow;++k)length|=(unsigned)p[at++]<<(4+8*k);if(length>bytes-begin||length<at-begin+1)return 0;unsigned end=begin+length,count=p[at++];if(count<2||count>4)return 0;uint64_t x=0,y=0;if(!p11_integer(p,end,&at,&x)||!p11_integer(p,end,&at,&y)||x>7||y>7)return 0;for(unsigned k=2;k<count;++k){uint64_t reserved;if(!p11_integer(p,end,&at,&reserved)||reserved)return 0;}if(at!=end||++found>1)return 0;*a=(unsigned)x;*b=(unsigned)y;i=end-1;
 }return found==1;
}
#endif
