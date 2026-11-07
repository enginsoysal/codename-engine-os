#ifndef ENGINE_NETWORK_NETWORK_H
#define ENGINE_NETWORK_NETWORK_H
#include <engine/network/nic.h>
#include <engine/network/wire.h>
enum { NET7_NO_INTERFACE,NET7_DISCONNECTED,NET7_CONFIGURING,NET7_CONNECTED,NET7_FAILED };
enum { NET7_DNS_IDLE,NET7_DNS_PENDING,NET7_DNS_READY,NET7_DNS_FAILED };
typedef struct {
    unsigned state,revision;
    uint32_t address,mask,router,dns;
    const char *message;
} net7_configuration;
void net7_init(void);
void net7_retry(void);
unsigned net7_clock_event(void);
unsigned net7_work_pending(void);
void net7_pump(void);
net7_configuration net7_current(void);
unsigned net7_random(void *,unsigned);
unsigned net7_resolve(const char *);
unsigned net7_dns_status(uint32_t *);
#endif
