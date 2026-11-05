/* Test-only enroller: compiled into ignored test media, never a release asset.
   Refuses non-TCG execution and any nonempty existing platform-key policy. */
#include "uefi.h"
#include "test-certificate.h"
static efi_guid global={0x8be4df61,0x93ca,0x11d2,{0xaa,0x0d,0,0xe0,0x98,3,0x2b,0x8c}};
static efi_guid database={0xd719b2cb,0x3d3a,0x4596,{0xa3,0xbc,0xda,0xd0,0x0e,0x67,0x65,0x6f}};
static efi_guid x509={0xa5c059a1,0x94e4,0x4aa7,{0x87,0xb5,0xab,0x15,0x5c,0x2b,0xf0,0x72}};
static efi_guid pkcs7={0x4aafd29d,0x68df,0x49ee,{0x8a,0xa9,0x34,0x7d,0x37,0x56,0x65,0xa7}};
static uint8_t payload[4096];
static void put32(unsigned offset,uint32_t value){memcpy(payload+offset,&value,4);}
static efi_status enroll(efi_runtime_services *rt,uint16_t *name,efi_guid *guid){
    if(name[0]=='P'&&name[1]=='K'&&!name[2])return rt->set_variable(name,guid,0x27,sizeof(test_pk_auth),(void*)test_pk_auth);
    memset(payload,0,sizeof(payload));uint16_t year=2026;memcpy(payload,&year,2);payload[2]=11;payload[3]=5;payload[4]=1;
    put32(16,24);uint16_t rev=0x200,type=0xef1;memcpy(payload+20,&rev,2);memcpy(payload+22,&type,2);memcpy(payload+24,&pkcs7,16);
    unsigned cert_size=sizeof(test_certificate),list_size=28+16+cert_size;if(40+list_size>sizeof(payload))return EFI_INVALID_PARAMETER;
    memcpy(payload+40,&x509,16);put32(56,list_size);put32(60,0);put32(64,16+cert_size);memcpy(payload+68,&global,16);memcpy(payload+84,test_certificate,cert_size);
    return rt->set_variable(name,guid,0x27,40+list_size,payload);
}
efi_status efi_main(efi_handle image,efi_system_table *system){
    (void)image;serial_init();uint32_t a,b,c,d;__asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(0x40000000u),"c"(0));char vendor[12];memcpy(vendor,&b,4);memcpy(vendor+4,&c,4);memcpy(vendor+8,&d,4);
    if(memcmp(vendor,"TCGTCGTCGTCG",12)){serial_text("ENROLL_REFUSED_NON_TCG\n");test_exit(0x32);}
    efi_runtime_services *rt=system->runtime;uint8_t setup=0;uint64_t size=1;if(EFI_ERROR(rt->get_variable((uint16_t*)u"SetupMode",&global,0,&size,&setup))||setup!=1){serial_text("ENROLL_REFUSED_EXISTING_POLICY\n");test_exit(0x32);}
    const uint16_t *names[3]={u"db",u"KEK",u"PK"};efi_guid *guids[3]={&database,&global,&global};
    for(unsigned i=0;i<3;++i){uint8_t byte;size=1;efi_status s=rt->get_variable((uint16_t*)names[i],guids[i],0,&size,&byte);if(s!=UINT64_C(0x800000000000000e)){serial_text("ENROLL_REFUSED_NONEMPTY_POLICY\n");test_exit(0x32);}}
    for(unsigned i=0;i<3;++i){efi_status s=enroll(rt,(uint16_t*)names[i],guids[i]);serial_text("ENROLL_VARIABLE status=");serial_hex(s);serial_text("\n");if(EFI_ERROR(s))test_exit(0x32);}
    serial_text("ENROLL_TEST_POLICY_PASS\n");test_exit(0x10);
}
