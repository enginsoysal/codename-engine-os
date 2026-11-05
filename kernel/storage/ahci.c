#include <engine/m4.h>
static block_info info;
static volatile uint32_t *hba,*port;
static uint64_t list,fis,table,bounce;
static unsigned filesystem_enabled;
static unsigned irq_ready;static volatile unsigned irq_error;
static inline uint32_t in32(uint16_t p){uint32_t v;__asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void out32(uint16_t p,uint32_t v){__asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));}
static uint32_t pci(unsigned bdf,unsigned reg){out32(0xcf8,0x80000000u|(bdf<<8)|(reg&252));return in32(0xcfc);}
static void pci_set(unsigned bdf,unsigned reg,uint32_t v){out32(0xcf8,0x80000000u|(bdf<<8)|(reg&252));out32(0xcfc,v);}
uint32_t m4_crc(const void *data,unsigned length){const uint8_t *p=data;uint32_t c=UINT32_MAX;for(unsigned i=0;i<length;++i){c^=p[i];for(unsigned b=0;b<8;++b)c=(c>>1)^(0xedb88320u&(0u-(c&1)));}return ~c;}
static int clear(volatile uint32_t *r,uint32_t mask){uint64_t end=m3_clock_ticks()+100;unsigned spins=0;while(*r&mask){if(m3_clock_ticks()>=end||++spins==10000000)return 0;__asm__ volatile("pause");}return 1;}
static int stop(void){port[6]&=~1u;if(!clear(port+6,1u<<15))return 0;port[6]&=~16u;return clear(port+6,1u<<14);}
static unsigned failure(unsigned code){++info.errors;if(code==B_TIMEOUT||code==B_IO){info.online=0;/* Retain all DMA pages: failure is not proof that DMA stopped. */(void)stop();serial_text(code==B_TIMEOUT?"M4_AHCI_TIMEOUT_DMA_QUARANTINED\n":"M4_AHCI_ERROR_DMA_QUARANTINED\n");}return code;}
unsigned block_irq(uint64_t *frame){
    if(!irq_ready)return 0;if(frame[15]==32){out8(0x20,0x20);return 1;}if(frame[15]!=146)return 0;
    ++info.irqs;uint32_t pending=hba[2];for(unsigned p=0;p<32;++p)if(pending&(1u<<p)){volatile uint32_t *source=hba+64+p*32;uint32_t status=source[4];if(source==port&&(status&(1u<<30)))irq_error=1;source[4]=status;}hba[2]=pending;*(volatile uint32_t*)(uintptr_t)0xfee000b0=0;return 1;
}
static unsigned command(uint8_t op,uint64_t lba,unsigned bytes){
    if(!info.online)return B_IO;
    if(!clear(port+8,0x88)||port[14]||port[13])return failure(B_TIMEOUT);
    memset((void*)(uintptr_t)list,0,1024);memset((void*)(uintptr_t)table,0,256);
    uint32_t *header=(uint32_t*)(uintptr_t)list;header[0]=5u|(op==0x35?64u:0)|(bytes?1u<<16:0);header[2]=(uint32_t)table;
    uint8_t *cfis=(uint8_t*)(uintptr_t)table;cfis[0]=0x27;cfis[1]=0x80;cfis[2]=op;
    if(op==0x25||op==0x35){cfis[7]=0x40;cfis[4]=(uint8_t)lba;cfis[5]=(uint8_t)(lba>>8);cfis[6]=(uint8_t)(lba>>16);cfis[8]=(uint8_t)(lba>>24);cfis[9]=(uint8_t)(lba>>32);cfis[10]=(uint8_t)(lba>>40);cfis[12]=(uint8_t)(bytes/512);}
    if(bytes){uint32_t *prdt=(uint32_t*)(uintptr_t)(table+128);prdt[0]=(uint32_t)bounce;prdt[3]=bytes-1;}
    irq_error=0;port[4]=UINT32_MAX;port[12]=UINT32_MAX;__asm__ volatile("mfence":::"memory");port[14]=1;++info.commands;
    uint64_t end=m3_clock_ticks()+100;unsigned spins=0;
    unsigned timed_out=0;if(irq_ready){uint64_t flags;__asm__ volatile("pushfq;popq %0;cli":"=r"(flags)::"memory");uint8_t mask=in8(0x21);out8(0x21,mask&~1u);
        for(;;){__asm__ volatile("cli":::"memory");if(!(port[14]&1)||irq_error)break;if(m3_clock_ticks()>=end){timed_out=1;break;}++info.wait_halts;__asm__ volatile("sti;hlt":::"memory");}
        out8(0x21,mask);if(flags&512)__asm__ volatile("sti":::"memory");
    }else while(port[14]&1){if(port[4]&(1u<<30))break;if(m3_clock_ticks()>=end||++spins==10000000){timed_out=1;break;}__asm__ volatile("pause");}
    if(timed_out)return failure(B_TIMEOUT);
    __asm__ volatile("mfence":::"memory");if(irq_error||(port[4]&(1u<<30))||(port[8]&1)||(bytes&&header[1]!=bytes))return failure(B_IO);return B_OK;
}
unsigned block_read(uint64_t lba,void *data){if(!info.online)return B_ABSENT;if(lba>=info.sectors)return B_DENIED;unsigned s=command(0x25,lba,512);if(!s){memcpy(data,(const void*)(uintptr_t)bounce,512);++info.reads;}return s;}
unsigned block_read_page(uint64_t lba,void *data){if(!info.online)return B_ABSENT;if((lba&7)||lba>info.sectors||info.sectors-lba<8)return B_DENIED;unsigned s=command(0x25,lba,4096);if(!s){memcpy(data,(const void*)(uintptr_t)bounce,4096);++info.reads;}return s;}
unsigned block_write_page(uint64_t lba,const void *data){if(!filesystem_enabled||!info.online||!info.validated||(lba&7)||lba<16||lba>info.sectors||info.sectors-lba<8)return B_DENIED;memcpy((void*)(uintptr_t)bounce,data,4096);unsigned s=command(0x35,lba,4096);if(!s)++info.writes;return s;}
unsigned block_write_record(const void *data){if(!info.online||!info.validated)return B_DENIED;memcpy((void*)(uintptr_t)bounce,data,512);unsigned s=command(0x35,1,512);if(!s)++info.writes;return s;}
unsigned block_filesystem_enable(void){if(!info.online||!info.validated)return B_DENIED;filesystem_enabled=1;return B_OK;}
unsigned block_write(uint64_t lba,const void *data){if(!filesystem_enabled||!info.online||!info.validated||lba<2||lba>=info.sectors)return B_DENIED;memcpy((void*)(uintptr_t)bounce,data,512);unsigned s=command(0x35,lba,512);if(!s)++info.writes;return s;}
unsigned block_flush(void){if(!info.online||!info.validated)return B_DENIED;unsigned s=command(0xea,0,0);if(!s)++info.flushes;return s;}
block_info block_stats(void){return info;}
static int bind(unsigned bdf){
    uint32_t bar=pci(bdf,0x24);if((bar&1)||(bar&6)||!(bar&~15u))return 0;uint64_t base=bar&~15u;if(base<0x10000000)return 0;
    for(uint64_t a=base&~UINT64_C(0x1fffff);a<base+0x2000;a+=0x200000)((uint64_t*)(uintptr_t)kernel_pd((unsigned)(a>>30)))[(a>>21)&511]|=0x18;
    uint64_t cr3;__asm__ volatile("movq %%cr3,%0; movq %0,%%cr3":"=r"(cr3)::"memory");
    hba=(volatile uint32_t*)(uintptr_t)base;info.pci=bdf;
    /* Do not echo PCI status W1C bits while enabling memory decode/bus mastering. */
    pci_set(bdf,4,(pci(bdf,4)&65535u)|6u);
    if(hba[9]&1){hba[10]|=2;if(!clear(hba+10,0x11))return 0;}
    hba[1]=0x80000001u;if(!clear(hba+1,1))return -1;hba[1]=0x80000000u;hba[2]=UINT32_MAX;
    unsigned cap=pci(bdf,0x34)&252;for(unsigned n=0;cap&&n<48;++n){unsigned value=pci(bdf,cap);if((value&255)==5){unsigned wide=value&(1u<<23);unsigned id=*(volatile uint32_t*)(uintptr_t)0xfee00020>>24;pci_set(bdf,cap+4,0xfee00000|(id<<12));if(wide)pci_set(bdf,cap+8,0);pci_set(bdf,cap+(wide?12:8),146);pci_set(bdf,cap,(value&~(7u<<20))|(1u<<16));irq_ready=1;hba[1]|=2;break;}cap=(value>>8)&252;}
    uint32_t implemented=hba[3];
    for(unsigned p=0;p<32;++p)if(implemented&(1u<<p)){
        port=hba+64+p*32;uint32_t status=port[10];if((status&15)!=3||((status>>8)&15)!=1||port[9]==0xeb140101u)continue;
        if(!stop()){++info.errors;return -1;}
        if(port[14]||port[13])continue;
        port[5]=irq_ready?0x40000001u:0;port[0]=(uint32_t)list;port[1]=0;port[2]=(uint32_t)fis;port[3]=0;port[4]=UINT32_MAX;port[12]=UINT32_MAX;
        port[6]|=16;port[6]|=1;info.online=1;info.port=p;
        if(command(0xec,0,512)==B_OK){
            const uint16_t *id=(const uint16_t*)(uintptr_t)bounce;
            uint64_t sectors=(uint64_t)id[100]|((uint64_t)id[101]<<16)|((uint64_t)id[102]<<32)|((uint64_t)id[103]<<48);
            unsigned sector_words=(unsigned)id[117]|((unsigned)id[118]<<16);
            if((id[83]&(1u<<10))&&sectors==M4_SECTORS&&(!((id[106]&0xd000)==0x5000)||sector_words==256)){
                info.sectors=sectors;uint8_t header[512];
                if(block_read(0,header)==B_OK&&!memcmp(header,M4_HEADER,16)){
                    uint32_t version,crc;uint64_t declared;memcpy(&version,header+16,4);memcpy(&declared,header+24,8);memcpy(&crc,header+508,4);
                    if(version==2&&declared==sectors&&m4_crc(header,508)==crc){info.validated=1;return 1;}
                }
            }
        }
        info.online=0;port[5]=0;port[4]=UINT32_MAX;hba[2]=1u<<p;if(!stop())return -1;
    }
    return 0;
}
unsigned block_init(void){
    uint64_t *pages[4]={&list,&fis,&table,&bounce};for(unsigned i=0;i<4;++i){*pages[i]=page_alloc();if(!*pages[i])return B_IO;++info.dma_pages;}
    uint8_t buses[256]={0},seen[256]={0};unsigned head=0,tail=1;seen[0]=1;
    while(head<tail){unsigned bus=buses[head++];for(unsigned dev=0;dev<32;++dev){unsigned first=(bus<<8)|(dev<<3);if((pci(first,0)&65535)==65535)continue;
        unsigned functions=(pci(first,12)&0x800000)?8:1;
        for(unsigned f=0;f<functions;++f){unsigned bdf=first|f;if((pci(bdf,0)&65535)==65535)continue;uint32_t cls=pci(bdf,8)>>8;
            if((cls>>8)==0x0604){unsigned next=(pci(bdf,24)>>8)&255;if(next&&!seen[next]&&tail<256){seen[next]=1;buses[tail++]=(uint8_t)next;}}
            if(cls==0x010601){int result=bind(bdf);if(result>0)return B_OK;if(result<0)return B_IO;}
        }
    }}return info.errors?B_IO:B_ABSENT;
}
