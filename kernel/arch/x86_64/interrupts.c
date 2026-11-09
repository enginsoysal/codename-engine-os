#ifdef ENGINE_M7
#include <engine/desktop/app.h>
#include <engine/network/network.h>
#endif
#include <engine/base.h>
#ifdef ENGINE_M4
#include <engine/m4.h>
#endif
#ifdef ENGINE_M3
#include <engine/m3.h>
#endif
#ifdef ENGINE_M2
#include <engine/process.h>
#endif
typedef struct __attribute__((packed)) {uint16_t low,selector;uint8_t ist,type;uint16_t middle;uint32_t high,reserved;} gate;
typedef struct __attribute__((packed)) {uint16_t limit;uint64_t base;} table_pointer;
typedef struct __attribute__((packed)) {uint32_t reserved0;uint64_t rsp[3],reserved1,ist[7],reserved2;uint16_t reserved3,iomap;} tss_type;
_Static_assert(sizeof(tss_type)==104 && sizeof(gate)==16,"x64 CPU layouts");
static _Alignas(16) gate idt[256];
static _Alignas(16) uint64_t gdt[7];
static tss_type tss;
static _Alignas(16) uint8_t emergency_stack[16384];
extern void *exception_stubs[256];
void interrupts_init(void) {
    __asm__ volatile("cli");out8(0x21,255);out8(0xa1,255);
    memset(&tss,0,sizeof(tss));tss.ist[0]=(uint64_t)(uintptr_t)(emergency_stack+sizeof(emergency_stack));
    __asm__ volatile("movq %%rsp,%0":"=r"(tss.rsp[0]));tss.iomap=sizeof(tss);
    gdt[0]=0;gdt[1]=UINT64_C(0x00af9a000000ffff);gdt[2]=UINT64_C(0x00cf92000000ffff);
    uint64_t base=(uint64_t)(uintptr_t)&tss,limit=sizeof(tss)-1;
    gdt[3]=UINT64_C(0x00affa000000ffff);gdt[4]=UINT64_C(0x00cff2000000ffff);
    gdt[5]=(limit&65535)|((base&0xffffff)<<16)|(UINT64_C(0x89)<<40)|((limit&0xf0000)<<32)|((base&0xff000000)<<32);
    gdt[6]=base>>32;table_pointer gdtr={sizeof(gdt)-1,(uint64_t)(uintptr_t)gdt};
    __asm__ volatile("lgdt %0"::"m"(gdtr):"memory");uint16_t selector=40;__asm__ volatile("ltr %0"::"r"(selector));
    for(unsigned i=0;i<256;++i) {
        uint64_t handler=(uint64_t)(uintptr_t)exception_stubs[i];
        idt[i]=(gate){(uint16_t)handler,8,(uint8_t)(i==8?1:0),0x8e,(uint16_t)(handler>>16),(uint32_t)(handler>>32),0};
#ifdef ENGINE_M2
        if(i==128)idt[i].type=0xee;
#endif
    }
    table_pointer idtr={sizeof(idt)-1,(uint64_t)(uintptr_t)idt};__asm__ volatile("lidt %0"::"m"(idtr):"memory");
}
void interrupts_set_kernel_stack(uint64_t address) {tss.rsp[0]=address;}
extern void kernel_panic_screen(void);
extern void kernel_fault_probe(void);
uint64_t *exception_dispatch(uint64_t *frame) {
#ifdef ENGINE_M8
    extern volatile unsigned w8_active;extern trap_frame *w8_trap(trap_frame*);
    if(w8_active&&(frame[18]&3)==3&&(frame[15]==32||frame[15]==128||frame[15]<32))return (uint64_t*)w8_trap((trap_frame*)frame);
#endif
#ifdef ENGINE_M74
    extern unsigned clock74_irq(uint64_t *);if(clock74_irq(frame))return frame;
    extern unsigned audio74_irq(uint64_t *);if(audio74_irq(frame))return frame;
#endif
#ifdef ENGINE_M7
    /* The app watchdog owns its user-mode timer deadline before device wait handlers. */
    if(frame[15]==32){
        unsigned network_tick=net7_clock_event();
        if(app7_active&&(frame[18]&3)==3)return (uint64_t *)app7_trap((trap_frame *)frame);
        if(network_tick){out8(0x20,0x20);return frame;}
    }
    if(net7_nic_irq(frame))return frame;
#endif
#ifdef ENGINE_M4
#ifdef ENGINE_M45
    extern unsigned virtio_storage_irq(uint64_t*);if(virtio_storage_irq(frame))return frame;
    extern unsigned nvme_irq(uint64_t*);if(nvme_irq(frame))return frame;
#endif
    if(block_irq(frame))return frame;
    if(usb_irq(frame))return frame;
    if(input_irq(frame))return frame;
#endif
#ifdef ENGINE_M3
    if(m3_cpu_interrupt(frame))return frame;
    if(m3_active && frame[15]==32 && !(frame[18]&3)){extern void timer_acknowledge(void);timer_acknowledge();m3_tick();return frame;}
#endif
#ifdef ENGINE_M2
    if((frame[18]&3)==3)return (uint64_t*)process_trap((trap_frame*)frame);
#endif
    uint64_t vector=frame[15],error=frame[16],rip=frame[17],cr2;
    __asm__ volatile("movq %%cr2,%0":"=r"(cr2));
    serial_text("KERNEL_PANIC exception=");serial_hex(vector);serial_text(" error=");serial_hex(error);
    serial_text(" rip=");serial_hex(rip);serial_text(" cr2=");serial_hex(cr2);serial_text("\n");
    uint64_t probe=(uint64_t)(uintptr_t)kernel_fault_probe;
    if(rip>=probe && rip<probe+16) {serial_text("symbol=kernel_fault_probe+");serial_hex(rip-probe);serial_text("\n");}
    serial_text("OWN_IDT_EXCEPTION_HANDLED\n");kernel_panic_screen();test_exit(0x21);
}

#ifdef ENGINE_M3
static uint64_t ap_gdt[3][7];static tss_type ap_tss[3];
void interrupts_ap(unsigned cpu,uint64_t stack,uint64_t emergency){
    if(!cpu||cpu>3)test_exit(0x31);tss_type *t=&ap_tss[cpu-1];uint64_t *g=ap_gdt[cpu-1];
    memset(t,0,sizeof(*t));t->rsp[0]=stack;t->ist[0]=emergency;t->iomap=sizeof(*t);
    memcpy(g,gdt,5*8);uint64_t base=(uint64_t)(uintptr_t)t,limit=sizeof(*t)-1;
    g[5]=(limit&65535)|((base&0xffffff)<<16)|(UINT64_C(0x89)<<40)|((base&0xf0000)<<32)|((base&0xff000000)<<32);g[6]=base>>32;
    table_pointer gdtr={55,(uint64_t)(uintptr_t)g},idtr={sizeof(idt)-1,(uint64_t)(uintptr_t)idt};
    __asm__ volatile("lgdt %0; pushq $8; leaq 1f(%%rip),%%rax; pushq %%rax; lretq; 1: movw $16,%%ax; movw %%ax,%%ds; movw %%ax,%%es; movw %%ax,%%ss"::"m"(gdtr):"rax","memory");
    uint16_t selector=40;__asm__ volatile("ltr %0; lidt %1"::"r"(selector),"m"(idtr):"memory");
}
#endif
