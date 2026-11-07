#ifndef ENGINE_INSTALL_DESKTOP74_H
#define ENGINE_INSTALL_DESKTOP74_H
#include <engine/install/disk71.h>

/* One trusted disk session, independent of the mounted volume's I/O context. */
unsigned disk74_select(unsigned);
const d71_disk *disk74_state(void);
unsigned disk74_prepare(unsigned,unsigned,uint64_t,const char *);
const d71_plan *disk74_plan(void);
unsigned disk74_apply(void);
unsigned system_disk74(void);
unsigned recovery74_prepare(unsigned);
unsigned recovery74_apply(void);
extern unsigned quiet_progress74;
#endif
