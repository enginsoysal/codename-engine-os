#ifndef ENGINE_STORAGE_H
#define ENGINE_STORAGE_H
#include <engine/m4.h>
#define STORAGE_MAX 16u
enum { STORAGE_AHCI=1,STORAGE_NVME,STORAGE_VIRTIO,STORAGE_USB,STORAGE_IDE };
typedef struct {unsigned transport,bdf,unit,sector_bytes,readable,marked;uint64_t sectors;unsigned context,status;} storage_disk;
typedef unsigned (*storage_reader)(unsigned,uint64_t,void*);
unsigned storage_register(storage_disk,storage_reader);
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
