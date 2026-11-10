/* Real registry/FAT32 code, isolated from privileged device instructions. */
#include <engine/hardware10.h>
#include <engine/storage/volume6.h>
#include "../../kernel/storage/registry.c"
void console_text(const char*s){(void)s;}void console_hex(uint64_t n){(void)n;}
void usb_hardware_inventory(unsigned bdf){(void)bdf;}
uint32_t m4_crc(const void*data,unsigned length){const uint8_t*p=data;uint32_t c=UINT32_MAX;for(unsigned i=0;i<length;++i){c^=p[i];for(unsigned b=0;b<8;++b)c=(c>>1)^(0xedb88320u&(0u-(c&1)));}return ~c;}
static void quiet_serial10(const char*s){(void)s;}
#define serial_text quiet_serial10
#include "../../kernel/storage/fat32.c"
static v6_volume*owned_root10;
v6_volume*m6_volume(void){return owned_root10;}
__declspec(dllexport) void host_root10(v6_volume*v){owned_root10=v;}
__declspec(dllexport) unsigned host_attach10(unsigned token,unsigned writable,uint64_t sectors,storage_reader r,storage_io io,storage_sync sync){
 if(count)storage_unavailable(0);
 storage_disk d={STORAGE_USB,8,1,512,1,0,sectors,token,0};
 return storage_register_io(d,r,writable?io:0,writable?sync:0);
}
