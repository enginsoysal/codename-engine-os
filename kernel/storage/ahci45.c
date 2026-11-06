#include <engine/storage.h>
typedef struct {volatile uint32_t *hba;uint64_t list,fis,table,bounce;unsigned bdf,irq,dead;} controller;
typedef struct {storage_disk disk;unsigned controller,port;} sata_disk;
static controller controllers[4];static sata_disk devices[STORAGE_MAX];static unsigned controller_count,disk_count,active=UINT32_MAX,selected=UINT32_MAX,fs_enabled;
static block_info info;static volatile unsigned irq_error;
static inline uint32_t in32(uint16_t p){uint32_t v;__asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void out32(uint16_t p,uint32_t v){__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));}
static uint32_t pci(unsigned bdf,unsigned reg){out32(0xcf8,0x80000000u|(bdf<<8)|(reg&252));return in32(0xcfc);}
static void pci_set(unsigned bdf,unsigned reg,uint32_t v){out32(0xcf8,0x80000000u|(bdf<<8)|(reg&252));out32(0xcfc,v);}
uint32_t m4_crc(const void *data,unsigned length){const uint8_t *p=data;uint32_t crc=UINT32_MAX;for(unsigned i=0;i<length;++i){crc^=p[i];for(unsigned b=0;b<8;++b)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1)));}return ~crc;}
static int clear(volatile uint32_t *r,unsigned mask){uint64_t end=m3_clock_ticks()+100;unsigned spins=0;while(*r&mask){if(m3_clock_ticks()>=end||++spins==10000000)return 0;__asm__ volatile("pause");}return 1;}
static volatile uint32_t *port_of(unsigned n){return controllers[devices[n].controller].hba+64+devices[n].port*32;}
static unsigned stop_active(void){if(active==UINT32_MAX)return B_OK;controller *c=&controllers[devices[active].controller];volatile uint32_t *p=port_of(active);p[5]=0;p[6]&=~1u;if(!clear(p+6,1u<<15)){c->dead=1;return B_TIMEOUT;}p[6]&=~16u;if(!clear(p+6,1u<<14)){c->dead=1;return B_TIMEOUT;}p[4]=UINT32_MAX;c->hba[2]=1u<<devices[active].port;active=UINT32_MAX;return B_OK;}
static unsigned activate(unsigned n){
    if(n>=disk_count)return B_DENIED;controller *c=&controllers[devices[n].controller];if(c->dead)return B_IO;if(active==n)return B_OK;if(stop_active())return B_TIMEOUT;
    volatile uint32_t *p=port_of(n);p[6]&=~1u;if(!clear(p+6,1u<<15))return B_TIMEOUT;p[6]&=~16u;if(!clear(p+6,1u<<14)||p[14]||p[13])return B_TIMEOUT;
    p[0]=(uint32_t)c->list;p[1]=0;p[2]=(uint32_t)c->fis;p[3]=0;p[4]=UINT32_MAX;p[12]=UINT32_MAX;p[5]=c->irq?0x40000001u:0;p[6]|=16|1;active=n;return B_OK;
}
unsigned block_irq(uint64_t *frame){
    if(!controller_count)return 0;if(frame[15]==32){out8(0x20,0x20);return 1;}if(frame[15]!=146)return 0;++info.irqs;
    for(unsigned i=0;i<controller_count;++i){controller *c=&controllers[i];unsigned pending=c->hba[2];for(unsigned p=0;p<32;++p)if(pending&(1u<<p)){volatile uint32_t *r=c->hba+64+p*32;unsigned status=r[4];if(active!=UINT32_MAX&&devices[active].controller==i&&devices[active].port==p&&(status&(1u<<30)))irq_error=1;r[4]=status;}c->hba[2]=pending;}
    *(volatile uint32_t*)(uintptr_t)0xfee000b0=0;return 1;
}
static unsigned command(uint8_t opcode,uint64_t lba,unsigned bytes){
    if(active==UINT32_MAX)return B_ABSENT;sata_disk *d=&devices[active];controller *c=&controllers[d->controller];volatile uint32_t *p=port_of(active);if(c->dead)return B_IO;
    if(!clear(p+8,0x88)||p[14]||p[13])return B_TIMEOUT;memset((void*)(uintptr_t)c->list,0,1024);memset((void*)(uintptr_t)c->table,0,256);
    uint32_t *h=(uint32_t*)(uintptr_t)c->list;h[0]=5u|(opcode==0x35?64u:0)|(bytes?1u<<16:0);h[2]=(uint32_t)c->table;uint8_t *fis=(uint8_t*)(uintptr_t)c->table;fis[0]=0x27;fis[1]=0x80;fis[2]=opcode;
    if(opcode==0x25||opcode==0x35){fis[7]=0x40;fis[4]=(uint8_t)lba;fis[5]=(uint8_t)(lba>>8);fis[6]=(uint8_t)(lba>>16);fis[8]=(uint8_t)(lba>>24);fis[9]=(uint8_t)(lba>>32);fis[10]=(uint8_t)(lba>>40);fis[12]=(uint8_t)(bytes/d->disk.sector_bytes);}
    if(bytes){uint32_t *prdt=(uint32_t*)(uintptr_t)(c->table+128);prdt[0]=(uint32_t)c->bounce;prdt[3]=bytes-1;}
    uint64_t flags;__asm__ volatile("pushfq;popq %0;cli":"=r"(flags)::"memory");irq_error=0;p[4]=UINT32_MAX;p[12]=UINT32_MAX;__asm__ volatile("mfence":::"memory");p[14]=1;++info.commands;
    uint8_t mask=in8(0x21);if(c->irq)out8(0x21,mask&~1u);uint64_t end=m3_clock_ticks()+100;unsigned spins=0,status=B_OK;
    while(p[14]&1){if(irq_error||(p[4]&(1u<<30))){status=B_IO;break;}if(m3_clock_ticks()>=end||(!c->irq&&++spins==10000000)){status=B_TIMEOUT;break;}
        if(c->irq){++info.wait_halts;__asm__ volatile("sti;hlt;cli":::"memory");}else __asm__ volatile("pause");}
    __asm__ volatile("mfence":::"memory");if(!status&&(irq_error||(p[4]&(1u<<30))||(p[8]&1)||(bytes&&h[1]!=bytes)))status=B_IO;if(c->irq)out8(0x21,mask);if(flags&512)__asm__ volatile("sti":::"memory");
    if(status){++info.errors;c->dead=1;(void)stop_active();serial_text("M45_AHCI_CONTROLLER_QUARANTINED status=");serial_hex(status);serial_text("\n");}return status;
}
static unsigned read_disk(unsigned n,uint64_t lba,void *data){
    if(n>=disk_count||lba>=devices[n].disk.sectors)return B_DENIED;unsigned bytes=devices[n].disk.sector_bytes;if(!devices[n].disk.readable)return B_UNSUPPORTED;
    unsigned s=activate(n);if(!s)s=command(0x25,lba,bytes);if(!s){memcpy(data,(void*)(uintptr_t)controllers[devices[n].controller].bounce,bytes);++info.reads;}
    if(selected!=UINT32_MAX&&active!=selected){unsigned restore=activate(selected);if(!s)s=restore;}else if(selected==UINT32_MAX){unsigned stopped=stop_active();if(!s)s=stopped;}return s;
}
static unsigned selected_ready(void){return selected==UINT32_MAX?B_ABSENT:activate(selected);}
unsigned block_read(uint64_t lba,void *data){if(selected==UINT32_MAX)return B_ABSENT;return read_disk(selected,lba,data);}
unsigned block_read_page(uint64_t lba,void *data){if(selected==UINT32_MAX)return B_ABSENT;if((lba&7)||lba>M4_SECTORS-8)return B_DENIED;unsigned s=selected_ready();if(!s)s=command(0x25,lba,4096);if(!s){memcpy(data,(void*)(uintptr_t)controllers[devices[selected].controller].bounce,4096);++info.reads;}return s;}
static unsigned write_selected(uint64_t lba,const void *data,unsigned bytes){if(selected==UINT32_MAX||!devices[selected].disk.marked||!fs_enabled)return B_DENIED;unsigned s=selected_ready();if(!s){memcpy((void*)(uintptr_t)controllers[devices[selected].controller].bounce,data,bytes);s=command(0x35,lba,bytes);if(!s)++info.writes;}return s;}
unsigned block_write_record(const void *data){return write_selected(1,data,512);}
unsigned block_write(uint64_t lba,const void *data){return lba<2||lba>=M4_SECTORS?B_DENIED:write_selected(lba,data,512);}
unsigned block_write_page(uint64_t lba,const void *data){return (lba&7)||lba<16||lba>M4_SECTORS-8?B_DENIED:write_selected(lba,data,4096);}
unsigned block_filesystem_enable(void){if(selected==UINT32_MAX)return B_DENIED;fs_enabled=1;return B_OK;}
unsigned block_flush(void){if(selected==UINT32_MAX||!fs_enabled)return B_DENIED;unsigned s=selected_ready();if(!s)s=command(0xea,0,0);if(!s)++info.flushes;return s;}
block_info block_stats(void){block_info s=info;if(selected!=UINT32_MAX){sata_disk *d=&devices[selected];s.pci=d->disk.bdf;s.port=d->port;s.sectors=d->disk.sectors;s.validated=d->disk.marked;s.online=!controllers[d->controller].dead;}return s;}
static unsigned bind(unsigned bdf){
    if(controller_count==4)return B_DENIED;unsigned bar=pci(bdf,0x24);if((bar&7)||bar<0x10000000||bar>0xffffd000)return B_UNSUPPORTED;
    uint64_t base=bar&~15u;for(uint64_t a=base&~UINT64_C(0x1fffff);a<base+0x2000;a+=0x200000)((uint64_t*)(uintptr_t)kernel_pd((unsigned)(a>>30)))[(a>>21)&511]|=0x18;
    uint64_t cr3;__asm__ volatile("movq %%cr3,%0;movq %0,%%cr3":"=r"(cr3)::"memory");controller *c=&controllers[controller_count];c->hba=(volatile uint32_t*)(uintptr_t)base;c->bdf=bdf;
    uint64_t *pages[4]={&c->list,&c->fis,&c->table,&c->bounce};for(unsigned i=0;i<4;++i){*pages[i]=page_alloc();if(!*pages[i])return B_IO;++info.dma_pages;}
    unsigned ci=controller_count++;pci_set(bdf,4,(pci(bdf,4)&65535)|6);if(c->hba[9]&1){c->hba[10]|=2;if(!clear(c->hba+10,0x11))return B_TIMEOUT;}c->hba[1]=0x80000001;if(!clear(c->hba+1,1))return B_TIMEOUT;c->hba[1]=0x80000000;c->hba[2]=UINT32_MAX;
    unsigned cap=pci(bdf,0x34)&252;for(unsigned j=0;cap&&j<48;++j){unsigned v=pci(bdf,cap);if((v&255)==5){unsigned wide=v&(1u<<23),id=*(volatile uint32_t*)(uintptr_t)0xfee00020>>24;pci_set(bdf,cap+4,0xfee00000|(id<<12));if(wide)pci_set(bdf,cap+8,0);pci_set(bdf,cap+(wide?12:8),146);pci_set(bdf,cap,(v&~(7u<<20))|(1u<<16));c->irq=1;c->hba[1]|=2;break;}cap=(v>>8)&252;}
    unsigned implemented=c->hba[3];for(unsigned p=0;p<32&&disk_count<STORAGE_MAX;++p)if(implemented&(1u<<p)){
        volatile uint32_t *r=c->hba+64+p*32;unsigned link=r[10];if((link&15)!=3||((link>>8)&15)!=1)continue;
        unsigned n=disk_count++;sata_disk *d=&devices[n];d->controller=ci;d->port=p;d->disk=(storage_disk){STORAGE_AHCI,bdf,p,512,1,0,0,n,0};
        if(activate(n)){--disk_count;if(c->dead)break;continue;}
        if(r[9]==0||r[9]==UINT32_MAX){r[6]&=~1u;if(!clear(r+6,1u<<15)){c->dead=1;break;}unsigned control=r[11];r[11]=(control&~15u)|1u;uint64_t delay=m3_clock_ticks()+1;unsigned spins=0;while(m3_clock_ticks()<delay&&++spins<1000000)__asm__ volatile("pause");r[11]=control&~15u;uint64_t deadline=m3_clock_ticks()+100;spins=0;while((r[9]==0||r[9]==UINT32_MAX)&&(m3_clock_ticks()<deadline)&&++spins<10000000)__asm__ volatile("pause");r[6]|=1;}
        if(r[9]!=0x101){serial_text("M45_AHCI_UNSUPPORTED_SIGNATURE port=");serial_hex(p);serial_text(" signature=");serial_hex(r[9]);serial_text("\n");(void)stop_active();--disk_count;continue;}
        if(command(0xec,0,512)){--disk_count;if(c->dead)break;continue;}
        const uint16_t *id=(void*)(uintptr_t)c->bounce;d->disk.sectors=(uint64_t)id[100]|((uint64_t)id[101]<<16)|((uint64_t)id[102]<<32)|((uint64_t)id[103]<<48);
        if((id[106]&0xd000)==0x5000){uint64_t words=(uint64_t)id[117]|((uint64_t)id[118]<<16);d->disk.sector_bytes=words<=UINT32_MAX/2?(unsigned)words*2:0;}
        d->disk.readable=(id[83]&(1u<<10))&&d->disk.sectors&&d->disk.sectors<=UINT64_C(0x1000000000000)&&(d->disk.sector_bytes==512||d->disk.sector_bytes==4096);
        if(d->disk.readable&&d->disk.sector_bytes==512&&d->disk.sectors==M4_SECTORS&&command(0x25,0,512)==B_OK){const uint8_t *h=(void*)(uintptr_t)c->bounce;uint32_t version,crc;uint64_t size;memcpy(&version,h+16,4);memcpy(&size,h+24,8);memcpy(&crc,h+508,4);d->disk.marked=!memcmp(h,M4_HEADER,16)&&version==2&&size==M4_SECTORS&&m4_crc(h,508)==crc;if(d->disk.marked&&selected==UINT32_MAX)selected=n;}
        (void)storage_register(d->disk,read_disk);if(stop_active())break;
    }
    return B_OK;
}
unsigned block_init(void){
    for(unsigned bus=0;bus<256;++bus)for(unsigned dev=0;dev<32;++dev){unsigned first=(bus<<8)|(dev<<3);if((pci(first,0)&65535)==65535)continue;unsigned functions=(pci(first,12)&0x800000)?8:1;for(unsigned f=0;f<functions;++f)if((pci(first|f,8)>>8)==0x010601){unsigned s=bind(first|f);if(s){serial_text("M45_AHCI_BIND_FAILED status=");serial_hex(s);serial_text("\n");}}}
    if(selected!=UINT32_MAX)return selected_ready();return disk_count?B_DENIED:B_ABSENT;
}
#ifdef ENGINE_M5
/* Internal installer transport: callers authorize the target and constrain ranges. */
unsigned m5_sata_transfer(unsigned context,uint64_t lba,void *data,unsigned bytes,unsigned write){
    if(context>=disk_count||!data||(bytes!=512&&bytes!=4096)||devices[context].disk.sector_bytes!=512||
       !devices[context].disk.readable||lba>=devices[context].disk.sectors||bytes/512>devices[context].disk.sectors-lba)return B_DENIED;
    unsigned status=activate(context);controller *c=&controllers[devices[context].controller];
    if(!status&&write)memcpy((void*)(uintptr_t)c->bounce,data,bytes);
    if(!status)status=command(write?0x35:0x25,lba,bytes);
    if(!status){if(write)++info.writes;else{memcpy(data,(void*)(uintptr_t)c->bounce,bytes);++info.reads;}}
    return status;
}
unsigned m5_sata_flush(unsigned context){if(context>=disk_count)return B_DENIED;unsigned s=activate(context);if(!s)s=command(0xea,0,0);if(!s)++info.flushes;return s;}
#endif
