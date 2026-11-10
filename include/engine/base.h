#ifndef ENGINE_BASE_H
#define ENGINE_BASE_H
#include <stdint.h>
#include <stddef.h>
#include <engine/product.h>
#define NORETURN __attribute__((noreturn))
static inline void out8(uint16_t port, uint8_t value) { __asm__ volatile("outb %0,%1"::"a"(value),"Nd"(port)); }
static inline uint8_t in8(uint16_t port) { uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(port)); return v; }
static inline void serial_init(void) {
    out8(0x3f9,0); out8(0x3fb,0x80); out8(0x3f8,1); out8(0x3f9,0);
    out8(0x3fb,3); out8(0x3fa,0xc7); out8(0x3fc,0x0b);
}
static inline void serial_char(char c) {
    if(c=='\n') { for(unsigned i=0;i<100000 && !(in8(0x3fd)&32);++i){} out8(0x3f8,'\r'); }
    for(unsigned i=0;i<100000 && !(in8(0x3fd)&32);++i){} out8(0x3f8,(uint8_t)c);
}
static inline void serial_text(const char *s) { while(*s) serial_char(*s++); }
static inline void serial_hex(uint64_t value) {
    serial_text("0x"); for(int i=60;i>=0;i-=4) serial_char("0123456789abcdef"[(value>>i)&15]);
}
static inline NORETURN void halt_forever(void) { for(;;) __asm__ volatile("cli; hlt"); }
static inline NORETURN void test_exit(uint32_t value) {
    __asm__ volatile("outl %0,%1"::"a"(value),"Nd"((uint16_t)0xf4)); halt_forever();
}
void *memcpy(void *, const void *, size_t);
void *memset(void *, int, size_t);
int memcmp(const void *, const void *, size_t);
#endif
