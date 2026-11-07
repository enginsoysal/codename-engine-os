#include <engine/desktop/app.h>
#include <engine/elf.h>
#include <engine/m3.h>
#include <engine/install/m6.h>

#define APP7_SLOTS 8u
#define APP7_STACK (USER_BASE+0x40000)
#define APP7_TRAP_STACK (KERNEL_STACK+0x70000)
#define NX (UINT64_C(1)<<63)
typedef struct {
    uint64_t pages[8],vas[8],flags[8],entry;
    unsigned count,refs;
} program;
typedef struct {
    uint64_t owned[12],cr3,pt,mailbox[5];
    unsigned count,live,kind;
    _Alignas(16) uint8_t fx[512];
} instance;
static program programs[2];
static instance instances[APP7_SLOTS];
static unsigned transition_ready,current_slot,call_ticks,call_result;
static uint8_t saved_pic;
volatile unsigned app7_active;
uint64_t app7_saved_rsp,app7_kernel_cr3;
extern void app7_enter(trap_frame *,uint64_t);
extern NORETURN void app7_escape(void);
extern uint64_t kernel_entry_pt(void);
extern const uint8_t app7_editor_image[],app7_calculator_image[];
extern const unsigned app7_editor_size,app7_calculator_size;

static const char *app_path(unsigned kind) {
    return kind==2?"/Applications/TextEditor/program.elf":"/Applications/Calculator/program.elf";
}
static const char *cache_path(unsigned kind) {
    return kind==2?"/System/Packages/TextEditor.elf":"/System/Packages/Calculator.elf";
}
static unsigned memory_source(void *ctx,uint64_t off,void *out,unsigned bytes) {
    memcpy(out,(const uint8_t *)ctx+off,bytes);return 0;
}
static unsigned cache_source(void *ctx,uint64_t off,void *out,unsigned bytes) {
    unsigned actual=0,status=v6_read(m6_volume(),ctx,off,out,bytes,&actual);
    return status||actual!=bytes?V6_IO:V6_OK;
}
static unsigned directory(v6_volume *volume,const char *path) {
    unsigned status=v6_mkdir(volume,path);return status==V6_EXISTS?0:status;
}
unsigned app7_install(v6_volume *volume,unsigned calculator) {
    if(!app7_editor_size||!app7_calculator_size||calculator>1)return V6_INVALID;
    if(directory(volume,"/System/Packages")||directory(volume,"/Applications/TextEditor")||directory(volume,"/Applications/Calculator"))return V6_IO;
    if(v6_put(volume,cache_path(2),app7_editor_size,memory_source,(void *)app7_editor_image)||
       v6_put(volume,app_path(2),app7_editor_size,memory_source,(void *)app7_editor_image)||
       v6_put(volume,cache_path(4),app7_calculator_size,memory_source,(void *)app7_calculator_image)||
       (calculator&&v6_put(volume,app_path(4),app7_calculator_size,memory_source,(void *)app7_calculator_image)))return V6_IO;
    return 0;
}
unsigned app7_available(unsigned kind) {
    if(kind!=2&&kind!=4)return 0;
    return !v6_verify(m6_volume(),app_path(kind));
}
unsigned app7_package(unsigned kind,unsigned install) {
    if(kind!=4||install>1)return V6_RO;
    for(unsigned i=0;i<APP7_SLOTS;++i)if(instances[i].live&&instances[i].kind==kind)return V6_RO;
    if(!install)return v6_remove(m6_volume(),app_path(kind));
    const v6_entry *file=0;v6_volume *volume=m6_volume();
    for(unsigned i=0;i<V6_ENTRIES;++i){const v6_entry *e=&volume->entries[i];
        if(e->kind==1&&!memcmp(e->path,cache_path(kind),sizeof("/System/Packages/Calculator.elf"))){file=e;break;}}
    if(!file||v6_verify(volume,cache_path(kind)))return V6_INVALID;
    return v6_put(volume,app_path(kind),file->size,cache_source,(void *)cache_path(kind));
}

