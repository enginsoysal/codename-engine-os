#ifndef ENGINE_DISK71_H
#define ENGINE_DISK71_H
#include <engine/storage/volume6.h>
enum {D71_OK,D71_IO,D71_INVALID,D71_UNSUPPORTED,D71_CHANGED,D71_PROTECTED,D71_FULL,D71_SMALL};
enum {D71_BLANK,D71_MBR,D71_GPT,D71_DAMAGED,D71_UNKNOWN};
enum {D71_ERASE=1,D71_INITIALIZE,D71_CREATE,D71_DELETE,D71_FAT32,D71_NATIVE,D71_REPAIR_PRIMARY,D71_REPAIR_BACKUP};
typedef unsigned (*d71_io)(void*,uint64_t,void*,unsigned);
typedef struct {void*context;d71_io read,write;v6_flush flush;uint64_t sectors;uint32_t sector_bytes;} d71_device;
typedef struct {uint64_t first,last;unsigned index,protected;char name[37],format[12];} d71_partition;
typedef struct {uint64_t first,last;} d71_gap;
typedef struct {
 unsigned scheme,valid_copies,editable,count,gaps,owned,fingerprint;
 uint64_t sectors,first_usable,last_usable;
 uint8_t mbr[512],headers[2][512],tables[2][16384];
 d71_partition parts[128];d71_gap free[129];
} d71_disk;
typedef struct {unsigned operation,index;uint64_t first,last;uint32_t boot_crc;char name[37];uint8_t nonce[16];} d71_plan;
unsigned d71_scan(const d71_device*,d71_disk*);
unsigned d71_prepare(const d71_device*,const d71_disk*,unsigned,unsigned,uint64_t,const char*,const uint8_t*,d71_plan*);
unsigned d71_apply(const d71_device*,const d71_disk*,const d71_plan*,v6_volume*);
const char*d71_error(unsigned);
#endif
