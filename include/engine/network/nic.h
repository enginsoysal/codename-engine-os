#ifndef ENGINE_NETWORK_NIC_H
#define ENGINE_NETWORK_NIC_H
#include <engine/base.h>
typedef struct {
    uint8_t mac[6];
    unsigned present,online,bdf,interrupt_vector,status;
    uint64_t rx_packets,tx_packets,dropped,interrupts,dma_pages;
} net7_nic_info;
unsigned net7_nic_init(void);
unsigned net7_nic_irq(uint64_t *);
unsigned net7_nic_pending(void);
unsigned net7_nic_poll(void (*)(const void *,unsigned));
unsigned net7_nic_send(const void *,unsigned);
net7_nic_info net7_nic_status(void);
#endif
