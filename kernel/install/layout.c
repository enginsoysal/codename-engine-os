#include <engine/install/layout.h>
#define ALIGN 2048u
#define MIB_SECTORS 2048u
#define MAX_SECTORS (UINT64_C(2)*1024*1024*1024*1024/512)
static uint64_t length(engine_partition p) { return p.last-p.first+1; }
unsigned engine_install_plan(uint64_t sectors, uint32_t sector_bytes, engine_install_layout *out) {
    if (!out) return ENGINE_LAYOUT_INVALID;
    /* The installed layout uses 512-byte logical sectors independently of its controller. */
    if (sector_bytes!=512 || sectors>MAX_SECTORS) return ENGINE_LAYOUT_UNSUPPORTED;
    if (sectors<UINT64_C(2)*1024*MIB_SECTORS) return ENGINE_LAYOUT_TOO_SMALL;
    engine_install_layout p={0};
    p.version=ENGINE_LAYOUT_VERSION; p.sector_bytes=sector_bytes; p.sectors=sectors;
    p.backup_header=sectors-1; p.backup_table=sectors-33;
    p.bios.first=ALIGN; p.bios.last=p.bios.first+4*MIB_SECTORS-1;
    p.esp.first=p.bios.last+1; p.esp.last=p.esp.first+128*MIB_SECTORS-1;
    p.system_a.first=p.esp.last+1; p.system_a.last=p.system_a.first+512*MIB_SECTORS-1;
    p.system_b.first=p.system_a.last+1; p.system_b.last=p.system_b.first+512*MIB_SECTORS-1;
    p.data.first=p.system_b.last+1;
    /* End at a MiB boundary before the backup GPT, including odd disk sizes. */
    p.data.last=((sectors-33)/ALIGN)*ALIGN-1;
    p.partition_sectors=length(p.bios)+length(p.esp)+length(p.system_a)+length(p.system_b)+length(p.data);
    p.reserved_sectors=sectors-p.partition_sectors;
    *out=p; return ENGINE_LAYOUT_OK;
}
