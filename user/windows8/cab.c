/* CAB container validation with an unmodified, digest-pinned libmspack LZX decoder. */
#include "cab.h"
#include <mspack.h>
#include <lzx.h>
typedef struct {uint8_t *bytes;unsigned size,at;} buffer8;
static uint16_t u16cab8(const uint8_t*p){return p[0]|(uint16_t)p[1]<<8;}
static uint32_t u32cab8(const uint8_t*p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void valid8(int valid,const char*message){if(!valid)fail8(message);}
static int read_cab8(struct mspack_file*opaque,void*out,int bytes){buffer8*f=(void*)opaque;if(bytes<0)return -1;unsigned n=(unsigned)bytes;if(n>f->size-f->at)n=f->size-f->at;memcpy(out,f->bytes+f->at,n);f->at+=n;return (int)n;}
static int write_cab8(struct mspack_file*opaque,void*data,int bytes){buffer8*f=(void*)opaque;if(bytes<0||(unsigned)bytes>f->size-f->at)return -1;memcpy(f->bytes+f->at,data,(unsigned)bytes);f->at+=(unsigned)bytes;return bytes;}
static void *alloc_cab8(struct mspack_system*sys,size_t bytes){(void)sys;return alloc8(bytes);}
static void copy_cab8(void*src,void*dest,size_t bytes){memcpy(dest,src,bytes);}
static uint32_t checksum8(const uint8_t*p,unsigned n){uint32_t sum=0;while(n>=4){sum^=u32cab8(p);p+=4;n-=4;}uint32_t tail=0;for(unsigned i=0;i<n;++i)tail|=(uint32_t)p[i]<<(8*(n-1-i));return sum^tail;}
cab8_archive *cab8_decode(msi8_blob cab){
 valid8(cab.data&&cab.size>=44&&u32cab8(cab.data)==0x4643534d,"MSI embedded cabinet is missing or invalid.");const uint8_t*p=cab.data;
 valid8(u32cab8(p+8)==cab.size&&p[24]==3&&p[25]==1&&u16cab8(p+26)==1&&!u16cab8(p+30),"Only a single-folder, self-contained CAB v1.3 is supported.");
 unsigned file_count=u16cab8(p+28),file_offset=u32cab8(p+16),data_offset=u32cab8(p+36),blocks=u16cab8(p+40),compression=u16cab8(p+42);
 valid8(file_count&&file_count<=256&&file_offset>=44&&file_offset<cab.size&&data_offset>=file_offset&&data_offset<cab.size&&blocks&&blocks<=512,"CAB directory or folder bounds are invalid.");
 valid8((compression&15)==3&&!(compression&0xe0f0)&&((compression>>8)&31)>=15&&((compression>>8)&31)<=21,"CAB compression is outside the supported LZX profile.");
 cab8_archive*a=calloc8(1,sizeof(*a));valid8(a!=0,"Not enough memory for cabinet metadata.");a->count=file_count;unsigned at=file_offset,total=0;
 for(unsigned i=0;i<file_count;++i){valid8(at+16<=data_offset,"CAB file record is truncated.");cab8_file*f=a->files+i;f->size=u32cab8(p+at);f->offset=u32cab8(p+at+4);valid8(!u16cab8(p+at+8),"CAB spanning folders are unsupported.");f->date=u16cab8(p+at+10);f->time=u16cab8(p+at+12);f->attributes=u16cab8(p+at+14);at+=16;
  unsigned n=0;while(at<data_offset&&p[at]){valid8(n+1<sizeof(f->name)&&p[at]>=32&&p[at]<127&&p[at]!='/'&&p[at]!='\\',"CAB file identifier is unsupported.");f->name[n++]=(char)p[at++];}valid8(at<data_offset&&n,"CAB file identifier is missing.");++at;f->name[n]=0;
  valid8(f->offset==total&&f->size<=8*1024*1024-total,"CAB file offsets overlap or exceed the expansion limit.");total+=f->size;for(unsigned j=0;j<i;++j)valid8(!equal8(f->name,a->files[j].name),"CAB file identifiers are duplicated.");
 }
 valid8(at<=data_offset&&total,"CAB file data is empty.");buffer8 input={alloc8(cab.size),cab.size,0};valid8(input.bytes!=0,"Not enough memory for cabinet input.");unsigned compressed=0,uncompressed=0;at=data_offset;
 for(unsigned i=0;i<blocks;++i){valid8(at+8<=cab.size,"CAB data block is truncated.");unsigned checksum=u32cab8(p+at),packed=u16cab8(p+at+4),plain=u16cab8(p+at+6);valid8(packed&&plain&&plain<=32768&&at+8+packed<=cab.size&&packed<=cab.size-compressed,"CAB data block length is invalid.");
  valid8(!checksum||checksum==(checksum8(p+at+4,4)^checksum8(p+at+8,packed)),"CAB data checksum failed.");memcpy(input.bytes+compressed,p+at+8,packed);compressed+=packed;valid8(plain<=total-uncompressed,"CAB decompressed size exceeds its file directory.");uncompressed+=plain;at+=8+packed;
 }
 valid8(at==cab.size&&uncompressed==total,"CAB data length does not match its directory.");input.size=compressed;
 a->expanded=(msi8_blob){alloc8(total),total};valid8(a->expanded.data!=0,"Not enough memory for cabinet output.");buffer8 output={a->expanded.data,total,0};
 struct mspack_system system={.read=read_cab8,.write=write_cab8,.alloc=alloc_cab8,.free=free8,.copy=copy_cab8};
 struct lzxd_stream*lzx=lzxd_init(&system,(void*)&input,(void*)&output,(int)((compression>>8)&31),0,4096,total,0);valid8(lzx!=0,"LZX decoder allocation failed.");
 int status=lzxd_decompress(lzx,total);lzxd_free(lzx);free8(input.bytes);valid8(!status&&output.at==total,"LZX cabinet decompression failed.");return a;
}