static unsigned read_exact(const char *path,uint64_t off,void *out,unsigned bytes) {
    unsigned actual=0,status=v6_read(m6_volume(),path,off,out,bytes,&actual);
    return status||actual!=bytes?V6_IO:0;
}
static void unload(program *p) {
    while(p->count)page_free(p->pages[--p->count]);
    memset(p,0,sizeof(*p));
}
static unsigned load_program(unsigned kind,program *p) {
    const char *path=app_path(kind);elf_header header;
    if(v6_verify(m6_volume(),path)||read_exact(path,0,&header,sizeof(header)))return V6_IO;
    if(memcmp(header.ident,"\177ELF\2\1\1",7)||header.type!=2||header.machine!=62||
       header.ehsize!=64||header.phentsize!=56||!header.phnum||header.phnum>4||header.phoff>65536||
       header.entry<USER_BASE||header.entry>=USER_BASE+0x4000)return V6_INVALID;
    p->entry=header.entry;unsigned entry_found=0;
    for(unsigned i=0;i<header.phnum;++i){
        elf_segment s;
        if(read_exact(path,header.phoff+i*56,&s,sizeof(s))){unload(p);return V6_IO;}
        if(s.type!=1||!s.memsz)continue;
        if((s.flags!=4&&s.flags!=5)||s.vaddr<USER_BASE||s.vaddr>=USER_BASE+0x8000||
           s.memsz>USER_BASE+0x8000-s.vaddr||s.filesz>s.memsz||s.offset>65536||s.filesz>65536-s.offset||
           (s.vaddr&4095)){unload(p);return V6_INVALID;}
        if(s.flags==5&&header.entry>=s.vaddr&&header.entry<s.vaddr+s.filesz)entry_found=1;
        for(uint64_t offset=0;offset<s.memsz;offset+=4096){
            if(p->count==8){unload(p);return V6_LIMIT;}
            for(unsigned j=0;j<p->count;++j)if(p->vas[j]==s.vaddr+offset){unload(p);return V6_INVALID;}
            uint64_t page=page_alloc();if(!page){unload(p);return V6_FULL;}
            unsigned n=p->count++;p->pages[n]=page;p->vas[n]=s.vaddr+offset;p->flags[n]=s.flags==5?5:5|NX;
            unsigned bytes=offset<s.filesz?(unsigned)(s.filesz-offset):0;if(bytes>4096)bytes=4096;
            if(bytes&&read_exact(path,s.offset+offset,(void *)(uintptr_t)page,bytes)){unload(p);return V6_IO;}
        }
    }
    if(!entry_found){unload(p);return V6_INVALID;}
    return 0;
}

static uint64_t own(instance *p) {
    if(p->count==12)return 0;
    uint64_t page=page_alloc();if(page)p->owned[p->count++]=page;return page;
}
void app7_close(unsigned slot) {
    if(slot>=APP7_SLOTS)return;
    instance *p=&instances[slot];unsigned live=p->live,kind=p->kind;
    while(p->count)page_free(p->owned[--p->count]);
    memset(p,0,sizeof(*p));
    if(live){program *program=&programs[kind==4];if(program->refs&&!--program->refs)unload(program);}
}
static unsigned transition_stack(void) {
    if(transition_ready)return 0;
    uint64_t allocated[4]={0};unsigned n=0;
    for(;n<4;++n){allocated[n]=page_alloc();if(!allocated[n])break;}
    if(n!=4){while(n)page_free(allocated[--n]);return V6_FULL;}
    uint64_t *pt=(uint64_t *)(uintptr_t)kernel_entry_pt();
    for(unsigned i=0;i<4;++i)pt[(((APP7_TRAP_STACK>>12)&511)+i)]=allocated[i]|3|NX;
    transition_ready=1;return 0;
}
unsigned app7_prepare(void) { return transition_stack(); }
unsigned app7_open(unsigned slot,unsigned kind) {
    if(slot>=APP7_SLOTS||(kind!=2&&kind!=4)||app7_active)return V6_INVALID;
    app7_close(slot);if(transition_stack())return V6_FULL;
    program *code=&programs[kind==4];
    if(!code->refs){unsigned s=load_program(kind,code);if(s)return s;}
    instance *p=&instances[slot];p->kind=kind;
    p->cr3=own(p);uint64_t pdpt=own(p),pd=own(p);p->pt=own(p);
    if(!p->cr3||!pdpt||!pd||!p->pt)goto fail;
    ((uint64_t *)(uintptr_t)p->cr3)[0]=pdpt|7;
    ((uint64_t *)(uintptr_t)p->cr3)[256]=kernel_stack_branch()|3;
    for(unsigned i=0;i<4;++i)((uint64_t *)(uintptr_t)pdpt)[i]=kernel_pd(i)|3;
    ((uint64_t *)(uintptr_t)pdpt)[4]=pd|7;((uint64_t *)(uintptr_t)pd)[0]=p->pt|7;
    uint64_t *pt=(uint64_t *)(uintptr_t)p->pt;
    for(unsigned i=0;i<code->count;++i)pt[(code->vas[i]>>12)&511]=code->pages[i]|code->flags[i];
    for(unsigned i=0;i<5;++i){p->mailbox[i]=own(p);if(!p->mailbox[i])goto fail;pt[((APP7_MAILBOX>>12)&511)+i]=p->mailbox[i]|7|NX;}
    uint64_t stack=own(p);if(!stack)goto fail;pt[(APP7_STACK>>12)&511]=stack|7|NX;
    *(uint16_t *)p->fx=0x37f;*(uint32_t *)(p->fx+24)=0x1f80;
    p->live=1;++code->refs;serial_text("M7_APP_RING3_CREATED kind=");serial_hex(kind);serial_text("\n");return 0;
fail:
    app7_close(slot);if(!code->refs)unload(code);return V6_FULL;
}

