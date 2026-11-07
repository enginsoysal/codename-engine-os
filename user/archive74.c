#define APP7_USER
#include <engine/desktop/app.h>
#include "config74.h"
#include "miniz_tinfl.h"

void *memcpy(void *out,const void *in,size_t bytes){unsigned char *d=out;const unsigned char *s=in;for(size_t i=0;i<bytes;++i)d[i]=s[i];return out;}
void *memset(void *out,int value,size_t bytes){unsigned char *d=out;for(size_t i=0;i<bytes;++i)d[i]=(unsigned char)value;return out;}
int memcmp(const void *a,const void *b,size_t bytes){const unsigned char *x=a,*y=b;for(size_t i=0;i<bytes;++i)if(x[i]!=y[i])return x[i]<y[i]?-1:1;return 0;}

typedef struct {
    tinfl_decompressor decoder;
    uint8_t dictionary[TINFL_LZ_DICT_SIZE];
    uint32_t magic,offset,pending_start,pending_bytes;
    int result;
} archive74_state;
_Static_assert(sizeof(archive74_state)<=APP74_STATE_PAGES*4096,"bounded inflate state");

#ifdef APP74_HOST
__declspec(dllexport)
#endif
void archive74_event(app7_mailbox *m,void *storage){
    archive74_state *s=storage;app7_view *v=&m->view;
    v->length=v->cursor=v->dirty=v->calc_error=v->entered=0;v->number=v->operand=0;v->operation=1;v->text[0]=0;
    if(m->magic!=APP7_MAGIC||m->bytes>=APP7_TEXT){v->calc_error=1;return;}
    if(m->command==APP74_INFLATE_END){v->operation=0;return;}
    if(m->command==APP74_INFLATE_BEGIN){memset(s,0,sizeof(*s));s->magic=0x41524348;tinfl_init(&s->decoder);s->result=TINFL_STATUS_NEEDS_MORE_INPUT;return;}
    if(m->command!=APP74_INFLATE_STEP||s->magic!=0x41524348){v->calc_error=1;return;}
    if(!s->pending_bytes&&s->result!=TINFL_STATUS_DONE){
        size_t input=m->bytes,output=TINFL_LZ_DICT_SIZE-s->offset;
        s->result=tinfl_decompress(&s->decoder,(const uint8_t *)m->input,&input,s->dictionary,s->dictionary+s->offset,&output,
                                  (m->key&1)?TINFL_FLAG_HAS_MORE_INPUT:0);
        if(s->result<0){v->calc_error=1;v->operation=(uint32_t)(-s->result);return;}
        s->pending_start=s->offset;s->pending_bytes=(uint32_t)output;s->offset=(s->offset+(uint32_t)output)&(TINFL_LZ_DICT_SIZE-1);
        v->operand=(int64_t)input;
    }
    unsigned output=s->pending_bytes;if(output>4096)output=4096;
    memcpy(v->text,s->dictionary+s->pending_start,output);s->pending_start+=output;s->pending_bytes-=output;
    v->text[output]=0;v->length=v->cursor=output;v->number=s->pending_bytes;
    v->operation=(s->result==TINFL_STATUS_DONE&&!s->pending_bytes)?0:1;
}

#ifndef APP74_HOST
__attribute__((noreturn)) void app7_main(void){
    archive74_event((app7_mailbox *)(uintptr_t)APP7_MAILBOX,(void *)(uintptr_t)APP74_STATE);
    __asm__ volatile("int $0x80"::"a"((uint64_t)APP7_DONE):"memory");for(;;)__asm__ volatile("ud2");
}
#endif
