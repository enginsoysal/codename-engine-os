#include <engine/user.h>
#define VM UINT64_C(0x100080000)
static volatile uint64_t event_handle,progress;
static uint64_t bench_event,roundtrips[64];static unsigned ready_count,roundtrip_count;
extern void user_main(uint64_t);
static uint64_t call(uint64_t n,uint64_t a,uint64_t b,uint64_t c,uint64_t d){
    register uint64_t ax __asm__("rax")=n;register uint64_t r10 __asm__("r10")=d;
    __asm__ volatile("int $0x80":"+a"(ax):"D"(a),"S"(b),"d"(c),"r"(r10):"cc","memory");return ax;
}
static void say(const char *s){uint64_t n=0;while(s[n])++n;call(S_WRITE,(uint64_t)(uintptr_t)s,n,0,0);}
static __attribute__((noreturn)) void fail(void){say("ADVANCED_ASSERT_FAIL");call(S_EXIT,1,0,0,0);for(;;){}}
static void eq(uint64_t a,uint64_t b){if(a!=b)fail();}
static uint64_t spawn(uint64_t role){uint64_t t=0;eq(call(S_THREAD_CREATE,(uint64_t)(uintptr_t)user_main,(uint64_t)(uintptr_t)&t,role,0),U_OK);return t;}
static void sleep_for(uint64_t ticks){eq(call(S_SLEEP,ticks,0,0,0),U_OK);}
static void join(uint64_t t,uint64_t result){eq(call(S_JOIN,t,500,0,0),result);}
static uint64_t event_create(uint64_t manual){uint64_t e=0;eq(call(S_EVENT_CREATE,(uint64_t)(uintptr_t)&e,manual,0,0),U_OK);return e;}
static uint64_t time_now(void){uint64_t t=0;eq(call(S_TIME,(uint64_t)(uintptr_t)&t,0,0,0),U_OK);return t;}
static uint64_t stamp(void){uint32_t a,d;__asm__ volatile("lfence; rdtsc; lfence":"=a"(a),"=d"(d)::"memory");return ((uint64_t)d<<32)|a;}
static void roundtrip_worker(void){
    if(__atomic_add_fetch(&ready_count,1,__ATOMIC_SEQ_CST)==2)eq(call(S_EVENT_SIGNAL,bench_event,0,0,0),U_OK);
    eq(call(S_WAIT,bench_event,100,0,0),U_OK);
    for(unsigned i=0;i<40;++i){uint64_t before=stamp();eq(call(S_YIELD,0,0,0,0),U_OK);uint64_t delta=stamp()-before;
        if(i>=4&&i<36){unsigned slot=__atomic_fetch_add(&roundtrip_count,1,__ATOMIC_SEQ_CST);if(slot>=64)fail();roundtrips[slot]=delta;}}
}
static char *append(char *p,const char *s){while(*s)*p++=*s++;return p;}
static char *hex(char *p,uint64_t value){p=append(p,"0x");for(int i=60;i>=0;i-=4)*p++="0123456789abcdef"[(value>>i)&15];return p;}
static void roundtrip_report(void){
    eq(roundtrip_count,64);for(unsigned i=1;i<64;++i){uint64_t v=roundtrips[i];unsigned j=i;while(j&&roundtrips[j-1]>v){roundtrips[j]=roundtrips[j-1];--j;}roundtrips[j]=v;}if(!roundtrips[0])fail();
    char buffer[128],*p=append(buffer,"M3_ROUNDTRIP_TSC samples=");p=hex(p,64);p=append(p," min=");p=hex(p,roundtrips[0]);*p=0;say(buffer);
    p=append(buffer,"M3_ROUNDTRIP_PERCENTILES p50=");p=hex(p,roundtrips[32]);p=append(p," p95=");p=hex(p,roundtrips[60]);p=append(p," max=");p=hex(p,roundtrips[63]);*p=0;say(buffer);
}
__attribute__((noreturn)) void m3_user(uint64_t role){
    if(role>=24){uint64_t marker=role,observed=0;__asm__ volatile("movq %0,%%xmm15"::"r"(marker));sleep_for(1);__asm__ volatile("movq %%xmm15,%0":"=r"(observed));eq(observed,marker);}
    if(role==17||role==18){
        uint64_t pattern=role==17?UINT64_C(0x1122334455667788):UINT64_C(0x8877665544332211),value=role,x=0,y=0,tls=0;
        uint32_t control=role==17?0x1f80:0x3f80,observed=0;
        eq(call(S_TLS,(uint64_t)(uintptr_t)&tls,0,0,0),U_OK);*(uint64_t*)(uintptr_t)tls=pattern;
        __asm__ volatile("movq %0,%%xmm0; movq %0,%%xmm15; fildq %1; ldmxcsr %2"::"r"(pattern),"m"(value),"m"(control):"memory");
        for(unsigned i=0;i<32;++i){
            sleep_for(1);
            __asm__ volatile("movq %%xmm0,%0; movq %%xmm15,%1; fistpq %2; stmxcsr %3":"=r"(x),"=r"(y),"=m"(value),"=m"(observed)::"memory");
            eq(x,pattern);eq(y,pattern);eq(value,role);eq(observed,control);
            __asm__ volatile("movq %%fs:0,%0":"=r"(x));eq(x,pattern);__asm__ volatile("fildq %0"::"m"(value));
        }
        roundtrip_worker();
    }else if(role==19){sleep_for(2);eq(call(S_EVENT_SIGNAL,event_handle,0,0,0),U_OK);}
    else if(role==20){eq(call(S_WAIT,event_handle,100,0,0),U_OK);__atomic_fetch_add(&progress,1,__ATOMIC_SEQ_CST);}
    else if(role==21){eq(call(S_WAIT,event_handle,100,0,0),U_CANCELLED);}
    else if(role==22){sleep_for(1000);fail();}
    else if(role==23){user_message m;eq(call(S_RECEIVE_WAIT,(uint64_t)(uintptr_t)&m,100,0,0),U_OK);eq(m.length,4);eq(m.data[0],'p');}
    else if(role==24){sleep_for(2);user_message msg;eq(call(S_RECEIVE_WAIT,(uint64_t)(uintptr_t)&msg,100,0,0),U_OK);eq(msg.length,sizeof(user_payload));const user_payload *payload=(const user_payload*)msg.data;
        eq(call(S_MEMORY_MAP,payload->handle,UINT64_C(0x100020000),0,0),U_OK);eq(call(S_CLOSE,payload->handle,0,0,0),U_OK);eq(*(volatile uint64_t*)(uintptr_t)UINT64_C(0x100020000),payload->marker);say("M3_TRANSFER_IPC_SHARED_LIFETIME_OK");eq(*(volatile uint64_t*)(uintptr_t)VM,0x111);eq(call(S_TIME,VM+4104,0,0,0),U_OK);*(volatile uint64_t*)(uintptr_t)VM=0x333;eq(*(volatile uint64_t*)(uintptr_t)VM,0x333);say("COW_CHILD_PRIVATE_WRITE_OK");say("AP_SIMD_CONTEXT_OK");}
    else if(role==25){sleep_for(2);eq(call(S_PRESSURE,1,0,0,0),U_OK);eq(call(S_VM_COMMIT,2,1,0,0),U_OK);sleep_for(3);*(volatile uint64_t*)(uintptr_t)VM=0x999;fail();}
    else if(role==26){volatile uint64_t v=*(volatile uint64_t*)(uintptr_t)(VM+15*4096);(void)v;fail();}
    else if(role==27){__asm__ volatile("movabsq $0x8000000000000000,%rsp; movq $9999,%rax; int $0x80; ud2");__builtin_unreachable();}
    else if(role==16){
        uint64_t h=event_create(0),read=0,value=0,self=0;eq(call(S_TID,(uint64_t)(uintptr_t)&self,0,0,0),U_OK);
        eq(call(S_DUP,h,R_READ,(uint64_t)(uintptr_t)&read,0),U_OK);eq(call(S_EVENT_SIGNAL,read,0,0,0),U_DENIED);eq(call(S_DUP,read,R_ALL,(uint64_t)(uintptr_t)&value,0),U_DENIED);
        eq(call(S_TRANSFER,read,self,R_READ,(uint64_t)(uintptr_t)&value),U_DENIED);eq(call(S_EVENT_SIGNAL,h,0,0,0),U_OK);eq(call(S_EVENT_QUERY,read,(uint64_t)(uintptr_t)&value,0,0),U_OK);eq(value,1);
        eq(call(S_CLOSE,h,0,0,0),U_OK);eq(call(S_EVENT_QUERY,h,(uint64_t)(uintptr_t)&value,0,0),U_BAD_HANDLE);eq(call(S_CLOSE,read,0,0,0),U_OK);
        uint64_t handles[16];for(unsigned i=0;i<16;++i)handles[i]=event_create(0);eq(call(S_EVENT_CREATE,(uint64_t)(uintptr_t)&h,0,0,0),U_LIMIT);for(unsigned i=0;i<16;++i)eq(call(S_CLOSE,handles[i],0,0,0),U_OK);
        eq(call(S_MEMORY_CREATE,(uint64_t)(uintptr_t)&h,0,0,0),U_OK);eq(call(S_EVENT_SIGNAL,h,0,0,0),U_BAD_TYPE);eq(call(S_MEMORY_MAP,h,UINT64_C(0x100020000),0,0),U_OK);*(volatile uint64_t*)(uintptr_t)UINT64_C(0x100020000)=0xabcdef;eq(call(S_CLOSE,h,0,0,0),U_OK);eq(*(volatile uint64_t*)(uintptr_t)UINT64_C(0x100020000),0xabcdef);say("M3_HANDLE_RIGHTS_QUOTA_AND_MAPPING_LIFETIME_OK");
        bench_event=event_create(1);uint64_t a=spawn(17),b=spawn(18);join(a,U_OK);join(b,U_OK);eq(call(S_CLOSE,bench_event,0,0,0),U_OK);roundtrip_report();say("SSE_X87_MXCSR_TLS_ISOLATION_OK");
        eq(call(S_CLOCK_TEST,0,0,0,0),U_OK);
        event_handle=event_create(0);eq(call(S_EVENT_SIGNAL,event_handle,0,0,0),U_OK);eq(call(S_WAIT,event_handle,0,0,0),U_OK);eq(call(S_WAIT,event_handle,0,0,0),U_TIMEOUT);eq(call(S_CLOSE,event_handle,0,0,0),U_OK);
        event_handle=event_create(1);eq(call(S_EVENT_SIGNAL,event_handle,0,0,0),U_OK);eq(call(S_WAIT,event_handle,0,0,0),U_OK);eq(call(S_WAIT,event_handle,0,0,0),U_OK);eq(call(S_EVENT_RESET,event_handle,0,0,0),U_OK);eq(call(S_WAIT,event_handle,2,0,0),U_TIMEOUT);eq(call(S_CLOSE,event_handle,0,0,0),U_OK);
        for(unsigned i=0;i<32;++i){event_handle=event_create(0);a=spawn(20);b=spawn(19);join(a,U_OK);join(b,U_OK);eq(call(S_CLOSE,event_handle,0,0,0),U_OK);}
        event_handle=event_create(1);a=spawn(20);b=spawn(20);uint64_t signaler=spawn(19);join(a,U_OK);join(b,U_OK);join(signaler,U_OK);eq(call(S_CLOSE,event_handle,0,0,0),U_OK);
        eq(progress,34);say("EVENT_ATOMIC_WAIT_WAKE_REUSE_OK");
        event_handle=event_create(0);a=spawn(21);sleep_for(1);eq(call(S_CLOSE,event_handle,0,0,0),U_OK);join(a,U_OK);
        a=spawn(22);sleep_for(1);eq(call(S_THREAD_CANCEL,a,0,0,0),U_OK);join(a,U_CANCELLED);eq(call(S_THREAD_CANCEL,a,0,0,0),U_INVALID);
        uint64_t ids[7],extra=0;
        eq(call(S_THREAD_CREATE,UINT64_C(0x100008000),(uint64_t)(uintptr_t)&extra,22,0),U_DENIED);
        eq(call(S_THREAD_CREATE,(uint64_t)(uintptr_t)user_main,0x200000,22,0),U_DENIED);eq(call(S_THREAD_CREATE,0x200000,(uint64_t)(uintptr_t)&extra,22,0),U_DENIED);
        eq(call(S_PRESSURE,1,0,0,0),U_OK);sleep_for(3);eq(call(S_THREAD_CREATE,(uint64_t)(uintptr_t)user_main,(uint64_t)(uintptr_t)&extra,22,0),U_NOMEM);eq(call(S_PRESSURE,0,0,0,0),U_OK);
        for(unsigned i=0;i<7;++i)ids[i]=spawn(22);
        eq(call(S_THREAD_CREATE,(uint64_t)(uintptr_t)user_main,(uint64_t)(uintptr_t)&extra,22,0),U_LIMIT);
        for(unsigned i=0;i<7;++i){eq(call(S_THREAD_CANCEL,ids[i],0,0,0),U_OK);join(ids[i],U_CANCELLED);}
        say("THREAD_QUOTA_NOMEM_ROLLBACK_AND_REUSE_OK");
        uint64_t before=time_now();sleep_for(3);uint64_t after=time_now();if(after<before+3)fail();say("DEADLINE_CANCEL_CLOSE_AND_STALE_THREAD_OK");
        user_message empty;eq(call(S_RECEIVE_WAIT,(uint64_t)(uintptr_t)&empty,2,0,0),U_TIMEOUT);
        a=spawn(23);sleep_for(2);eq(call(S_SEND,a,(uint64_t)(uintptr_t)"ping",4,0),U_OK);join(a,U_OK);say("BLOCKING_MAILBOX_OK");
        eq(call(S_VM_RESERVE,16,0,0,0),U_OK);eq(call(S_VM_COMMIT,0,2,0,0),U_OK);eq(call(S_VM_COMMIT,16,1,0,0),U_INVALID);eq(call(S_VM_COMMIT,0,1,0,0),U_BUSY);*(volatile uint64_t*)(uintptr_t)VM=0x111;*(volatile uint64_t*)(uintptr_t)(VM+4096)=0x222;
        eq(call(S_PRESSURE,1,0,0,0),U_OK);sleep_for(3);eq(call(S_VM_CLONE,(uint64_t)(uintptr_t)user_main,24,0,0),U_NOMEM);eq(call(S_PRESSURE,0,0,0,0),U_OK);eq(call(S_PRESSURE,6,0,0,0),U_OK);sleep_for(3);eq(call(S_VM_CLONE,(uint64_t)(uintptr_t)user_main,24,0,0),U_NOMEM);eq(call(S_PRESSURE,0,0,0,0),U_OK);
        uint64_t child=0,transferred=0;eq(call(S_VM_CLONE,(uint64_t)(uintptr_t)user_main,24,0,(uint64_t)(uintptr_t)&child),U_OK);
        eq(call(S_MEMORY_CREATE,(uint64_t)(uintptr_t)&h,0,0,0),U_OK);eq(call(S_TRANSFER,h,child,R_READ,(uint64_t)(uintptr_t)&transferred),U_OK);user_payload payload={transferred,0};eq(call(S_SEND,child,(uint64_t)(uintptr_t)&payload,sizeof(payload),0),U_OK);eq(call(S_CLOSE,h,0,0,0),U_OK);join(child,U_OK);eq(*(volatile uint64_t*)(uintptr_t)VM,0x111);eq(*(volatile uint64_t*)(uintptr_t)(VM+4104),0);*(volatile uint64_t*)(uintptr_t)VM=0x444;
        eq(call(S_PRESSURE,2,0,0,0),U_OK);sleep_for(3);eq(call(S_VM_COMMIT,2,4,0,0),U_NOMEM);eq(call(S_PRESSURE,0,0,0,0),U_OK);eq(call(S_VM_COMMIT,2,4,0,0),U_OK);
        *(volatile uint64_t*)(uintptr_t)(VM+8192)=0x555;eq(call(S_VM_DECOMMIT,2,4,0,0),U_OK);eq(call(S_VM_COMMIT,2,1,0,0),U_OK);eq(*(volatile uint64_t*)(uintptr_t)(VM+8192),0);eq(call(S_VM_DECOMMIT,2,1,0,0),U_OK);
        eq(call(S_VM_CLONE,(uint64_t)(uintptr_t)user_main,25,1,(uint64_t)(uintptr_t)&child),U_OK);join(child,U_CANCELLED);eq(*(volatile uint64_t*)(uintptr_t)VM,0x444);eq(call(S_VM_DECOMMIT,0,2,0,0),U_OK);say("VM_RESERVE_COMMIT_COW_PRESSURE_RECOVERY_OK");
        eq(call(S_VM_CLONE,(uint64_t)(uintptr_t)user_main,26,0,(uint64_t)(uintptr_t)&child),U_OK);join(child,U_DENIED);eq(call(S_VM_CLONE,(uint64_t)(uintptr_t)user_main,27,0,(uint64_t)(uintptr_t)&child),U_OK);join(child,U_DENIED);say("ADVANCED_FAULT_AND_RETURN_ISOLATION_OK");
        spawn(22);say("PROCESS_EXIT_CANCELS_BLOCKED_THREADS_OK");eq(call(S_EXIT,0,0,0,0),U_OK);fail();
    }else fail();
    call(S_THREAD_EXIT,0,0,0,0);fail();
}
