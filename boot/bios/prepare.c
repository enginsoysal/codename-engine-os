#include <engine/base.h>
#include <engine/boot/validate_v0.h>
#include "bios-config.h"
extern uint16_t bios_e820_count;extern uint8_t bios_video;
typedef struct {uint32_t type,pad;uint64_t physical,virtual_address,pages,attributes;} memory_entry;
typedef struct __attribute__((packed)){uint64_t base,length;uint32_t type,attributes;} e820_entry;
static _Alignas(8) os_boot_info_v0 boot;
static _Alignas(8) memory_entry map[192];static unsigned map_count;
static _Alignas(8) os_boot_range reserved[6]={{0x4000,0x3000},{0x8000,0x1000},{0x10000,0x8000},{0x60000,0x10000},{0x100000,0x6000},{0x200000,BIOS_KERNEL_BYTES}};
static e820_entry raw[64];
static NORETURN void reject(const char *s){serial_text("BIOS_REJECT: ");serial_text(s);serial_text("\n");test_exit(0x23);}
static void add(uint64_t begin,uint64_t end,unsigned type){if(begin>UINT64_MAX-4095)reject("memory alignment overflow");begin=(begin+4095)&~UINT64_C(4095);end&=~UINT64_C(4095);if(begin>=end)return;if(map_count==192)reject("memory map bound");map[map_count++]=(memory_entry){type,0,begin,0,(end-begin)>>12,0};}
static unsigned available(uint64_t start,uint64_t length){for(unsigned i=0;i<bios_e820_count;++i)if(raw[i].type==1&&raw[i].base<=start&&raw[i].length>=length&&start-raw[i].base<=raw[i].length-length)return 1;return 0;}
static uint32_t checksum(const void *data,unsigned length){const uint8_t *p=data;uint32_t crc=UINT32_MAX;for(unsigned i=0;i<length;++i){crc^=p[i];for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1)));}return ~crc;}
static uint32_t rsdp(uint32_t start,uint32_t end){for(uint32_t p=start;p+36<=end;p+=16){const uint8_t *r=(void*)(uintptr_t)p;if(memcmp(r,"RSD PTR ",8))continue;uint8_t sum=0;for(unsigned i=0;i<20;++i)sum=(uint8_t)(sum+r[i]);if(sum)continue;if(r[15]>=2){if(os_read32(r+20)!=36)continue;for(unsigned i=20;i<36;++i)sum=(uint8_t)(sum+r[i]);if(sum)continue;}return p;}return 0;}
uint32_t bios_prepare(void){
    serial_init();serial_text("ENGINE_BIOS32_PREPARE\n");uint32_t a,b,c,d;__asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(0x80000000u),"c"(0));if(a<0x80000001u)reject("x86-64 CPUID required");__asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(0x80000001u),"c"(0));if((d&((1u<<29)|(1u<<20)))!=((1u<<29)|(1u<<20)))reject("x86-64 and NX required");
    if(!bios_e820_count||bios_e820_count>64)reject("E820 count");memcpy(raw,(void*)(uintptr_t)0x5400,bios_e820_count*24);
    for(unsigned i=0;i<bios_e820_count;++i){if(raw[i].length>UINT64_MAX-raw[i].base)reject("E820 overflow");for(unsigned j=i;j&&raw[j].base<raw[j-1].base;--j){e820_entry tmp=raw[j];raw[j]=raw[j-1];raw[j-1]=tmp;}}
    for(unsigned i=1;i<bios_e820_count;++i)if(raw[i-1].length&&raw[i].length&&raw[i].base<raw[i-1].base+raw[i-1].length)reject("E820 overlap");
    for(unsigned i=0;i<6;++i)if(!available(reserved[i].physical_address,reserved[i].byte_length))reject("loader/kernel region not usable");
    if(checksum((void*)(uintptr_t)0x200000,BIOS_KERNEL_BYTES)!=BIOS_KERNEL_CRC)reject("kernel raw CRC");
    for(unsigned i=0;i<bios_e820_count;++i){uint64_t start=raw[i].base,end=start+raw[i].length;unsigned type=raw[i].type==1?7:raw[i].type==3?9:raw[i].type==4?10:0;if(type!=7){add(start,end,type);continue;}
        if(start<4096){add(start,end<4096?end:4096,0);start=end<4096?end:4096;}
        for(unsigned j=0;j<6&&start<end;++j){uint64_t a=reserved[j].physical_address,b=a+reserved[j].byte_length;if(b<=start||a>=end)continue;if(start<a)add(start,a,7);add(start>a?start:a,b<end?b:end,2);if(b>start)start=b;}if(start<end)add(start,end,7);
    }
    boot.magic=OS_BOOT_MAGIC;boot.version=0;boot.structure_size=160;boot.memory_map=(os_boot_range){(uint32_t)(uintptr_t)map,map_count*40};boot.memory_descriptor_size=40;boot.memory_descriptor_version=1;boot.kernel=(os_boot_range){0x200000,BIOS_KERNEL_BYTES};boot.initimage=(os_boot_range){(uint32_t)(uintptr_t)bios_init,sizeof(bios_init)};boot.reservations=(os_boot_range){(uint32_t)(uintptr_t)reserved,sizeof(reserved)};
    if(bios_video){const uint8_t *v=(void*)(uintptr_t)0x4200;uint16_t width,height,pitch;memcpy(&pitch,v+16,2);memcpy(&width,v+18,2);memcpy(&height,v+20,2);unsigned physical=os_read32(v+40);
        if(width&&height&&pitch>=(uint32_t)width*4&&v[25]==32&&v[31]==8&&v[33]==8&&v[35]==8&&v[34]==8&&((v[32]==16&&v[36]==0)||(v[32]==0&&v[36]==16))&&physical){boot.framebuffer=(os_boot_range){physical,(uint32_t)pitch*height};boot.framebuffer_width=width;boot.framebuffer_height=height;boot.framebuffer_stride_bytes=pitch;boot.framebuffer_format=v[32]==0?1:2;}}
    uint16_t ebda;memcpy(&ebda,(void*)(uintptr_t)0x40e,2);uint32_t root=ebda?rsdp((uint32_t)ebda<<4,((uint32_t)ebda<<4)+1024):0;if(!root)root=rsdp(0xe0000,0x100000);if(root){const uint8_t *p=(void*)(uintptr_t)root;boot.acpi=(os_boot_range){root,p[15]>=2?36:20};}
    uint64_t *tables=(void*)(uintptr_t)0x100000;memset(tables,0,0x6000);tables[0]=0x101003;for(unsigned i=0;i<4;++i){tables[512+i]=(0x102000+i*4096)|3;for(unsigned j=0;j<512;++j)tables[1024+i*512+j]=((uint64_t)(i*512+j)<<21)|0x83;}
    boot.crc32=os_boot_crc((uint8_t*)&boot);
#if BIOS_BAD_BOOT
    boot.crc32^=1;
#endif
    serial_text("BIOS_FRAMEBUFFER width=");serial_hex(boot.framebuffer_width);serial_text(" height=");serial_hex(boot.framebuffer_height);serial_text("\n");serial_text("BIOS_E820_NORMALIZED own_kernel_crc_ok\n");serial_text("BIOS_KERNEL_HANDOFF\n");return (uint32_t)(uintptr_t)&boot;
}
