#ifndef ENGINE_M4_H
#define ENGINE_M4_H
#include <engine/m3.h>
#define M4_SECTORS 32768u
#define M4_HEADER "ENGINE-M4-DATA01"
#define M4_RECORD "ENGM4REC"
enum { B_OK,B_ABSENT,B_DENIED,B_IO,B_TIMEOUT,B_UNSUPPORTED };
typedef struct {unsigned pci,port,online,validated;uint64_t sectors,commands,reads,writes,flushes,errors,dma_pages;} block_info;
unsigned block_init(void);
unsigned block_read(uint64_t,void*);
unsigned block_write_record(const void*);
unsigned block_flush(void);
block_info block_stats(void);
uint32_t m4_crc(const void*,unsigned);
void console_begin(const os_boot_info_v0*);
void console_text(const char*);
void console_hex(uint64_t);
void input_init(void);
unsigned input_irq(uint64_t*);
int input_pop(void);
uint64_t input_dropped(void);
NORETURN void m4_start(const os_boot_info_v0*,unsigned);
#endif
