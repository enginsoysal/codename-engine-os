#include <engine/process.h>
#define NX (UINT64_C(1)<<63)
static uint64_t pd_addresses[4];
static uint64_t stack_branch,low_pt,entry_pt;
extern uint8_t __text_start[],__text_end[],__rodata_start[],__rodata_end[];
static uint64_t need_page(void) {uint64_t p=page_alloc();if(!p){serial_text("M2_BOOT_OUT_OF_MEMORY\n");test_exit(0x31);}return p;}
uint64_t kernel_pd(unsigned i) {return i<4?pd_addresses[i]:0;}
uint64_t kernel_stack_branch(void) {return stack_branch;}
uint64_t kernel_space_init(const os_boot_info_v0 *boot) {
    if(boot->kernel.physical_address!=0x200000 || boot->kernel.byte_length>
#ifdef ENGINE_M8
       0xc00000
#elif defined(ENGINE_M74)
       0x800000
#elif defined(ENGINE_M7)
       0x400000
#else
       0x200000
#endif
    ) {
        serial_text("M2_KERNEL_LAYOUT_UNSUPPORTED\n");test_exit(0x31);
    }
    uint32_t a,b,c,d;__asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(0x80000001u),"c"(0));
    if(!(d&(1u<<20))){serial_text("NX_REQUIRED\n");test_exit(0x31);}
    uint32_t low,high;__asm__ volatile("rdmsr":"=a"(low),"=d"(high):"c"(0xc0000080u));
    low=(low|(1u<<11))&~1u; /* Enable NX; disable unconfigured SYSCALL entry until a real fast gateway exists. */
    __asm__ volatile("wrmsr"::"a"(low),"d"(high),"c"(0xc0000080u));
    __asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(1u),"c"(0));
    if(d&(1u<<11))__asm__ volatile("wrmsr"::"a"(0u),"d"(0u),"c"(0x174u)); /* No legacy SYSENTER CS. */
    uint64_t pml4=need_page(),pdpt=need_page();((uint64_t*)(uintptr_t)pml4)[0]=pdpt|3;
    for(unsigned i=0;i<4;++i) {
        pd_addresses[i]=need_page();((uint64_t*)(uintptr_t)pdpt)[i]=pd_addresses[i]|3;
        uint64_t *pd=(uint64_t*)(uintptr_t)pd_addresses[i];
        for(unsigned j=0;j<512;++j)pd[j]=(((uint64_t)i*512+j)*0x200000)|0x83|NX;
    }
    uint64_t *pd0=(uint64_t*)(uintptr_t)pd_addresses[0];
    uint64_t null_pt=need_page();low_pt=null_pt;pd0[0]=null_pt|3;
    for(unsigned i=1;i<512;++i)((uint64_t*)(uintptr_t)null_pt)[i]=(uint64_t)i*4096|3|NX;
    unsigned kernel_chunks=1;
#ifdef ENGINE_M74
    kernel_chunks=(unsigned)((boot->kernel.byte_length+0x1fffff)/0x200000);
#elif defined(ENGINE_M7)
    kernel_chunks=2;
#endif
    for(unsigned chunk=0;chunk<kernel_chunks;++chunk){
        uint64_t text_pt=need_page();pd0[1+chunk]=text_pt|3;
        for(unsigned i=0;i<512;++i){
            uint64_t address=0x200000+(uint64_t)chunk*0x200000+(uint64_t)i*4096,flags=3|NX;
            if(address>=(uint64_t)(uintptr_t)__text_start&&address<(uint64_t)(uintptr_t)__text_end)flags=1;
            else if(address>=(uint64_t)(uintptr_t)__rodata_start&&address<(uint64_t)(uintptr_t)__rodata_end)flags=1|NX;
            ((uint64_t*)(uintptr_t)text_pt)[i]=address|flags;
        }
    }
    stack_branch=need_page();uint64_t stack_pd=need_page(),stack_pt=need_page();
    ((uint64_t*)(uintptr_t)pml4)[256]=stack_branch|3;
    ((uint64_t*)(uintptr_t)stack_branch)[0]=stack_pd|3;
    ((uint64_t*)(uintptr_t)stack_pd)[0]=stack_pt|3;
    for(unsigned i=0;i<4;++i)((uint64_t*)(uintptr_t)stack_pt)[((KERNEL_STACK>>12)&511)+i]=need_page()|3|NX;
    entry_pt=stack_pt;
    uint64_t cr0;__asm__ volatile("movq %%cr0,%0":"=r"(cr0));cr0|=UINT64_C(1)<<16;
    __asm__ volatile("movq %0,%%cr0"::"r"(cr0):"memory");
    __asm__ volatile("movq %0,%%cr3"::"r"(pml4):"memory");
    interrupts_set_kernel_stack(KERNEL_STACK_TOP);
    serial_text("M2_KERNEL_WX_NX_NULL_GUARD_OK\n");return pml4;
}

#ifdef ENGINE_M3
uint64_t kernel_low_pt(void){return low_pt;}
uint64_t kernel_entry_pt(void){return entry_pt;}
uint64_t kernel_cpu_stack(unsigned cpu){
    uint64_t bottom=KERNEL_STACK+(uint64_t)cpu*0x10000;
    for(unsigned i=0;i<4;++i){((uint64_t*)(uintptr_t)entry_pt)[((bottom>>12)&511)+i]=need_page()|3|NX;
        ((uint64_t*)(uintptr_t)entry_pt)[(((bottom+0x8000)>>12)&511)+i]=need_page()|3|NX;}
    return bottom+16384;
}
#endif
