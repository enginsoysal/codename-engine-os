#include <engine/storage/volume6.h>
#ifdef ENGINE_M74
#include <engine/audio74.h>
#endif
/* Copy-on-write extent volume. Native little-endian x64 format, version 1. */
_Static_assert(sizeof(v6_entry)==280,"disk entry layout");
#define TABLE_BYTES (V6_ENTRIES*sizeof(v6_entry))
#define TABLE_PAGES ((TABLE_BYTES+V6_PAGE-1)/V6_PAGE)
#define BANK_PAGES (1+TABLE_PAGES)
#ifdef ENGINE_M7
static struct {
 v6_volume *volume;uint64_t first,pages,size,limit;uint32_t sum;
 unsigned used;char path[256];uint8_t page[V6_PAGE];
#ifdef ENGINE_M11
 uint64_t verified;uint32_t verify_sum;unsigned sealed,verified_done;
#endif
} stream7;
#endif
_Static_assert(2*BANK_PAGES<=V6_DATA_START,"metadata banks must fit before file data");
static void copy(void *to,const void *from,unsigned n){uint8_t *d=to;const uint8_t *s=from;while(n--)*d++=*s++;}
static void zero(void *p,unsigned n){uint8_t *d=p;while(n--)*d++=0;}
static int equal(const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
static uint64_t r64(const void*p){uint64_t v;copy(&v,p,8);return v;}
static uint32_t r32(const void*p){uint32_t v;copy(&v,p,4);return v;}
static void w64(void*p,uint64_t v){copy(p,&v,8);}
static void w32(void*p,uint32_t v){copy(p,&v,4);}
#ifdef ENGINE_M11
/* IEEE CRC table: unchanged disk format, one KiB of immutable lookup data. */
static const uint32_t crc_table11[256]={
0x00000000u,0x77073096u,0xee0e612cu,0x990951bau,0x076dc419u,0x706af48fu,0xe963a535u,0x9e6495a3u,
0x0edb8832u,0x79dcb8a4u,0xe0d5e91eu,0x97d2d988u,0x09b64c2bu,0x7eb17cbdu,0xe7b82d07u,0x90bf1d91u,
0x1db71064u,0x6ab020f2u,0xf3b97148u,0x84be41deu,0x1adad47du,0x6ddde4ebu,0xf4d4b551u,0x83d385c7u,
0x136c9856u,0x646ba8c0u,0xfd62f97au,0x8a65c9ecu,0x14015c4fu,0x63066cd9u,0xfa0f3d63u,0x8d080df5u,
0x3b6e20c8u,0x4c69105eu,0xd56041e4u,0xa2677172u,0x3c03e4d1u,0x4b04d447u,0xd20d85fdu,0xa50ab56bu,
0x35b5a8fau,0x42b2986cu,0xdbbbc9d6u,0xacbcf940u,0x32d86ce3u,0x45df5c75u,0xdcd60dcfu,0xabd13d59u,
0x26d930acu,0x51de003au,0xc8d75180u,0xbfd06116u,0x21b4f4b5u,0x56b3c423u,0xcfba9599u,0xb8bda50fu,
0x2802b89eu,0x5f058808u,0xc60cd9b2u,0xb10be924u,0x2f6f7c87u,0x58684c11u,0xc1611dabu,0xb6662d3du,
0x76dc4190u,0x01db7106u,0x98d220bcu,0xefd5102au,0x71b18589u,0x06b6b51fu,0x9fbfe4a5u,0xe8b8d433u,
0x7807c9a2u,0x0f00f934u,0x9609a88eu,0xe10e9818u,0x7f6a0dbbu,0x086d3d2du,0x91646c97u,0xe6635c01u,
0x6b6b51f4u,0x1c6c6162u,0x856530d8u,0xf262004eu,0x6c0695edu,0x1b01a57bu,0x8208f4c1u,0xf50fc457u,
0x65b0d9c6u,0x12b7e950u,0x8bbeb8eau,0xfcb9887cu,0x62dd1ddfu,0x15da2d49u,0x8cd37cf3u,0xfbd44c65u,
0x4db26158u,0x3ab551ceu,0xa3bc0074u,0xd4bb30e2u,0x4adfa541u,0x3dd895d7u,0xa4d1c46du,0xd3d6f4fbu,
0x4369e96au,0x346ed9fcu,0xad678846u,0xda60b8d0u,0x44042d73u,0x33031de5u,0xaa0a4c5fu,0xdd0d7cc9u,
0x5005713cu,0x270241aau,0xbe0b1010u,0xc90c2086u,0x5768b525u,0x206f85b3u,0xb966d409u,0xce61e49fu,
0x5edef90eu,0x29d9c998u,0xb0d09822u,0xc7d7a8b4u,0x59b33d17u,0x2eb40d81u,0xb7bd5c3bu,0xc0ba6cadu,
0xedb88320u,0x9abfb3b6u,0x03b6e20cu,0x74b1d29au,0xead54739u,0x9dd277afu,0x04db2615u,0x73dc1683u,
0xe3630b12u,0x94643b84u,0x0d6d6a3eu,0x7a6a5aa8u,0xe40ecf0bu,0x9309ff9du,0x0a00ae27u,0x7d079eb1u,
0xf00f9344u,0x8708a3d2u,0x1e01f268u,0x6906c2feu,0xf762575du,0x806567cbu,0x196c3671u,0x6e6b06e7u,
0xfed41b76u,0x89d32be0u,0x10da7a5au,0x67dd4accu,0xf9b9df6fu,0x8ebeeff9u,0x17b7be43u,0x60b08ed5u,
0xd6d6a3e8u,0xa1d1937eu,0x38d8c2c4u,0x4fdff252u,0xd1bb67f1u,0xa6bc5767u,0x3fb506ddu,0x48b2364bu,
0xd80d2bdau,0xaf0a1b4cu,0x36034af6u,0x41047a60u,0xdf60efc3u,0xa867df55u,0x316e8eefu,0x4669be79u,
0xcb61b38cu,0xbc66831au,0x256fd2a0u,0x5268e236u,0xcc0c7795u,0xbb0b4703u,0x220216b9u,0x5505262fu,
0xc5ba3bbeu,0xb2bd0b28u,0x2bb45a92u,0x5cb36a04u,0xc2d7ffa7u,0xb5d0cf31u,0x2cd99e8bu,0x5bdeae1du,
0x9b64c2b0u,0xec63f226u,0x756aa39cu,0x026d930au,0x9c0906a9u,0xeb0e363fu,0x72076785u,0x05005713u,
0x95bf4a82u,0xe2b87a14u,0x7bb12baeu,0x0cb61b38u,0x92d28e9bu,0xe5d5be0du,0x7cdcefb7u,0x0bdbdf21u,
0x86d3d2d4u,0xf1d4e242u,0x68ddb3f8u,0x1fda836eu,0x81be16cdu,0xf6b9265bu,0x6fb077e1u,0x18b74777u,
0x88085ae6u,0xff0f6a70u,0x66063bcau,0x11010b5cu,0x8f659effu,0xf862ae69u,0x616bffd3u,0x166ccf45u,
0xa00ae278u,0xd70dd2eeu,0x4e048354u,0x3903b3c2u,0xa7672661u,0xd06016f7u,0x4969474du,0x3e6e77dbu,
0xaed16a4au,0xd9d65adcu,0x40df0b66u,0x37d83bf0u,0xa9bcae53u,0xdebb9ec5u,0x47b2cf7fu,0x30b5ffe9u,
0xbdbdf21cu,0xcabac28au,0x53b39330u,0x24b4a3a6u,0xbad03605u,0xcdd70693u,0x54de5729u,0x23d967bfu,
0xb3667a2eu,0xc4614ab8u,0x5d681b02u,0x2a6f2b94u,0xb40bbe37u,0xc30c8ea1u,0x5a05df1bu,0x2d02ef8du
};
static uint32_t crc_next(uint32_t c,const void*p,unsigned n){const uint8_t*b=p;while(n--)c=crc_table11[(c^*b++)&255]^(c>>8);return c;}
#else
static uint32_t crc_next(uint32_t c,const void *p,unsigned n){const uint8_t*b=p;while(n--){c^=*b++;for(unsigned j=0;j<8;++j)c=(c>>1)^(0xedb88320u&-(c&1));}return c;}
#endif
static uint32_t crc(const void*p,unsigned n){return ~crc_next(~0u,p,n);}
static uint64_t count(uint64_t size){return size/V6_PAGE+(size%V6_PAGE!=0);}
static unsigned valid_path(const char*p){
 if(!p||p[0]!='/'||!p[1])return 0;unsigned start=1;
 for(unsigned i=1;i<256;++i){unsigned c=(uint8_t)p[i];if(c&&c<32)return 0;if(c=='\\'||c==127)return 0;
  if(!c||c=='/'){unsigned n=i-start;if(!n||n>63||(n==1&&p[start]=='.')||(n==2&&p[start]=='.'&&p[start+1]=='.'))return 0;if(!c)return 1;start=i+1;}
 }return 0;
}
static int find(const v6_entry*t,const char*p){for(unsigned i=0;i<V6_ENTRIES;++i)if(t[i].kind&&equal(t[i].path,p))return (int)i;return -1;}
static unsigned parent_exists(v6_volume*v,const char*p){char b[256];unsigned last=0,i=0;for(;p[i];++i){b[i]=p[i];if(p[i]=='/')last=i;}if(!last)return 1;b[last]=0;int n=find(v->entries,b);return n>=0&&v->entries[n].kind==2;}
static unsigned validate(v6_volume*v,const v6_entry*t){
 for(unsigned i=0;i<V6_ENTRIES;++i){const v6_entry *e=t+i;if(!e->kind){const uint8_t *b=(const uint8_t*)e;for(unsigned k=0;k<sizeof(*e);++k)if(b[k])return V6_INVALID;continue;}
  if((e->kind!=1&&e->kind!=2)||!valid_path(e->path))return V6_INVALID;
  uint64_t n=count(e->size);if(e->kind==2&&(e->first||e->size||e->crc))return V6_INVALID;
  if(e->kind==1&&((!n&&(e->first||e->crc))||(n&&(e->first<V6_DATA_START||e->first>=v->blocks||n>v->blocks-e->first))))return V6_INVALID;
  for(unsigned j=0;j<i;++j){const v6_entry *b=t+j;if(!b->kind)continue;if(equal(e->path,b->path))return V6_INVALID;uint64_t m=count(b->size);if(n&&m&&e->first<b->first+m&&b->first<e->first+n)return V6_INVALID;}
  char p[256];unsigned last=0;for(unsigned j=0;e->path[j];++j){p[j]=e->path[j];if(e->path[j]=='/')last=j;}
  if(last){p[last]=0;int parent=find(t,p);if(parent<0||t[parent].kind!=2)return V6_INVALID;}
 }return V6_OK;
}
static unsigned read_bank(v6_volume*v,unsigned bank,v6_entry*t,uint64_t*gen){
 if(v->read(v->context,bank*BANK_PAGES,v->page,V6_PAGE))return V6_IO;
 const char *magic="ENGINEV6";for(unsigned i=0;i<8;++i)if(v->page[i]!=(uint8_t)magic[i])return V6_INVALID;
 if(r32(v->page+8)!=1||r32(v->page+12)!=sizeof(v6_entry)||r64(v->page+16)!=v->blocks||!r64(v->page+24)||r32(v->page+32)!=V6_ENTRIES||r32(v->page+4092)!=crc(v->page,4092))return V6_INVALID;
 *gen=r64(v->page+24);uint32_t expected=r32(v->page+36);unsigned offset=0;
 for(unsigned i=0;i<TABLE_PAGES;++i){if(v->read(v->context,bank*BANK_PAGES+1+i,v->page,V6_PAGE))return V6_IO;unsigned bytes=TABLE_BYTES-offset;if(bytes>V6_PAGE)bytes=V6_PAGE;copy((uint8_t*)t+offset,v->page,bytes);offset+=bytes;}
 if(crc(t,TABLE_BYTES)!=expected)return V6_INVALID;return validate(v,t);
}
unsigned v6_mount(v6_volume*v){
 v->mounted=0;v->readonly=1;v->previous_valid=0;
 if(!v->read||!v->write||!v->flush||v->blocks<V6_DATA_START+1)return V6_INVALID;
 uint64_t a=0,b=0;unsigned sa=read_bank(v,0,v->entries,&a),sb=read_bank(v,1,v->previous,&b);
 if(sa&&sb)return sa==V6_IO||sb==V6_IO?V6_IO:V6_INVALID;
 if(!sa&&!sb&&a==b)return V6_INVALID;
 if(sa||(!sb&&b>a)){copy(v->pending,v->entries,TABLE_BYTES);copy(v->entries,v->previous,TABLE_BYTES);copy(v->previous,v->pending,TABLE_BYTES);v->active=1;v->generation=b;v->previous_valid=!sa;}
 else{v->active=0;v->generation=a;v->previous_valid=!sb;}
 v->mounted=1;v->readonly=0;return V6_OK;
}
static unsigned commit(v6_volume*v){
#ifdef ENGINE_M11
 uint32_t requested_crc11=crc(v->pending,TABLE_BYTES);
#endif
 if(v->generation==UINT64_MAX)return V6_LIMIT;unsigned bank=v->active^1,offset=0;
 for(unsigned i=0;i<TABLE_PAGES;++i){zero(v->page,V6_PAGE);unsigned bytes=TABLE_BYTES-offset;if(bytes>V6_PAGE)bytes=V6_PAGE;copy(v->page,(uint8_t*)v->pending+offset,bytes);offset+=bytes;if(v->write(v->context,bank*BANK_PAGES+1+i,v->page,V6_PAGE)){v->readonly=1;return V6_IO;}}
 if(v->flush(v->context)){v->readonly=1;return V6_IO;}
 zero(v->page,V6_PAGE);copy(v->page,"ENGINEV6",8);w32(v->page+8,1);w32(v->page+12,sizeof(v6_entry));w64(v->page+16,v->blocks);w64(v->page+24,v->generation+1);w32(v->page+32,V6_ENTRIES);w32(v->page+36,crc(v->pending,TABLE_BYTES));w32(v->page+4092,crc(v->page,4092));
 if(v->write(v->context,bank*BANK_PAGES,v->page,V6_PAGE)||v->flush(v->context)){v->readonly=1;return V6_IO;}
#ifdef ENGINE_M11
 uint64_t confirmed_generation11=0;
 if(read_bank(v,bank,v->pending,&confirmed_generation11)||confirmed_generation11!=v->generation+1||crc(v->pending,TABLE_BYTES)!=requested_crc11){v->readonly=1;return V6_IO;}
#endif
 copy(v->previous,v->entries,TABLE_BYTES);copy(v->entries,v->pending,TABLE_BYTES);v->previous_valid=1;v->active=bank;++v->generation;return V6_OK;
}
unsigned v6_format(v6_volume*v){
 if(!v->read||!v->write||!v->flush||v->blocks<V6_DATA_START+1)return V6_INVALID;
 v->mounted=0;v->readonly=1;zero(v->page,V6_PAGE);
 /* Caller owns destructive confirmation. Invalidate both banks before format. */
 if(v->write(v->context,0,v->page,V6_PAGE)||v->write(v->context,BANK_PAGES,v->page,V6_PAGE)||v->flush(v->context))return V6_IO;
 zero(v->entries,TABLE_BYTES);zero(v->previous,TABLE_BYTES);zero(v->pending,TABLE_BYTES);v->generation=0;v->active=1;v->previous_valid=0;
 unsigned s=commit(v);if(s)return s;v->mounted=1;v->readonly=0;return V6_OK;
}
static unsigned prepare(v6_volume*v,const char*p,int*index){
 if(!v->mounted||v->readonly)return V6_RO;if(!valid_path(p)||!parent_exists(v,p))return V6_INVALID;
 *index=find(v->entries,p);if(*index<0)for(unsigned i=0;i<V6_ENTRIES;++i)if(!v->entries[i].kind){*index=(int)i;break;}
 if(*index<0)return V6_FULL;copy(v->pending,v->entries,TABLE_BYTES);return V6_OK;
}
unsigned v6_mkdir(v6_volume*v,const char*p){int n;unsigned s=prepare(v,p,&n);if(s)return s;if(v->entries[n].kind)return V6_EXISTS;v6_entry *e=v->pending+n;zero(e,sizeof(*e));unsigned i=0;do{e->path[i]=p[i];}while(p[i++]);e->kind=2;return commit(v);}
static uint64_t allocate(v6_volume*v,uint64_t n){
 uint64_t first=V6_DATA_START;
 /* Each conflict advances past an extent; at most two tables of extents. */
 for(unsigned attempts=0;attempts<=V6_ENTRIES*2;++attempts){if(first>v->blocks||n>v->blocks-first)return 0;uint64_t next=first;
#ifdef ENGINE_M7
  if(stream7.volume==v&&first<stream7.first+stream7.pages&&stream7.first<first+n)next=stream7.first+stream7.pages;
#endif
  for(unsigned bank=0;bank<1+v->previous_valid;++bank){const v6_entry*t=bank?v->previous:v->entries;for(unsigned i=0;i<V6_ENTRIES;++i){uint64_t m=count(t[i].size);if(m&&first<t[i].first+m&&t[i].first<first+n&&next<t[i].first+m)next=t[i].first+m;}}
  if(next==first)return first;first=next;
 }return 0;
}
unsigned v6_put(v6_volume*v,const char*p,uint64_t size,v6_source source,void*ctx){
 int n;unsigned s=prepare(v,p,&n);if(s)return s;if(v->entries[n].kind==2||(!source&&size))return V6_INVALID;
 uint64_t pages=count(size);if(pages>v->blocks-V6_DATA_START)return V6_FULL;uint64_t first=pages?allocate(v,pages):0;if(pages&&!first)return V6_FULL;
 uint32_t sum=~0u;for(uint64_t i=0;i<pages;++i){
#ifdef ENGINE_M74
 audio74_pump();
#endif
 unsigned bytes=(unsigned)(size-i*V6_PAGE>V6_PAGE?V6_PAGE:size-i*V6_PAGE);zero(v->page,V6_PAGE);if(source(ctx,i*V6_PAGE,v->page,bytes))return V6_IO;sum=crc_next(sum,v->page,bytes);if(v->write(v->context,first+i,v->page,V6_PAGE)){v->readonly=1;return V6_IO;}}
 if(v->flush(v->context)){v->readonly=1;return V6_IO;}
#ifdef ENGINE_M11
 uint32_t check11=~0u;for(uint64_t at=0;at<pages;++at){if(v->read(v->context,first+at,v->page,V6_PAGE)){v->readonly=1;return V6_IO;}unsigned bytes=(unsigned)(size-at*V6_PAGE>V6_PAGE?V6_PAGE:size-at*V6_PAGE);check11=crc_next(check11,v->page,bytes);}if(check11!=sum){v->readonly=1;return V6_IO;}
#endif
 v6_entry *e=v->pending+n;zero(e,sizeof(*e));unsigned i=0;do{e->path[i]=p[i];}while(p[i++]);e->first=first;e->size=size;e->crc=~sum;e->kind=1;return commit(v);
}
unsigned v6_read(v6_volume*v,const char*p,uint64_t off,void*data,unsigned cap,unsigned*bytes){
 if(!bytes||!v->mounted||!valid_path(p)||(!data&&cap))return V6_INVALID;*bytes=0;int n=find(v->entries,p);if(n<0)return V6_NOTFOUND;const v6_entry *e=v->entries+n;if(e->kind!=1||off>e->size)return V6_INVALID;
 if(e->size-off<cap)cap=(unsigned)(e->size-off);while(*bytes<cap){uint64_t pos=off+*bytes;unsigned at=(unsigned)(pos%V6_PAGE),length=V6_PAGE-at;if(length>cap-*bytes)length=cap-*bytes;if(v->read(v->context,e->first+pos/V6_PAGE,v->page,V6_PAGE))return V6_IO;copy((uint8_t*)data+*bytes,v->page+at,length);*bytes+=length;}return V6_OK;
}
unsigned v6_verify(v6_volume*v,const char*p){if(!v->mounted||!valid_path(p))return V6_INVALID;int n=find(v->entries,p);if(n<0)return V6_NOTFOUND;const v6_entry *e=v->entries+n;if(e->kind!=1)return V6_INVALID;uint32_t sum=~0u;
 for(uint64_t i=0;i<count(e->size);++i){
#ifdef ENGINE_M74
 audio74_pump();
#endif
 if(v->read(v->context,e->first+i,v->page,V6_PAGE))return V6_IO;unsigned bytes=(unsigned)(e->size-i*V6_PAGE>V6_PAGE?V6_PAGE:e->size-i*V6_PAGE);sum=crc_next(sum,v->page,bytes);}return ~sum==e->crc?V6_OK:V6_INVALID;
}
unsigned v6_remove(v6_volume*v,const char*p){int n;unsigned s=prepare(v,p,&n);if(s)return s;if(!v->entries[n].kind)return V6_NOTFOUND;if(v->entries[n].kind==2){unsigned len=0;while(p[len])++len;for(unsigned i=0;i<V6_ENTRIES;++i)if(v->entries[i].kind){unsigned j=0;while(j<len&&v->entries[i].path[j]==p[j])++j;if(j==len&&v->entries[i].path[j]=='/')return V6_INVALID;}}
 zero(v->pending+n,sizeof(v6_entry));return commit(v);
}
uint64_t v6_capacity(const v6_volume*v){return v->blocks>=V6_DATA_START?(v->blocks-V6_DATA_START)*V6_PAGE:0;}
#ifdef ENGINE_M8
unsigned v6_rename(v6_volume*v,const char*source,const char*target,unsigned replace){
 if(!valid_path(source)||!valid_path(target)||replace>1)return V6_INVALID;
 int from;unsigned s=prepare(v,source,&from);if(s)return s;if(!v->entries[from].kind)return V6_NOTFOUND;
 if(equal(source,target))return V6_OK;if(v->entries[from].kind!=1||!parent_exists(v,target))return V6_INVALID;
 int to=find(v->entries,target);if(to>=0&&(!replace||v->entries[to].kind!=1))return V6_EXISTS;
 if(to>=0)zero(v->pending+to,sizeof(v6_entry));zero(v->pending[from].path,256);unsigned i=0;do{v->pending[from].path[i]=target[i];}while(target[i++]);
 return commit(v);
}
#endif
uint64_t v6_used(const v6_volume*v){uint64_t n=0;for(unsigned i=0;i<V6_ENTRIES;++i)n+=count(v->entries[i].size)*V6_PAGE;return n;}
#ifdef ENGINE_M74
unsigned v6_read_scratch(v6_volume *v,const char *p,uint64_t off,void *data,unsigned cap,unsigned *bytes,void *scratch){
 if(!v||!bytes||!scratch||data==scratch||!v->mounted||!valid_path(p)||(!data&&cap))return V6_INVALID;*bytes=0;
 int n=find(v->entries,p);if(n<0)return V6_NOTFOUND;const v6_entry *e=v->entries+n;if(e->kind!=1||off>e->size)return V6_INVALID;
 if(e->size-off<cap)cap=(unsigned)(e->size-off);
 while(*bytes<cap){uint64_t pos=off+*bytes;unsigned at=(unsigned)(pos%V6_PAGE),length=V6_PAGE-at;if(length>cap-*bytes)length=cap-*bytes;
  if(v->read(v->context,e->first+pos/V6_PAGE,scratch,V6_PAGE))return V6_IO;copy((uint8_t *)data+*bytes,(uint8_t *)scratch+at,length);*bytes+=length;
 }return V6_OK;
}
#endif
#ifdef ENGINE_M7
void v6_stream_abort(v6_volume*v){if(stream7.volume==v)zero(&stream7,sizeof(stream7));}
unsigned v6_stream_begin(v6_volume*v,const char*p,uint64_t limit){
 if(stream7.volume||!limit||limit>64u*1024*1024)return V6_LIMIT;
 int n;unsigned s=prepare(v,p,&n);if(s)return s;if(v->entries[n].kind)return V6_EXISTS;
 uint64_t pages=count(limit),first=allocate(v,pages);if(!first)return V6_FULL;
 zero(&stream7,sizeof(stream7));stream7.volume=v;stream7.first=first;stream7.pages=pages;stream7.limit=limit;stream7.sum=~0u;
 unsigned i=0;do{stream7.path[i]=p[i];}while(p[i++]);return V6_OK;
}
unsigned v6_stream_append(v6_volume*v,const void*data,unsigned bytes){
#ifdef ENGINE_M74
 audio74_pump();
#endif
#ifdef ENGINE_M11
 if(stream7.sealed)return V6_INVALID;
#endif
 if(stream7.volume!=v||(!data&&bytes)||bytes>V6_PAGE-stream7.used||bytes>stream7.limit-stream7.size)return V6_INVALID;
 if(v->readonly){v6_stream_abort(v);return V6_RO;}
 copy(stream7.page+stream7.used,data,bytes);stream7.used+=bytes;stream7.size+=bytes;stream7.sum=crc_next(stream7.sum,data,bytes);
 if(stream7.used==V6_PAGE){
  if(v->write(v->context,stream7.first+(stream7.size-1)/V6_PAGE,stream7.page,V6_PAGE)){v->readonly=1;v6_stream_abort(v);return V6_IO;}
  stream7.used=0;zero(stream7.page,V6_PAGE);
 }return V6_OK;
}
unsigned v6_stream_finish(v6_volume*v){
 if(stream7.volume!=v)return V6_INVALID;
 int n;unsigned s=prepare(v,stream7.path,&n);
 if(!s&&v->entries[n].kind)s=V6_EXISTS;
#ifdef ENGINE_M11
 if(!s&&!stream7.sealed){s=v6_stream_seal11(v);unsigned done11=0;while(!s&&!done11)s=v6_stream_check11(v,&done11);}
 if(!s&&!stream7.verified_done)s=V6_INVALID;
#else
 if(!s&&stream7.used&&v->write(v->context,stream7.first+stream7.size/V6_PAGE,stream7.page,V6_PAGE)){v->readonly=1;s=V6_IO;}
 if(!s&&v->flush(v->context)){v->readonly=1;s=V6_IO;}
#endif
 if(!s){v6_entry *e=v->pending+n;zero(e,sizeof(*e));copy(e->path,stream7.path,256);
  e->kind=1;e->first=stream7.size?stream7.first:0;e->size=stream7.size;e->crc=~stream7.sum;s=commit(v);}
 v6_stream_abort(v);return s;
}
#endif

#ifdef ENGINE_M11
static unsigned within11(const char*path,const char*root){unsigned n=0;while(root[n]&&path[n]==root[n])++n;return !root[n]&&(!path[n]||path[n]=='/');}
unsigned v6_move_tree11(v6_volume*v,const char*source,const char*target,unsigned replace){
 if(!valid_path(source)||!valid_path(target)||replace>1)return V6_INVALID;
 int from;unsigned status=prepare(v,source,&from);if(status)return status;if(!v->entries[from].kind)return V6_NOTFOUND;
 if(equal(source,target))return V6_OK;
 if(v->entries[from].kind==1)return v6_rename(v,source,target,replace);
 if(within11(target,source)||!parent_exists(v,target))return V6_INVALID;
 if(find(v->entries,target)>=0)return V6_EXISTS;
 unsigned old_length=0,new_length=0;while(source[old_length])++old_length;while(target[new_length])++new_length;
 for(unsigned i=0;i<V6_ENTRIES;++i){const v6_entry*e=v->entries+i;if(!e->kind||!within11(e->path,source))continue;
  unsigned size=0;while(e->path[size])++size;if(new_length+size-old_length>=256)return V6_LIMIT;
  char changed[256];copy(changed,target,new_length);copy(changed+new_length,e->path+old_length,size-old_length+1);
  if(!valid_path(changed))return V6_LIMIT;zero(v->pending[i].path,256);copy(v->pending[i].path,changed,new_length+size-old_length+1);
 }
 if(validate(v,v->pending))return V6_INVALID;return commit(v);
}
unsigned v6_remove_tree11(v6_volume*v,const char*source){
 int from;unsigned status=prepare(v,source,&from);if(status)return status;if(!v->entries[from].kind)return V6_NOTFOUND;
 for(unsigned i=0;i<V6_ENTRIES;++i)if(v->entries[i].kind&&within11(v->entries[i].path,source))zero(v->pending+i,sizeof(v6_entry));
 return commit(v);
}
unsigned v6_stream_seal11(v6_volume*v){
 if(stream7.volume!=v||stream7.sealed)return V6_INVALID;
 if(v->readonly)return V6_RO;
 if(stream7.used&&v->write(v->context,stream7.first+stream7.size/V6_PAGE,stream7.page,V6_PAGE)){v->readonly=1;return V6_IO;}
 if(v->flush(v->context)){v->readonly=1;return V6_IO;}
 stream7.sealed=1;stream7.verified=0;stream7.verify_sum=~0u;stream7.verified_done=0;return V6_OK;
}
unsigned v6_stream_check11(v6_volume*v,unsigned*done){
 if(!done||stream7.volume!=v||!stream7.sealed)return V6_INVALID;*done=0;
 if(v->readonly)return V6_RO;
 if(stream7.verified<stream7.size){unsigned bytes=(unsigned)(stream7.size-stream7.verified>V6_PAGE?V6_PAGE:stream7.size-stream7.verified);
  if(v->read(v->context,stream7.first+stream7.verified/V6_PAGE,v->page,V6_PAGE)){v->readonly=1;return V6_IO;}
  stream7.verify_sum=crc_next(stream7.verify_sum,v->page,bytes);stream7.verified+=bytes;
 }
 if(stream7.verified==stream7.size){if(stream7.verify_sum!=stream7.sum){v->readonly=1;return V6_IO;}stream7.verified_done=1;*done=1;}return V6_OK;
}
unsigned v6_stream_finish_expected11(v6_volume*v,uint64_t size,uint32_t expected){
 if(stream7.volume!=v)return V6_INVALID;
 if(stream7.size!=size||~stream7.sum!=expected){v6_stream_abort(v);return V6_INVALID;}
 if(!stream7.sealed||!stream7.verified_done)return V6_LIMIT;
 return v6_stream_finish(v);
}
#endif
