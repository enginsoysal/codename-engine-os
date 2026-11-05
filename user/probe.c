#include <engine/user.h>
#define BASE UINT64_C(0x100000000)
#define SHARED (BASE+0x20000)
#define STACK (BASE+0x10000)
static volatile uint64_t private_value;
static uint64_t call(uint64_t n,uint64_t a,uint64_t b,uint64_t c,uint64_t d) {
    register uint64_t rax __asm__("rax")=n;register uint64_t r10 __asm__("r10")=d;
    __asm__ volatile("int $0x80":"+a"(rax):"D"(a),"S"(b),"d"(c),"r"(r10):"memory","cc");return rax;
}
static void say(const char *s) {uint64_t n=0;while(s[n])++n;call(S_WRITE,(uint64_t)(uintptr_t)s,n,0,0);}
static __attribute__((noreturn)) void fail(void) {say("USER_ASSERT_FAIL");call(S_EXIT,1,0,0,0);for(;;){} }
static void expect(uint64_t actual,uint64_t expected) {if(actual!=expected)fail();}
static __attribute__((naked,noreturn)) void bad_stack(void) {
    __asm__ volatile("movabsq $0x8000000000000000,%rsp; movq $9999,%rax; int $0x80; ud2");
}
__attribute__((noreturn)) void user_main(uint64_t role) {
    uint64_t h=0,copy=0,value=0,pid=0;expect(call(S_PID,(uint64_t)(uintptr_t)&pid,0,0,0),U_OK);
    if(private_value)fail();
#ifdef ENGINE_M3
    if(role==14) {
        volatile uint64_t *shared=(volatile uint64_t*)(uintptr_t)SHARED;
        uint64_t carry_count=0,expected=0;
        while(shared[1]<16) {
            /* An interrupt may land between STC and ADC: carry must survive scheduling. */
            __asm__ volatile("stc; adcq $0,%0":"+r"(carry_count)::"cc");
            if(carry_count!=++expected)fail();
            ++shared[0];
        }
        say("NON_YIELDING_HOG_RESUMED_OK");
    } else if(role==15) {
        volatile uint64_t *shared=(volatile uint64_t*)(uintptr_t)SHARED;
        uint64_t previous=0;
        while(shared[1]<16) {
            uint64_t observed=shared[0];
            if(observed!=previous){previous=observed;++shared[1];}
        }
        say("NON_YIELDING_PEER_PROGRESS_OK");
    } else
#endif
    if(role==1) {
        private_value=0x11111111;
        user_message empty;expect(call(S_RECEIVE,(uint64_t)(uintptr_t)&empty,0,0,0),U_EMPTY);
        for(unsigned i=0;i<128;++i) {
            expect(call(S_EVENT_CREATE,(uint64_t)(uintptr_t)&h,0,0,0),U_OK);
            expect(call(S_DUP,h,R_READ,(uint64_t)(uintptr_t)&copy,0),U_OK);
            expect(call(S_DUP,copy,R_ALL,(uint64_t)(uintptr_t)&value,0),U_DENIED);
            expect(call(S_EVENT_SIGNAL,copy,0,0,0),U_DENIED);
            expect(call(S_EVENT_SIGNAL,h,0,0,0),U_OK);
            expect(call(S_EVENT_QUERY,copy,(uint64_t)(uintptr_t)&value,0,0),U_OK);expect(value,1);
            expect(call(S_CLOSE,h,0,0,0),U_OK);expect(call(S_EVENT_QUERY,h,(uint64_t)(uintptr_t)&value,0,0),U_BAD_HANDLE);
            expect(call(S_EVENT_QUERY,copy,(uint64_t)(uintptr_t)&value,0,0),U_OK);expect(value,1);
            expect(call(S_CLOSE,copy,0,0,0),U_OK);
            expect(call(S_MEMORY_CREATE,(uint64_t)(uintptr_t)&h,0,0,0),U_OK);expect(call(S_CLOSE,h,0,0,0),U_OK);
        }
        say("HANDLE_RIGHTS_STALE_AND_REUSE_OK");
        uint64_t full[16];
        for(unsigned i=0;i<16;++i)expect(call(S_MEMORY_CREATE,(uint64_t)(uintptr_t)&full[i],0,0,0),U_OK);
        expect(call(S_MEMORY_CREATE,(uint64_t)(uintptr_t)&h,0,0,0),U_LIMIT);
        for(unsigned i=0;i<16;++i)expect(call(S_CLOSE,full[i],0,0,0),U_OK);
        say("HANDLE_QUOTA_AND_CLEANUP_OK");
        expect(call(S_MEMORY_CREATE,(uint64_t)(uintptr_t)&h,0,0,0),U_OK);
        expect(call(S_EVENT_SIGNAL,h,0,0,0),U_BAD_TYPE);
        expect(call(S_MEMORY_MAP,h,SHARED,0,0),U_OK);*(volatile uint64_t*)(uintptr_t)SHARED=0xabcdef;
        expect(call(S_TRANSFER,h,2,R_READ,(uint64_t)(uintptr_t)&copy),U_OK);
        user_payload payload={copy,0xabcdef};expect(call(S_SEND,2,(uint64_t)(uintptr_t)&payload,sizeof(payload),0),U_OK);
        for(unsigned i=0;i<3;++i)expect(call(S_SEND,2,(uint64_t)(uintptr_t)"q",1,0),U_OK);
        expect(call(S_SEND,2,(uint64_t)(uintptr_t)"q",1,0),U_LIMIT);say("IPC_QUEUE_BOUND_OK");
        expect(call(S_YIELD,0,0,0,0),U_OK);
        user_message answer;expect(call(S_RECEIVE,(uint64_t)(uintptr_t)&answer,0,0,0),U_OK);
        if(answer.sender!=2||answer.length!=4||answer.data[0]!='p')fail();
        if(private_value!=0x11111111)fail();say("PRODUCER_SURVIVED_OTHER_FAULTS");
        expect(call(S_CLOSE,h,0,0,0),U_OK);
    } else if(role==2) {
        private_value=0x22222222;user_message msg;
        expect(call(S_RECEIVE,(uint64_t)(uintptr_t)&msg,0,0,0),U_OK);
        if(msg.sender!=1||msg.length!=sizeof(user_payload))fail();
        const user_payload *payload=(const user_payload*)msg.data;
        expect(call(S_MEMORY_MAP,payload->handle,SHARED,0,0),U_OK);
        if(*(volatile uint64_t*)(uintptr_t)SHARED!=payload->marker)fail();
        expect(call(S_CLOSE,payload->handle,0,0,0),U_OK);
        if(*(volatile uint64_t*)(uintptr_t)SHARED!=0xabcdef)fail();
        expect(call(S_SEND,1,(uint64_t)(uintptr_t)"pong",4,0),U_OK);say("IPC_AND_EXPLICIT_SHARED_MEMORY_OK");
        *(volatile uint64_t*)(uintptr_t)SHARED=0; /* Must fault: receiver has read-only rights. */
    } else if(role==3) {*(volatile uint8_t*)(uintptr_t)0x200000=0;}
    else if(role==4) {volatile uint8_t code=0xc3;((void(*)(void))(uintptr_t)&code)();}
    else if(role==5) {*(volatile uint64_t*)(uintptr_t)(STACK-4096)=0;}
    else if(role==6) {
        private_value=0x66666666;expect(call(S_YIELD,0,0,0,0),U_OK);
        if(private_value!=0x66666666)fail();say("PRIVATE_DATA_SURVIVOR_OK");
    } else if(role==7) {*(volatile uint8_t*)(uintptr_t)BASE=0;}
    else if(role==8) {__asm__ volatile("outb %%al,$0x80"::"a"((uint8_t)0));}
    else if(role==9) {
        expect(call(S_WRITE,0x200000,8,0,0),U_DENIED);
        expect(call(S_WRITE,UINT64_MAX-3,8,0,0),U_DENIED);
        expect(call(S_WRITE,STACK+4096-4,8,0,0),U_DENIED);
        expect(call(S_EVENT_CREATE,BASE,0,0,0),U_DENIED);
        expect(call(9999,0,0,0,0),U_UNSUPPORTED);say("INVALID_POINTERS_AND_SYSCALL_OK");
    } else if(role==10) {__asm__ volatile("cli");}
    else if(role==11)bad_stack();
    else if(role==12) {__asm__ volatile("syscall":::"rcx","r11","memory");}
    else if(role==13) {__asm__ volatile("sysenter":::"memory");}
    else fail();
    if(role==2||role==3||role==4||role==5||role==7||role==8||role==10||role==12||role==13)fail();
    expect(call(S_EXIT,0,0,0,0),U_OK);for(;;){}
}
