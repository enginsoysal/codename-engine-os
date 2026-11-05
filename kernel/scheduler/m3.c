#include <engine/m3.h>
#include <engine/user.h>
#define NX (UINT64_C(1)<<63)
#define MASK UINT64_C(0x000ffffffffff000)
#define STACK(i) (USER_BASE+0x30000+(uint64_t)(i)*0x2000)
#define TLS(i) (USER_BASE+0x60000+(uint64_t)(i)*4096)
enum { FREE,RUN,WAIT,ZOMBIE };
enum { W_SLEEP=1,W_EVENT,W_JOIN,W_MESSAGE };
typedef struct {uint64_t page;unsigned refs;} backing;
typedef struct {unsigned refs,type,signaled,manual;backing *memory;} event;
typedef struct {unsigned generation,rights;event *obj;} handle;
typedef struct {
    uint64_t cr3,pt,pid,owned[5];unsigned count,live,reserved,cpu;
    backing *vm[M3_PAGES];handle handles[16];event *mapping;
    user_message queue[4];unsigned head,messages;
} address_space;
typedef struct {
    trap_frame frame;
    _Alignas(16) uint8_t fx[512];
    address_space *space;event *wait_object;uint64_t stack,tls,token,deadline,target,output,join_owner;
    unsigned state,wait,slot,generation,result,role,test_admin;
} thread;
static address_space spaces[2];static thread threads[M3_THREADS];static backing backings[64];
static event objects[32];static uint64_t next_pid=1000;
static m3_image image;static void (*done)(void);
static unsigned current[M3_CPUS],cpu_count,next_cpu,run_lock;
static uint64_t user_cpu_mask;
static trap_frame ap_idle_frames[M3_CPUS];
static unsigned live,started,exited,cancelled,checks,cow_faults,oom_faults,fault_exits,unexpected,copy_cow;
static uint64_t samples[256],sample_count;
static uint64_t ticks,switches,idle_halts,deadline_lateness,baseline,emergency[4],dispatch_cycles,max_dispatch;
_Atomic unsigned m3_active;
static _Atomic uint64_t configured_cpus;
static void lock(void){while(__atomic_exchange_n(&run_lock,1,__ATOMIC_ACQUIRE))__asm__ volatile("pause");}
static void unlock(void){__atomic_store_n(&run_lock,0,__ATOMIC_RELEASE);}
static NORETURN void bad(const char*);
static void cpu_state_init(void){
    uint32_t lo,hi,a0,b0,c0,d0;
    __asm__ volatile("rdmsr":"=a"(lo),"=d"(hi):"c"(0xc0000080u));lo&=~1u;
    __asm__ volatile("wrmsr"::"a"(lo),"d"(hi),"c"(0xc0000080u));
    __asm__ volatile("rdmsr":"=a"(lo),"=d"(hi):"c"(0xc0000080u));if(lo&1)bad("unconfigured SYSCALL enabled");
    __asm__ volatile("cpuid":"=a"(a0),"=b"(b0),"=c"(c0),"=d"(d0):"a"(1u),"c"(0));
    if(d0&(1u<<11)){
        __asm__ volatile("wrmsr"::"a"(0u),"d"(0u),"c"(0x174u));
        __asm__ volatile("rdmsr":"=a"(lo),"=d"(hi):"c"(0x174u));if(lo||hi)bad("unconfigured SYSENTER enabled");
    }
    uint64_t a;__asm__ volatile("movq %%cr0,%0":"=r"(a));a=(a|2)&~UINT64_C(12);__asm__ volatile("movq %0,%%cr0"::"r"(a));
    __asm__ volatile("movq %%cr4,%0":"=r"(a));a=(a|0x600)&~((UINT64_C(1)<<18)|(UINT64_C(1)<<16)|(UINT64_C(1)<<2));__asm__ volatile("movq %0,%%cr4"::"r"(a));
    configured_cpus|=UINT64_C(1)<<m3_cpu();
}
extern void timer_start(void),timer_stop(void),timer_acknowledge(void);
static NORETURN void bad(const char *s){serial_text("M3_RUNTIME_FAIL ");serial_text(s);serial_text("\n");test_exit(0x31);}
static void check(int yes,const char *s){if(!yes)bad(s);++checks;}
static uint64_t allocate(void){uint64_t p=page_alloc();if(!p)bad("bootstrap allocation");return p;}
static void cr3(uint64_t p){
    uint64_t active;__asm__ volatile("movq %%cr3,%0":"=r"(active)::"memory");
    if(active!=p)__asm__ volatile("movq %0,%%cr3"::"r"(p):"memory");
}
static uint64_t *leaf(address_space *s,uint64_t va){return &((uint64_t*)(uintptr_t)s->pt)[(va>>12)&511];}
static void invalidate(uint64_t va){__asm__ volatile("invlpg (%0)"::"r"(va):"memory");}
static int access(address_space *s,uint64_t va,uint64_t len,int write){
    if(va<USER_BASE||va>=USER_LIMIT||len>USER_LIMIT-va)return 0;
    for(uint64_t a=va&~UINT64_C(4095);a<va+len;a+=4096){uint64_t l=*leaf(s,a);if((l&5)!=5||(write&&!(l&2)))return 0;}return 1;
}
static void copy(address_space *s,uint64_t va,void *buffer,uint64_t len,int output){
    uint8_t *b=buffer;while(len){uint64_t n=4096-(va&4095);if(n>len)n=len;
        void *p=(void*)(uintptr_t)((*leaf(s,va)&MASK)+(va&4095));
        if(output)memcpy(p,b,n);else memcpy(b,p,n);b+=n;va+=n;len-=n;}
}
static backing *new_backing(void){
    for(unsigned i=0;i<64;++i)if(!backings[i].refs){uint64_t p=page_alloc();if(!p)return 0;backings[i]=(backing){p,1};return &backings[i];}
    return 0;
}
static void release(backing *b){if(!b||!b->refs)bad("backing reference");if(!--b->refs){page_free(b->page);b->page=0;}}
static void drop_object(event *e){if(!e||!e->refs)bad("object reference");if(!--e->refs){if(e->memory)release(e->memory);memset(e,0,sizeof(*e));}}
static handle *lookup(address_space *s,uint64_t token){unsigned i=(unsigned)token;if(!i||i>16)return 0;handle *h=&s->handles[i-1];return h->obj&&h->generation==(token>>32)?h:0;}
static unsigned install(address_space *s,event *obj,unsigned rights,uint64_t *token){for(unsigned i=0;i<16;++i){handle *h=&s->handles[i];if(!h->obj&&h->generation!=UINT32_MAX){++h->generation;h->rights=rights;h->obj=obj;*token=((uint64_t)h->generation<<32)|(i+1);return U_OK;}}return U_LIMIT;}
static unsigned writable(address_space *s,uint64_t va,uint64_t length){
    if(!access(s,va,length,0))return U_DENIED;
    for(uint64_t a=va&~UINT64_C(4095);a<va+length;a+=4096){
        if(*leaf(s,a)&2)continue;
        if(a<M3_VM||a>=M3_VM+s->reserved*4096)return U_DENIED;
        unsigned i=(unsigned)((a-M3_VM)/4096);backing *old=s->vm[i];if(!old)return U_DENIED;
        if(old->refs>1){backing *b=new_backing();if(!b)return U_NOMEM;memcpy((void*)(uintptr_t)b->page,(void*)(uintptr_t)old->page,4096);release(old);s->vm[i]=b;}
        *leaf(s,a)=s->vm[i]->page|7|NX;invalidate(a);++copy_cow;
    }return U_OK;
}
static unsigned out(address_space *s,uint64_t va,uint64_t value){unsigned status=writable(s,va,8);if(status)return status;copy(s,va,&value,8,1);return U_OK;}
static address_space *new_space(void){
    address_space *s=0;for(unsigned i=0;i<2;++i)if(!spaces[i].live){s=&spaces[i];break;}if(!s)return 0;
    memset(s,0,sizeof(*s));s->live=1;s->pid=++next_pid;
    for(unsigned i=0;i<5;++i){uint64_t page=page_alloc();if(!page){while(s->count)page_free(s->owned[--s->count]);s->live=0;return 0;}s->owned[s->count++]=page;}
    s->cr3=s->owned[0];uint64_t pdpt=s->owned[1],pd=s->owned[2];s->pt=s->owned[3];
    ((uint64_t*)(uintptr_t)s->cr3)[0]=pdpt|7;((uint64_t*)(uintptr_t)s->cr3)[256]=kernel_stack_branch()|3;
    for(unsigned i=0;i<4;++i)((uint64_t*)(uintptr_t)pdpt)[i]=kernel_pd(i)|3;
    ((uint64_t*)(uintptr_t)pdpt)[4]=pd|7;((uint64_t*)(uintptr_t)pd)[0]=s->pt|7;
    for(unsigned i=0;i<image.count;++i)*leaf(s,image.vas[i])=image.pages[i]|image.flags[i];
    memcpy((void*)(uintptr_t)s->owned[4],image.data,4096);*leaf(s,USER_DATA)=s->owned[4]|7|NX;return s;
}
static void destroy_space(address_space *s){
    cr3(image.kernel_cr3);for(unsigned i=0;i<16;++i)if(s->handles[i].obj)drop_object(s->handles[i].obj);
    if(s->mapping)drop_object(s->mapping);
    for(unsigned i=0;i<M3_PAGES;++i)if(s->vm[i])release(s->vm[i]);
    while(s->count)page_free(s->owned[--s->count]);s->live=0;
}
static thread *find(uint64_t token){unsigned slot=(unsigned)token;if(!slot||slot>M3_THREADS)return 0;thread *t=&threads[slot-1];return t->state&&t->token==token?t:0;}
static thread *spawn(address_space *s,uint64_t role){
    for(unsigned i=0;i<M3_THREADS;++i)if(threads[i].state==FREE){thread *t=&threads[i];unsigned generation=t->generation+1;if(!generation)continue;
        memset(t,0,sizeof(*t));t->slot=i;t->generation=generation;t->token=((uint64_t)generation<<32)|(i+1);t->space=s;t->role=(unsigned)role;
        t->stack=page_alloc();if(!t->stack)return 0;t->tls=page_alloc();if(!t->tls){page_free(t->stack);return 0;}
        *leaf(s,STACK(i))=t->stack|7|NX;*leaf(s,TLS(i))=t->tls|7|NX;
        t->frame=(trap_frame){.rip=image.entry,.cs=0x1b,.ss=0x23,.rflags=0x202,.rsp=STACK(i)+4088,.rdi=role};
        t->fx[0]=0x7f;t->fx[1]=3;t->fx[24]=0x80;t->fx[25]=0x1f;
        t->state=RUN;++live;++started;return t;
    }
    return 0;
}
static void wake(thread *t,unsigned status){if(t->wait_object){drop_object(t->wait_object);t->wait_object=0;}t->state=RUN;t->wait=0;t->frame.rax=status;}
static void end(thread *t,int cancel){
    if(t->state!=RUN&&t->state!=WAIT)bad("thread lifecycle");
    address_space *s=t->space;if(t->wait_object){drop_object(t->wait_object);t->wait_object=0;}t->state=ZOMBIE;t->result=cancel==2?U_DENIED:cancel?U_CANCELLED:U_OK;--live;if(cancel==2)++fault_exits;else if(cancel)++cancelled;else ++exited;
    *leaf(s,STACK(t->slot))=0;*leaf(s,TLS(t->slot))=0;invalidate(STACK(t->slot));invalidate(TLS(t->slot));
    page_free(t->stack);page_free(t->tls);t->stack=t->tls=0;
    for(unsigned i=0;i<M3_THREADS;++i)if(threads[i].state==WAIT&&threads[i].wait==W_JOIN&&threads[i].target==t->token){wake(&threads[i],t->result);t->state=FREE;}
    unsigned remaining=0;for(unsigned i=0;i<M3_THREADS;++i)remaining+=(threads[i].space==s&&(threads[i].state==RUN||threads[i].state==WAIT));
    if(!remaining){destroy_space(s);for(unsigned i=0;i<M3_THREADS;++i){
        if(threads[i].join_owner==s->pid)threads[i].join_owner=0;
        if(threads[i].state==ZOMBIE&&!threads[i].join_owner)threads[i].state=FREE;
    }}

}
static void tick_locked(void){
    uint64_t now=m3_clock_ticks();if(now<ticks)bad("monotonic clock regression");ticks=now;for(unsigned i=0;i<M3_THREADS;++i){thread *t=&threads[i];if(t->state==WAIT&&t->deadline!=UINT64_MAX&&ticks>=t->deadline){
        uint64_t late=ticks-t->deadline;if(late>deadline_lateness)deadline_lateness=late;wake(t,t->wait==W_SLEEP?U_OK:U_TIMEOUT);}}
}
void m3_tick(void){lock();tick_locked();unlock();}
static unsigned block(thread *t,unsigned why,uint64_t target,uint64_t duration){
    if(!duration)return U_TIMEOUT;if(duration!=UINT64_MAX&&(duration>1000||duration>UINT64_MAX-ticks))return U_INVALID;
    t->state=WAIT;t->wait=why;t->target=target;t->deadline=duration==UINT64_MAX?UINT64_MAX:ticks+duration;return U_OK;
}
static address_space *target_space(uint64_t id){thread *t=find(id);if(t&&t->state!=ZOMBIE)return t->space;for(unsigned i=0;i<2;++i)if(spaces[i].live&&spaces[i].pid==id)return &spaces[i];return 0;}
static unsigned receive(thread *t,uint64_t va){address_space *s=t->space;unsigned status=writable(s,va,sizeof(user_message));if(status)return status;if(!s->messages)return U_EMPTY;
    copy(s,va,&s->queue[s->head],sizeof(user_message),1);s->head=(s->head+1)%4;--s->messages;return U_OK;}
