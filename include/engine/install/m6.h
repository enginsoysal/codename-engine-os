#ifndef ENGINE_INSTALL_M6_H
#define ENGINE_INSTALL_M6_H
#include <engine/m5.h>
#include <engine/install/layout.h>
#include <engine/storage/volume6.h>
typedef struct {char user[32];uint32_t accent,guide;} m6_preferences;
#ifdef ENGINE_M7
unsigned m7_calculator_selection(void);
void m7_terminal_command(const char *,unsigned);
void console7_capture(void (*)(char,void *),void *);
#endif
typedef struct {const char*path;const uint8_t*data;unsigned size;} m6_distribution_file;
extern const m6_distribution_file m6_files[];
extern const unsigned m6_file_count;
extern const uint8_t m6_bundle[];
extern const unsigned m6_bundle_size,m6_bundle_crc;
/* Removable file transfer is qualified separately from OS installation. */
static inline unsigned m6_target_writable(const storage_disk*d){
#ifdef ENGINE_M10
 if(d&&d->transport==STORAGE_USB)return 0;
#endif
 return storage_writable(d);
}
unsigned m6_live(void);
unsigned m6_prepare(unsigned);
#ifdef ENGINE_M71
unsigned m71_prepare_replace(unsigned);
v6_volume *m71_workspace(void);
#endif
unsigned m6_install(const m6_preferences*);
unsigned m6_open(void);
unsigned m6_maintenance_prepare(unsigned,unsigned);
unsigned m6_maintenance(void);
v6_volume *m6_volume(void);
unsigned m6_settings(m6_preferences*);
unsigned m6_save_settings(const m6_preferences*);
void m6_cancel(void);
const engine_install_layout *m6_plan(void);
void m6_progress(unsigned,const char*);
NORETURN void m6_start(const os_boot_info_v0*);
void gui6_rect(unsigned,unsigned,unsigned,unsigned,uint32_t);
void gui6_text(unsigned,unsigned,const char*,unsigned,uint32_t);
void m6_pointer_report(int,int,unsigned);
void usb_mouse_limits(unsigned,unsigned);
void usb_mouse_state(unsigned*,unsigned*,unsigned*);
#endif
