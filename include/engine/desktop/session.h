#ifndef ENGINE_DESKTOP_SESSION_H
#define ENGINE_DESKTOP_SESSION_H
#include <engine/install/m6.h>
#include <engine/desktop/window.h>
NORETURN void desktop_start(const os_boot_info_v0 *, const m6_preferences *);
void gui7_clip(unsigned, unsigned, unsigned, unsigned);
void gui7_clip_reset(void);
#endif
