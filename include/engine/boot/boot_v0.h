#ifndef OS_M0_BOOT_V0_H
#define OS_M0_BOOT_V0_H
/* Versioned boot contract shared by loader, kernel and host tests. Little endian, 8-byte aligned. */
#include <stdint.h>
#include <stddef.h>
#define OS_BOOT_MAGIC UINT64_C(0x304F464E49534F42)
#define OS_BOOT_VERSION 0u
typedef struct { uint64_t physical_address, byte_length; } os_boot_range;
typedef struct {
    uint64_t magic;
    uint32_t version, structure_size;
    uint64_t flags;
    uint32_t crc32, reserved0;
    os_boot_range memory_map, kernel, initimage, framebuffer, acpi, reservations;
    uint32_t memory_descriptor_size, memory_descriptor_version;
    uint32_t framebuffer_stride_bytes, framebuffer_format;
    uint32_t framebuffer_width, framebuffer_height;
    uint64_t reserved1;
} os_boot_info_v0;
_Static_assert(sizeof(os_boot_range) == 16, "boot range layout");
_Static_assert(sizeof(os_boot_info_v0) == 160, "boot v0 layout");
_Static_assert(offsetof(os_boot_info_v0, memory_map) == 32, "memory map offset");
_Static_assert(offsetof(os_boot_info_v0, memory_descriptor_size) == 128, "descriptor offset");
#endif
