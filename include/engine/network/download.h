#ifndef ENGINE_NETWORK_DOWNLOAD_H
#define ENGINE_NETWORK_DOWNLOAD_H
#include <engine/storage/volume6.h>
enum { DOWNLOAD7_IDLE,DOWNLOAD7_RESOLVING,DOWNLOAD7_CONNECTING,DOWNLOAD7_RECEIVING,DOWNLOAD7_DONE,DOWNLOAD7_FAILED,DOWNLOAD7_CANCELLED };
typedef struct {unsigned state,revision,tls_error;uint64_t bytes;const char *message;char path[256];} download7_info;
unsigned download7_start(v6_volume *,const char *,const char *);
void download7_cancel(void);
void download7_pump(void);
download7_info download7_current(void);
#ifdef ENGINE_M74
unsigned download74_start(v6_volume *,const char *,const char *,uint64_t);
unsigned download74_url(char *,unsigned);
const char *download74_mime(void);
#endif
#endif
