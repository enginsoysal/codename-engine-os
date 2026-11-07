#include "uefi.h"
#include <engine/elf.h>
#include <engine/boot/validate_v0.h>
#include "build.h"
static efi_boot_services *bs;
#ifdef ENGINE_M72
#include "bootentry72.inc"
#endif
static os_boot_range reservations[32];
static unsigned reservation_count;
static int table_valid(const efi_header *table,uint64_t signature,uint32_t minimum) {
    if(!table || table->signature!=signature || table->size<minimum || table->size>4096 || table->reserved)return 0;
    const uint8_t *bytes=(const uint8_t*)table;uint32_t crc=UINT32_MAX;
    for(uint32_t i=0;i<table->size;++i) {
        crc^=(i>=16&&i<20)?0:bytes[i];
        for(unsigned j=0;j<8;++j)crc=(crc>>1)^((crc&1)?UINT32_C(0xedb88320):0);
    }
    return ~crc==table->crc;
}
static NORETURN void fail(const char *reason) {serial_text("LOADER_REJECT: ");serial_text(reason);serial_text("\n");test_exit(0x22);}
static void reserve(uint64_t address,uint64_t length) {
    if(!length || length>UINT64_MAX-address || reservation_count==32)fail("reservation range");
    reservations[reservation_count++]=(os_boot_range){address,length};
}
static void *allocate(uint64_t pages,uint32_t type) {
    uint64_t address=UINT64_C(0xffffffff);
    if(!pages || EFI_ERROR(bs->allocate_pages(1,type,pages,&address)))fail("allocate low pages");
    reserve(address,pages*4096);memset((void*)(uintptr_t)address,0,(size_t)(pages*4096));return (void*)(uintptr_t)address;
}
static efi_guid loaded_guid={0x5b1b31a1,0x9562,0x11d2,{0x8e,0x3f,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
static efi_guid fs_guid={0x964e5b22,0x6459,0x11d2,{0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
static efi_guid info_guid={0x09576e92,0x6d3f,0x11d2,{0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
static efi_guid gop_guid={0x9042a9de,0x23dc,0x4a38,{0x96,0xfb,0x7a,0xde,0xd0,0x80,0x51,0x6a}};
static efi_guid acpi_guid={0x8868e871,0xe4f1,0x11d3,{0xbc,0x22,0x00,0x80,0xc7,0x3c,0x88,0x81}};
static void *read_file(efi_file *root,uint16_t *path,uint64_t maximum,uint64_t *length) {
    efi_file *file=0;efi_status status=root->open(root,&file,path,1,0);
    if(EFI_ERROR(status)||!file)fail("open boot file");
    _Alignas(8) uint8_t info[1024];uint64_t info_size=sizeof(info);
    if(EFI_ERROR(file->get_info(file,&info_guid,&info_size,info)) || info_size<80)fail("file info");
    uint64_t size=os_read64(info+8);
    if(!size || size>maximum)fail("boot file size");
    void *buffer=allocate((size+4095)/4096,2);uint64_t actual=size;
    if(EFI_ERROR(file->read(file,&actual,buffer))||actual!=size)fail("short boot file");
    if(EFI_ERROR(file->close(file)))fail("close file");*length=size;return buffer;
}
extern NORETURN void kernel_handoff(os_boot_info_v0*,uint64_t,uint64_t,uint64_t);
#ifdef ENGINE_M73
#include "display73.inc"
#endif
efi_status efi_main(efi_handle image,efi_system_table *system) {
    serial_init();serial_text("ENGINE_OS_LOADER build=" BUILD_ID "\n");
    if(!system || !table_valid(&system->header,UINT64_C(0x5453595320494249),sizeof(*system)) || !system->boot)fail("system table");
    bs=system->boot;
    if(!table_valid(&bs->header,UINT64_C(0x56524553544f4f42),sizeof(*bs)))fail("boot services table");
    bs->watchdog(0,0,0,0);
    uint64_t cr4;__asm__ volatile("movq %%cr4,%0":"=r"(cr4));if(cr4&(1u<<12))fail("LA57 not supported by M1");
    efi_loaded_image *loaded=0;efi_filesystem *fs=0;efi_file *root=0;
    if(EFI_ERROR(bs->handle_protocol(image,&loaded_guid,(void**)&loaded))||!loaded)fail("loaded image protocol");
    if(!loaded->base || (uint64_t)(uintptr_t)loaded->base>=UINT64_C(0x100000000) ||
       loaded->size>UINT64_C(0x100000000)-(uint64_t)(uintptr_t)loaded->base)fail("loader image size");
    uint64_t loader_start=(uint64_t)(uintptr_t)loaded->base&~UINT64_C(4095);
    uint64_t loader_end=((uint64_t)(uintptr_t)loaded->base+loaded->size+4095)&~UINT64_C(4095);
    if(loader_end>UINT64_C(0x100000000)||loader_end<=loader_start)fail("loader identity range");
    reserve(loader_start,loader_end-loader_start);
    if(EFI_ERROR(bs->handle_protocol(loaded->device,&fs_guid,(void**)&fs))||!fs ||
       EFI_ERROR(fs->open_volume(fs,&root))||!root)fail("boot filesystem");
    uint64_t elf_size=0,init_size=0;
    uint8_t *elf=read_file(root,(uint16_t*)u"\\KERNEL.ELF",4*1024*1024,&elf_size);
    uint8_t *init=read_file(root,(uint16_t*)u"\\INIT.BIN",4096,&init_size);
#ifdef ENGINE_SECURE_PAYLOAD
    extern unsigned engine_payload_trust(efi_system_table*,const void*,size_t,const void*,size_t);
    if(engine_payload_trust(system,elf,elf_size,init,init_size))fail("payload SHA256 or firmware trust state");
#endif
    if(EFI_ERROR(root->close(root)))fail("close root");
    if(init_size!=48 || memcmp(init,"ENGINIT1",8) || os_read32(init+8)!=48 || os_read32(init+12)>
#ifdef ENGINE_M5
       7 ||
#elif defined(ENGINE_M45)
       6 ||
#else
       4 ||
#endif
       memcmp(init+16,BUILD_ID,32))fail("initimage manifest");
    elf_plan plan;if(elf_validate(elf,elf_size,&plan))fail("ELF validation");
#ifdef ENGINE_M72
    efi_runtime_services*rt=system->runtime;
    if(!rt||!table_valid(&rt->header,UINT64_C(0x56524553544e5552),sizeof(*rt)))fail("runtime services table");
    efi_status registration=boot72_register(rt);
    if(EFI_ERROR(registration)){
        serial_text("M72_UEFI_BOOT_REGISTER_FAILED status=");serial_hex(registration);serial_text("\n");
        typedef struct {void*reset;efi_status(*write)(void*,uint16_t*);} boot72_console;
        boot72_console*output=system->output;
        if(output&&output->write)output->write(output,(uint16_t*)u"\r\nEngine OS could not save its firmware startup entry.\r\nEnable writable UEFI variables or use the BIOS test profile.\r\nNo installation disk has been erased. See the installation guide.\r\n");
        fail("Engine OS firmware boot registration; NVRAM must be writable");
    }
    serial_text("M72_UEFI_BOOT_REGISTERED\n");
#endif
    uint64_t kernel_address=plan.low,kernel_pages=(plan.high-plan.low)/4096;
    if(EFI_ERROR(bs->allocate_pages(2,1,kernel_pages,&kernel_address)))fail("kernel fixed allocation");
    reserve(kernel_address,kernel_pages*4096);memset((void*)(uintptr_t)kernel_address,0,(size_t)(kernel_pages*4096));
    elf_header h;memcpy(&h,elf,sizeof(h));
    for(uint16_t i=0;i<h.phnum;++i) {
        elf_segment p;memcpy(&p,elf+h.phoff+(uint64_t)i*56,sizeof(p));
        if(p.type==1)memcpy((void*)(uintptr_t)p.paddr,elf+p.offset,(size_t)p.filesz);
    }
#ifdef ENGINE_M45
    /* Loaded segments own their backing now; retain neither the file copy nor its reservation. */
    uint64_t temporary_address=(uint64_t)(uintptr_t)elf,temporary_bytes=(elf_size+4095)&~UINT64_C(4095);unsigned removed=0;
    if(EFI_ERROR(bs->free_pages(temporary_address,temporary_bytes/4096)))fail("release temporary kernel image");
    for(unsigned i=0;i<reservation_count;++i)if(reservations[i].physical_address==temporary_address&&reservations[i].byte_length==temporary_bytes){for(unsigned j=i+1;j<reservation_count;++j)reservations[j-1]=reservations[j];--reservation_count;removed=1;break;}
    if(!removed)fail("temporary reservation ownership");serial_text("UEFI_TEMP_KERNEL_BUFFER_RELEASED bytes=");serial_hex(temporary_bytes);serial_text("\n");elf=0;
#endif
#ifdef ENGINE_M3
    uint64_t trampoline=0x8000;
    if(EFI_ERROR(bs->allocate_pages(2,1,1,&trampoline)))fail("AP trampoline allocation");
    reserve(trampoline,4096);memset((void*)(uintptr_t)trampoline,0,4096);
#endif
    os_boot_info_v0 *boot=allocate(2,2);
    boot->magic=OS_BOOT_MAGIC;boot->version=0;boot->structure_size=sizeof(*boot);
    boot->kernel=(os_boot_range){plan.low,plan.high-plan.low};
    boot->initimage=(os_boot_range){(uint64_t)(uintptr_t)init,init_size};
    efi_gop *gop=0;
    if(!EFI_ERROR(bs->locate_protocol(&gop_guid,0,(void**)&gop)) && gop && gop->mode && gop->mode->info && gop->mode->info_size>=36) {
#ifdef ENGINE_M73
        preferred_display73(gop);
#endif
        efi_gop_mode *m=gop->mode;efi_gop_info *v=m->info;
        if(v->format<=1 && m->framebuffer && m->framebuffer<UINT64_C(0x100000000) &&
           m->framebuffer_size<=UINT64_C(0x100000000)-m->framebuffer && v->pixels_per_scanline<=UINT32_MAX/4) {
            boot->framebuffer=(os_boot_range){m->framebuffer,m->framebuffer_size};
            boot->framebuffer_stride_bytes=v->pixels_per_scanline*4;boot->framebuffer_format=v->format+1;
            boot->framebuffer_width=v->width;boot->framebuffer_height=v->height;
        }
    }
    if(system->table_count>1024 || (system->table_count&&!system->tables))fail("configuration table count");
    for(uint64_t i=0;i<system->table_count;++i)if(!memcmp(&system->tables[i].guid,&acpi_guid,16)) {
        uint8_t *rsdp=system->tables[i].table;uint8_t sum=0;
        if(!rsdp || (uint64_t)(uintptr_t)rsdp>UINT64_C(0x100000000)-36 || memcmp(rsdp,"RSD PTR ",8))fail("ACPI root");
        for(unsigned j=0;j<20;++j)sum=(uint8_t)(sum+rsdp[j]);if(sum)fail("ACPI checksum");
        uint32_t len=rsdp[15]>=2?os_read32(rsdp+20):20;
        if(len!=20 && len!=36)fail("ACPI size");
        sum=0;for(unsigned j=0;j<len;++j)sum=(uint8_t)(sum+rsdp[j]);if(sum)fail("ACPI extended checksum");
        boot->acpi=(os_boot_range){(uint64_t)(uintptr_t)rsdp,len};break;
    }
    uint64_t *tables=allocate(6,2);tables[0]=(uint64_t)(uintptr_t)(tables+512)|3;
    for(unsigned i=0;i<4;++i) {
        tables[512+i]=(uint64_t)(uintptr_t)(tables+1024+i*512)|3;
        for(unsigned j=0;j<512;++j)tables[1024+i*512+j]=((uint64_t)i*512+j)*0x200000|0x83;
    }
    uint8_t *stack=allocate(8,2);
    uint64_t map_pages=64;
#ifdef ENGINE_M2
    uint64_t needed=0,key_probe=0,stride_probe=0;uint32_t version_probe=0;
    if(bs->memory_map(&needed,0,&key_probe,&stride_probe,&version_probe)!=EFI_BUFFER_TOO_SMALL ||
       !stride_probe || stride_probe>4096 || needed>64*4096-stride_probe*32)fail("memory map sizing");
    map_pages=(needed+stride_probe*32+4095)/4096;
#endif
    uint8_t *map=allocate(map_pages,2);const uint64_t map_capacity=map_pages*4096;
    uint8_t *reservation_bytes=(uint8_t*)boot+256;
    memcpy(reservation_bytes,reservations,reservation_count*sizeof(os_boot_range));
    boot->reservations=(os_boot_range){(uint64_t)(uintptr_t)reservation_bytes,reservation_count*sizeof(os_boot_range)};
    serial_text("ELF_LOADED entry=");serial_hex(plan.entry);serial_text("\n");
    for(unsigned attempt=0;attempt<4;++attempt) {
        uint64_t map_size=map_capacity,key=0,stride=0;uint32_t version=0;
        if(EFI_ERROR(bs->memory_map(&map_size,map,&key,&stride,&version))||stride>UINT32_MAX)fail("final memory map");
        boot->memory_map=(os_boot_range){(uint64_t)(uintptr_t)map,map_size};
        boot->memory_descriptor_size=(uint32_t)stride;boot->memory_descriptor_version=version;
        boot->crc32=os_boot_crc((uint8_t*)boot);
        if(os_boot_validate((uint8_t*)boot,sizeof(*boot)))fail("outgoing boot header");
        efi_status status=bs->exit_boot_services(image,(os_read32(init+12)==4&&attempt==0)?key+1:key);
        if(status==0) {
            __asm__ volatile("cli; cld":::"memory");
            serial_text("EXIT_BOOT_SERVICES_OK\n");
            if(os_read32(init+12)==3)boot->crc32^=1;
            kernel_handoff(boot,plan.entry,(uint64_t)(uintptr_t)(stack+8*4096),(uint64_t)(uintptr_t)tables);
        }
        if(status!=EFI_INVALID_PARAMETER)fail("ExitBootServices");
        serial_text("EXIT_BOOT_SERVICES_RETRY\n");
    }
    fail("ExitBootServices retry exhausted");
}
