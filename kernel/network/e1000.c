#include <engine/network/nic.h>
#include <engine/m3.h>

#define RX_COUNT 16u
#define TX_COUNT 8u
#define NIC_BRANCH 24u
#define NIC_VECTOR 149u
typedef struct {uint64_t address;uint16_t length,checksum;uint8_t status,errors;uint16_t special;} rx_descriptor;
typedef struct {uint64_t address;uint16_t length;uint8_t checksum_offset,command,status,checksum_start;uint16_t special;} tx_descriptor;
_Static_assert(sizeof(rx_descriptor)==16&&sizeof(tx_descriptor)==16,"e1000 legacy DMA descriptor layout");
static volatile uint32_t *registers;
static volatile rx_descriptor *rx;
static volatile tx_descriptor *tx;
static uint64_t owned[16],rx_buffers[RX_COUNT],tx_buffers[TX_COUNT];
static unsigned owned_count,rx_index,tx_index,legacy_irq,ready;
static unsigned pending;
static net7_nic_info info;

static uint32_t pci(unsigned bdf,unsigned reg) {
    uint32_t address=0x80000000u|(bdf<<8)|(reg&252),value;
    __asm__ volatile("outl %0,%1"::"a"(address),"Nd"((uint16_t)0xcf8));
    __asm__ volatile("inl %1,%0":"=a"(value):"Nd"((uint16_t)0xcfc));return value;
}
static void pci_set(unsigned bdf,unsigned reg,uint32_t value) {
    uint32_t address=0x80000000u|(bdf<<8)|(reg&252);
    __asm__ volatile("outl %0,%1"::"a"(address),"Nd"((uint16_t)0xcf8));
    __asm__ volatile("outl %0,%1"::"a"(value),"Nd"((uint16_t)0xcfc));
}
static uint32_t read_register(unsigned reg) {return registers[reg/4];}
static void write_register(unsigned reg,uint32_t value) {registers[reg/4]=value;(void)read_register(8);}
static uint64_t allocate(void) {
    if(owned_count==16)return 0;
    uint64_t page=page_alloc();if(page){owned[owned_count++]=page;info.dma_pages=owned_count;}return page;
}
static void quarantine(unsigned status) {
    if(registers){write_register(0xd8,UINT32_MAX);write_register(0x100,0);write_register(0x400,0);}
    if(info.present)pci_set(info.bdf,4,((pci(info.bdf,4)&65535)|0x400)&~4u);
    ready=0;info.online=0;info.status=status;__atomic_store_n(&pending,1,__ATOMIC_RELEASE);
    /* Keep already-visible DMA pages reserved after a hardware error. */
    serial_text("M7_NETWORK_QUARANTINED status=");serial_hex(status);serial_text("\n");
}
static unsigned map_registers(unsigned bdf) {
    uint32_t low=pci(bdf,16);if(low&1)return 1;
    uint64_t physical=low&~15u;
    if((low&6)==4)physical|=(uint64_t)pci(bdf,20)<<32;
    if(physical<0x10000000||physical>UINT64_C(0x000ffffffff00000))return 1;
    uint64_t page=allocate();if(!page)return 1;
    uint64_t base=physical&~UINT64_C(0x1fffff);
    ((uint64_t *)(uintptr_t)page)[0]=base|UINT64_C(0x800000000000009b);
    ((uint64_t *)(uintptr_t)page)[1]=(base+0x200000)|UINT64_C(0x800000000000009b);
    ((uint64_t *)(uintptr_t)kernel_stack_branch())[NIC_BRANCH]=page|3;
    uint64_t cr3;__asm__ volatile("movq %%cr3,%0;movq %0,%%cr3":"=r"(cr3)::"memory");
    registers=(volatile uint32_t *)(uintptr_t)(UINT64_C(0xffff800000000000)+(uint64_t)NIC_BRANCH*0x40000000+(physical&0x1fffff));
    return 0;
}
static unsigned msi(unsigned bdf) {
    if(!(pci(bdf,4)&(1u<<20)))return 0;
    unsigned cap=pci(bdf,0x34)&255;uint64_t visited=0;
    for(unsigned n=0;n<48&&cap>=0x40&&cap<=0xfc&&!(cap&3);++n){
        uint64_t bit=UINT64_C(1)<<(cap/4);if(visited&bit)break;visited|=bit;
        uint32_t header=pci(bdf,cap);unsigned control=header>>16;
        if((header&255)==5){
            unsigned is64=(control>>7)&1,data_offset=is64?12:8;
            if(cap+data_offset>0xfc)return 0;
            unsigned destination=*(volatile uint32_t *)(uintptr_t)0xfee00020>>24;
            pci_set(bdf,cap+4,0xfee00000u|(destination<<12));
            if(is64)pci_set(bdf,cap+8,0);
            pci_set(bdf,cap+data_offset,(pci(bdf,cap+data_offset)&0xffff0000u)|NIC_VECTOR);
            if(control&0x100){if(cap+data_offset+4>0xfc)return 0;pci_set(bdf,cap+data_offset+4,0);}
            pci_set(bdf,cap,(header&65535)|(((control&~0x70u)|1)<<16));
            return 1;
        }
        cap=(header>>8)&255;
    }
    return 0;
}
unsigned net7_nic_init(void) {
    if(info.present)return ready?0:info.status;
    unsigned found=UINT32_MAX;
    for(unsigned bdf=0;bdf<65536;++bdf){
        uint32_t id=pci(bdf,0);if((id&65535)!=0x8086)continue;
        if((id>>16)!=0x100e&&(id>>16)!=0x100f&&(id>>16)!=0x10d3)continue;
        if((pci(bdf,8)>>16)!=0x0200)continue;
        found=bdf;break;
    }
    if(found==UINT32_MAX){info.status=2;return 2;}
    info.present=1;info.bdf=found;
    pci_set(found,4,((pci(found,4)&65535)|2|0x400)&~4u);
    if(map_registers(found)){info.status=3;return 3;}
    uint32_t mac_low=read_register(0x5400),mac_high=read_register(0x5404);
    write_register(0xd8,UINT32_MAX);write_register(0x100,0);write_register(0x400,0);
    write_register(0,read_register(0)|(1u<<26));
    uint64_t end=m3_clock_ticks()+100;unsigned spins=0;
    while(read_register(0)&(1u<<26))if(m3_clock_ticks()>=end||++spins==10000000){quarantine(4);return 4;}
    write_register(0xd8,UINT32_MAX);(void)read_register(0xc0);
    if(!(mac_high&0x80000000u)){mac_low=read_register(0x5400);mac_high=read_register(0x5404);}
    for(unsigned i=0;i<4;++i)info.mac[i]=(uint8_t)(mac_low>>(i*8));
    info.mac[4]=(uint8_t)mac_high;info.mac[5]=(uint8_t)(mac_high>>8);
    unsigned any=0;for(unsigned i=0;i<6;++i)any|=info.mac[i];
    if(!any||(info.mac[0]&1)){quarantine(5);return 5;}
    write_register(0x5400,mac_low);write_register(0x5404,(mac_high&65535)|0x80000000u);
    for(unsigned i=1;i<16;++i)write_register(0x5404+i*8,0);
    for(unsigned i=0;i<128;++i)write_register(0x5200+i*4,0);
    uint64_t rings=allocate();if(!rings){quarantine(6);return 6;}
    rx=(volatile rx_descriptor *)(uintptr_t)rings;tx=(volatile tx_descriptor *)(uintptr_t)(rings+512);
    for(unsigned i=0;i<RX_COUNT;i+=2){uint64_t page=allocate();if(!page){quarantine(6);return 6;}
        rx_buffers[i]=page;rx_buffers[i+1]=page+2048;rx[i].address=page;rx[i+1].address=page+2048;}
    for(unsigned i=0;i<TX_COUNT;i+=2){uint64_t page=allocate();if(!page){quarantine(6);return 6;}
        tx_buffers[i]=page;tx_buffers[i+1]=page+2048;tx[i].address=page;tx[i+1].address=page+2048;tx[i].status=tx[i+1].status=1;}
    unsigned uses_msi=msi(found);
    serial_text("M7_NETWORK_PCI_INTERRUPT id=");serial_hex(pci(found,0));serial_text(" line_pin=");serial_hex(pci(found,0x3c));serial_text(" msi=");serial_hex(uses_msi);serial_text("\n");
    if(!uses_msi){
        legacy_irq=pci(found,0x3c)&255;
        if(legacy_irq<3||legacy_irq>15||legacy_irq==4||legacy_irq==12){quarantine(7);return 7;}
        info.interrupt_vector=32+legacy_irq;
    }else info.interrupt_vector=NIC_VECTOR;
    pci_set(found,4,(pci(found,4)&65535)|6|(uses_msi?0x400u:0));
    if(!uses_msi)pci_set(found,4,(pci(found,4)&65535)&~0x400u);
    write_register(0x2800,(uint32_t)rings);write_register(0x2804,0);write_register(0x2808,RX_COUNT*16);
    write_register(0x2810,0);write_register(0x2818,RX_COUNT-1);write_register(0x2820,0);write_register(0x282c,0);
    write_register(0x3800,(uint32_t)(rings+512));write_register(0x3804,0);write_register(0x3808,TX_COUNT*16);
    write_register(0x3810,0);write_register(0x3818,0);write_register(0x3820,0);
    write_register(0xc4,2000);write_register(0x410,0x0060200a);
    write_register(0,read_register(0)|0x40);write_register(0x400,0x0004010a);
    ready=1;info.dma_pages=owned_count;
    write_register(0x100,0x04008002);(void)read_register(0xc0);write_register(0xd0,0xd5);
    if(!uses_msi){
        if(legacy_irq>=8){out8(0xa1,(uint8_t)(in8(0xa1)&~(1u<<(legacy_irq-8))));out8(0x21,in8(0x21)&~4u);}
        else out8(0x21,(uint8_t)(in8(0x21)&~(1u<<legacy_irq)));
    }
    info.online=!!(read_register(8)&2);info.status=0;
    __atomic_store_n(&pending,1,__ATOMIC_RELEASE);
    serial_text("M7_NETWORK_E1000_READY bdf=");serial_hex(found);serial_text(" irq=");serial_hex(info.interrupt_vector);serial_text("\n");return 0;
}

