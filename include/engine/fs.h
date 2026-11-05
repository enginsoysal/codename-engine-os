#ifndef ENGINE_FS_H
#define ENGINE_FS_H
#include <engine/m4.h>
enum { F_OK,F_IO,F_INVALID,F_NOTFOUND,F_EXISTS,F_NOSPC,F_BUSY,F_RO,F_LIMIT,F_UNSUPPORTED };
#define FS_MAX_FILE 49152u
unsigned fs_mount(void);
unsigned fs_mkdir(const char*);
unsigned fs_write(const char*,const void*,unsigned);
unsigned fs_read(const char*,void*,unsigned,unsigned*);
unsigned fs_rename(const char*,const char*);
unsigned fs_remove(const char*);
unsigned fs_list(const char*);
unsigned fs_open(const char*,unsigned,unsigned,uint32_t*);
unsigned fs_close(uint32_t);
unsigned fs_read_handle(uint32_t,void*,unsigned,unsigned*);
unsigned fs_check(void);
unsigned fs_selftest(uint64_t);
unsigned fs_readonly(void);
#endif
