#ifndef ENGINE_DESKTOP_APP_H
#define ENGINE_DESKTOP_APP_H
#include <stdint.h>
#define APP7_TEXT 8192u
#define APP7_MAILBOX UINT64_C(0x100080000)
#define APP7_MAGIC 0x37415050u
#define APP7_DONE 0x700u
enum { APP7_LOAD=1,APP7_KEY,APP7_INSERT,APP7_CLEAR,APP7_SAVED,
       APP7_CRASH=100,APP7_SPIN,APP7_KERNEL_WRITE,APP7_CODE_WRITE,APP7_BAD_RESULT };
#ifdef ENGINE_M74
enum {APP74_INFLATE_BEGIN=20,APP74_INFLATE_STEP,APP74_INFLATE_END};
enum {APP74_HTML_BEGIN=30,APP74_HTML_FEED,APP74_HTML_LINK,APP74_HTML_BACK,APP74_HTML_BASE,APP74_HTML_LINKS};
#define APP74_HTML_TEXT 7679u
#define APP74_HTML_LINK_OFFSET 7680u
#define APP74_STATE UINT64_C(0x100090000)
#define APP74_STATE_PAGES 12u
#endif
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
unsigned app7_install(v6_volume *,unsigned);
unsigned app7_package(unsigned,unsigned);
unsigned app7_available(unsigned);
unsigned app7_open(unsigned,unsigned);
unsigned app7_request(unsigned,unsigned,unsigned,const void *,unsigned,app7_view *);
void app7_close(unsigned);
#ifdef ENGINE_M74
unsigned desktop74_in_use(unsigned);
void desktop74_removed(unsigned);
#endif
trap_frame *app7_trap(trap_frame *);
#endif
#endif