unsigned net7_nic_irq(uint64_t *frame) {
    if(!ready||frame[15]!=info.interrupt_vector)return 0;
    uint32_t cause=read_register(0xc0);++info.interrupts;
    if(cause==UINT32_MAX)quarantine(8);
    else {info.online=!!(read_register(8)&2);__atomic_store_n(&pending,1,__ATOMIC_RELEASE);}
    if(legacy_irq){if(legacy_irq>=8)out8(0xa0,0x20);out8(0x20,0x20);}
    else *(volatile uint32_t *)(uintptr_t)0xfee000b0=0;
    return 1;
}
unsigned net7_nic_pending(void) {return __atomic_load_n(&pending,__ATOMIC_ACQUIRE);}
unsigned net7_nic_poll(void (*receive)(const void *,unsigned)) {
    if(!ready||!receive){__atomic_exchange_n(&pending,0,__ATOMIC_ACQ_REL);return 0;}
    __atomic_exchange_n(&pending,0,__ATOMIC_ACQ_REL);unsigned count=0;
    for(;count<RX_COUNT;++count){
        volatile rx_descriptor *d=rx+rx_index;
        if(!(d->status&1))break;
        __asm__ volatile("mfence":::"memory");
        unsigned bytes=d->length;
        if((d->status&2)&&!d->errors&&bytes>=14&&bytes<=1518&&d->address==rx_buffers[rx_index]){
            ++info.rx_packets;receive((const void *)(uintptr_t)rx_buffers[rx_index],bytes);
        }else ++info.dropped;
        d->status=0;__asm__ volatile("mfence":::"memory");write_register(0x2818,rx_index);rx_index=(rx_index+1)%RX_COUNT;
    }
    if(rx[rx_index].status&1)__atomic_store_n(&pending,1,__ATOMIC_RELEASE);
    return count;
}
unsigned net7_nic_send(const void *frame,unsigned bytes) {
    if(!ready||!info.online)return 2;
    if(!frame||bytes<14||bytes>1514)return 3;
    volatile tx_descriptor *d=tx+tx_index;if(!(d->status&1))return 1;
    memcpy((void *)(uintptr_t)tx_buffers[tx_index],frame,bytes);
    d->length=(uint16_t)bytes;d->checksum_offset=d->checksum_start=0;d->special=0;d->command=0x0b;d->status=0;
    __asm__ volatile("mfence":::"memory");tx_index=(tx_index+1)%TX_COUNT;write_register(0x3818,tx_index);++info.tx_packets;return 0;
}
net7_nic_info net7_nic_status(void) {return info;}
