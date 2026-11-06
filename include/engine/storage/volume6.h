#ifndef ENGINE_VOLUME6_H
#define ENGINE_VOLUME6_H
#include <stdint.h>
#define V6_ENTRIES 1024u
#define V6_PAGE 4096u
#define V6_DATA_START 160u
#define V6_OK 0u
#define V6_IO 1u
#define V6_INVALID 2u
#define V6_NOTFOUND 3u
#define V6_EXISTS 4u
#define V6_FULL 5u
#define V6_RO 7u
#define V6_LIMIT 8u
typedef struct {char path[256];uint64_t first,size;uint32_t crc,kind;} v6_entry;
typedef unsigned (*v6_transfer)(void*,uint64_t,void*,unsigned);
typedef unsigned (*v6_flush)(void*);
typedef unsigned (*v6_source)(void*,uint64_t,void*,unsigned);
typedef struct {
 void *context;v6_transfer read,write;v6_flush flush;uint64_t blocks,generation;
 unsigned active,mounted,readonly,previous_valid;
 v6_entry entries[V6_ENTRIES],previous[V6_ENTRIES],pending[V6_ENTRIES];
 uint8_t page[V6_PAGE];
} v6_volume;
unsigned v6_format(v6_volume*);
unsigned v6_mount(v6_volume*);
unsigned v6_mkdir(v6_volume*,const char*);
unsigned v6_put(v6_volume*,const char*,uint64_t,v6_source,void*);
unsigned v6_read(v6_volume*,const char*,uint64_t,void*,unsigned,unsigned*);
unsigned v6_remove(v6_volume*,const char*);
unsigned v6_verify(v6_volume*,const char*);
uint64_t v6_capacity(const v6_volume*);
uint64_t v6_used(const v6_volume*);
#endif
