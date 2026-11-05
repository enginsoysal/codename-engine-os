#include <engine/m3.h>
#include <engine/boot/validate_v0.h>
#define SHARED UINT64_C(0xffff800000080000)
static volatile uint32_t *apic;
static volatile uint64_t *hpet;static uint64_t clock_divisor;
static unsigned ids[M3_CPUS],cpu_count,bsp_id,use_rdtscp;
static uint64_t retained,period,counter,pressure_ticks[M3_CPUS];
static unsigned pressure_active,pressure_checks,early_completions;
typedef struct {unsigned ready,request,done,command;uint64_t ticks,value,failures;} cpu_state;
static cpu_state cpus[M3_CPUS];
extern uint8_t ap_trampoline[],ap_trampoline_end[];
extern void m3_invalidate_shared(void);
extern uint64_t kernel_cpu_stack(unsigned),kernel_entry_pt(void);
static NORETURN void bad(const char *s){serial_text("M3_SMP_FAIL ");serial_text(s);serial_text("\n");test_exit(0x31);}
static unsigned load(unsigned *p){return __atomic_load_n(p,__ATOMIC_ACQUIRE);}
static void store(unsigned *p,unsigned v){__atomic_store_n(p,v,__ATOMIC_RELEASE);}
static uint32_t reg(unsigned offset){return apic[offset/4];}
static void set(unsigned offset,uint32_t value){apic[offset/4]=value;(void)reg(0x20);}
static void delay(unsigned count){
    uint8_t saved=in8(0x61);out8(0x61,saved&~3u);out8(0x43,0xb0);out8(0x42,(uint8_t)count);out8(0x42,(uint8_t)(count>>8));out8(0x61,(saved&~2u)|1);
    unsigned n=0;while(!(in8(0x61)&32)&&++n<20000000)__asm__ volatile("pause");out8(0x61,saved);if(n==20000000)bad("PIT delay timeout");
}
static void ipi(unsigned id,unsigned value){
    unsigned n=0;while((reg(0x300)&0x1000)&&++n<1000000)__asm__ volatile("pause");if(n==1000000)bad("IPI send timeout");
    set(0x310,id<<24);set(0x300,value);
}
static void cpu_aux(unsigned index){
    if(!use_rdtscp)return;uint32_t lo,hi;
    __asm__ volatile("wrmsr"::"c"(0xc0000103u),"a"(index),"d"(0u));
    __asm__ volatile("rdmsr":"=a"(lo),"=d"(hi):"c"(0xc0000103u));if(lo!=index||hi)bad("TSC_AUX setup");
}
unsigned m3_cpu(void){
    if(use_rdtscp){uint32_t a,d,index;__asm__ volatile("rdtscp":"=a"(a),"=d"(d),"=c"(index)::"memory");if(index>=cpu_count)bad("TSC_AUX CPU index");return index;}
    unsigned id=reg(0x20)>>24;for(unsigned i=0;i<cpu_count;++i)if(ids[i]==id)return i;bad("APIC CPU index");
}
unsigned m3_cpu_interrupt(uint64_t *frame){
    if(!apic)return 0;unsigned vector=(unsigned)frame[15];
    if(vector==255)return 1;
    if(vector!=240&&vector!=241)return 0;
    unsigned i=m3_cpu();if(i>=cpu_count)bad("unknown interrupt CPU");
    if(vector==241){__atomic_fetch_add(&cpus[i].ticks,1,__ATOMIC_RELAXED);if(m3_active&&!(frame[18]&3))m3_tick();}
    else __asm__ volatile("invlpg (%0)"::"r"(UINT64_C(0x8000)):"memory");
    set(0xb0,0);return !(vector==241&&m3_active&&(frame[18]&3)==3);
}
static void exercise(cpu_state *s){
    for(unsigned i=0;i<512;++i){uint64_t p=page_alloc();if(!p){++s->failures;continue;}uint64_t *v=(uint64_t*)(uintptr_t)p;
        for(unsigned j=0;j<512;++j)if(v[j])++s->failures;
        v[0]=UINT64_C(0xaabbccdd11223344);__atomic_fetch_add(&counter,1,__ATOMIC_RELAXED);page_free(p);}
}
static NORETURN void ap_main(unsigned index){
    uint64_t top=KERNEL_STACK+(uint64_t)index*0x10000+16384;
    cpu_aux(index);interrupts_ap(index,top,top+0x8000);set(0xf0,0x1ff);set(0x350,0x10000);set(0x360,0x10000);
    set(0x3e0,3);set(0x320,0x20000|241);set(0x380,(uint32_t)period);
    store(&cpus[index].ready,1);m3_ap_return(index);
}
NORETURN void m3_ap_return(unsigned index){
    cpu_state *s=&cpus[index];
    for(;;){
        __asm__ volatile("cli":::"memory");unsigned request=load(&s->request);
        if(request==load(&s->done)){__asm__ volatile("sti; hlt":::"memory");continue;}
        __asm__ volatile("sti":::"memory");unsigned command=s->command;
        if(command==4){store(&s->done,request);m3_ap_enter();}
        if(command==1)exercise(s);
        /* A published mailbox may be observed before its doorbell IPI. Invalidate before ACK. */
        if(command==0||command==2)m3_invalidate_shared();
        if(command==2)s->value=*(volatile uint64_t*)(uintptr_t)SHARED;
        if(command==3){set(0x320,0x10000|241);store(&s->done,request);store(&s->ready,2);halt_forever();}
        store(&s->done,request);
    }
}
void m3_smp_launch(unsigned cpu){
    if(!cpu||cpu>=cpu_count)bad("user CPU affinity");cpus[cpu].command=4;store(&cpus[cpu].request,load(&cpus[cpu].request)+1);ipi(ids[cpu],240);
}

