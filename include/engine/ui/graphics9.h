#ifndef ENGINE_UI_GRAPHICS9_H
#define ENGINE_UI_GRAPHICS9_H
#include <stdint.h>
typedef struct {
 uint64_t shadow_bytes,presented_pixels,presents,copy_pixels,copy_frames,fifo_updates,fifo_copies,fifo_failures;
 uint32_t adapter_bdf,adapter_id,backend,capabilities;
} graphics9_info;
const graphics9_info *graphics9_current(void);
void graphics9_present(unsigned,unsigned,unsigned,unsigned);
void graphics9_hide_cursor(void);
void graphics9_flush(void);
void graphics9_begin_batch(void);
void graphics9_end_batch(void);
unsigned graphics9_move(unsigned,unsigned,unsigned,unsigned,unsigned,unsigned);
#endif
