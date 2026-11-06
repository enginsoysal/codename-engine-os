#ifndef ENGINE_M5_H
#define ENGINE_M5_H
#include <engine/storage.h>
#define M5_ESP 8192u
#define M5_DATA 139264u
#define M5_END 172032u
#define M5_MIN_SECTORS 524288u
extern const uint8_t m5_template[],m5_install[];
extern const unsigned m5_template_size,m5_install_size;
unsigned m5_sata_transfer(unsigned,uint64_t,void*,unsigned,unsigned);
unsigned m5_sata_flush(unsigned);
unsigned m5_volume_start(void);
unsigned m5_command(const char*);
void m5_welcome(void);
unsigned m5_read(uint64_t,void*);
unsigned m5_read_page(uint64_t,void*);
unsigned m5_write(uint64_t,const void*);
unsigned m5_write_page(uint64_t,const void*);
unsigned m5_record(const void*);
unsigned m5_flush(void);
unsigned m5_enable(void);
#endif
