#ifndef ENGINE_STORAGE_H
#define ENGINE_STORAGE_H
#include <engine/m4.h>
#define STORAGE_MAX 16u
enum { STORAGE_AHCI=1,STORAGE_NVME,STORAGE_VIRTIO,STORAGE_USB,STORAGE_IDE,STORAGE_SCSI };
typedef struct {unsigned transport,bdf,unit,sector_bytes,readable,marked;uint64_t sectors;unsigned context,status;} storage_disk;
typedef unsigned (*storage_reader)(unsigned,uint64_t,void*);
unsigned storage_register(storage_disk,storage_reader);
#ifdef ENGINE_M92
typedef unsigned (*storage_io)(unsigned,uint64_t,void*,unsigned,unsigned);
typedef unsigned (*storage_sync)(unsigned);
unsigned storage_register_io(storage_disk,storage_reader,storage_io,storage_sync);
unsigned storage_can_write(const storage_disk*);
unsigned storage_transfer(unsigned,uint64_t,void*,unsigned,unsigned);
unsigned storage_flush(unsigned);
unsigned scsi92_discover(void);
#endif
const storage_disk *storage_get(unsigned);
unsigned m5_sata_transfer(unsigned,uint64_t,void*,unsigned,unsigned);
unsigned m5_sata_flush(unsigned);
static inline unsigned storage_writable(const storage_disk*d){
#ifdef ENGINE_M92
return storage_can_write(d);
#else
return d&&d->readable&&d->transport==STORAGE_AHCI;
#endif
}
static inline unsigned storage_device_io(unsigned disk,uint64_t lba,void*data,unsigned bytes,unsigned write){
#ifdef ENGINE_M92
return storage_transfer(disk,lba,data,bytes,write);
#else
const storage_disk*d=storage_get(disk);return d&&storage_writable(d)&&d->sector_bytes==512?m5_sata_transfer(d->context,lba,data,bytes,write):B_DENIED;
#endif
}
static inline unsigned storage_device_flush(unsigned disk){
#ifdef ENGINE_M92
return storage_flush(disk);
#else
const storage_disk*d=storage_get(disk);return d?m5_sata_flush(d->context):B_DENIED;
#endif
}
unsigned storage_count(void);
void storage_unavailable(unsigned);
unsigned usb_work_pending(void);
void usb_process_changes(void);
void usb_storage_diagnostics(void);
const storage_disk *storage_get(unsigned);
unsigned storage_read(unsigned,uint64_t,void*);
void storage_inventory(void);
unsigned storage_partitions(unsigned);
unsigned storage_partition(unsigned,unsigned,uint64_t*,uint64_t*);
unsigned ext_read(unsigned,unsigned,const char*,void*,unsigned,unsigned*);
unsigned ext_list(unsigned,unsigned,const char*);
unsigned fat_read(unsigned,unsigned,const char*,void*,unsigned,unsigned*);
unsigned fat_list(unsigned,unsigned);
unsigned storage_selftest(void);
void console_decimal(uint64_t);
unsigned nvme_discover(void);
unsigned nvme_irq(uint64_t*);
void nvme_diagnostics(void);
void virtio_diagnostics(void);
void hardware_inventory(void);
void usb_hardware_inventory(unsigned bdf);
unsigned ide_discover(void);
unsigned virtio_discover(void);
unsigned virtio_storage_irq(uint64_t*);
#endif
