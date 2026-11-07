#ifndef ENGINE_DESKTOP_WINDOW_H
#define ENGINE_DESKTOP_WINDOW_H
#include <stdint.h>

#define DESKTOP_WINDOWS 8u
#define DESKTOP_MIN_WIDTH 320
#define DESKTOP_MIN_HEIGHT 200

typedef struct { int32_t x, y, width, height; } desktop_rect;
typedef struct {
    desktop_rect bounds;
    uint32_t generation, app;
    uint8_t live, minimized;
} desktop_window;
typedef struct {
    desktop_window windows[DESKTOP_WINDOWS];
    uint8_t order[DESKTOP_WINDOWS];
    uint32_t count, focused, width, height;
    desktop_rect damage;
    uint8_t damaged;
    uint64_t changes;
} desktop_model;

/* Tokens combine slot and generation. Closing/reusing a slot invalidates old tokens. */
void desktop_init(desktop_model *, uint32_t, uint32_t);
uint64_t desktop_open(desktop_model *, uint32_t, desktop_rect);
int desktop_close(desktop_model *, uint64_t);
int desktop_focus(desktop_model *, uint64_t);
int desktop_place(desktop_model *, uint64_t, desktop_rect);
int desktop_minimize(desktop_model *, uint64_t, int);
uint64_t desktop_hit(const desktop_model *, int32_t, int32_t);
desktop_window *desktop_lookup(desktop_model *, uint64_t);
void desktop_damage(desktop_model *, desktop_rect);
int desktop_take_damage(desktop_model *, desktop_rect *);
uint64_t desktop_token(const desktop_model *, unsigned);
#endif
