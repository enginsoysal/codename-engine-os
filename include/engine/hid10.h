#ifndef ENGINE_HID10_H
#define ENGINE_HID10_H
#include <stdint.h>
/* Admit a report wheel only when descriptor bits prove the familiar boot
 * button/X/Y layout. Report IDs and other layouts retain boot-protocol input. */
static inline unsigned hid10_boot_wheel(const uint8_t*data,unsigned length){
 unsigned page=0,size=0,count=0,bits=0,usage[8],usages=0,minimum=0,buttons=0,x=0,y=0,wheel=0;
 if(!data||length>256)return 0;
 for(unsigned at=0;at<length;){unsigned prefix=data[at++],n=prefix&3;if(n==3)n=4;if(prefix==254||n>length-at)return 0;unsigned value=0;for(unsigned j=0;j<n;++j)value|=(unsigned)data[at++]<<(8*j);unsigned type=(prefix>>2)&3,tag=prefix>>4;
  if(type==1){if(tag==0)page=value;else if(tag==7)size=value;else if(tag==9)count=value;else if(tag==8||tag==10||tag==11)return 0;}
  else if(type==2){if(tag==0){if(usages==8)return 0;usage[usages++]=value;}else if(tag==1)minimum=value;}
  else if(type==0){if(tag==8){if(!size||size>32||!count||count>32||bits>64||size*count>64-bits)return 0;
    if(!(value&1)){if(page==9&&bits==0&&size==1&&count<=8&&minimum==1)buttons=1;
     if(page==1&&size==8&&(value&6)==6)for(unsigned i=0;i<count&&i<usages;++i){unsigned where=bits+i*size;if(usage[i]==0x30)x=where==8;if(usage[i]==0x31)y=where==16;if(usage[i]==0x38)wheel=where==24;}}
    bits+=size*count;
   }usages=minimum=0;
  }
 }return buttons&&x&&y&&wheel&&bits>=32;
}
#endif
