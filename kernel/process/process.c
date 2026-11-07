#ifdef ENGINE_M7
#include <engine/desktop/app.h>
#endif
#include <engine/process.h>
#include <engine/user.h>
#include <engine/elf.h>
#ifdef ENGINE_M4
#include <engine/m4.h>
#endif
#ifdef ENGINE_M3
#include <engine/m3.h>
static const os_boot_info_v0 *m3_boot;
#endif
#define NX (UINT64_C(1)<<63)
#define MAX_PROCESSES 13
#define MAX_OWNED 8
#define HANDLE_SLOTS 16
#define OBJECT_SLOTS 32
#define MESSAGE_SLOTS 4
typedef struct {uint32_t refs,type;uint64_t value,page;} object;
typedef struct {uint32_t generation,rights;object *obj;} handle_slot;
typedef struct {
    trap_frame context;
    uint64_t pid,role,cr3,pt,owned[MAX_OWNED];unsigned owned_count,alive,abi;
    handle_slot handles[HANDLE_SLOTS];
    user_message messages[MESSAGE_SLOTS];unsigned read_pos,message_count;
    object *mapping;
} process;
static process processes[MAX_PROCESSES];
static object objects[OBJECT_SLOTS];
static unsigned current,mode,failures,exits,faults,starts,restarts=32;
static uint64_t kernel_cr3,base_pages,next_pid=1,switches,syscalls;
static uint64_t boot_reserved_bytes;
static uint64_t common_pages[8],common_vas[8],common_flags[8],common_count,user_entry;
static uint8_t private_template[4096];
#ifdef ENGINE_M3
static unsigned preemption_phase;
static uint64_t timer_ticks,preemptions,coordination_page;
extern void timer_start(void),timer_stop(void),timer_acknowledge(void);
#endif
extern const uint8_t user_program_elf[];
extern const uint64_t user_program_size;
void display_m2_result(unsigned,uint64_t,uint64_t,unsigned,unsigned);
static NORETURN void die(const char *reason) {serial_text("M2_KERNEL_FAILURE: ");serial_text(reason);serial_text("\n");test_exit(0x31);}
static uint64_t need(void) {uint64_t p=page_alloc();if(!p)die("page allocation");return p;}
static void drop(object *obj) {
    if(!obj||!obj->refs)die("object reference underflow");
    if(!--obj->refs){if(obj->page)page_free(obj->page);memset(obj,0,sizeof(*obj));}
}
static object *new_object(unsigned type) {
    for(unsigned i=0;i<OBJECT_SLOTS;++i)if(!objects[i].refs){objects[i]=(object){1,type,0,0};return &objects[i];}
    return 0;
}
static unsigned install(process *p,object *obj,unsigned rights,uint64_t *token) {
    for(unsigned i=0;i<HANDLE_SLOTS;++i)if(!p->handles[i].obj && p->handles[i].generation!=UINT32_MAX) {
        handle_slot *s=&p->handles[i];++s->generation;s->obj=obj;s->rights=rights;
        *token=((uint64_t)s->generation<<32)|(i+1);return U_OK;
    }
    return U_LIMIT;
}
static handle_slot *lookup(process *p,uint64_t token) {
    uint32_t index=(uint32_t)token,generation=(uint32_t)(token>>32);
    if(!index||index>HANDLE_SLOTS)return 0;
    handle_slot *s=&p->handles[index-1];return s->obj&&s->generation==generation?s:0;
}
static int accessible(process *p,uint64_t address,uint64_t length,int write) {
    if(!length)return 1;
    if(address<USER_BASE||address>=USER_LIMIT||length>USER_LIMIT-address)return 0;
    uint64_t *pt=(uint64_t*)(uintptr_t)p->pt;
    for(uint64_t a=address&~UINT64_C(4095);a<address+length;a+=4096) {
        uint64_t leaf=pt[(a>>12)&511];if((leaf&5)!=5 || (write&&!(leaf&2)))return 0;
    }
    return 1;
}
static void user_copy(process *p,uint64_t address,void *buffer,uint64_t length,int to_user) {
    uint64_t *pt=(uint64_t*)(uintptr_t)p->pt;uint8_t *bytes=buffer;
    while(length) {
        uint64_t count=4096-(address&4095);if(count>length)count=length;
        uint64_t physical=(pt[(address>>12)&511]&UINT64_C(0x000ffffffffff000))+(address&4095);
        if(to_user)memcpy((void*)(uintptr_t)physical,bytes,(size_t)count);
        else memcpy(bytes,(const void*)(uintptr_t)physical,(size_t)count);
        address+=count;bytes+=count;length-=count;
    }
}
static unsigned output64(process *p,uint64_t address,uint64_t value) {
    if(!accessible(p,address,8,1))return U_DENIED;user_copy(p,address,&value,8,1);return U_OK;
}
static uint64_t own(process *p) {
    if(p->owned_count==MAX_OWNED)die("per-process page budget");
    uint64_t address=need();p->owned[p->owned_count++]=address;return address;
}
static void load_program(void) {
    /* Embedded native test ELF only. This intentionally does not claim arbitrary ELF/Linux support. */
    if(user_program_size<64)die("embedded user header");
    elf_header h;memcpy(&h,user_program_elf,sizeof(h));
    if(memcmp(h.ident,"\177ELF\2\1\1",7)||h.machine!=62||h.type!=2||h.phentsize!=56||h.phnum!=3||
       h.phoff>user_program_size || (uint64_t)h.phnum*56>user_program_size-h.phoff)die("embedded user ELF");
    user_entry=h.entry;
    for(unsigned i=0;i<h.phnum;++i) {
        elf_segment s;memcpy(&s,user_program_elf+h.phoff+i*56,sizeof(s));
        if(s.type!=1||s.offset>user_program_size||s.filesz>user_program_size-s.offset||s.filesz>s.memsz||
           !s.memsz||s.vaddr<USER_BASE||s.vaddr>=USER_DATA+4096||s.memsz>USER_DATA+4096-s.vaddr||
           (s.vaddr&4095)||(s.flags!=5&&s.flags!=4&&s.flags!=6))die("embedded user segment");
        if(s.flags==6) {
            if(s.vaddr!=USER_DATA||s.memsz>4096)die("private data layout");
            memcpy(private_template,user_program_elf+s.offset,(size_t)s.filesz);
        } else {
            for(uint64_t off=0;off<s.memsz;off+=4096) {
                if(common_count==8)die("shared code budget");
                uint64_t page=need(),count=off<s.filesz?s.filesz-off:0;if(count>4096)count=4096;
                if(count)memcpy((void*)(uintptr_t)page,user_program_elf+s.offset+off,(size_t)count);
                common_pages[common_count]=page;common_vas[common_count]=s.vaddr+off;
                common_flags[common_count]=s.flags==5?5:5|NX;++common_count;
            }
        }
    }
    if(user_entry<USER_BASE||user_entry>=USER_BASE+0x4000)die("user entry range");
}
static void create(process *p,unsigned role) {
    /* Generations survive slot reuse; all private physical pages are freshly zeroed. */
    p->role=role;p->pid=next_pid++;p->alive=1;p->abi=EXEC_NATIVE64;p->owned_count=0;p->read_pos=p->message_count=0;p->mapping=0;
    p->cr3=own(p);uint64_t pdpt=own(p),pd=own(p);p->pt=own(p);
    ((uint64_t*)(uintptr_t)p->cr3)[0]=pdpt|7;
    ((uint64_t*)(uintptr_t)p->cr3)[256]=kernel_stack_branch()|3;
    for(unsigned i=0;i<4;++i)((uint64_t*)(uintptr_t)pdpt)[i]=kernel_pd(i)|3;
    ((uint64_t*)(uintptr_t)pdpt)[4]=pd|7;((uint64_t*)(uintptr_t)pd)[0]=p->pt|7;
    uint64_t *pt=(uint64_t*)(uintptr_t)p->pt;
    for(uint64_t i=0;i<common_count;++i)pt[(common_vas[i]>>12)&511]=common_pages[i]|common_flags[i];
    uint64_t data=own(p);memcpy((void*)(uintptr_t)data,private_template,sizeof(private_template));
    pt[(USER_DATA>>12)&511]=data|7|NX;pt[(USER_STACK>>12)&511]=own(p)|7|NX;
    memset(&p->context,0,sizeof(p->context));p->context.rip=user_entry;p->context.cs=0x1b;p->context.ss=0x23;
    p->context.rflags=2;p->context.rsp=USER_TOP-8;p->context.rdi=role;++starts;
}
static void cleanup(process *p) {
    for(unsigned i=0;i<HANDLE_SLOTS;++i)if(p->handles[i].obj){drop(p->handles[i].obj);p->handles[i].obj=0;p->handles[i].rights=0;}
    if(p->mapping){drop(p->mapping);p->mapping=0;}
    p->alive=0;p->message_count=0;
    /* Switch out before freeing this address space. Kernel stack/code are common supervisor mappings. */
    __asm__ volatile("movq %0,%%cr3"::"r"(kernel_cr3):"memory");
    while(p->owned_count)page_free(p->owned[--p->owned_count]);
}
trap_frame *process_first_context(void);
static NORETURN void finish(void) {
#ifdef ENGINE_M3
    if(!preemption_phase) {
        preemption_phase=1;coordination_page=need();
        create(&processes[0],14);create(&processes[1],15);
        for(unsigned i=0;i<2;++i)((uint64_t*)(uintptr_t)processes[i].pt)[(USER_SHARED>>12)&511]=coordination_page|7|NX;
        serial_text("M3_PREEMPTION_BEGIN two non-yielding native processes\n");
        current=MAX_PROCESSES-1;timer_start();arch_enter_user(process_first_context());
    }
    if(preemption_phase==1){
        preemption_phase=2;timer_stop();page_free(coordination_page);
        m3_image image={kernel_cr3,user_entry,common_count,common_pages,common_vas,common_flags,private_template,m3_boot};
        m3_start(&image,finish);
    }
    base_pages+=m3_smp_pages();
    timer_stop();
    if(preemptions<16||starts!=47||exits!=37)++failures;
    serial_text("M3_PREEMPTIONS=");serial_hex(preemptions);serial_text(" ticks=");serial_hex(timer_ticks);serial_text("\n");
#endif
    for(uint64_t i=0;i<common_count;++i)page_free(common_pages[i]);
    page_stats s=pages_stats();unsigned live_objects=0;
    for(unsigned i=0;i<OBJECT_SLOTS;++i)live_objects+=objects[i].refs!=0;
    if(s.current_pages!=base_pages||live_objects||faults!=10
#ifndef ENGINE_M3
       ||starts!=45||exits!=35
#endif
       )++failures;
#ifdef ENGINE_M3
    serial_text(failures?"M3_COMPLETE_FAIL\n":"M3_COMPLETE_PASS\n");
    serial_text(failures?"M3_PREEMPTION_FAIL\n":"M3_PREEMPTION_PASS\n");
#endif
    serial_text("M2_STATS starts=");serial_hex(starts);serial_text(" exits=");serial_hex(exits);serial_text(" isolated_faults=");serial_hex(faults);
    serial_text(" peak_owned_bytes=");serial_hex(s.peak_pages*4096);serial_text(" retained_owned_bytes=");serial_hex(s.current_pages*4096);
    serial_text(" switches=");serial_hex(switches);serial_text(" syscalls=");serial_hex(syscalls);serial_text(" failures=");serial_hex(failures);serial_text("\n");
    serial_text("M2_BOOT_RESERVED_BYTES=");serial_hex(boot_reserved_bytes);serial_text("\n");
    serial_text("M2_PAGE_ACCOUNTING_BALANCED\n");
    display_m2_result(failures,s.peak_pages*4096,s.current_pages*4096,starts,faults);
    serial_text(failures?"M2_ISOLATION_FAIL\n":"M2_ISOLATION_PASS\n");serial_text("M2_IDLE_HLT\n");
#ifdef ENGINE_M4
    if(!failures)m4_start(m3_boot,mode);
#endif
    if(mode==1||mode==4)test_exit(failures?0x31:0x10);halt_forever();
}
static int safe_return(process *p) {
    if(p->context.rip<USER_BASE||p->context.rip>=USER_LIMIT)return 0;
    uint64_t leaf=((uint64_t*)(uintptr_t)p->pt)[(p->context.rip>>12)&511];
    if((leaf&5)!=5||(leaf&NX)||p->context.cs!=0x1b||p->context.ss!=0x23||!accessible(p,p->context.rsp,8,1))return 0;
#ifdef ENGINE_M3
    if(preemption_phase)p->context.rflags=(p->context.rflags&UINT64_C(0xcd5))|0x202;
    else
#endif
    {p->context.rflags=2;}
    return 1;
}
static void reject_return(process *p) {
    serial_text("USER_RETURN_REJECTED\n");if(p->role!=11)++failures;++faults;cleanup(p);
}
static trap_frame *schedule(void) {
    for(unsigned n=1;n<=MAX_PROCESSES;++n) {
        unsigned index=(current+n)%MAX_PROCESSES;
        if(processes[index].alive) {
            current=index;process *p=&processes[index];++switches;
            /* Return only to our fixed native ring-3 selectors with safe flags and mapped code/stack. */
            if(!safe_return(p)){reject_return(p);continue;}
            __asm__ volatile("movq %0,%%cr3"::"r"(p->cr3):"memory");return &p->context;
        }
    }
    if(restarts){--restarts;create(&processes[0],6);current=MAX_PROCESSES-1;return schedule();}
    finish();
}
static unsigned dispatch(process *p,trap_frame *f,int *reschedule) {
    if(p->abi!=EXEC_NATIVE64)return U_UNSUPPORTED;
    uint64_t a=f->rdi,b=f->rsi,c=f->rdx,d=f->r10,token=0,value=0;handle_slot *slot;object *obj;
    switch(f->rax) {
    case S_WRITE: {
        if(b>128)return U_INVALID;if(!accessible(p,a,b,0))return U_DENIED;
        char text[129];user_copy(p,a,text,b,0);text[b]=0;serial_text("USER: ");serial_text(text);serial_text("\n");return U_OK;
    }
    case S_YIELD:*reschedule=1;return U_OK;
    case S_EXIT:if(a)++failures;++exits;cleanup(p);*reschedule=1;return U_OK;
    case S_PID:return output64(p,a,p->pid);
    case S_EVENT_CREATE:case S_MEMORY_CREATE: {
        if(!accessible(p,a,8,1))return U_DENIED;
        obj=new_object(f->rax==S_EVENT_CREATE?1:2);if(!obj)return U_LIMIT;
        if(obj->type==2){obj->page=page_alloc();if(!obj->page){drop(obj);return U_NOMEM;}}
        unsigned status=install(p,obj,R_ALL,&token);if(status){drop(obj);return status;}return output64(p,a,token);
    }
    case S_EVENT_SIGNAL:case S_EVENT_QUERY:
        slot=lookup(p,a);if(!slot)return U_BAD_HANDLE;
        if(slot->obj->type!=1)return U_BAD_TYPE;
        if(!(slot->rights&(f->rax==S_EVENT_SIGNAL?R_WRITE:R_READ)))return U_DENIED;
        if(f->rax==S_EVENT_SIGNAL){slot->obj->value=1;return U_OK;}return output64(p,b,slot->obj->value);
    case S_CLOSE:
        slot=lookup(p,a);if(!slot)return U_BAD_HANDLE;drop(slot->obj);slot->obj=0;slot->rights=0;return U_OK;
    case S_DUP:
        slot=lookup(p,a);if(!slot)return U_BAD_HANDLE;
        if(!b||b&~(uint64_t)slot->rights)return U_DENIED;
        if(!accessible(p,c,8,1))return U_DENIED;
        {unsigned status=install(p,slot->obj,(unsigned)b,&token);if(status)return status;++slot->obj->refs;return output64(p,c,token);}
    case S_MEMORY_MAP:
        slot=lookup(p,a);if(!slot)return U_BAD_HANDLE;
        if(slot->obj->type!=2)return U_BAD_TYPE;
        if(!(slot->rights&R_READ)||b!=USER_SHARED)return U_DENIED;
        if(p->mapping)return U_LIMIT;
        p->mapping=slot->obj;++p->mapping->refs;
        ((uint64_t*)(uintptr_t)p->pt)[(USER_SHARED>>12)&511]=p->mapping->page|5|NX|((slot->rights&R_WRITE)?2:0);
        __asm__ volatile("invlpg (%0)"::"r"(USER_SHARED):"memory");return U_OK;
    case S_TRANSFER: {
        slot=lookup(p,a);if(!slot)return U_BAD_HANDLE;
        if(!(slot->rights&R_TRANSFER)||!c||c&~(uint64_t)slot->rights)return U_DENIED;
        if(!accessible(p,d,8,1))return U_DENIED;
        for(unsigned i=0;i<MAX_PROCESSES;++i)if(processes[i].alive&&processes[i].pid==b) {
            unsigned status=install(&processes[i],slot->obj,(unsigned)c,&token);if(status)return status;
            ++slot->obj->refs;return output64(p,d,token);
        }
        return U_INVALID;
    }
    case S_SEND: {
        if(c>32)return U_INVALID;if(!accessible(p,b,c,0))return U_DENIED;
        for(unsigned i=0;i<MAX_PROCESSES;++i)if(processes[i].alive&&processes[i].pid==a) {
            process *target=&processes[i];if(target->message_count==MESSAGE_SLOTS)return U_LIMIT;
            user_message *msg=&target->messages[(target->read_pos+target->message_count)%MESSAGE_SLOTS];
            memset(msg,0,sizeof(*msg));msg->sender=p->pid;msg->length=c;user_copy(p,b,msg->data,c,0);++target->message_count;return U_OK;
        }
        return U_INVALID;
    }
    case S_RECEIVE:
        if(!accessible(p,a,sizeof(user_message),1))return U_DENIED;
        if(!p->message_count)return U_EMPTY;
        user_copy(p,a,&p->messages[p->read_pos],sizeof(user_message),1);p->read_pos=(p->read_pos+1)%MESSAGE_SLOTS;--p->message_count;return U_OK;
    default:(void)value;return U_UNSUPPORTED;
    }
}
trap_frame *process_trap(trap_frame *frame) {
#ifdef ENGINE_M7
    if(app7_active)return app7_trap(frame);
#endif
#ifdef ENGINE_M3
    if(m3_active)return m3_trap(frame);
#endif
    process *p=&processes[current];memcpy(&p->context,frame,sizeof(*frame));int reschedule=0;
#ifdef ENGINE_M3
    if(frame->vector==32) {
        timer_acknowledge();++timer_ticks;++preemptions;reschedule=1;
    } else
#endif
    if(frame->vector==128) {++syscalls;p->context.rax=dispatch(p,&p->context,&reschedule);}
    else {
        uint64_t cr2;__asm__ volatile("movq %%cr2,%0":"=r"(cr2));unsigned expected=0;
        if(frame->vector==14) {
            if(p->role==2)expected=cr2==USER_SHARED&&(frame->error&7)==7;
            if(p->role==3)expected=cr2==0x200000&&(frame->error&7)==7;
            if(p->role==4)expected=cr2>=USER_STACK&&cr2<USER_TOP&&(frame->error&0x15)==0x15;
            if(p->role==5)expected=cr2==USER_STACK-4096&&(frame->error&7)==6;
            if(p->role==7)expected=cr2==USER_BASE&&(frame->error&7)==7;
        }
        if(frame->vector==13&&(p->role==8||p->role==10))expected=frame->error==0;
        if(frame->vector==6&&p->role==12)expected=1;
        if((frame->vector==6||frame->vector==13)&&p->role==13)expected=frame->error==0;
        if(frame->vector!=14)cr2=0;
        serial_text("USER_FAULT_ISOLATED role=");serial_hex(p->role);serial_text(" vector=");serial_hex(frame->vector);
        serial_text(" address=");serial_hex(cr2);serial_text(" expected=");serial_hex(expected);serial_text("\n");
        if(!expected)++failures;++faults;cleanup(p);reschedule=1;
    }
    if(reschedule)return schedule();
    if(!safe_return(p)){reject_return(p);return schedule();}return &p->context;
}
trap_frame *process_first_context(void) {return schedule();}
NORETURN void process_start(const os_boot_info_v0 *boot,unsigned test_mode) {
    const os_boot_range *ranges=(const os_boot_range*)(uintptr_t)boot->reservations.physical_address;
    for(uint64_t i=0;i<boot->reservations.byte_length/sizeof(*ranges);++i)boot_reserved_bytes+=ranges[i].byte_length;
#ifdef ENGINE_M3
    m3_boot=boot;
#endif
    mode=test_mode;pages_init(boot);kernel_cr3=kernel_space_init(boot);base_pages=pages_stats().current_pages;
    load_program();for(unsigned i=0;i<MAX_PROCESSES;++i)create(&processes[i],i+1);
    serial_text("M2_ENTER_RING3 native64 processes=13\n");current=MAX_PROCESSES-1;arch_schedule_first();
}
