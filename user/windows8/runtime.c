#include "runtime.h"
w8_mailbox *const job8=(void*)(uintptr_t)W8_MAILBOX;
uint64_t sys8(uint64_t n,uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e){
 register uint64_t r8 __asm__("r8")=e;register uint64_t r10 __asm__("r10")=d;
 __asm__ volatile("int $0x80":"+a"(n):"D"(a),"S"(b),"d"(c),"r"(r10),"r"(r8):"memory","rcx","r11");return n;
}
unsigned len8(const char*s){unsigned n=0;if(s)while(s[n])++n;return n;}
unsigned wlen8(const WCHAR*s){unsigned n=0;if(s)while(s[n])++n;return n;}
int equal8(const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
int case8(const char*a,const char*b){while(*a&&*b){unsigned x=(uint8_t)*a++,y=(uint8_t)*b++;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return (int)x-(int)y;}return (uint8_t)*a-(uint8_t)*b;}
void copy8(char*out,const char*in,unsigned cap){if(!cap)return;unsigned n=0;while(n+1<cap&&in[n]){out[n]=in[n];++n;}out[n]=0;}
void append8(char*out,const char*in,unsigned cap){unsigned n=len8(out);if(n<cap)copy8(out+n,in,cap-n);}
void *memcpy(void*d,const void*s,size_t n){uint8_t*out=d;const uint8_t*in=s;for(size_t i=0;i<n;++i)out[i]=in[i];return d;}
void *memset(void*d,int v,size_t n){uint8_t*p=d;for(size_t i=0;i<n;++i)p[i]=(uint8_t)v;return d;}
void *memmove(void*d,const void*s,size_t n){uint8_t*a=d;const uint8_t*b=s;if(a<b)for(size_t i=0;i<n;++i)a[i]=b[i];else while(n){--n;a[n]=b[n];}return d;}
int memcmp(const void*a,const void*b,size_t n){const uint8_t*x=a,*y=b;for(size_t i=0;i<n;++i)if(x[i]!=y[i])return x[i]-y[i];return 0;}
size_t strlen(const char*s){return len8(s);}
void print8(const char*s){unsigned n=len8(s);while(n){unsigned part=n>4096?4096:n;sys8(W8_SYS_OUTPUT,(uint64_t)s,part,0,0,0);s+=part;n-=part;}}
void number8(uint64_t v){char s[32],r[24];unsigned n=0,p=0;do{r[n++]=(char)('0'+v%10);v/=10;}while(v);while(n)s[p++]=r[--n];s[p]=0;print8(s);}
NORETURN void exit8(unsigned status){job8->status=status;sys8(W8_SYS_EXIT,status,0,0,0,0);for(;;)__asm__ volatile("ud2");}
NORETURN void fail8(const char*s){copy8(job8->message,s,sizeof(job8->message));print8("\nEngine OS: ");print8(s);print8("\n");exit8(1);}
typedef struct block8 {size_t size;struct block8*next;unsigned free,reserved;} block8;
static block8 *heap8;static uint64_t heap_end8=W8_HEAP;
void *alloc8(size_t bytes){
 if(!bytes)bytes=1;if(bytes>24*1024*1024)return 0;bytes=(bytes+15)&~(size_t)15;
 for(block8*b=heap8;b;b=b->next)if(b->free&&b->size>=bytes){
  if(b->size>bytes+sizeof(block8)+32){block8*n=(void*)((uint8_t*)(b+1)+bytes);*n=(block8){b->size-bytes-sizeof(*n),b->next,1,0};b->size=bytes;b->next=n;}b->free=0;return b+1;}
 size_t total=(bytes+sizeof(block8)+4095)&~(size_t)4095;if(heap_end8+total>W8_HEAP+32*1024*1024)return 0;
 if(sys8(W8_SYS_MAP,heap_end8,total,3,0,0))return 0;block8*b=(void*)(uintptr_t)heap_end8;heap_end8+=total;*b=(block8){total-sizeof(*b),0,0,0};
 if(!heap8)heap8=b;else{block8*last=heap8;while(last->next)last=last->next;last->next=b;}return b+1;
}
void free8(void*p){if(!p)return;block8*b=(block8*)p-1;if((uintptr_t)b<W8_HEAP||(uintptr_t)b>=heap_end8||b->free)fail8("Invalid heap release in Windows process.");b->free=1;
 for(block8*x=heap8;x&&x->next;){if(x->free&&x->next->free&&(uint8_t*)(x+1)+x->size==(uint8_t*)x->next){x->size+=sizeof(*x)+x->next->size;x->next=x->next->next;}else x=x->next;}}
void *realloc8(void*p,size_t n){if(!p)return alloc8(n);if(!n){free8(p);return 0;}block8*b=(block8*)p-1;if(b->size>=n)return p;void*q=alloc8(n);if(!q)return 0;memcpy(q,p,b->size);free8(p);return q;}
void *calloc8(size_t n,size_t s){if(n&&s>SIZE_MAX/n)return 0;void*p=alloc8(n*s);if(p)memset(p,0,n*s);return p;}
int utf16_to8(const WCHAR*s,char*out,unsigned cap){unsigned at=0;if(!cap)return -1;for(unsigned i=0;s&&s[i];++i){unsigned v=s[i];if(v>=0xd800&&v<=0xdbff){unsigned lo=s[++i];if(lo<0xdc00||lo>0xdfff)return -1;v=0x10000+((v-0xd800)<<10)+(lo-0xdc00);}else if(v>=0xdc00&&v<=0xdfff)return -1;
 unsigned n=v<128?1:v<2048?2:v<65536?3:4;if(at+n>=cap)return -1;if(n==1)out[at++]=(char)v;else{out[at++]=(char)((n==2?0xc0:n==3?0xe0:0xf0)|(v>>(6*(n-1))));for(unsigned j=n-1;j;--j)out[at++]=(char)(0x80|((v>>(6*(j-1)))&63));}}
 out[at]=0;return (int)at;}
int utf8_to16(const char*s,WCHAR*out,unsigned cap){unsigned at=0;for(unsigned i=0;s&&s[i];){unsigned v=(uint8_t)s[i++],n=0,min=0;if(v>=0xf0){v&=7;n=3;min=65536;}else if(v>=0xe0){v&=15;n=2;min=2048;}else if(v>=0xc2){v&=31;n=1;min=128;}else if(v>=128)return -1;for(unsigned j=0;j<n;++j){unsigned b=(uint8_t)s[i++];if((b&0xc0)!=0x80)return -1;v=(v<<6)|(b&63);}if(v<min||v>0x10ffff||(v>=0xd800&&v<=0xdfff))return -1;
 if(v>65535){if(at+2>=cap)return -1;v-=0x10000;out[at++]=(WCHAR)(0xd800+(v>>10));out[at++]=(WCHAR)(0xdc00+(v&1023));}else{if(at+1>=cap)return -1;out[at++]=(WCHAR)v;}}
 if(!cap)return -1;out[at]=0;return (int)at;}
int open8(const char*p,unsigned flags){return (int)sys8(W8_SYS_OPEN,(uint64_t)p,flags,0,0,0);}
int read8(int f,void*out,unsigned n){unsigned done=0;while(n){unsigned part=n>4096?4096:n;int got=(int)sys8(W8_SYS_READ,(unsigned)f,(uint64_t)out,part,0,0);if(got<0)return -1;done+=(unsigned)got;if((unsigned)got<part)break;out=(uint8_t*)out+got;n-=(unsigned)got;}return (int)done;}
int write8(int f,const void*p,unsigned n){unsigned done=0;while(n){unsigned part=n>4096?4096:n;int got=(int)sys8(W8_SYS_WRITE,(unsigned)f,(uint64_t)p,part,0,0);if(got<0)return -1;done+=(unsigned)got;if((unsigned)got!=part)break;p=(const uint8_t*)p+got;n-=(unsigned)got;}return (int)done;}
int64_t seek8(int f,int64_t n,unsigned from){return (int64_t)sys8(W8_SYS_SEEK,(unsigned)f,(uint64_t)n,from,0,0);}
int close8(int f){return (int)sys8(W8_SYS_CLOSE,(unsigned)f,0,0,0,0);}
int stat8(const char*p,w8_file_info*out){return (int)sys8(W8_SYS_STAT,(uint64_t)p,(uint64_t)out,0,0,0);}
unsigned load8(const char*p,void**out,unsigned max){w8_file_info info;if(stat8(p,&info)||info.kind!=1||info.size>max)return 0;void*data=alloc8((size_t)info.size+1);if(!data)return 0;int f=open8(p,0);if(f<0){free8(data);return 0;}int got=read8(f,data,(unsigned)info.size);close8(f);if(got<0||(unsigned)got!=info.size){free8(data);return 0;}((char*)data)[info.size]=0;*out=data;return (unsigned)info.size;}
int save8(const char*p,const void*data,unsigned n){int f=open8(p,1);if(f<0)return 1;unsigned bad=write8(f,data,n)!=(int)n;return close8(f)||bad;}
int mkdir8(const char*p){return (int)sys8(W8_SYS_MKDIR,(uint64_t)p,0,0,0,0);}
int remove8(const char*p){return (int)sys8(W8_SYS_DELETE,(uint64_t)p,0,0,0,0);}
void progress8(unsigned p,const char*text){job8->percent=p;copy8(job8->message,text,sizeof(job8->message));sys8(W8_SYS_YIELD,0,0,0,0,0);}
NORETURN void windows8_main(void){if(job8->magic!=W8_MAGIC)exit8(2);if(job8->command==W8_PREVIEW||job8->command==W8_INSTALL||job8->command==W8_REMOVE)msi8_execute();else if(job8->command==W8_RUN){init_win8();pe8_run(job8->source,job8->arguments);}else fail8("Unsupported Windows task.");exit8(0);}
