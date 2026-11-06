#ifndef ENGINE_INSTALL_LAYOUT_H
#define ENGINE_INSTALL_LAYOUT_H
#include <stdint.h>
#define ENGINE_LAYOUT_VERSION 1u
#define ENGINE_LAYOUT_OK 0u
#define ENGINE_LAYOUT_INVALID 1u
#define ENGINE_LAYOUT_UNSUPPORTED 2u
#define ENGINE_LAYOUT_TOO_SMALL 3u
/* Inclusive LBAs; no disk I/O or allocation occurs in the planner. */
typedef struct { uint64_t first, last; } engine_partition;
typedef struct {
    uint32_t version, sector_bytes;
    uint64_t sectors, backup_table, backup_header;
    engine_partition bios, esp, system_a, system_b, data;
    uint64_t reserved_sectors, partition_sectors;
} engine_install_layout;
unsigned engine_install_plan(uint64_t sectors, uint32_t sector_bytes, engine_install_layout *out);
#endif
