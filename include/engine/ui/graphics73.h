#ifndef ENGINE_UI_GRAPHICS73_H
#define ENGINE_UI_GRAPHICS73_H
#include <engine/boot/boot_v0.h>
const os_boot_info_v0 *ui73_begin(const os_boot_info_v0 *);
unsigned ui73_scale(void);
unsigned ui73_scale_choice(void);
const os_boot_info_v0 *ui73_set_scale(unsigned);
const char *ui73_font_license(void);
unsigned ui73_width(void);
unsigned ui73_height(void);
uint64_t ui73_framebuffer_bytes(void);
unsigned ui73_measure(const char *,unsigned);
void ui73_round(unsigned,unsigned,unsigned,unsigned,unsigned,uint32_t);
void ui73_shadow(unsigned,unsigned,unsigned,unsigned);
void ui73_fit(unsigned,unsigned,unsigned,const char *,unsigned,uint32_t);
unsigned ui73_paragraph(unsigned,unsigned,unsigned,unsigned,const char *,uint32_t);
void ui73_folder(unsigned,unsigned,unsigned,uint32_t);
#endif
