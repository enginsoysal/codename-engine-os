#ifndef ENGINE_HARDWARE10_H
#define ENGINE_HARDWARE10_H
#include <engine/storage.h>
enum {HW10_DISCOVERED,HW10_READY,HW10_LIMITED,HW10_UNAVAILABLE};
enum {HW10_READ=1,HW10_WRITE=2,HW10_FLUSH=4,HW10_INPUT=8,HW10_DISPLAY=16,HW10_NETWORK=32};
typedef struct {unsigned bdf,vendor,product,class_code,state,capabilities;const char *name,*detail;} hw10_device;
typedef struct {unsigned active,bdf,status,keyboards,mice,disks,hubs;uint64_t pages,keys,motion;} usb10_info;
void hw10_refresh(void);
unsigned hw10_count(void);
const hw10_device *hw10_get(unsigned);
usb10_info usb10_current(void);
unsigned usb10_eject(unsigned,unsigned);
unsigned input10_layout(void);
void input10_toggle_caps(void);
void input10_set_layout(unsigned);
unsigned input10_hid(unsigned,unsigned);
void input10_wheel(int);
int input10_take_wheel(void);
typedef struct {char name[256];unsigned kind;uint32_t size;} rem10_entry;
unsigned rem10_volume(unsigned,unsigned*,unsigned*);
unsigned rem10_list(unsigned,unsigned,unsigned,const char*,unsigned,rem10_entry*,unsigned,unsigned*);
unsigned rem10_export(unsigned,unsigned,unsigned,const char*,const char*);
unsigned rem10_import(unsigned,unsigned,unsigned,const char*,const char*);
#endif
