#include <engine/base.h>
typedef struct __attribute__((packed)) {uint16_t low,selector;uint8_t ist,type;uint16_t middle;uint32_t high,reserved;} gate;
typedef struct __attribute__((packed)) {uint16_t limit;uint64_t base;} table_pointer;
typedef struct __attribute__((packed)) {uint32_t reserved0;uint64_t rsp[3],reserved1,ist[7],reserved2;uint16_t reserved3,iomap;} tss_type;
_Static_assert(sizeof(tss_type)==104 && sizeof(gate)==16,"x64 CPU layouts");
static _Alignas(16) gate idt[256];
static _Alignas(16) uint64_t gdt[5];
static tss_type tss;
static _Alignas(16) uint8_t emergency_stack[16384];
extern void *exception_stubs[256];
void interrupts_init(void) {
    __asm__ volatile("cli");out8(0x21,255);out8(0xa1,255);
    memset(&tss,0,sizeof(tss));tss.ist[0]=(uint64_t)(uintptr_t)(emergency_stack+sizeof(emergency_stack));
    __asm__ volatile("movq %%rsp,%0":"=r"(tss.rsp[0]));tss.iomap=sizeof(tss);
    gdt[0]=0;gdt[1]=UINT64_C(0x00af9a000000ffff);gdt[2]=UINT64_C(0x00cf92000000ffff);
    uint64_t base=(uint64_t)(uintptr_t)&tss,limit=sizeof(tss)-1;
    gdt[3]=(limit&65535)|((base&0xffffff)<<16)|(UINT64_C(0x89)<<40)|((limit&0xf0000)<<32)|((base&0xff000000)<<32);
    gdt[4]=base>>32;table_pointer gdtr={sizeof(gdt)-1,(uint64_t)(uintptr_t)gdt};
    __asm__ volatile("lgdt %0"::"m"(gdtr):"memory");uint16_t selector=24;__asm__ volatile("ltr %0"::"r"(selector));
    for(unsigned i=0;i<256;++i) {
        uint64_t handler=(uint64_t)(uintptr_t)exception_stubs[i];
        idt[i]=(gate){(uint16_t)handler,8,(uint8_t)(i==8?1:0),0x8e,(uint16_t)(handler>>16),(uint32_t)(handler>>32),0};
    }
    table_pointer idtr={sizeof(idt)-1,(uint64_t)(uintptr_t)idt};__asm__ volatile("lidt %0"::"m"(idtr):"memory");
}
extern void kernel_panic_screen(void);
extern void kernel_fault_probe(void);
NORETURN void exception_dispatch(const uint64_t *frame) {
    uint64_t vector=frame[15],error=frame[16],rip=frame[17],cr2;
    __asm__ volatile("movq %%cr2,%0":"=r"(cr2));
    serial_text("KERNEL_PANIC exception=");serial_hex(vector);serial_text(" error=");serial_hex(error);
    serial_text(" rip=");serial_hex(rip);serial_text(" cr2=");serial_hex(cr2);serial_text("\n");
    uint64_t probe=(uint64_t)(uintptr_t)kernel_fault_probe;
    if(rip>=probe && rip<probe+16) {serial_text("symbol=kernel_fault_probe+");serial_hex(rip-probe);serial_text("\n");}
    serial_text("OWN_IDT_EXCEPTION_HANDLED\n");kernel_panic_screen();test_exit(0x21);
}
