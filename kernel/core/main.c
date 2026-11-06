#include <engine/base.h>
#include <engine/boot/validate_v0.h>
#include "build.h"
#ifdef ENGINE_M2
#include <engine/process.h>
#endif
void interrupts_init(void);
void display_boot(const os_boot_info_v0*);
void kernel_fault_probe(void);
static NORETURN void reject(const char *reason) {serial_text("KERNEL_REJECT: ");serial_text(reason);serial_text("\n");test_exit(0x23);}
static int low_range(uint64_t address,uint64_t size) {return address && size && address<UINT64_C(0x100000000) && size<=UINT64_C(0x100000000)-address;}
static int loader_memory(const os_boot_info_v0 *boot,uint64_t address,uint64_t size) {
    if(!low_range(address,size))return 0;
    uint64_t end=address+size;
    while(address<end) {
        uint64_t next=address;
        for(uint64_t off=0;off<boot->memory_map.byte_length;off+=boot->memory_descriptor_size) {
            const uint8_t *d=(const uint8_t*)(uintptr_t)(boot->memory_map.physical_address+off);
            uint32_t type=os_read32(d);uint64_t start=os_read64(d+8),pages=os_read64(d+24);
            if(pages>UINT64_MAX/4096 || start>UINT64_MAX-pages*4096)return 0;
            uint64_t stop=start+pages*4096;
            if((type==1||type==2) && start<=address && address<stop && stop>next)next=stop;
        }
        if(next==address)return 0;address=next>end?end:next;
    }
    return 1;
}
NORETURN void kernel_main(const os_boot_info_v0 *boot) {
    serial_init();interrupts_init();
    serial_text("ENGINE_OS_KERNEL_ENTRY build=" BUILD_ID "\n");
    if(!low_range((uint64_t)(uintptr_t)boot,sizeof(*boot)))reject("boot pointer");
    if(os_boot_validate((const uint8_t*)boot,sizeof(*boot)))reject("boot header");
    if(!low_range(boot->memory_map.physical_address,boot->memory_map.byte_length) || boot->memory_map.byte_length>64*4096)reject("memory map range");
    if(!loader_memory(boot,(uint64_t)(uintptr_t)boot,sizeof(*boot)) ||
       !loader_memory(boot,boot->memory_map.physical_address,boot->memory_map.byte_length) ||
       !loader_memory(boot,boot->kernel.physical_address,boot->kernel.byte_length) ||
       !loader_memory(boot,boot->initimage.physical_address,boot->initimage.byte_length) ||
       !loader_memory(boot,boot->reservations.physical_address,boot->reservations.byte_length))reject("memory ownership");
    if(boot->reservations.byte_length>32*sizeof(os_boot_range))reject("reservation count");
    const os_boot_range *ranges=(const os_boot_range*)(uintptr_t)boot->reservations.physical_address;
    for(uint64_t i=0;i<boot->reservations.byte_length/sizeof(*ranges);++i) {
        if(!loader_memory(boot,ranges[i].physical_address,ranges[i].byte_length))reject("reserved memory coverage");
        for(uint64_t j=0;j<i;++j)if(ranges[i].physical_address<ranges[j].physical_address+ranges[j].byte_length &&
           ranges[j].physical_address<ranges[i].physical_address+ranges[i].byte_length)reject("reserved memory overlap");
    }
    const uint8_t *init=(const uint8_t*)(uintptr_t)boot->initimage.physical_address;
    if(boot->initimage.byte_length!=48 || memcmp(init,"ENGINIT1",8) || os_read32(init+8)!=48 ||
       memcmp(init+16,BUILD_ID,32) || os_read32(init+12)>
#ifdef ENGINE_M5
       7)reject("initimage");
#elif defined(ENGINE_M45)
       6)reject("initimage");
#else
       4)reject("initimage");
#endif
    if(boot->framebuffer.byte_length && (!low_range(boot->framebuffer.physical_address,boot->framebuffer.byte_length) ||
       boot->framebuffer_width>8192 || boot->framebuffer_height>8192))reject("framebuffer bounds");
    display_boot(boot);
    uint64_t cr3;uint16_t cs;__asm__ volatile("movq %%cr3,%0":"=r"(cr3));__asm__ volatile("movw %%cs,%0":"=r"(cs));
    serial_text("OWN_GDT_TSS_IDT_OK cs=");serial_hex(cs);serial_text(" cr3=");serial_hex(cr3);serial_text("\n");
    serial_text("BOOTINFO_AND_MEMORY_OWNERSHIP_OK\n");
    serial_text("ENGINE_OS_KERNEL_ONLINE build=" BUILD_ID "\n");
    serial_text("WINDOWS_95_11_PROFILES_RESERVED_NOT_IMPLEMENTED\n");
    uint32_t mode=os_read32(init+12);
    if(mode==2)kernel_fault_probe();
#ifdef ENGINE_M2
    process_start(boot,mode);
#endif
    if(mode==1||mode==4)test_exit(0x10);
    halt_forever();
}
