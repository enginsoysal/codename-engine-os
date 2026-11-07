#define APP7_USER
#include <engine/desktop/app.h>

static void insert(app7_view *v,const char *bytes,unsigned count) {
    if(count>APP7_TEXT-1-v->length)return;
    for(unsigned i=v->length+1;i>v->cursor;--i)v->text[i+count-1]=v->text[i-1];
    for(unsigned i=0;i<count;++i)v->text[v->cursor+i]=bytes[i];
    v->cursor+=count;v->length+=count;v->dirty=1;
}

static void calculate(app7_view *v,unsigned key) {
    if(key=='C'||key=='c'){
        v->number=v->operand=0;v->operation=v->entered=v->calc_error=0;return;
    }
    if(key>='0'&&key<='9'){
        if(!v->entered){v->number=0;v->entered=1;}
        int64_t value;
        if(__builtin_mul_overflow(v->number,(int64_t)10,&value)||
           __builtin_add_overflow(value,(int64_t)(key-'0'),&value))v->calc_error=1;
        else v->number=value;
        return;
    }
    if(key=='='){
        int64_t value=0;int error=0;
        if(v->operation=='+')error=__builtin_add_overflow(v->operand,v->number,&value);
        else if(v->operation=='-')error=__builtin_sub_overflow(v->operand,v->number,&value);
        else if(v->operation=='*')error=__builtin_mul_overflow(v->operand,v->number,&value);
        else if(v->operation=='/'){
            if(!v->number||(v->operand==INT64_MIN&&v->number==-1))error=1;
            else value=v->operand/v->number;
        }else return;
        v->calc_error=(unsigned)error;if(!error)v->number=value;
        v->operation=0;v->entered=0;
    }else if(key=='+'||key=='-'||key=='*'||key=='/'){
        v->operand=v->number;v->operation=key;v->entered=0;
    }
}

__attribute__((noreturn)) void app7_main(void) {
    app7_mailbox *m=(app7_mailbox *)(uintptr_t)APP7_MAILBOX;
    app7_view *v=&m->view;
    if(m->magic==APP7_MAGIC&&m->bytes<APP7_TEXT&&v->length<APP7_TEXT&&v->cursor<=v->length){
        if(m->command==APP7_CRASH)__asm__ volatile("ud2");
        if(m->command==APP7_SPIN)for(;;)__asm__ volatile("pause");
        if(m->command==APP7_KERNEL_WRITE)*(volatile unsigned *)(uintptr_t)0x200000=0;
        if(m->command==APP7_CODE_WRITE)*(volatile unsigned *)(uintptr_t)0x100000000=0;
        if(m->command==APP7_BAD_RESULT)v->length=UINT32_MAX;
        if(APP7_KIND==4&&m->command==APP7_KEY)calculate(v,m->key);
        if(APP7_KIND==2){
            if(m->command==APP7_LOAD){
                for(unsigned i=0;i<m->bytes;++i)v->text[i]=m->input[i];
                v->text[m->bytes]=0;v->length=v->cursor=m->bytes;v->dirty=0;
            }
            if(m->command==APP7_INSERT)insert(v,m->input,m->bytes);
            if(m->command==APP7_CLEAR){v->text[0]=0;v->length=v->cursor=0;v->dirty=1;}
            if(m->command==APP7_SAVED)v->dirty=0;
            if(m->command==APP7_KEY){
                unsigned key=m->key;
                if(key==128&&v->cursor)--v->cursor;
                else if(key==129&&v->cursor<v->length)++v->cursor;
                else if(key==130)v->cursor=0;
                else if(key==131)v->cursor=v->length;
                else if(key==8&&v->cursor){
                    for(unsigned i=v->cursor;i<=v->length;++i)v->text[i-1]=v->text[i];
                    --v->cursor;--v->length;v->dirty=1;
                }else if(key==132&&v->cursor<v->length){
                    for(unsigned i=v->cursor;i<v->length;++i)v->text[i]=v->text[i+1];
                    --v->length;v->dirty=1;
                }else if((key>=32&&key<=126)||key==10){char value=(char)key;insert(v,&value,1);}
            }
        }
    }
    __asm__ volatile("int $0x80"::"a"((uint64_t)APP7_DONE):"memory");
    for(;;)__asm__ volatile("ud2");
}
