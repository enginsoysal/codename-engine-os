#ifndef ENGINE_NETWORK_TLS_H
#define ENGINE_NETWORK_TLS_H
#include <stdint.h>
enum { TLS7_CLOSED,TLS7_CONNECTING,TLS7_READY,TLS7_PEER_CLOSED,TLS7_FAILED };
unsigned tls7_start(const char *,uint32_t,unsigned,uint32_t,uint32_t);
unsigned tls7_status(void);
unsigned tls7_error(void);
void tls7_pump(void);
unsigned tls7_write(const void *,unsigned);
unsigned tls7_read(void *,unsigned);
void tls7_abort(void);
void tls7_close(void);
#endif