static unsigned syscall(thread *t){
    trap_frame *f=&t->frame;address_space *s=t->space;uint64_t a=f->rdi,b=f->rsi,c=f->rdx,d=f->r10;event *e;handle *h;thread *other;
    switch(f->rax){
    case S_WRITE:{if(b>128||!access(s,a,b,0))return U_DENIED;char text[129];copy(s,a,text,b,0);text[b]=0;serial_text("M3_USER: ");serial_text(text);serial_text("\n");return U_OK;}
    case S_YIELD:return U_OK;
    case S_PID:return out(s,a,s->pid);
    case S_TID:return out(s,a,t->token);
    case S_THREAD_CREATE:{if(!access(s,a,1,0)||(*leaf(s,a)&NX))return U_DENIED;unsigned status=writable(s,b,8);if(status)return status;
        unsigned available=0;for(unsigned i=0;i<M3_THREADS;++i)available+=threads[i].state==FREE&&threads[i].generation!=UINT32_MAX;if(!available)return U_LIMIT;
        other=spawn(s,c);if(!other)return U_NOMEM;other->frame.rip=a;return out(s,b,other->token);}
    case S_THREAD_EXIT:if(a)++unexpected;end(t,0);return U_OK;
    case S_EXIT:
        if(a)++unexpected;for(unsigned i=0;i<M3_THREADS;++i)if(&threads[i]!=t&&threads[i].space==s&&(threads[i].state==RUN||threads[i].state==WAIT))end(&threads[i],1);end(t,0);return U_OK;
    case S_THREAD_CANCEL:other=find(a);if(!other||other->space!=s||other==t)return U_INVALID;if(other->state==ZOMBIE)return U_INVALID;end(other,1);return U_OK;
    case S_JOIN:other=find(a);if(!other||other==t||(other->space!=s&&!t->test_admin))return U_INVALID;if(other->state==ZOMBIE){unsigned result=other->result;other->state=FREE;return result;}return block(t,W_JOIN,a,b);
    case S_TLS:return out(s,a,TLS(t->slot));
    case S_TIME:return out(s,a,ticks);
    case S_CLOCK_TEST:if(!t->test_admin)return U_DENIED;m3_clock_probe();return U_OK;
    case S_SLEEP:return block(t,W_SLEEP,0,a);
    case S_EVENT_CREATE:case S_MEMORY_CREATE:{
        unsigned status=writable(s,a,8);if(status)return status;if(f->rax==S_EVENT_CREATE&&b>1)return U_INVALID;
        e=0;for(unsigned i=0;i<32;++i)if(!objects[i].refs){e=&objects[i];break;}if(!e)return U_LIMIT;
        *e=(event){.refs=1,.type=f->rax==S_EVENT_CREATE?1:2,.manual=(unsigned)b};
        if(e->type==2){e->memory=new_backing();if(!e->memory){drop_object(e);return U_NOMEM;}}
        uint64_t token=0;status=install(s,e,R_ALL,&token);if(status){drop_object(e);return status;}return out(s,a,token);}
    case S_EVENT_SIGNAL:case S_EVENT_RESET:case S_EVENT_QUERY:case S_WAIT:
        h=lookup(s,a);if(!h)return U_BAD_HANDLE;e=h->obj;if(e->type!=1)return U_BAD_TYPE;
        if(!(h->rights&((f->rax==S_EVENT_QUERY||f->rax==S_WAIT)?R_READ:R_WRITE)))return U_DENIED;
        if(f->rax==S_EVENT_QUERY)return out(s,b,e->signaled);
        if(f->rax==S_EVENT_RESET){e->signaled=0;return U_OK;}
        if(f->rax==S_WAIT){if(e->signaled){if(!e->manual)e->signaled=0;return U_OK;}unsigned status=block(t,W_EVENT,a,b);if(t->state==WAIT){t->wait_object=e;++e->refs;}return status;}
        e->signaled=1;
        for(unsigned i=0;i<M3_THREADS;++i){other=&threads[i];if(other->state==WAIT&&other->wait_object==e){wake(other,U_OK);if(!e->manual){e->signaled=0;break;}}}return U_OK;
    case S_DUP:{
        h=lookup(s,a);if(!h)return U_BAD_HANDLE;if(!b||b&~(uint64_t)h->rights)return U_DENIED;
        unsigned status=writable(s,c,8);if(status)return status;uint64_t token=0;status=install(s,h->obj,(unsigned)b,&token);if(status)return status;++h->obj->refs;return out(s,c,token);}
    case S_CLOSE:
        h=lookup(s,a);if(!h)return U_BAD_HANDLE;
        for(unsigned i=0;i<M3_THREADS;++i)if(threads[i].space==s&&threads[i].state==WAIT&&threads[i].wait==W_EVENT&&threads[i].target==a)wake(&threads[i],U_CANCELLED);
        drop_object(h->obj);h->obj=0;h->rights=0;return U_OK;
    case S_MEMORY_MAP:
        h=lookup(s,a);if(!h)return U_BAD_HANDLE;if(h->obj->type!=2)return U_BAD_TYPE;
        if(!(h->rights&R_READ)||b!=USER_SHARED)return U_DENIED;if(s->mapping)return U_LIMIT;
        s->mapping=h->obj;++s->mapping->refs;*leaf(s,USER_SHARED)=s->mapping->memory->page|5|NX|((h->rights&R_WRITE)?2:0);invalidate(USER_SHARED);return U_OK;
    case S_TRANSFER:{
        h=lookup(s,a);if(!h)return U_BAD_HANDLE;if(!(h->rights&R_TRANSFER)||!c||c&~(uint64_t)h->rights)return U_DENIED;
        address_space *target=target_space(b);if(!target)return U_INVALID;unsigned status=writable(s,d,8);if(status)return status;
        uint64_t token=0;status=install(target,h->obj,(unsigned)c,&token);if(status)return status;++h->obj->refs;return out(s,d,token);}
    case S_SEND:{
        address_space *target=target_space(a);if(!target||c>32)return U_INVALID;if(!access(s,b,c,0))return U_DENIED;
        if(target->messages==4)return U_LIMIT;
        user_message *m=&target->queue[(target->head+target->messages)%4];memset(m,0,sizeof(*m));m->sender=s->pid;m->length=c;copy(s,b,m->data,c,0);++target->messages;
        for(unsigned i=0;i<M3_THREADS;++i)if(threads[i].space==target&&threads[i].state==WAIT&&threads[i].wait==W_MESSAGE){wake(&threads[i],receive(&threads[i],threads[i].output));break;}return U_OK;}
    case S_RECEIVE:return receive(t,a);
    case S_RECEIVE_WAIT:{unsigned status=receive(t,a);if(status!=U_EMPTY)return status;t->output=a;return block(t,W_MESSAGE,0,b);}
    case S_VM_RESERVE:if(!a||a>M3_PAGES||s->reserved)return U_INVALID;s->reserved=(unsigned)a;return U_OK;
    case S_VM_COMMIT:{
        if(!b||a>=s->reserved||b>s->reserved-a)return U_INVALID;
        for(uint64_t i=a;i<a+b;++i)if(s->vm[i])return U_BUSY;
        uint64_t i=a;for(;i<a+b;++i){s->vm[i]=new_backing();if(!s->vm[i])break;}
        if(i!=a+b){while(i>a){--i;release(s->vm[i]);s->vm[i]=0;}return U_NOMEM;}
        for(i=a;i<a+b;++i){*leaf(s,M3_VM+i*4096)=s->vm[i]->page|7|NX;invalidate(M3_VM+i*4096);}return U_OK;}
    case S_VM_DECOMMIT:
        if(!b||a>=s->reserved||b>s->reserved-a)return U_INVALID;
        for(uint64_t i=a;i<a+b;++i)if(s->vm[i]){*leaf(s,M3_VM+i*4096)=0;invalidate(M3_VM+i*4096);release(s->vm[i]);s->vm[i]=0;}return U_OK;
    case S_VM_CLONE:{
        if(!t->test_admin)return U_DENIED;if(c>1||!access(s,a,1,0)||(*leaf(s,a)&NX))return U_INVALID;if(d){unsigned status=writable(s,d,8);if(status)return status;}if(spaces[0].live&&spaces[1].live)return U_LIMIT;address_space *child=new_space();if(!child)return U_NOMEM;child->reserved=s->reserved;
        for(unsigned i=0;i<s->reserved;++i)if(s->vm[i]){child->vm[i]=s->vm[i];++s->vm[i]->refs;*leaf(child,M3_VM+i*4096)=s->vm[i]->page|5|NX;*leaf(s,M3_VM+i*4096)&=~UINT64_C(2);invalidate(M3_VM+i*4096);}
        child->cpu=cpu_count>1?1+(next_cpu%(cpu_count-1)):0;
        other=spawn(child,b);if(!other){destroy_space(child);for(unsigned i=0;i<s->reserved;++i)if(s->vm[i]&&s->vm[i]->refs==1){*leaf(s,M3_VM+i*4096)|=2;invalidate(M3_VM+i*4096);}return U_NOMEM;}other->frame.rip=a;other->test_admin=(unsigned)c;++next_cpu;
        if(d){other->join_owner=s->pid;(void)out(s,d,other->token);}
        if(child->cpu)m3_smp_launch(child->cpu);return U_OK;}
    case S_PRESSURE:
        /* Privileged test harness budget control, absent from guest ABI profiles. */
        if(!t->test_admin)return U_DENIED;if(a>8)return U_INVALID;
        m3_smp_pressure(a!=0);pages_limit(a?pages_stats().current_pages+a:0);return U_OK;
    default:(void)d;return U_UNSUPPORTED;
    }
}
static trap_frame *select_thread(void){
    unsigned cpu=m3_cpu();
    for(;;){
        for(unsigned n=1;n<=M3_THREADS;++n){unsigned i=(current[cpu]+n)%M3_THREADS;thread *t=&threads[i];if(t->state!=RUN||t->space->cpu!=cpu)continue;
            uint64_t start=m3_tsc();current[cpu]=i;user_cpu_mask|=UINT64_C(1)<<cpu;cr3(t->space->cr3);
            if(!access(t->space,t->frame.rip,1,0)||(*leaf(t->space,t->frame.rip)&NX)||!access(t->space,t->frame.rsp,8,1)){tick_locked();if(t->role!=27)++unexpected;end(t,2);continue;}
            t->frame.cs=0x1b;t->frame.ss=0x23;t->frame.rflags=(t->frame.rflags&0xcd5)|0x202;
            uint64_t fs=TLS(i);__asm__ volatile("wrmsr"::"c"(0xc0000100u),"a"((uint32_t)fs),"d"((uint32_t)(fs>>32)));
            __asm__ volatile("fxrstor64 %0"::"m"(t->fx):"memory");
            uint64_t cost=m3_tsc()-start;dispatch_cycles+=cost;if(switches>=16&&sample_count<256)samples[sample_count++]=cost;if(cost>max_dispatch)max_dispatch=cost;++switches;unlock();return &t->frame;
        }
        if(cpu){
            unsigned waiting=0;for(unsigned i=0;i<M3_THREADS;++i)waiting+=(threads[i].space&&threads[i].space->cpu==cpu&&(threads[i].state==RUN||threads[i].state==WAIT));
            if(!waiting){cr3(image.kernel_cr3);trap_frame *f=&ap_idle_frames[cpu];memset(f,0,sizeof(*f));*f=(trap_frame){.rip=(uint64_t)(uintptr_t)m3_ap_return,.cs=8,.ss=16,.rflags=0x202,.rsp=KERNEL_STACK+(uint64_t)cpu*0x10000+16376,.rdi=cpu};unlock();return f;}
        }
        if(!live){
            timer_stop();m3_active=0;pages_limit(0);unlock();cr3(image.kernel_cr3);
            for(unsigned i=0;i<4;++i){for(unsigned j=0;j<512;++j)check(!((uint64_t*)(uintptr_t)emergency[i])[j],"emergency reserve intact");page_free(emergency[i]);}
            check(pages_stats().current_pages==baseline,"runtime page accounting");
            for(unsigned i=0;i<32;++i)check(!objects[i].refs,"object accounting");
            for(unsigned i=0;i<64;++i)check(!backings[i].refs,"backing accounting");
            check(started==85&&exited==73&&cancelled==10&&cow_faults==2&&copy_cow==1&&oom_faults==1&&fault_exits==2&&!unexpected,"workload totals");
            serial_text("M3_DEADLINE_OBSERVATION lateness=");serial_hex(deadline_lateness);serial_text(" idle_halts=");serial_hex(idle_halts);serial_text("\n");
            check(idle_halts>0,"interrupt-aware idle");
            for(unsigned i=0;i<M3_THREADS;++i)check(threads[i].state==FREE,"thread record accounting");
            m3_smp_test();m3_smp_stop();
            for(uint64_t i=1;i<sample_count;++i){uint64_t v=samples[i],j=i;while(j&&samples[j-1]>v){samples[j]=samples[j-1];--j;}samples[j]=v;}
            check(sample_count==256,"dispatch samples");check(configured_cpus==((UINT64_C(1)<<cpu_count)-1),"per-CPU CPU-state policy");check(user_cpu_mask==((UINT64_C(1)<<cpu_count)-1),"user execution on active CPUs");
            serial_text("M3_DISPATCH_TSC samples=");serial_hex(sample_count);serial_text(" min=");serial_hex(samples[0]);serial_text(" p50=");serial_hex(samples[sample_count/2]);serial_text(" p95=");serial_hex(samples[sample_count*95/100]);serial_text(" max=");serial_hex(samples[sample_count-1]);serial_text("\n");
            serial_text("M3_CPU_STATE_MASK=");serial_hex(configured_cpus);serial_text("\n");
            serial_text("M3_USER_CPU_MASK=");serial_hex(user_cpu_mask);serial_text("\n");
            serial_text("M3_RUNTIME_STATS threads=");serial_hex(started);serial_text(" exits=");serial_hex(exited);serial_text(" cancellations=");serial_hex(cancelled);
            serial_text(" isolated_faults=");serial_hex(fault_exits);serial_text(" cow_faults=");serial_hex(cow_faults);serial_text(" copy_cow=");serial_hex(copy_cow);serial_text(" oom_faults=");serial_hex(oom_faults);serial_text(" idle_halts=");serial_hex(idle_halts);
            serial_text(" switches=");serial_hex(switches);serial_text(" dispatch_tsc_total=");serial_hex(dispatch_cycles);serial_text(" dispatch_tsc_max=");serial_hex(max_dispatch);
            serial_text(" deadline_late_ticks=");serial_hex(deadline_lateness);serial_text(" runtime_peak_owned_bytes=");serial_hex(pages_window_peak(0)*4096);serial_text(" thread_resident_bytes=");serial_hex(sizeof(thread)+8192);serial_text(" thread_metadata_bytes=");serial_hex(sizeof(thread));serial_text(" checks=");serial_hex(checks);serial_text("\nM3_RUNTIME_PASS\n");done();bad("completion returned");
        }
        ++idle_halts;unlock();__asm__ volatile("sti; hlt; cli":::"memory");lock();
    }
}
trap_frame *m3_trap(trap_frame *f){
    thread *t=&threads[current[m3_cpu()]];__asm__ volatile("fxsave64 %0":"=m"(t->fx)::"memory");lock();memcpy(&t->frame,f,sizeof(*f));
    if(f->vector!=128)tick_locked();
    else switch(f->rax){
        case S_TIME:case S_SLEEP:case S_WAIT:case S_JOIN:case S_RECEIVE_WAIT:case S_EVENT_SIGNAL:
        case S_CLOSE:case S_THREAD_CANCEL:case S_THREAD_EXIT:case S_EXIT:case S_SEND:tick_locked();break;
        default:break;
    }
    if(f->vector==32){timer_acknowledge();}
    else if(f->vector==241){}
    else if(f->vector==128)t->frame.rax=syscall(t);
    else if(f->vector==14){uint64_t va;__asm__ volatile("movq %%cr2,%0":"=r"(va));address_space *s=t->space;
        if(va<M3_VM||va>=M3_VM+s->reserved*4096||(f->error&7)!=7){
            if(t->role!=26||va!=M3_VM+15*4096||(f->error&7)!=4)++unexpected;
            end(t,2);return select_thread();}
        unsigned i=(unsigned)((va-M3_VM)/4096);backing *old=s->vm[i];if(!old)bad("uncommitted write");
        if(old->refs>1){backing *b=new_backing();if(!b){++oom_faults;pages_limit(0);m3_smp_pressure(0);end(t,1);return select_thread();}memcpy((void*)(uintptr_t)b->page,(void*)(uintptr_t)old->page,4096);release(old);s->vm[i]=b;}
        *leaf(s,M3_VM+i*4096)=s->vm[i]->page|7|NX;invalidate(M3_VM+i*4096);++cow_faults;
    }else {++unexpected;end(t,2);}
    return select_thread();
}
NORETURN void m3_start(const m3_image *program,void (*callback)(void)){
    image=*program;done=callback;cr3(image.kernel_cr3);
    cpu_count=m3_smp_start(image.boot,image.kernel_cr3);check(cpu_count>=1,"CPU startup");for(unsigned i=0;i<M3_CPUS;++i)current[i]=M3_THREADS-1;baseline=pages_stats().current_pages;ticks=m3_clock_ticks();(void)pages_window_peak(1);
    for(unsigned i=0;i<4;++i)emergency[i]=allocate();
    cpu_state_init();
    address_space *initial=new_space();check(initial!=0,"initial address space");thread *root=spawn(initial,16);check(root!=0,"initial thread");root->test_admin=1;m3_active=1;timer_start();lock();arch_enter_user(select_thread());
}

NORETURN void m3_ap_enter(void){__asm__ volatile("cli");cpu_state_init();lock();arch_enter_user(select_thread());}

void m3_summary(unsigned *cpus,unsigned *count){*cpus=cpu_count;*count=started;}
