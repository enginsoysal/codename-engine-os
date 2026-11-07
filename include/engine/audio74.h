#ifndef ENGINE_AUDIO74_H
#define ENGINE_AUDIO74_H
#include <engine/base.h>
typedef struct {
    unsigned state,revision,rate,channels,volume;
    uint64_t frames,total_frames,owned_bytes,interrupts;
    const char *message;
} audio74_info;
enum {AUDIO74_STOPPED,AUDIO74_PLAYING,AUDIO74_PAUSED,AUDIO74_FAILED};
unsigned audio74_play(const char *);
unsigned audio74_inspect(const char *);
void audio74_stop(void);
void audio74_pause(void);
void audio74_volume(unsigned);
unsigned audio74_irq(uint64_t *);
unsigned audio74_pending(void);
void audio74_pump(void);
audio74_info audio74_current(void);
#endif
