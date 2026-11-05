#ifndef OS_M0_VALIDATE_V0_H
#define OS_M0_VALIDATE_V0_H
#include <engine/boot/boot_v0.h>
#include <engine/abi/syscall_v0.h>
/* Byte readers allow unaligned input; no physical/user pointer is dereferenced here. */
static inline uint32_t os_read32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline uint64_t os_read64(const uint8_t *p) {
    return (uint64_t)os_read32(p) | ((uint64_t)os_read32(p + 4) << 32);
}
enum { OS_BOOT_VALID = 0, OS_BOOT_SHORT = 1, OS_BOOT_HEADER = 2,
       OS_BOOT_CRC = 3, OS_BOOT_RANGE = 4, OS_BOOT_OVERLAP = 5,
       OS_BOOT_DESCRIPTORS = 6, OS_BOOT_FRAMEBUFFER = 7 };
static inline uint32_t os_boot_crc(const uint8_t *bytes) {
    uint32_t crc = UINT32_MAX;
    for (uint32_t i = 0; i < 160; ++i) {
        crc ^= (i >= 24 && i < 28) ? 0 : bytes[i];
        for (uint32_t bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((crc & 1) ? UINT32_C(0xedb88320) : 0);
    }
    return ~crc;
}
static inline int os_boot_validate(const uint8_t *bytes, uint64_t available) {
    if (!bytes || available < 160) return OS_BOOT_SHORT;
    if (os_read64(bytes) != OS_BOOT_MAGIC || os_read32(bytes + 8) != 0 ||
        os_read32(bytes + 12) != 160 || os_read64(bytes + 16) ||
        os_read32(bytes + 28) || os_read64(bytes + 152)) return OS_BOOT_HEADER;
    if (os_read32(bytes + 24) != os_boot_crc(bytes)) return OS_BOOT_CRC;
    uint64_t start[6], size[6];
    for (uint32_t i = 0; i < 6; ++i) {
        start[i] = os_read64(bytes + 32 + i * 16);
        size[i] = os_read64(bytes + 40 + i * 16);
        if ((!start[i]) != (!size[i]) || size[i] > UINT64_MAX - start[i]) return OS_BOOT_RANGE;
        if ((i < 3 || i == 5) && !size[i]) return OS_BOOT_RANGE;
    }
    for (uint32_t i = 0; i < 6; ++i)
        for (uint32_t j = i + 1; j < 6; ++j)
            if (size[i] && size[j] && start[i] < start[j] + size[j] && start[j] < start[i] + size[i])
                return OS_BOOT_OVERLAP;
    uint32_t stride = os_read32(bytes + 128);
    if (stride < 40 || stride % 8 || size[0] % stride || os_read32(bytes + 132) != 1 || size[5] % 16)
        return OS_BOOT_DESCRIPTORS;
    uint32_t row = os_read32(bytes + 136), format = os_read32(bytes + 140);
    uint32_t width = os_read32(bytes + 144), height = os_read32(bytes + 148);
    if (!size[3]) {
        if (row || format || width || height) return OS_BOOT_FRAMEBUFFER;
    } else if ((format != 1 && format != 2) || !width || !height ||
               (uint64_t)row < (uint64_t)width * 4 || (uint64_t)row * height > size[3])
        return OS_BOOT_FRAMEBUFFER;
    return OS_BOOT_VALID;
}
/* Only validates wire shape and arithmetic. Caller must copy/pin/recheck mapped user pages. */
static inline os_status os_user_buffer_validate(const uint8_t *bytes, uint64_t available, uint64_t user_limit) {
    if (!bytes || available < 24 || os_read32(bytes) != 24 || os_read32(bytes + 4) != 0)
        return OS_INVALID_ARGUMENT;
    uint64_t address = os_read64(bytes + 8), length = os_read64(bytes + 16);
    if (!length) return OS_OK;
    if (!address || address >= user_limit || length > user_limit - address) return OS_INVALID_ARGUMENT;
    return OS_OK;
}
#endif
