#include <engine/m5.h>
#include "build.h"
extern const unsigned m5_template_crc,m5_install_crc;
static uint64_t overlay[4096];static unsigned overlay_pages,volume_disk=UINT32_MAX,enabled;
#ifdef ENGINE_M51
static unsigned guided;
extern void m51_progress(unsigned);
#endif
static unsigned planned=UINT32_MAX,repairing;static storage_disk target;
static uint8_t intent[512],plan_digest[512],page[4096],verify_page[4096],entries[16384];
static uint32_t r32(const void *p){uint32_t v;memcpy(&v,p,4);return v;}
static uint64_t r64(const void *p){uint64_t v;memcpy(&v,p,8);return v;}
static void w32(void *p,uint32_t v){memcpy(p,&v,4);}
static void w64(void *p,uint64_t v){memcpy(p,&v,8);}
static int eq(const char *a,const char *b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
static int prefix(const char *a,const char *b){while(*b)if(*a++!=*b++)return 0;return 1;}
static int zero(const void *data,unsigned n){const uint8_t *p=data;while(n--)if(*p++)return 0;return 1;}
static unsigned parse(const char **p,uint64_t *out){uint64_t n=0;unsigned digits=0;while(**p>='0'&&**p<='9'){unsigned d=(unsigned)(*(*p)++-'0');if(n>(UINT64_MAX-d)/10)return 0;n=n*10+d;++digits;}*out=n;return digits;}
static unsigned io(unsigned disk,uint64_t lba,void *data,unsigned bytes,unsigned write){const storage_disk *d=storage_get(disk);if(!d||d->transport!=STORAGE_AHCI||d->sector_bytes!=512||!d->readable)return B_DENIED;return m5_sata_transfer(d->context,lba,data,bytes,write);}
static unsigned flush(unsigned disk){const storage_disk *d=storage_get(disk);return d?m5_sata_flush(d->context):B_DENIED;}
static const uint8_t *next_record(const uint8_t *r){return r+((r32(r)&0x80000000u)?4:4100);}
static unsigned record_lba(const uint8_t *r){return r32(r)&0x7fffffffu;}
static unsigned packed_valid(const uint8_t *data,unsigned size,unsigned crc){
    if(size<4||size>2*1024*1024||m4_crc(data,size)!=crc)return 0;unsigned n=r32(data);if(!n||n>4096)return 0;
    uint64_t end=0;unsigned offset=4;for(unsigned i=0;i<n;++i){if(offset>size||size-offset<4)return 0;unsigned value=r32(data+offset),lba=value&0x7fffffffu,bytes=(value&0x80000000u)?4:4100;if(bytes>size-offset||(lba&7)||lba<end||lba>=M5_END)return 0;end=lba+8;offset+=bytes;}return offset==size;
}
static void template_page(unsigned n,void *data){
    memset(data,0,4096);unsigned count=r32(m5_template);const uint8_t *r=m5_template+4;for(unsigned i=0;i<count;++i,r=next_record(r)){if(record_lba(r)==n*8){if(!(r32(r)&0x80000000u))memcpy(data,r+4,4096);return;}}
}
unsigned m5_read_page(uint64_t lba,void *data){
    if((lba&7)||lba>M4_SECTORS-8)return B_DENIED;
    if(volume_disk!=UINT32_MAX)return io(volume_disk,M5_DATA+lba,data,4096,0);
    unsigned n=(unsigned)(lba/8);if(overlay[n])memcpy(data,(void*)(uintptr_t)overlay[n],4096);else template_page(n,data);return B_OK;
}
unsigned m5_read(uint64_t lba,void *data){if(lba>=M4_SECTORS)return B_DENIED;uint8_t b[4096];unsigned s=m5_read_page(lba&~UINT64_C(7),b);if(!s)memcpy(data,b+(lba&7)*512,512);return s;}
unsigned m5_write_page(uint64_t lba,const void *data){
    if(!enabled||(lba&7)||lba<16||lba>M4_SECTORS-8)return B_DENIED;
    if(volume_disk!=UINT32_MAX)return io(volume_disk,M5_DATA+lba,(void*)data,4096,1);
    unsigned n=(unsigned)(lba/8);if(!overlay[n]){if(overlay_pages==128)return B_DENIED;overlay[n]=page_alloc();if(!overlay[n])return B_IO;++overlay_pages;}memcpy((void*)(uintptr_t)overlay[n],data,4096);return B_OK;
}
static unsigned sector_write(uint64_t lba,const void *data){
    if(!enabled||lba>=M4_SECTORS||!lba)return B_DENIED;
    if(volume_disk!=UINT32_MAX)return io(volume_disk,M5_DATA+lba,(void*)data,512,1);
    unsigned n=(unsigned)(lba/8);if(!overlay[n]){if(overlay_pages==128)return B_DENIED;overlay[n]=page_alloc();if(!overlay[n])return B_IO;++overlay_pages;template_page(n,(void*)(uintptr_t)overlay[n]);}memcpy((uint8_t*)(uintptr_t)overlay[n]+(lba&7)*512,data,512);return B_OK;
}
unsigned m5_write(uint64_t lba,const void *data){return lba<2?B_DENIED:sector_write(lba,data);}
unsigned m5_record(const void *data){return sector_write(1,data);}
unsigned m5_enable(void){enabled=1;return B_OK;}
unsigned m5_flush(void){return volume_disk==UINT32_MAX?B_OK:flush(volume_disk);}
static unsigned owned(const uint8_t *record,const storage_disk *d){return !memcmp(record,"ENGM5V1\0",8)&&r32(record+8)==1&&(r32(record+12)==1||r32(record+12)==2)&&r64(record+16)==d->sectors&&!memcmp(record+24,BUILD_ID,32)&&r64(record+88)==M5_DATA&&r64(record+96)==M4_SECTORS&&r32(record+508)==m4_crc(record,508);}
static unsigned layout_intact(unsigned disk,const uint8_t *record){
    const storage_disk *d=storage_get(disk);uint32_t table_crc=0;
    for(unsigned copy=0;copy<2;++copy){
        uint64_t where=copy?d->sectors-1:1,table=copy?d->sectors-33:2;
        if(io(disk,where,page,512,0)||memcmp(page,"EFI PART",8)||r32(page+12)!=92||r64(page+24)!=where||r64(page+32)!=(copy?1:d->sectors-1)||r64(page+72)!=table||r32(page+80)!=128||r32(page+84)!=128||memcmp(page+56,record+56,16))return 0;
        unsigned crc=r32(page+16);w32(page+16,0);if(crc!=m4_crc(page,92))return 0;
        unsigned expected=r32(page+88);if(copy&&expected!=table_crc)return 0;table_crc=expected;
        for(unsigned i=0;i<32;++i){if(io(disk,table+i,page,512,0))return 0;if(copy){if(memcmp(page,entries+i*512,512))return 0;}else memcpy(entries+i*512,page,512);}
        if(!copy&&m4_crc(entries,sizeof(entries))!=table_crc)return 0;
    }
    return r64(entries+32)==M5_ESP&&r64(entries+40)==M5_DATA-1&&r64(entries+256+32)==M5_DATA&&r64(entries+256+40)==M5_END-1;
}
unsigned m5_volume_start(void){
    if(!packed_valid(m5_template,m5_template_size,m5_template_crc))return B_IO;
    if(!m5_install_size){unsigned matches=0,candidates=0,found=UINT32_MAX;for(unsigned i=0;i<storage_count();++i){const storage_disk *d=storage_get(i);if(d->transport!=STORAGE_AHCI||d->sector_bytes!=512||d->sectors<M5_MIN_SECTORS)continue;++candidates;if(!io(i,41,intent,512,0)&&owned(intent,d)&&r32(intent+12)==2&&layout_intact(i,intent)){++matches;found=i;}}
        if(matches==1&&candidates==1){volume_disk=found;serial_text("M5_INSTALLED_VOLUME\n");}else if(matches&&candidates>1){serial_text("M5_AMBIGUOUS_INSTALLS_REFUSED\n");return B_DENIED;}else serial_text("M5_INCOMPLETE_INSTALL_LIVE_FALLBACK\n");
    }else serial_text("M5_LIVE_VOLATILE_VOLUME\n");return B_OK;
}
void m5_welcome(void){console_text(m5_install_size?"M5 live installer. File writes are temporary RAM changes.\n":"M5 installed kernel.\n");console_text(volume_disk==UINT32_MAX?"Volume: volatile; changes disappear on reboot.\n":"Volume: persistent native ext2; 16 MiB bounded profile.\n");console_text("For installation: disks, plan N, then the exact confirmation shown.\nFor same-build boot repair: repairplan N. Type help for commands.\n");}
static void record_seal(uint8_t *r,unsigned state){w32(r+12,state);w32(r+508,m4_crc(r,508));}
static unsigned blank(unsigned n){
    const storage_disk *d=storage_get(n);for(unsigned i=0;i<64;++i){unsigned s=io(n,i,page,512,0);if(s)return s;if(!zero(page,512))return B_DENIED;}
    for(unsigned i=1;i<=33;++i){unsigned s=io(n,d->sectors-i,page,512,0);if(s)return s;if(!zero(page,512))return B_DENIED;}return B_OK;
}
static unsigned prepare(unsigned n,unsigned repair){
    planned=UINT32_MAX;if(!m5_install_size||!packed_valid(m5_install,m5_install_size,m5_install_crc))return B_DENIED;
    const storage_disk *d=storage_get(n);if(!d||d->transport!=STORAGE_AHCI||d->sector_bytes!=512||!d->readable||d->sectors<M5_MIN_SECTORS||d->sectors>UINT32_MAX)return B_DENIED;
    target=*d;memset(intent,0,512);
    if(repair){if(io(n,40,intent,512,0)||!owned(intent,d)){if(io(n,41,intent,512,0)||!owned(intent,d))return B_DENIED;}}
    else{unsigned s=blank(n);if(s)return s;memcpy(intent,"ENGM5V1\0",8);w32(intent+8,1);w64(intent+16,d->sectors);memcpy(intent+24,BUILD_ID,32);uint32_t low,high;__asm__ volatile("rdtsc":"=a"(low),"=d"(high));uint64_t stamp=((uint64_t)high<<32)|low;w64(intent+56,stamp);w64(intent+64,stamp^d->sectors^d->bdf);w64(intent+72,1);w32(intent+80,m5_install_crc);w64(intent+88,M5_DATA);w64(intent+96,M4_SECTORS);memcpy(intent+104,BUILD_ID,32);}
    if(io(n,0,plan_digest,512,0))return B_IO;planned=n;repairing=repair;
    #ifdef ENGINE_M51
    if(!guided){
    #endif
    console_text(repair?"REPAIR PLAN - system boot files only; preserve data partition.\n":"INSTALL PLAN - target contents may be lost; this is not secure erasure.\n");
    console_text("Disk ");console_decimal(n);console_text(" SATA/AHCI PCI ");console_hex(d->bdf);console_text(" port ");console_decimal(d->unit);console_text(" sectors ");console_decimal(d->sectors);console_text("\nGPT: BIOS boot area, 64 MiB EFI, 16 MiB native data. Rest unallocated.\n");
    console_text("Unsigned BIOS + UEFI fallback boot; disable Secure Boot.\nNo changes yet. Type cancel or confirm exactly:\n");console_text(repair?"repair ":"install ");console_decimal(n);console_text(repair?" REPAIR-":" ERASE-");console_decimal(d->sectors);console_text("\n");
#ifdef ENGINE_M51
    }
#endif
    return B_OK;
}
static void partition(unsigned i,const uint8_t type[16],uint64_t first,uint64_t last,const char *name){
    uint8_t *p=entries+i*128;memcpy(p,type,16);memcpy(p+16,intent+56,16);p[31]^=(uint8_t)(i+1);w64(p+32,first);w64(p+40,last);for(unsigned j=0;name[j]&&j<36;++j)p[56+j*2]=(uint8_t)name[j];
}
static void header(void *out,uint64_t current,uint64_t backup,uint64_t table){
    uint8_t *p=out;memset(p,0,512);memcpy(p,"EFI PART",8);w32(p+8,0x10000);w32(p+12,92);w64(p+24,current);w64(p+32,backup);w64(p+40,34);w64(p+48,target.sectors-34);memcpy(p+56,intent+56,16);w64(p+72,table);w32(p+80,128);w32(p+84,128);w32(p+88,m4_crc(entries,sizeof(entries)));w32(p+16,m4_crc(p,92));
}
static unsigned verified_write(unsigned disk,uint64_t lba,void *data,unsigned bytes){unsigned s=io(disk,lba,data,bytes,1);if(!s)s=io(disk,lba,verify_page,bytes,0);if(!s&&memcmp(data,verify_page,bytes))s=B_IO;return s;}
static unsigned gpt_write(unsigned disk){
    static const uint8_t esp[16]={0x28,0x73,0x2a,0xc1,0x1f,0xf8,0xd2,0x11,0xba,0x4b,0,0xa0,0xc9,0x3e,0xc9,0x3b};
    static const uint8_t bios[16]={0x48,0x61,0x68,0x21,0x49,0x64,0x6f,0x6e,0x74,0x4e,0x65,0x65,0x64,0x45,0x46,0x49};
    static const uint8_t data[16]={0xaf,0x3d,0xc6,0x0f,0x83,0x84,0x72,0x47,0x8e,0x79,0x3d,0x69,0xd8,0x47,0x7d,0xe4};
    memset(entries,0,sizeof(entries));partition(0,esp,M5_ESP,M5_DATA-1,"" ENGINE_PRODUCT_NAME " EFI");partition(1,bios,64,M5_ESP-1,"Engine BIOS");partition(2,data,M5_DATA,M5_END-1,"Engine native data");
    for(unsigned i=0;i<32;++i){unsigned s=verified_write(disk,2+i,entries+i*512,512);if(!s)s=verified_write(disk,target.sectors-33+i,entries+i*512,512);if(s)return s;}
    header(page,1,target.sectors-1,2);unsigned s=verified_write(disk,1,page,512);if(s)return s;header(page,target.sectors-1,1,target.sectors-33);s=verified_write(disk,target.sectors-1,page,512);if(s)return s;
    if(io(disk,0,page,512,0))return B_IO;memset(page+446,0,64);page[450]=0xee;w32(page+454,1);w32(page+458,(uint32_t)(target.sectors-1));return verified_write(disk,0,page,512);
}
static unsigned run(unsigned disk,unsigned repair){
    const storage_disk *d=storage_get(disk);if(planned!=disk||repairing!=repair||!d||(d->transport!=target.transport||d->bdf!=target.bdf||d->unit!=target.unit||d->context!=target.context||d->sectors!=target.sectors||d->sector_bytes!=512||!d->readable)||io(disk,0,page,512,0)||memcmp(page,plan_digest,512))return B_DENIED;
    planned=UINT32_MAX;if(!repair){unsigned s=blank(disk);if(s)return s;}
    unsigned needs_data=!repair||r32(intent+84)!=1;record_seal(intent,1);unsigned s=verified_write(disk,40,intent,512);memset(page,0,512);if(!s)s=verified_write(disk,41,page,512);if(!s)s=flush(disk);if(s)return s;
    serial_text(repair?"M5_REPAIR_BEGIN\n":"M5_INSTALL_BEGIN\n");
    unsigned count=r32(m5_install);const uint8_t *r=m5_install+4;for(unsigned i=0;!s&&i<count;++i,r=next_record(r)){memset(page,0,4096);
#ifdef ENGINE_M51
    if(guided)m51_progress(i*80/count);
#endif
    s=verified_write(disk,record_lba(r),(r32(r)&0x80000000u)?page:(void*)(r+4),4096);}
    r=m5_template+4;if(needs_data)for(unsigned i=0;!s&&i<r32(m5_template);++i,r=next_record(r)){memset(page,0,4096);s=verified_write(disk,M5_DATA+record_lba(r),(r32(r)&0x80000000u)?page:(void*)(r+4),4096);}
    if(!s){w32(intent+84,1);record_seal(intent,1);s=verified_write(disk,40,intent,512);if(!s)s=flush(disk);}
    #ifdef ENGINE_M51
    if(guided)m51_progress(90);
    #endif
    if(!s)s=gpt_write(disk);if(!s)s=flush(disk);if(s){console_text("Copy/verify failed. Installation remains incomplete; use this ISO to repair.\n");return s;}
    record_seal(intent,2);s=verified_write(disk,41,intent,512);if(!s)s=flush(disk);if(s)return s;
    #ifdef ENGINE_M51
    if(guided)m51_progress(100);
    #endif
    serial_text(repair?"M5_REPAIR_PASS\n":"M5_INSTALL_PASS\n");console_text("Verified. Power off the VM, detach ISO/USB, boot this disk.\n");return B_OK;
}
unsigned m5_command(const char *line){
    unsigned action=0,repair=0;const char *p=line;
    if(eq(line,"cancel")){planned=UINT32_MAX;console_text("Plan cancelled. No writes performed by cancel.\n");return 1;}
    if(eq(line,"volume")){console_text(volume_disk==UINT32_MAX?"Volatile RAM volume. Overlay pages: ":"Persistent native volume. Overlay pages: ");console_decimal(overlay_pages);console_text(" / 128\n");return 1;}
    if(prefix(p,"plan ")){action=1;p+=5;}else if(prefix(p,"repairplan ")){action=1;repair=1;p+=11;}else if(prefix(p,"install ")){action=2;p+=8;}else if(prefix(p,"repair ")){action=2;repair=1;p+=7;}else return 0;
    uint64_t n=0,capacity=0;unsigned status=B_DENIED;
    if(parse(&p,&n)&&n<STORAGE_MAX){if(action==1&&!*p)status=prepare((unsigned)n,repair);else if(action==2&&*p++==' '&&prefix(p,repair?"REPAIR-":"ERASE-")){p+=repair?7:6;if(parse(&p,&capacity)&&!*p&&capacity==target.sectors)status=run((unsigned)n,repair);}}
    console_text("Installer status: ");console_decimal(status);console_text("\n");return 1;
}

/* Mode 7 runs in a disposable live guest and restores every test allocation. */
unsigned m5_overlay_test(void){
    if(volume_disk!=UINT32_MAX||overlay_pages)return B_DENIED;page_stats before=pages_stats();enabled=1;memset(page,0x5a,4096);
    for(unsigned i=0;i<128;++i)if(m5_write_page(16+i*8,page))return B_IO;
    if(overlay_pages!=128||m5_write_page(16+128*8,page)!=B_DENIED||pages_stats().current_pages!=before.current_pages+128)return B_IO;
    if(m5_read_page(16,verify_page)||memcmp(page,verify_page,4096)||m5_write_page(16,page))return B_IO;
    for(unsigned i=0;i<4096;++i)if(overlay[i]){page_free(overlay[i]);overlay[i]=0;}
    overlay_pages=0;enabled=0;if(pages_stats().current_pages!=before.current_pages)return B_IO;
    serial_text("M5_OVERLAY_LIMIT_BYTES=0x0000000000080000\nM5_OVERLAY_RELEASE_BASELINE_RESTORED\n");return B_OK;
}

#ifdef ENGINE_M51
unsigned m51_prepare(unsigned disk,unsigned repair){guided=1;return prepare(disk,repair);}
unsigned m51_run(unsigned disk,unsigned repair){return run(disk,repair);}
unsigned m51_is_live(void){return m5_install_size!=0;}
unsigned m51_persistent(void){return volume_disk!=UINT32_MAX;}
void m51_cancel(void){planned=UINT32_MAX;guided=0;}
#endif
