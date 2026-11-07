#ifndef ENGINE_NETWORK_TCP_H
#define ENGINE_NETWORK_TCP_H
#include <engine/network/wire.h>
enum { TCP7_CLOSED,TCP7_CONNECTING,TCP7_OPEN,TCP7_PEER_CLOSED,TCP7_CLOSING,TCP7_FAILED };
unsigned tcp7_connect(uint32_t,unsigned);
unsigned tcp7_status(void);
unsigned tcp7_write(const void *,unsigned);
unsigned tcp7_read(void *,unsigned);
void tcp7_close(void);
void tcp7_abort(void);
void tcp7_receive(const net7_ipv4 *);
void tcp7_pump(void);
unsigned tcp7_active(void);
#endif
