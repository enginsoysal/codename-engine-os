#include <engine/storage.h>
static storage_disk disks[STORAGE_MAX];static storage_reader readers[STORAGE_MAX];static unsigned count;
void console_decimal(uint64_t value){char out[21];unsigned i=20;out[i]=0;do{out[--i]=(char)('0'+value%10);value/=10;}while(value);console_text(out+i);}
unsigned storage_register(storage_disk disk,storage_reader reader){if(count==STORAGE_MAX)return B_DENIED;if(disk.readable){uint8_t first[4096];disk.status=reader(disk.context,0,first);if(disk.status)disk.readable=0;}disks[count]=disk;readers[count++]=reader;return B_OK;}
unsigned storage_count(void){return count;}
void storage_unavailable(unsigned index){if(index<count){disks[index].readable=0;disks[index].status=B_ABSENT;}}
const storage_disk *storage_get(unsigned index){return index<count?&disks[index]:0;}
unsigned storage_read(unsigned index,uint64_t lba,void *buffer){if(index>=count||!buffer)return B_DENIED;storage_disk *d=&disks[index];if(!d->readable)return d->status?d->status:B_UNSUPPORTED;if(lba>=d->sectors)return B_DENIED;d->status=readers[index](d->context,lba,buffer);return d->status;}
void storage_inventory(void){
    if(!count){console_text("No readable storage devices discovered.\n");return;}
    for(unsigned i=0;i<count;++i){const storage_disk *d=&disks[i];console_text("Disk ");console_decimal(i);console_text(d->transport==STORAGE_AHCI?" SATA/AHCI":d->transport==STORAGE_NVME?" NVMe":d->transport==STORAGE_USB?" USB":d->transport==STORAGE_IDE?" ATA/PIO":d->transport==STORAGE_SCSI?" SCSI":" VirtIO");console_text(" - ");
        if(d->sector_bytes&&d->sectors<=UINT64_MAX/d->sector_bytes){console_decimal(d->sectors*d->sector_bytes/(1024*1024));console_text(" MiB");}else console_text("capacity overflow");
        console_text(" - ");console_decimal(d->sector_bytes);console_text(" bytes/sector - ");if(d->status){console_text("read error ");console_decimal(d->status);}else console_text(d->readable?"readable":"unsupported sector/profile");console_text(d->marked?" - development volume\n":" - read-only discovery\n");}
}
static uint32_t pci(unsigned bdf,unsigned reg){uint32_t value,address=0x80000000u|(bdf<<8)|(reg&252);__asm__ volatile("outl %0,%1"::"a"(address),"Nd"((uint16_t)0xcf8));__asm__ volatile("inl %1,%0":"=a"(value):"Nd"((uint16_t)0xcfc));return value;}
void hardware_inventory(void){unsigned shown=0;for(unsigned bus=0;bus<256&&shown<128;++bus)for(unsigned dev=0;dev<32&&shown<128;++dev){unsigned first=(bus<<8)|(dev<<3);if((pci(first,0)&65535)==65535)continue;unsigned functions=pci(first,12)&0x800000?8:1;for(unsigned f=0;f<functions&&shown<128;++f){unsigned bdf=first|f;if((pci(bdf,0)&65535)==65535)continue;unsigned cls=pci(bdf,8)>>8;console_text("PCI ");console_decimal(bus);console_text(":");console_decimal(dev);console_text(".");console_decimal(f);console_text(" class=");console_hex(cls);unsigned bound=0;for(unsigned i=0;i<count;++i)bound|=disks[i].bdf==bdf&&disks[i].readable;
            console_text(bound?" - storage data transfer verified\n":cls==0x0c0330?" - xHCI; driver status follows\n":(cls>>16)==1?" - storage driver/device unsupported or unavailable\n":(cls>>16)==2?" - network driver pending\n":" - discovered; no capability claim\n");if(cls==0x0c0330)usb_hardware_inventory(bdf);++shown;}}}
unsigned storage_selftest(void){uint8_t data[4096];for(unsigned i=0;i<count;++i){const storage_disk *d=&disks[i];if(d->readable&&(storage_read(i,0,data)||storage_read(i,d->sectors-1,data)||storage_read(i,d->sectors,data)!=B_DENIED))return B_IO;if(d->readable&&(d->transport==STORAGE_NVME||d->transport==STORAGE_VIRTIO||d->transport==STORAGE_USB))for(unsigned n=0;n<(d->transport==STORAGE_USB?140u:64u)&&n<d->sectors;++n)if(storage_read(i,n,data))return B_IO;}return B_OK;}

#ifdef ENGINE_M92
static storage_io transfers92[STORAGE_MAX];static storage_sync syncs92[STORAGE_MAX];
unsigned storage_register_io(storage_disk disk,storage_reader reader,storage_io io,storage_sync sync){
 unsigned index=count;if(index==STORAGE_MAX||!reader)return B_DENIED;unsigned s=storage_register(disk,reader);if(!s){transfers92[index]=io;syncs92[index]=sync;}return s;
}
unsigned storage_can_write(const storage_disk*d){
 if(!d||!d->readable||d->sector_bytes!=512)return 0;
 for(unsigned i=0;i<count;++i)if(disks[i].transport==d->transport&&disks[i].bdf==d->bdf&&disks[i].unit==d->unit&&disks[i].context==d->context)return transfers92[i]&&syncs92[i];return 0;
}
unsigned storage_transfer(unsigned index,uint64_t lba,void*data,unsigned bytes,unsigned write){
 if(index>=count||!data||!bytes||bytes>4096)return B_DENIED;storage_disk*d=&disks[index];
 if(!d->readable||!d->sector_bytes||bytes%d->sector_bytes||lba>=d->sectors||bytes/d->sector_bytes>d->sectors-lba)return B_DENIED;
 if(transfers92[index]){unsigned status=transfers92[index](d->context,lba,data,bytes,write);if(status==B_IO||status==B_TIMEOUT||status==B_ABSENT){d->status=status;d->readable=0;}return status;}
 if(write)return B_UNSUPPORTED;uint8_t*p=data;while(bytes){unsigned s=storage_read(index,lba++,p);if(s)return s;p+=d->sector_bytes;bytes-=d->sector_bytes;}return B_OK;
}
unsigned storage_flush(unsigned index){if(index>=count||!storage_can_write(&disks[index]))return B_DENIED;unsigned status=syncs92[index](disks[index].context);if(status){disks[index].status=status;disks[index].readable=0;}return status;}
#endif
