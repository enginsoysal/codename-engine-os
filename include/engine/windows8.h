#ifndef ENGINE_WINDOWS8_H
#define ENGINE_WINDOWS8_H
#include <stdint.h>
#include <stddef.h>
#define W8_BASE UINT64_C(0x100000000)
#define W8_LIMIT UINT64_C(0x140000000)
#define W8_MAILBOX UINT64_C(0x100800000)
#define W8_STACK_LOW UINT64_C(0x10a000000)
#define W8_STACK_TOP UINT64_C(0x10a800000)
#define W8_TEB UINT64_C(0x100a00000)
#define W8_HEAP UINT64_C(0x120000000)
#define W8_MAGIC 0x38574f53u
#define W8_IO 4096u
#define W8_TEXT 16384u
enum {W8_PREVIEW=1,W8_INSTALL,W8_RUN,W8_REMOVE};
enum {W8_SYS_MAP=0x800,W8_SYS_UNMAP,W8_SYS_PROTECT,W8_SYS_OPEN,W8_SYS_READ,
 W8_SYS_WRITE,W8_SYS_SEEK,W8_SYS_CLOSE,W8_SYS_STAT,W8_SYS_ENUM,W8_SYS_MKDIR,
 W8_SYS_DELETE,W8_SYS_MOVE,W8_SYS_OUTPUT,W8_SYS_EXIT,W8_SYS_TIME,W8_SYS_YIELD,
 W8_SYS_METADATA,W8_SYS_INFO,W8_SYS_TRUNCATE};
typedef struct {uint64_t size,time[3];uint32_t attributes,crc,kind;char name[256];} w8_file_info;
typedef struct {
 uint32_t magic,command,status,percent;
 char source[256],product_root[256],user_root[256],windows_root[256],arguments[1024];
 char product[96],version[32],publisher[96],code[40];
 uint32_t files,registry,shortcuts,features;
 uint64_t total_bytes;
 char message[192];
} w8_mailbox;
#ifndef W8_USER
#include <engine/process.h>
typedef struct {unsigned state,command,status,percent,revision;uint64_t peak_pages,quanta;
 char product[96],version[32],publisher[96],code[40],message[192],output[W8_TEXT];
 unsigned files;uint64_t total_bytes;} w8_info;
extern volatile unsigned w8_active;
unsigned w8_start(unsigned,const char*,const char*,const char*,const char*);
unsigned w8_pending(void);
void w8_pump(void);
void w8_cancel(void);
const w8_info *w8_current(void);
trap_frame *w8_trap(trap_frame*);
unsigned w8_install_runtime(void*);
#endif
#endif
