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
static uint32_t crc_next(uint32_t c,const void *p,unsigned n){const uint8_t*b=p;while(n--){c^=*b++;for(unsigned j=0;j<8;++j)c=(c>>1)^(0xedb88320u&-(c&1));}return c;}
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
 if(v->generation==UINT64_MAX)return V6_LIMIT;unsigned bank=v->active^1,offset=0;
 for(unsigned i=0;i<TABLE_PAGES;++i){zero(v->page,V6_PAGE);unsigned bytes=TABLE_BYTES-offset;if(bytes>V6_PAGE)bytes=V6_PAGE;copy(v->page,(uint8_t*)v->pending+offset,bytes);offset+=bytes;if(v->write(v->context,bank*BANK_PAGES+1+i,v->page,V6_PAGE)){v->readonly=1;return V6_IO;}}
 if(v->flush(v->context)){v->readonly=1;return V6_IO;}
 zero(v->page,V6_PAGE);copy(v->page,"ENGINEV6",8);w32(v->page+8,1);w32(v->page+12,sizeof(v6_entry));w64(v->page+16,v->blocks);w64(v->page+24,v->generation+1);w32(v->page+32,V6_ENTRIES);w32(v->page+36,crc(v->pending,TABLE_BYTES));w32(v->page+4092,crc(v->page,4092));
 if(v->write(v->context,bank*BANK_PAGES,v->page,V6_PAGE)||v->flush(v->context)){v->readonly=1;return V6_IO;}
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
 if(!s&&stream7.used&&v->write(v->context,stream7.first+stream7.size/V6_PAGE,stream7.page,V6_PAGE)){v->readonly=1;s=V6_IO;}
 if(!s&&v->flush(v->context)){v->readonly=1;s=V6_IO;}
 if(!s){v6_entry *e=v->pending+n;zero(e,sizeof(*e));copy(e->path,stream7.path,256);
  e->kind=1;e->first=stream7.size?stream7.first:0;e->size=stream7.size;e->crc=~stream7.sum;s=commit(v);}
 v6_stream_abort(v);return s;
}
#endif
