#include "uefi.h"
#include <engine/sha256.h>
#include "payload-digests.h"
unsigned engine_payload_trust(efi_system_table *system,const void *kernel,size_t kernel_size,const void *init,size_t init_size){
    efi_guid global={0x8be4df61,0x93ca,0x11d2,{0xaa,0x0d,0,0xe0,0x98,3,0x2b,0x8c}};efi_runtime_services *rt=system->runtime;uint8_t secure=0,setup=0;uint64_t size=1;
    if(EFI_ERROR(rt->get_variable((uint16_t*)u"SecureBoot",&global,0,&size,&secure))||size!=1||secure>1)return 1;size=1;if(EFI_ERROR(rt->get_variable((uint16_t*)u"SetupMode",&global,0,&size,&setup))||size!=1||setup>1)return 1;
    serial_text("ENGINE_TRUST_POLICY SecureBoot=");serial_hex(secure);serial_text(" SetupMode=");serial_hex(setup);serial_text("\n");if(secure&&setup)return 1;
    uint8_t hash[32];engine_sha256(kernel,kernel_size,hash);if(memcmp(hash,trusted_kernel_sha256,32))return 1;engine_sha256(init,init_size,hash);if(memcmp(hash,trusted_init_sha256,32))return 1;
    serial_text("SIGNED_LOADER_PAYLOAD_SHA256_OK\n");return 0;
}
