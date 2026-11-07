#include <engine/base.h>
void *memcpy(void *dst,const void *src,size_t n) {
    uint8_t *d=dst; const uint8_t *s=src; for(size_t i=0;i<n;++i)d[i]=s[i]; return dst;
}
void *memset(void *dst,int value,size_t n) {
    uint8_t *d=dst; for(size_t i=0;i<n;++i)d[i]=(uint8_t)value; return dst;
}
void *memmove(void *dst,const void *src,size_t n) {
    uint8_t *d=dst; const uint8_t *s=src;
    if((uintptr_t)d<(uintptr_t)s)for(size_t i=0;i<n;++i)d[i]=s[i];
    else for(size_t i=n;i>0;--i)d[i-1]=s[i-1]; return dst;
}
int memcmp(const void *left,const void *right,size_t n) {
    const uint8_t *a=left,*b=right; for(size_t i=0;i<n;++i)if(a[i]!=b[i])return a[i]<b[i]?-1:1; return 0;
}
#ifdef ENGINE_M7
size_t strlen(const char *text){size_t size=0;while(text[size])++size;return size;}
#endif
