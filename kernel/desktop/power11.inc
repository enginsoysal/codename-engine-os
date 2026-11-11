#include <engine/boot/validate_v0.h>
#include <engine/desktop/power11.h>
static uint16_t power_a11,power_b11,power_smi11,power_reset_port11;static unsigned power_type_a11,power_type_b11;static uint8_t power_enable11,power_reset_value11;
static unsigned power_sum11(const uint8_t*p,unsigned n){uint8_t sum=0;for(unsigned i=0;i<n;++i)sum=(uint8_t)(sum+p[i]);return sum==0;}
static const uint8_t *power_table11(uint64_t address,const char*sig){if(address<4096||address>UINT64_C(0x100000000)-36)return 0;const uint8_t*p=(const uint8_t*)(uintptr_t)address;unsigned size=os_read32(p+4);if(memcmp(p,sig,4)||size<36||size>65536||size>UINT64_C(0x100000000)-address||!power_sum11(p,size))return 0;return p;}
static uint16_t power_port11(const uint8_t*f,unsigned size,unsigned legacy,unsigned extended){if(size>=extended+12&&os_read64(f+extended+4)){uint64_t port=os_read64(f+extended+4);return f[extended]==1&&f[extended+1]==16&&!f[extended+2]&&(f[extended+3]==0||f[extended+3]==2)&&port<=65534?(uint16_t)port:0;}uint32_t port=os_read32(f+legacy);return port<=65534?(uint16_t)port:0;}
static void power_init11(const os_boot_info_v0*boot){
 start_power_shutdown11=0;uint32_t max,b,c,d;__asm__ volatile("cpuid":"=a"(max),"=b"(b),"=c"(c),"=d"(d):"a"(0x40000000u),"c"(0));char vendor[13];memcpy(vendor,&b,4);memcpy(vendor+4,&c,4);memcpy(vendor+8,&d,4);vendor[12]=0;unsigned known=equal(vendor,"VMwareVMware")||equal(vendor,"TCGTCGTCGTCG")||!memcmp(vendor,"KVMKVMKVM",9);if(max<0x40000000u||!known)return;
 if(boot->acpi.byte_length<20||boot->acpi.physical_address<4096||boot->acpi.physical_address>UINT64_C(0x100000000)-boot->acpi.byte_length||boot->acpi.byte_length>4096)return;const uint8_t*r=(const uint8_t*)(uintptr_t)boot->acpi.physical_address;if(!r||memcmp(r,"RSD PTR ",8)||!power_sum11(r,20))return;
 unsigned width=4;uint64_t address=os_read32(r+16);if(r[15]>=2&&boot->acpi.byte_length>=36&&os_read32(r+20)>=36&&os_read32(r+20)<=boot->acpi.byte_length&&power_sum11(r,os_read32(r+20))&&os_read64(r+24)){width=8;address=os_read64(r+24);}
 const uint8_t*root=power_table11(address,width==8?"XSDT":"RSDT");if(!root)return;unsigned size=os_read32(root+4);if((size-36)%width||(size-36)/width>256)return;
 const uint8_t*f=0;for(unsigned at=36;at<size;at+=width){const uint8_t*found=power_table11(width==8?os_read64(root+at):os_read32(root+at),"FACP");if(found){if(f)return;f=found;}}if(!f||(size=os_read32(f+4))<116)return;
 if(size>=129&&(os_read32(f+112)&(1u<<10))&&f[116]==1&&f[117]==8&&!f[118]&&(f[119]==0||f[119]==1)){uint64_t port=os_read64(f+120);if(port&&port<=65535){power_reset_port11=(uint16_t)port;power_reset_value11=f[128];}}
 if((os_read32(f+112)&(1u<<20))||f[89]!=2)return;power_a11=power_port11(f,size,64,172);power_b11=power_port11(f,size,68,184);if(!power_a11||((os_read32(f+68)||(size>=196&&os_read64(f+188)))&&!power_b11))return;
 uint64_t dsdt=size>=148&&os_read64(f+140)?os_read64(f+140):os_read32(f+40);const uint8_t*aml=power_table11(dsdt,"DSDT");if(!aml||!p11_s5(aml+36,os_read32(aml+4)-36,&power_type_a11,&power_type_b11))return;
 uint32_t smi=os_read32(f+48);if(smi>65535)return;power_smi11=(uint16_t)smi;power_enable11=f[52];start_power_shutdown11=1;serial_text("M11_POWER_S5_PROVIDER_READY\n");
}
static uint16_t power_read16_11(uint16_t port){uint16_t value;__asm__ volatile("inw %1,%0":"=a"(value):"Nd"(port));return value;}
static void power_write16_11(uint16_t port,uint16_t value){__asm__ volatile("outw %0,%1"::"a"(value),"Nd"(port));}
static unsigned power_request11(unsigned shutdown){
 if(m6_volume()->readonly){start_message11("The system volume needs recovery. A safe power operation could not be confirmed.");return 0;}
 if(file_job_state11.active||file_job_state11.waiting||w8_pending()||transfer_owner74){start_message11("Finish or cancel the active transfer/installer before using power controls.");return 0;}
 for(unsigned i=0;i<DESKTOP_WINDOWS;++i)if(model.windows[i].live&&apps[i].dirty){start_message11("Save your unsaved work and try again. Power controls keep the system running.");return 0;}
 if(shutdown&&!start_power_shutdown11){start_message11("Shutdown is not available for this firmware profile. Your session is still running.");return 0;}
 for(unsigned i=0;i<storage_count();++i)if(storage_writable(storage_get(i))&&storage_flush(i)){start_message11("A disk did not confirm its cache flush. Power operation stopped; check recovery.");return 0;}
 audio74_stop();serial_text(shutdown?"M11_USER_SHUTDOWN_CONFIRMED\n":"M11_USER_RESTART_CONFIRMED\n");
 if(!shutdown){if(power_reset_port11)out8(power_reset_port11,power_reset_value11);out8(0xcf9,6);for(unsigned i=0;i<100000;++i)if(!(in8(0x64)&2)){out8(0x64,0xfe);break;}start_message11("Restart did not complete. Your session is still running.");return 0;}
 if(!(power_read16_11(power_a11)&1)){if(!power_smi11||!power_enable11){start_message11("Firmware ACPI mode is unavailable. Shutdown was not completed.");return 0;}out8(power_smi11,power_enable11);uint64_t until=m3_clock_ticks()+500;while(!(power_read16_11(power_a11)&1)&&m3_clock_ticks()<until)__asm__ volatile("sti; hlt":::"memory");if(!(power_read16_11(power_a11)&1)){start_message11("Firmware did not enter ACPI mode. Your session is still running.");return 0;}}
 uint16_t a=(uint16_t)((power_read16_11(power_a11)&~0x3c00u)|(power_type_a11<<10));uint16_t b=power_b11?(uint16_t)((power_read16_11(power_b11)&~0x3c00u)|(power_type_b11<<10)):0;power_write16_11(power_a11,a);if(power_b11)power_write16_11(power_b11,b);if(power_b11)power_write16_11(power_b11,(uint16_t)(b|0x2000));power_write16_11(power_a11,(uint16_t)(a|0x2000));start_message11("Shutdown did not complete. Your session is still running.");return 0;
}
