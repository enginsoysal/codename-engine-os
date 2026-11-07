#ifndef ENGINE_NETWORK_WIRE_H
#define ENGINE_NETWORK_WIRE_H
#include <stdint.h>
#include <stddef.h>
enum { NET7_OK=0,NET7_INVALID=1,NET7_NOT_FOUND=2,NET7_SERVER_ERROR=3 };
typedef struct {
    uint32_t source,destination;
    const uint8_t *data;
    unsigned bytes,protocol;
} net7_ipv4;
typedef struct { uint16_t source,destination;const uint8_t *data;unsigned bytes; } net7_udp;
typedef struct {
    uint16_t source,destination,window;
    uint32_t sequence,acknowledgement;
    const uint8_t *data;
    unsigned bytes,flags;
} net7_tcp;
typedef struct { uint32_t address,mask,router,dns,server,seconds,type; } net7_lease;
typedef struct { char host[254],path[2048];uint16_t port; } net7_url;
static inline uint16_t net7_get16(const void *p){const uint8_t *b=p;return (uint16_t)((unsigned)b[0]<<8|b[1]);}
static inline uint32_t net7_get32(const void *p){const uint8_t *b=p;return (uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3];}
static inline void net7_put16(void *p,uint16_t v){uint8_t *b=p;b[0]=(uint8_t)(v>>8);b[1]=(uint8_t)v;}
static inline void net7_put32(void *p,uint32_t v){uint8_t *b=p;b[0]=(uint8_t)(v>>24);b[1]=(uint8_t)(v>>16);b[2]=(uint8_t)(v>>8);b[3]=(uint8_t)v;}
uint16_t net7_checksum(const void *,unsigned,uint32_t);
uint32_t net7_pseudo(uint32_t,uint32_t,unsigned,unsigned);
unsigned net7_read_ipv4(const void *,unsigned,const uint8_t[6],net7_ipv4 *);
unsigned net7_read_udp(const net7_ipv4 *,net7_udp *);
unsigned net7_read_tcp(const net7_ipv4 *,net7_tcp *);
unsigned net7_read_dhcp(const void *,unsigned,uint32_t,const uint8_t[6],net7_lease *);
unsigned net7_dns_answer(const void *,unsigned,uint16_t,const char *,uint32_t *);
unsigned net7_parse_url(const char *,net7_url *);
#endif