static void transfer(instance *p,unsigned offset,void *buffer,unsigned bytes,int to_user) {
    uint8_t *data=buffer;
    while(bytes){unsigned index=offset/4096,in_page=offset%4096,count=4096-in_page;if(count>bytes)count=bytes;
        void *physical=(void *)(uintptr_t)(p->mailbox[index]+in_page);
        if(to_user)memcpy(physical,data,count);else memcpy(data,physical,count);
        offset+=count;data+=count;bytes-=count;
    }
}
unsigned app7_request(unsigned slot,unsigned command,unsigned key,const void *data,unsigned bytes,app7_view *view) {
    if(slot>=APP7_SLOTS||!instances[slot].live||app7_active||bytes>=APP7_TEXT||(!data&&bytes)||!view)return V6_INVALID;
    instance *p=&instances[slot];unsigned header[]={APP7_MAGIC,command,key,bytes};
    transfer(p,0,header,sizeof(header),1);
    if(bytes)transfer(p,offsetof(app7_mailbox,input),(void *)data,bytes,1);
    trap_frame frame={0};frame.cs=0x1b;frame.ss=0x23;frame.rflags=0x202;
    frame.rip=programs[p->kind==4].entry;frame.rsp=APP7_STACK+4096-8;
    uint64_t flags;__asm__ volatile("pushfq; popq %0; cli":"=r"(flags)::"memory");
    __asm__ volatile("movq %%cr3,%0":"=r"(app7_kernel_cr3));
    saved_pic=in8(0x21);out8(0x21,0xfe); /* 100-Hz PIT only; restore input IRQs after this bounded call. */
    current_slot=slot;call_ticks=0;call_result=0;
    interrupts_set_kernel_stack(APP7_TRAP_STACK+4*4096);
    __asm__ volatile("fxrstor64 %0"::"m"(p->fx):"memory");
    app7_active=1;app7_enter(&frame,p->cr3);
    __asm__ volatile("fxsave64 %0":"=m"(p->fx)::"memory");
    out8(0x21,saved_pic);interrupts_set_kernel_stack(KERNEL_STACK_TOP);
    if(flags&512)__asm__ volatile("sti":::"memory");
    if(!call_result){
        /* Validate scalar fields before copying the bounded display snapshot. */
        struct {uint32_t length,cursor,dirty,calc_error;int64_t number,operand;uint32_t operation,entered;} scalars;
        transfer(p,offsetof(app7_mailbox,view)+offsetof(app7_view,length),&scalars.length,sizeof(scalars),0);
        if(scalars.length>=APP7_TEXT||scalars.cursor>scalars.length||scalars.dirty>1||scalars.calc_error>1||scalars.entered>1)call_result=V6_INVALID;
        else {
            char end;transfer(p,offsetof(app7_mailbox,view)+scalars.length,&end,1,0);
            if(end)call_result=V6_INVALID;
        }
    }
    if(!call_result)transfer(p,offsetof(app7_mailbox,view),view,sizeof(*view),0);
    else {serial_text("M7_APP_CONTAINED kind=");serial_hex(p->kind);serial_text(" status=");serial_hex(call_result);serial_text("\n");app7_close(slot);}
    return call_result;
}

trap_frame *app7_trap(trap_frame *frame) {
    if(!app7_active||current_slot>=APP7_SLOTS)for(;;)__asm__ volatile("cli; hlt");
    if(frame->vector==32){out8(0x20,0x20);if(++call_ticks<10)return frame;call_result=V6_LIMIT;}
    else if(frame->vector==128&&frame->rax==APP7_DONE)call_result=0;
    else {call_result=V6_INVALID;serial_text("M7_APP_FAULT vector=");serial_hex(frame->vector);serial_text("\n");}
    app7_active=0;app7_escape();
}
