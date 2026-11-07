#ifdef ENGINE_M7
#include <engine/desktop/app.h>
#endif
#include <engine/install/m6.h>
#include "build.h"
#ifdef ENGINE_M71
#include <engine/install/disk71.h>
static unsigned replace_disk;static d71_disk replacement;static d71_plan erase_plan;
#endif
static engine_install_layout layout;static storage_disk target;static unsigned selected=UINT32_MAX,persistent;
static unsigned restarting,maintenance_mode,active_slot;static v6_volume volume;static uint8_t page[4096],check[4096],entries[16384],identity[512],record[512];
static int equals(const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
static uint32_t r32(const void*p){uint32_t v;memcpy(&v,p,4);return v;}
static uint64_t r64(const void*p){uint64_t v;memcpy(&v,p,8);return v;}
static void w32(void*p,uint32_t v){memcpy(p,&v,4);}
static void w64(void*p,uint64_t v){memcpy(p,&v,8);}
static unsigned io(unsigned disk,uint64_t lba,void*p,unsigned bytes,unsigned write){const storage_disk*d=storage_get(disk);return !d||d->transport!=STORAGE_AHCI||d->sector_bytes!=512?B_DENIED:m5_sata_transfer(d->context,lba,p,bytes,write);}
static unsigned flush(unsigned disk){const storage_disk*d=storage_get(disk);return d?m5_sata_flush(d->context):B_DENIED;}
static unsigned verified(unsigned disk,uint64_t lba,void*p,unsigned bytes){if(io(disk,lba,p,bytes,1)||io(disk,lba,check,bytes,0)||memcmp(p,check,bytes))return B_IO;return B_OK;}
static unsigned read_volume(void*ctx,uint64_t block,void*p,unsigned bytes){(void)ctx;if(block>=volume.blocks||bytes!=4096)return B_DENIED;return io(selected,layout.data.first+block*8,p,bytes,0);}
static unsigned write_volume(void*ctx,uint64_t block,void*p,unsigned bytes){(void)ctx;if(block>=volume.blocks||bytes!=4096)return B_DENIED;return verified(selected,layout.data.first+block*8,p,bytes);}
static unsigned flush_volume(void*ctx){(void)ctx;return flush(selected);}
static void configure_volume(void){memset(&volume,0,sizeof(volume));volume.blocks=(layout.data.last-layout.data.first+1)/8;volume.read=read_volume;volume.write=write_volume;volume.flush=flush_volume;}
static unsigned activation(unsigned);
static unsigned blank(unsigned disk){const storage_disk*d=storage_get(disk);for(unsigned i=0;i<97;++i){uint64_t lba=i<64?i:d->sectors-(i-63);if(io(disk,lba,page,512,0))return B_IO;for(unsigned j=0;j<512;++j)if(page[j])return B_DENIED;}return B_OK;}
unsigned m6_live(void){return m6_bundle_size!=0;}
unsigned m6_prepare(unsigned disk){m6_cancel();const storage_disk*d=storage_get(disk);if(!m6_live()||!d||!d->readable||d->transport!=STORAGE_AHCI||engine_install_plan(d->sectors,d->sector_bytes,&layout))return B_DENIED;unsigned s=blank(disk);restarting=0;if(s==B_DENIED){if(io(disk,40,record,512,0)||memcmp(record,"ENGINEI6",8)||r64(record+8)!=d->sectors||r32(record+72)!=1||r32(record+508)!=m4_crc(record,508))return B_DENIED;if(activation(disk)!=B_DENIED)return B_DENIED;restarting=1;}else if(s)return s;target=*d;selected=disk;if(io(disk,0,identity,512,0)){m6_cancel();return B_IO;}return B_OK;}
void m6_cancel(void){if(!persistent)selected=UINT32_MAX;
#ifdef ENGINE_M71
 replace_disk=0;
#endif
}
#ifdef ENGINE_M71
static unsigned utility_read(void*c,uint64_t l,void*p,unsigned n){return io((unsigned)(uintptr_t)c,l,p,n,0);}
static unsigned utility_write(void*c,uint64_t l,void*p,unsigned n){return io((unsigned)(uintptr_t)c,l,p,n,1);}
static unsigned utility_flush(void*c){return flush((unsigned)(uintptr_t)c);}
static d71_device utility(unsigned disk){const storage_disk*d=storage_get(disk);return (d71_device){(void*)(uintptr_t)disk,utility_read,utility_write,utility_flush,d?d->sectors:0,d?d->sector_bytes:0};}
v6_volume*m71_workspace(void){return !persistent?&volume:0;}
unsigned m71_prepare_replace(unsigned disk){m6_cancel();const storage_disk*d=storage_get(disk);if(!m6_live()||!d||!d->readable||d->transport!=STORAGE_AHCI||engine_install_plan(d->sectors,d->sector_bytes,&layout))return B_DENIED;
 d71_device dev=utility(disk);if(d71_scan(&dev,&replacement)||d71_prepare(&dev,&replacement,D71_ERASE,0,0,0,0,&erase_plan))return B_IO;
 target=*d;selected=disk;restarting=0;if(io(disk,0,identity,512,0))return B_IO;replace_disk=1;return B_OK;
}
#endif
const engine_install_layout *m6_plan(void){return &layout;}
v6_volume *m6_volume(void){return persistent?&volume:0;}
static void partition(unsigned n,const uint8_t*type,engine_partition part,const char*name){uint8_t*p=entries+n*128;memcpy(p,type,16);memcpy(p+16,record+56,16);p[31]^=(uint8_t)(n+1);w64(p+32,part.first);w64(p+40,part.last);for(unsigned i=0;name[i]&&i<36;++i)p[56+i*2]=(uint8_t)name[i];}
static void header(uint64_t where,uint64_t other,uint64_t table){memset(page,0,512);memcpy(page,"EFI PART",8);w32(page+8,0x10000);w32(page+12,92);w64(page+24,where);w64(page+32,other);w64(page+40,34);w64(page+48,layout.sectors-34);memcpy(page+56,record+56,16);w64(page+72,table);w32(page+80,128);w32(page+84,128);w32(page+88,m4_crc(entries,sizeof(entries)));w32(page+16,m4_crc(page,92));}
static unsigned gpt(void){
 static const uint8_t efi[16]={0x28,0x73,0x2a,0xc1,0x1f,0xf8,0xd2,0x11,0xba,0x4b,0,0xa0,0xc9,0x3e,0xc9,0x3b},bios[16]={0x48,0x61,0x68,0x21,0x49,0x64,0x6f,0x6e,0x74,0x4e,0x65,0x65,0x64,0x45,0x46,0x49},system[16]={0x2b,0xd4,0xc3,0xe3,0x47,0x1c,0x0e,0x5e,0x99,0xcb,0xea,0x67,0x1c,0x38,0x72,0xb3},data[16]={0x67,0xba,0x5c,0x40,0xd5,0xd6,0xea,0x5d,0x98,0xe5,0x04,0x21,0x7a,0xa2,0x63,0xf6};
 memset(entries,0,sizeof(entries));partition(0,bios,layout.bios,"Engine BIOS");partition(1,efi,layout.esp,"Engine EFI");partition(2,system,layout.system_a,"Engine System A");partition(3,system,layout.system_b,"Engine System B");partition(4,data,layout.data,"Engine Data");
 for(unsigned i=0;i<32;++i)if(verified(selected,2+i,entries+i*512,512)||verified(selected,layout.backup_table+i,entries+i*512,512))return B_IO;
 header(1,layout.backup_header,2);if(verified(selected,1,page,512))return B_IO;header(layout.backup_header,1,layout.backup_table);if(verified(selected,layout.backup_header,page,512))return B_IO;
 if(io(selected,0,page,512,0))return B_IO;memset(page+446,0,64);page[450]=0xee;w32(page+454,1);w32(page+458,layout.sectors-1>UINT32_MAX?UINT32_MAX:(uint32_t)(layout.sectors-1));return verified(selected,0,page,512);
}
static unsigned activation(unsigned disk){const storage_disk*d=storage_get(disk);unsigned found=0;for(unsigned copy=0;copy<2;++copy){if(io(disk,41+copy,page,512,0))continue;if(memcmp(page,"ENGINEI6",8)||r64(page+8)!=d->sectors||r32(page+72)!=2||r32(page+76)>1||r32(page+508)!=m4_crc(page,508))continue;if(found&&r64(page+80)==r64(record+80)&&memcmp(page,record,512))return B_IO;if(!found||r64(page+80)>r64(record+80))memcpy(record,page,512);found=1;}return found?B_OK:B_DENIED;}
static unsigned activate_record(void){uint64_t gen=r64(record+80);if(gen==UINT64_MAX)return B_DENIED;w64(record+80,gen+1);w32(record+508,m4_crc(record,508));unsigned sector=((gen+1)&1)?41:42;return verified(selected,sector,record,512)||flush(selected)?B_IO:B_OK;}
static unsigned source_memory(void*ctx,uint64_t off,void*p,unsigned n){memcpy(p,(const uint8_t*)ctx+off,n);return 0;}
static unsigned directory(const char*p){unsigned s=v6_mkdir(&volume,p);return s==V6_EXISTS?0:s;}
unsigned m6_save_settings(const m6_preferences*p){return !persistent?B_DENIED:v6_put(&volume,"/System/Settings/setup.bin",sizeof(*p),source_memory,(void*)p);}
unsigned m6_settings(m6_preferences*p){unsigned bytes=0;if(!persistent||v6_verify(&volume,"/System/Settings/setup.bin")||v6_read(&volume,"/System/Settings/setup.bin",0,p,sizeof(*p),&bytes)||bytes!=sizeof(*p)||p->guide>1||p->accent>0xffffff)return B_IO;unsigned n=0;while(n<sizeof(p->user)&&p->user[n]){char c=p->user[n++];if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'))return B_IO;}return !n||n==sizeof(p->user)?B_IO:B_OK;}
static unsigned save_slot(unsigned);
static unsigned bundle_valid(void){if(m6_bundle_size<4||m4_crc(m6_bundle,m6_bundle_size)!=m6_bundle_crc)return 0;unsigned count=r32(m6_bundle),off=4;uint64_t end=0;
 for(unsigned i=0;i<count;++i){if(off>m6_bundle_size||m6_bundle_size-off<4)return 0;unsigned value=r32(m6_bundle+off),bytes=(value&0x80000000u)?4:4100,lba=value&0x7fffffffu;if(bytes>m6_bundle_size-off)return 0;if((lba&7)||lba<end)return 0;end=(uint64_t)lba+8;
 if(lba&&!(lba>=layout.bios.first&&end<=layout.bios.last+1)&&!(lba>=layout.esp.first&&end<=layout.esp.last+1))return 0;off+=bytes;}return off==m6_bundle_size;
}
unsigned m6_install(const m6_preferences*p){const storage_disk*d=storage_get(selected);if(!d||persistent||!m6_live()||!bundle_valid()||memcmp(d,&target,sizeof(*d))||io(selected,0,page,512,0)||memcmp(page,identity,512))return B_DENIED;
 unsigned userlen=0;while(userlen<32&&p->user[userlen]){char c=p->user[userlen++];if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return B_DENIED;}if(!userlen||userlen==32||p->guide>1)return B_DENIED;
#ifdef ENGINE_M71
 if(replace_disk){replace_disk=0;d71_device dev=utility(selected);unsigned erase_status=d71_apply(&dev,&replacement,&erase_plan,&volume);if(erase_status)return erase_status==D71_CHANGED?B_DENIED:B_IO;serial_text("M71_REPLACEMENT_METADATA_CLEARED\n");}
#endif
 unsigned s=restarting?B_OK:blank(selected);if(s)return s;if(restarting){if(io(selected,40,record,512,0)||memcmp(record,"ENGINEI6",8)||r64(record+8)!=target.sectors||r32(record+72)!=1||r32(record+508)!=m4_crc(record,508))return B_DENIED;if(activation(selected)!=B_DENIED)return B_DENIED;}memset(record,0,512);memcpy(record,"ENGINEI6",8);w64(record+8,layout.sectors);memcpy(record+24,BUILD_ID,32);uint32_t low,high;__asm__ volatile("rdtsc":"=a"(low),"=d"(high));w64(record+56,((uint64_t)high<<32)|low);w64(record+64,layout.sectors^target.bdf^low);w32(record+72,1);w32(record+508,m4_crc(record,508));if(verified(selected,40,record,512)||flush(selected))return B_IO;
 if(save_slot(0))return B_IO;w32(record+76,0);
 m6_progress(5,"Preparing system files");unsigned count=r32(m6_bundle),offset=4;for(unsigned i=0;i<count;++i){const uint8_t*r=m6_bundle+offset;unsigned zero=r32(r)&0x80000000u;memset(page,0,4096);if(verified(selected,r32(r)&0x7fffffffu,zero?page:(void*)(r+4),4096))return B_IO;offset+=zero?4:4100;if(i%16==0)m6_progress(5+i*60/count,"Copying and verifying boot files");}
 configure_volume();m6_progress(70,"Creating your data volume");if(v6_format(&volume))return B_IO;
 for(unsigned i=0;i<8;++i){const char *names[]={"/System","/Applications","/Users","/Data","/Cache","/Logs","/Temporary","/Volumes"};if(directory(names[i]))return B_IO;}
 if(directory("/System/Settings"))return B_IO;char user[256]="/Users/";memcpy(user+7,p->user,userlen+1);if(directory(user))return B_IO;
 for(unsigned i=0;i<5;++i){const char*names[]={"Desktop","Documents","Downloads","Pictures","AppData"};char sub[256];memcpy(sub,user,7+userlen);sub[7+userlen]='/';unsigned k=0;do{sub[8+userlen+k]=names[i][k];}while(names[i][k++]);if(directory(sub))return B_IO;}
 if(p->guide){char guidepath[256];memcpy(guidepath,user,7+userlen);const char*tail="/Documents/Welcome.txt";unsigned j=0;do{guidepath[7+userlen+j]=tail[j];}while(tail[j++]);const char*guide=
#ifdef ENGINE_M7
 "Welcome to Engine OS. Open Files, Text Editor, Settings and Downloads from your desktop. Your documents persist on disk.\n";
#else
 "Welcome to Engine OS. Your documents persist on disk. The full desktop follows in M7. Remove this guide independently of the base system.\n";
#endif
unsigned size=0;while(guide[size])++size;if(v6_put(&volume,guidepath,size,source_memory,(void*)guide))return B_IO;}
 for(unsigned file=0;file<m6_file_count;++file){const m6_distribution_file*f=m6_files+file;if(!p->guide&&equals(f->path,"/Users/test/Documents/Welcome.txt"))continue;char path[256];unsigned n=0,off=0;if(!memcmp(f->path,"/Users/test/",12)){memcpy(path,"/Users/",7);n=7;for(unsigned j=0;j<userlen;++j)path[n++]=p->user[j];path[n++]='/';off=12;}for(;f->path[off]&&n<255;++off)path[n++]=f->path[off];if(f->path[off])return B_DENIED;path[n]=0;for(unsigned j=1;j<n;++j)if(path[j]=='/'){path[j]=0;unsigned status=directory(path);path[j]='/';if(status)return B_IO;}if(v6_put(&volume,path,f->size,source_memory,(void*)f->data))return B_IO;}

#ifdef ENGINE_M7
 if(app7_install(&volume,m7_calculator_selection()))return B_IO;
#endif
 persistent=1;if(m6_save_settings(p)){persistent=0;return B_IO;}m6_progress(90,"Verifying partition and boot setup");if(gpt()||flush(selected)){persistent=0;return B_IO;}
 w32(record+72,2);if(activate_record()){persistent=0;return B_IO;}if(verified(selected,42,record,512)||verified(selected,40,record,512)||flush(selected)){persistent=0;return B_IO;}
 m6_progress(100,"Installation verified");serial_text("M6_INSTALL_PASS\n");return B_OK;
}
static unsigned valid_layout(void){uint32_t expected=0;for(unsigned copy=0;copy<2;++copy){uint64_t at=copy?layout.backup_header:1,table=copy?layout.backup_table:2;if(io(selected,at,page,512,0)||memcmp(page,"EFI PART",8)||r32(page+12)!=92||r64(page+24)!=at||r64(page+32)!=(copy?1:layout.backup_header)||r64(page+72)!=table||r32(page+80)!=128||r32(page+84)!=128||memcmp(page+56,record+56,16))return B_IO;uint32_t crc=r32(page+16),sum=r32(page+88);w32(page+16,0);if(m4_crc(page,92)!=crc||(copy&&sum!=expected))return B_IO;expected=sum;for(unsigned i=0;i<32;++i){if(io(selected,table+i,page,512,0))return B_IO;if(copy){if(memcmp(page,entries+i*512,512))return B_IO;}else memcpy(entries+i*512,page,512);}if(m4_crc(entries,sizeof(entries))!=sum)return B_IO;}
 const engine_partition *parts[]={&layout.bios,&layout.esp,&layout.system_a,&layout.system_b,&layout.data};for(unsigned i=0;i<5;++i)if(r64(entries+i*128+32)!=parts[i]->first||r64(entries+i*128+40)!=parts[i]->last)return B_IO;return B_OK;}
unsigned m6_open(void){
#ifndef ENGINE_M72
 if(m6_live())return B_ABSENT;
#endif
 unsigned found=UINT32_MAX,matches=0;
 for(unsigned i=0;i<storage_count();++i){const storage_disk*d=storage_get(i);engine_install_layout plan;if(!d||engine_install_plan(d->sectors,d->sector_bytes,&plan)||d->transport!=STORAGE_AHCI)continue;
 if(!activation(i)){found=i;++matches;}}
 if(matches!=1)return B_DENIED;selected=found;target=*storage_get(found);if(activation(found))return B_IO;if(memcmp(record+24,BUILD_ID,32)){serial_text("M6_BOOT_VERSION_MISMATCH\n");return B_IO;}if(engine_install_plan(target.sectors,target.sector_bytes,&layout))return B_DENIED;if(valid_layout())return B_IO;configure_volume();unsigned s=v6_mount(&volume);if(s)return s;persistent=1;serial_text("M6_INSTALLED_VOLUME\n");return B_OK;
}
/* Legacy marked-volume entry points are unavailable in the M6 storage path. */
unsigned m5_read(uint64_t l,void*p){(void)l;(void)p;return B_DENIED;}
unsigned m5_read_page(uint64_t l,void*p){(void)l;(void)p;return B_DENIED;}
unsigned m5_write(uint64_t l,const void*p){(void)l;(void)p;return B_DENIED;}
unsigned m5_write_page(uint64_t l,const void*p){(void)l;(void)p;return B_DENIED;}
unsigned m5_record(const void*p){(void)p;return B_DENIED;}
unsigned m5_flush(void){return B_DENIED;}
unsigned m5_enable(void){return B_DENIED;}

static uint32_t crc_update(uint32_t c,const void*p,unsigned n){const uint8_t*b=p;while(n--){c^=*b++;for(unsigned j=0;j<8;++j)c=(c>>1)^(0xedb88320u&-(c&1));}return c;}
static uint64_t slot_first(unsigned slot){return slot?layout.system_b.first:layout.system_a.first;}
static unsigned save_slot(unsigned slot){
 if(!bundle_valid()||m6_bundle_size>(layout.system_a.last-layout.system_a.first)*512)return B_DENIED;
 uint64_t first=slot_first(slot);memset(page,0,4096);if(verified(selected,first,page,4096)||flush(selected))return B_IO;
 for(unsigned off=0;off<m6_bundle_size;off+=4096){unsigned bytes=m6_bundle_size-off;if(bytes>4096)bytes=4096;memset(page,0,4096);memcpy(page,m6_bundle+off,bytes);if(verified(selected,first+8+off/512,page,4096))return B_IO;}
 if(flush(selected))return B_IO;memset(page,0,4096);memcpy(page,"ENGINEB6",8);w32(page+8,m6_bundle_size);w32(page+12,m6_bundle_crc);memcpy(page+16,BUILD_ID,32);w32(page+4092,m4_crc(page,4092));if(verified(selected,first,page,4096)||flush(selected))return B_IO;return B_OK;
}
static unsigned slot_header(unsigned slot,unsigned*bytes,unsigned*sum){if(io(selected,slot_first(slot),page,4096,0)||memcmp(page,"ENGINEB6",8)||r32(page+4092)!=m4_crc(page,4092))return B_IO;*bytes=r32(page+8);*sum=r32(page+12);if(*bytes<4||*bytes>(layout.system_a.last-layout.system_a.first)*512)return B_DENIED;return B_OK;}
static unsigned slot_read(unsigned slot,unsigned off,void*p,unsigned bytes){
 uint8_t*out=p;while(bytes){unsigned at=off%4096,n=4096-at;if(n>bytes)n=bytes;if(io(selected,slot_first(slot)+8+(off/4096)*8,page,4096,0))return B_IO;memcpy(out,page+at,n);out+=n;off+=n;bytes-=n;}return B_OK;
}
static unsigned replay_slot(unsigned slot){unsigned bytes,sum;if(slot_header(slot,&bytes,&sum))return B_IO;uint32_t actual=~0u;uint8_t data[4096];
 for(unsigned off=0;off<bytes;off+=4096){unsigned n=bytes-off;if(n>4096)n=4096;if(slot_read(slot,off,data,n))return B_IO;actual=crc_update(actual,data,n);}if(~actual!=sum)return B_IO;
 uint32_t count;if(slot_read(slot,0,&count,4)||!count||count>(bytes-4)/4)return B_IO;uint64_t end=0;
 /* Validate every destination before changing any boot file. */
 unsigned offset=4;for(unsigned i=0;i<count;++i){uint32_t value;if(offset>bytes||bytes-offset<4||slot_read(slot,offset,&value,4))return B_IO;unsigned size=(value&0x80000000u)?4:4100,lba=value&0x7fffffffu;if(size>bytes-offset)return B_DENIED;uint64_t limit=(uint64_t)lba+8;if((lba&7)||lba<end||(lba&&!(lba>=layout.bios.first&&limit<=layout.bios.last+1)&&!(lba>=layout.esp.first&&limit<=layout.esp.last+1)))return B_DENIED;end=limit;offset+=size;}if(offset!=bytes)return B_DENIED;
 offset=4;for(unsigned i=0;i<count;++i){uint32_t value;if(slot_read(slot,offset,&value,4))return B_IO;unsigned zero=value&0x80000000u;memset(data,0,4096);if((!zero&&slot_read(slot,offset+4,data,4096))||verified(selected,value&0x7fffffffu,data,4096))return B_IO;offset+=zero?4:4100;if(i%16==0)m6_progress(10+i*70/count,"Restoring verified boot files");}return flush(selected);
}
unsigned m6_maintenance_prepare(unsigned disk,unsigned mode){
 if(!m6_live()||mode<1||mode>3)return B_DENIED;const storage_disk*d=storage_get(disk);if(!d||d->transport!=STORAGE_AHCI||engine_install_plan(d->sectors,d->sector_bytes,&layout)||activation(disk))return B_DENIED;
 target=*d;selected=disk;active_slot=r32(record+76);maintenance_mode=mode;if(io(disk,0,identity,512,0))return B_IO;unsigned bytes,sum;return slot_header(mode==3?(active_slot^1):active_slot,&bytes,&sum);
}
unsigned m6_maintenance(void){const storage_disk*d=storage_get(selected);if(!d||memcmp(d,&target,sizeof(*d))||io(selected,0,page,512,0)||memcmp(page,identity,512))return B_DENIED;
 unsigned chosen=active_slot;if(maintenance_mode==2){chosen^=1;unsigned s=save_slot(chosen);if(s)return s;}else if(maintenance_mode==3)chosen^=1;
 m6_progress(5,"Verifying recovery slot");unsigned s=replay_slot(chosen);if(s)return s;if(gpt()||flush(selected))return B_IO;
 if(io(selected,slot_first(chosen),page,4096,0))return B_IO;memcpy(record+24,page+16,32);w32(record+76,chosen);if(activate_record())return B_IO;serial_text("M6_MAINTENANCE_PASS\n");return B_OK;
}

unsigned m6_write_text(const char*path,const void*data,unsigned size){return !persistent?B_DENIED:v6_put(&volume,path,size,source_memory,(void*)data);}