static int checksum(const uint8_t *p,unsigned length){uint8_t sum=0;for(unsigned i=0;i<length;++i)sum=(uint8_t)(sum+p[i]);return sum==0;}
static const uint8_t *table(uint64_t addr,const char *signature){
    if(addr<4096||addr>UINT64_C(0x100000000)-36)return 0;const uint8_t *p=(const uint8_t*)(uintptr_t)addr;uint32_t len=os_read32(p+4);
    if(memcmp(p,signature,4)||len<36||len>65536||addr+len>UINT64_C(0x100000000)||!checksum(p,len))return 0;return p;
}
static void discover(const os_boot_info_v0 *boot){
    const uint8_t *rsdp=(const uint8_t*)(uintptr_t)boot->acpi.physical_address;if(!rsdp)bad("ACPI unavailable");
    unsigned width=boot->acpi.byte_length>=36&&os_read64(rsdp+24)?8:4;uint64_t address=width==8?os_read64(rsdp+24):os_read32(rsdp+16);
    const uint8_t *root=table(address,width==8?"XSDT":"RSDT");if(!root)bad("ACPI root");const uint8_t *madt=0,*clock=0;unsigned len=os_read32(root+4);
    if((len-36)%width||(len-36)/width>256)bad("ACPI root entries");
    for(unsigned off=36;off<len;off+=width){uint64_t p=width==8?os_read64(root+off):os_read32(root+off);const uint8_t *candidate=table(p,"APIC");if(candidate)madt=candidate;candidate=table(p,"HPET");if(candidate)clock=candidate;}
    if(!clock||os_read32(clock+4)<56||clock[40]!=0)bad("HPET table");
    uint64_t base=os_read64(clock+44);if(base<4096||base>UINT64_C(0xffffffff)-4096||(base&4095))bad("HPET address");
    ((uint64_t*)(uintptr_t)kernel_pd((unsigned)(base>>30)))[(base>>21)&511]|=0x18;
    hpet=(volatile uint64_t*)(uintptr_t)base;uint64_t caps=hpet[0],period_fs=caps>>32;
    if(!(caps&(1u<<13))||!period_fs||period_fs>100000000)bad("64-bit HPET required");
    clock_divisor=(UINT64_C(10000000000000)+period_fs-1)/period_fs;hpet[2]=0;
    for(unsigned i=0;i<((caps>>8)&31)+1;++i)hpet[32+i*4]&=~UINT64_C(4);
    hpet[30]=0;hpet[2]=1;
    if(!madt||os_read32(madt+4)<44)bad("MADT missing");
    len=os_read32(madt+4);for(unsigned off=44;off<len;){if(len-off<2||madt[off+1]<2||madt[off+1]>len-off)bad("MADT entry");
        if(madt[off]==0&&madt[off+1]>=8&&(os_read32(madt+off+4)&1)){unsigned id=madt[off+3];if(id!=bsp_id&&cpu_count<M3_CPUS){unsigned duplicate=0;for(unsigned j=0;j<cpu_count;++j)duplicate|=ids[j]==id;if(!duplicate)ids[cpu_count++]=id;}}
        off+=madt[off+1];}
}
unsigned m3_smp_start(const os_boot_info_v0 *boot,uint64_t kernel_cr3){
    uint32_t lo,hi;__asm__ volatile("rdmsr":"=a"(lo),"=d"(hi):"c"(0x1bu));if(hi||(lo&0x400)||!(lo&0x800))bad("xAPIC required");
    apic=(volatile uint32_t*)(uintptr_t)(lo&0xfffff000u);bsp_id=reg(0x20)>>24;ids[0]=bsp_id;cpu_count=1;
    ((uint64_t*)(uintptr_t)kernel_pd(3))[((lo&0xfffff000u)>>21)&511]|=0x18;__asm__ volatile("movq %0,%%cr3"::"r"(kernel_cr3):"memory");
    uint32_t a,b,c,d;__asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(0x80000001u),"c"(0));use_rdtscp=(d>>27)&1;cpu_aux(0);
    discover(boot);set(0xf0,0x1ff);set(0x3e0,3);set(0x320,0x10000|241);set(0x380,0xffffffff);delay(11932);period=0xffffffffu-reg(0x390);set(0x380,0);if(!period)bad("APIC calibration");
    if((uint64_t)(ap_trampoline_end-ap_trampoline)>0xf00)bad("trampoline size");
    uint64_t *low=(uint64_t*)(uintptr_t)kernel_low_pt();low[8]=0x8003;__asm__ volatile("invlpg (%0)"::"r"(UINT64_C(0x8000)):"memory");
    memcpy((void*)(uintptr_t)0x8000,ap_trampoline,(size_t)(ap_trampoline_end-ap_trampoline));
    for(unsigned i=1;i<cpu_count;++i){uint64_t top=kernel_cpu_stack(i);retained+=8;
        volatile uint64_t *p=(volatile uint64_t*)(uintptr_t)0x8f00;p[0]=kernel_cr3;p[1]=top;p[2]=i;p[3]=(uint64_t)(uintptr_t)ap_main;
        __atomic_thread_fence(__ATOMIC_SEQ_CST);ipi(ids[i],0xc500);delay(11932);ipi(ids[i],0x8500);delay(239);ipi(ids[i],0x608);delay(239);if(!load(&cpus[i].ready))ipi(ids[i],0x608);
        unsigned n=0;while(!load(&cpus[i].ready)&&++n<10000000)__asm__ volatile("pause");if(n==10000000)bad("AP startup timeout");
    }
    low[8]=0x8003|(UINT64_C(1)<<63);__asm__ volatile("invlpg (%0)"::"r"(UINT64_C(0x8000)):"memory");
    serial_text("M3_CPUS_ACTIVE=");serial_hex(cpu_count);serial_text(" apic_period=");serial_hex(period);serial_text("\n");return cpu_count;
}
static void command(unsigned value){
    for(unsigned i=1;i<cpu_count;++i){cpus[i].command=value;store(&cpus[i].request,load(&cpus[i].request)+1);
        if(value==0||value==2){delay(23864);if(load(&cpus[i].done)==load(&cpus[i].request))++early_completions;}
        ipi(ids[i],240);}
}
static void wait_all(void){for(unsigned i=1;i<cpu_count;++i){unsigned n=0;while(load(&cpus[i].done)!=load(&cpus[i].request)&&++n<20000000)__asm__ volatile("pause");if(n==20000000)bad("worker deadline");}}
void m3_smp_pressure(unsigned begin){
    if(begin){if(pressure_active)bad("nested pressure budget");pressure_active=1;for(unsigned i=1;i<cpu_count;++i)pressure_ticks[i]=__atomic_load_n(&cpus[i].ticks,__ATOMIC_RELAXED);}
    else if(pressure_active){for(unsigned i=1;i<cpu_count;++i)if(__atomic_load_n(&cpus[i].ticks,__ATOMIC_RELAXED)<=pressure_ticks[i])bad("CPU unresponsive under pressure");pressure_active=0;++pressure_checks;}
}
void m3_smp_test(void){
    if(pressure_checks!=5)bad("pressure responsiveness coverage");
    uint64_t baseline=pages_stats().current_pages;command(1);exercise(&cpus[0]);wait_all();
    if(counter!=(uint64_t)cpu_count*512||pages_stats().current_pages!=baseline)bad("concurrent allocator accounting");
    uint64_t a=page_alloc(),b=page_alloc();if(!a||!b)bad("shootdown pages");*(uint64_t*)(uintptr_t)a=0x111;*(uint64_t*)(uintptr_t)b=0x222;
    uint64_t *pt=(uint64_t*)(uintptr_t)kernel_entry_pt();pt[(SHARED>>12)&511]=a|3|(UINT64_C(1)<<63);__asm__ volatile("invlpg (%0)"::"r"(SHARED):"memory");command(2);wait_all();
    for(unsigned i=1;i<cpu_count;++i)if(cpus[i].value!=0x111)bad("TLB initial value");
    pt[(SHARED>>12)&511]=b|3|(UINT64_C(1)<<63);__asm__ volatile("invlpg (%0)"::"r"(SHARED):"memory");command(2);wait_all();
    for(unsigned i=1;i<cpu_count;++i)if(cpus[i].value!=0x222||!__atomic_load_n(&cpus[i].ticks,__ATOMIC_RELAXED)||cpus[i].failures)bad("TLB or CPU state");
    pt[(SHARED>>12)&511]=0;command(0);wait_all();__asm__ volatile("invlpg (%0)"::"r"(SHARED):"memory");page_free(a);page_free(b);
    serial_text("M3_SMP_STATS cpus=");serial_hex(cpu_count);serial_text(" concurrent_allocations=");serial_hex(counter);serial_text(" rdtscp_index=");serial_hex(use_rdtscp);serial_text(" early_mailbox_completions=");serial_hex(early_completions);serial_text(" pressure_checks=");serial_hex(pressure_checks);serial_text("\nM3_SMP_ALLOCATOR_TLB_TIMER_PASS\n");
}
void m3_smp_stop(void){command(3);wait_all();for(unsigned i=1;i<cpu_count;++i)if(load(&cpus[i].ready)!=2)bad("CPU quiesce");}
uint64_t m3_smp_pages(void){return retained;}

uint64_t m3_clock_ticks(void){return hpet[30]/clock_divisor;}
void m3_clock_probe(void){
    uint64_t before=m3_clock_ticks();unsigned n=0;while(m3_clock_ticks()<before+3&&++n<20000000)__asm__ volatile("pause");
    if(n==20000000)bad("monotonic clock probe timeout");serial_text("M3_MONOTONIC_CLOCK_CATCHUP_OK\n");
}
