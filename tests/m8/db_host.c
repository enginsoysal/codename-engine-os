/* Format-only host harness. This never executes an MSI or a Windows payload. */
#include "msi_db.h"
#include "cab.h"
static uint8_t arena8[32*1024*1024];
static unsigned used8;
static void (*failure8)(const char*);
static msi8_table current8;
void *memcpy(void*d,const void*s,size_t n){uint8_t*a=d;const uint8_t*b=s;while(n--)*a++=*b++;return d;}
void *memset(void*d,int c,size_t n){uint8_t*a=d;while(n--)*a++=(uint8_t)c;return d;}
int memcmp(const void*a,const void*b,size_t n){const uint8_t*x=a,*y=b;while(n--){if(*x!=*y)return *x-*y;++x;++y;}return 0;}
void *alloc8(size_t n){n=(n+15)&~(size_t)15;if(n>sizeof(arena8)-used8)return 0;void*p=arena8+used8;used8+=(unsigned)n;return p;}
void *calloc8(size_t n,size_t size){if(size&&n>SIZE_MAX/size)return 0;void*p=alloc8(n*size);if(p)memset(p,0,n*size);return p;}
void free8(void*p){(void)p;}
NORETURN void fail8(const char*s){failure8(s);__builtin_trap();}
int equal8(const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
void copy8(char*out,const char*in,unsigned cap){unsigned n=0;while(n+1<cap&&in[n]){out[n]=in[n];++n;}out[n]=0;}
__declspec(dllexport) void db8_open(const void*data,unsigned bytes,void(*failure)(const char*)){used8=0;failure8=failure;msi8_database(data,bytes);}
__declspec(dllexport) unsigned db8_table(const char*name){current8=msi8_table_open(name);return current8.rows;}
__declspec(dllexport) unsigned db8_columns(void){return current8.columns;}
__declspec(dllexport) const char *db8_name(unsigned col){return current8.column[col].name;}
__declspec(dllexport) unsigned db8_type(unsigned col){return current8.column[col].type;}
__declspec(dllexport) const char *db8_text(unsigned row,unsigned col){return msi8_text(&current8,row,col);}
__declspec(dllexport) int db8_int(unsigned row,unsigned col){return msi8_integer(&current8,row,col);}
__declspec(dllexport) unsigned db8_stream(const char*name){return msi8_stream(name,0).size;}

static cab8_archive *cab_host8;
__declspec(dllexport) unsigned db8_cab(void){cab_host8=cab8_decode(msi8_stream("product.cab",0));return cab_host8->count;}
__declspec(dllexport) const char *db8_cab_name(unsigned i){return cab_host8->files[i].name;}
__declspec(dllexport) unsigned db8_cab_size(unsigned i){return cab_host8->files[i].size;}
__declspec(dllexport) const void *db8_cab_data(unsigned i){return cab_host8->expanded.data+cab_host8->files[i].offset;}
