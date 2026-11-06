/* Disposable TCG VM test only: authenticated db replacement, never host policy. */
#include "uefi.h"
#include "policy-payload.h"
static efi_guid global={0x8be4df61,0x93ca,0x11d2,{0xaa,0x0d,0,0xe0,0x98,3,0x2b,0x8c}};
static efi_guid database={0xd719b2cb,0x3d3a,0x4596,{0xa3,0xbc,0xda,0xd0,0x0e,0x67,0x65,0x6f}};
static uint8_t buffer[4096];
static unsigned variable(efi_runtime_services *rt,const uint16_t *name,efi_guid *guid,const unsigned char *expected,unsigned length){
    uint64_t size=sizeof(buffer);uint32_t attributes=0;
    return !EFI_ERROR(rt->get_variable((uint16_t*)name,guid,&attributes,&size,buffer))&&size==length&&attributes==0x27&&!memcmp(buffer,expected,length);
}
efi_status efi_main(efi_handle image,efi_system_table *system){
    (void)image;serial_init();uint32_t a,b,c,d;__asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(0x40000000u),"c"(0));char vendor[12];memcpy(vendor,&b,4);memcpy(vendor+4,&c,4);memcpy(vendor+8,&d,4);
    if(memcmp(vendor,"TCGTCGTCGTCG",12)){serial_text("POLICY_REFUSED_NON_TCG\n");test_exit(0x32);}
    efi_runtime_services *rt=system->runtime;uint8_t value;uint64_t size=1;
    if(EFI_ERROR(rt->get_variable((uint16_t*)u"SetupMode",&global,0,&size,&value))||value){serial_text("POLICY_REFUSED_SETUP\n");test_exit(0x32);}
    size=1;if(EFI_ERROR(rt->get_variable((uint16_t*)u"SecureBoot",&global,0,&size,&value))||value!=1)test_exit(0x32);
    if(!variable(rt,u"db",&database,expected_before,sizeof(expected_before))||!variable(rt,u"PK",&global,expected_authority,sizeof(expected_authority))||!variable(rt,u"KEK",&global,expected_authority,sizeof(expected_authority))){serial_text("POLICY_REFUSED_UNEXPECTED_DATABASE\n");test_exit(0x32);}
    efi_status status=rt->set_variable((uint16_t*)u"db",&database,0x27,sizeof(auth_update),(void*)auth_update);
    serial_text("POLICY_UPDATE_STATUS=");serial_hex(status);serial_text("\n");
#ifdef EXPECT_DENIED
    if(status!=UINT64_C(0x800000000000001a)){serial_text("POLICY_EXPECTED_SECURITY_VIOLATION\n");test_exit(0x32);}
#else
    if(EFI_ERROR(status))test_exit(0x32);
#endif
    if(!variable(rt,u"db",&database,expected_after,sizeof(expected_after))||!variable(rt,u"PK",&global,expected_authority,sizeof(expected_authority))||!variable(rt,u"KEK",&global,expected_authority,sizeof(expected_authority)))test_exit(0x32);
    serial_text("POLICY_UPDATE_TEST_PASS\n");test_exit(0x10);
}
