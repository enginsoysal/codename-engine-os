#ifndef ENGINE_DESKTOP_APP_H
#define ENGINE_DESKTOP_APP_H
#include <stdint.h>
#define APP7_TEXT 8192u
#define APP7_MAILBOX UINT64_C(0x100080000)
#define APP7_MAGIC 0x37415050u
#define APP7_DONE 0x700u
enum { APP7_LOAD=1,APP7_KEY,APP7_INSERT,APP7_CLEAR,APP7_SAVED,
       APP7_CRASH=100,APP7_SPIN,APP7_KERNEL_WRITE,APP7_CODE_WRITE,APP7_BAD_RESULT };
typedef struct {
    char text[APP7_TEXT];
    uint32_t length,cursor,dirty,calc_error;
    int64_t number,operand;
    uint32_t operation,entered;
} app7_view;
typedef struct {
    uint32_t magic,command,key,bytes;
    char input[APP7_TEXT];
    app7_view view;
} app7_mailbox;
_Static_assert(sizeof(app7_mailbox)<5*4096,"bounded application mailbox");
#ifndef APP7_USER
#include <engine/process.h>
#include <engine/storage/volume6.h>
extern volatile unsigned app7_active;
unsigned app7_prepare(void);
unsigned app7_install(v6_volume *);
unsigned app7_package(unsigned,unsigned);
unsigned app7_available(unsigned);
unsigned app7_open(unsigned,unsigned);
unsigned app7_request(unsigned,unsigned,unsigned,const void *,unsigned,app7_view *);
void app7_close(unsigned);
trap_frame *app7_trap(trap_frame *);
#endif
#endif
