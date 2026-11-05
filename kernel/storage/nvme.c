#include <engine/storage.h>
typedef struct {volatile uint32_t *registers;uint64_t asq,acq,sq,cq,buffer;unsigned atail,ahead,aphase,tail,head,phase,cid,stride,irq,dead;} nvme_controller;
typedef struct {unsigned controller,nsid,sector_bytes;} nvme_namespace;
static nvme_controller controllers[2];static nvme_namespace namespaces[STORAGE_MAX];static unsigned controller_count,namespace_count;static uint64_t irqs,halts;
static inline uint32_t pci(unsigned bdf,unsigned reg){uint32_t v,a=0x80000000u|(bdf<<8)|(reg&252);__asm__ volatile("outl %0,%1"::"a"(a),"Nd"((uint16_t)0xcf8));__asm__ volatile("inl %1,%0":"=a"(v):"Nd"((uint16_t)0xcfc));return v;}
static void pci_set(unsigned bdf,unsigned reg,unsigned value){uint32_t a=0x80000000u|(bdf<<8)|(reg&252);__asm__ volatile("outl %0,%1"::"a"(a),"Nd"((uint16_t)0xcf8));__asm__ volatile("outl %0,%1"::"a"(value),"Nd"((uint16_t)0xcfc));}
static uint64_t r64(const void *p){uint64_t x;memcpy(&x,p,8);return x;}
static void register64(volatile uint32_t *p,uint64_t value){p[0]=(uint32_t)value;p[1]=(uint32_t)(value>>32);}
static unsigned ready(nvme_controller *c,unsigned value){uint64_t end=m3_clock_ticks()+300;unsigned spins=0;while((c->registers[7]&1)!=value){if(m3_clock_ticks()>=end||++spins==30000000)return B_TIMEOUT;__asm__ volatile("pause");}return B_OK;}
unsigned nvme_irq(uint64_t *frame){if(!controller_count)return 0;if(frame[15]==32){out8(0x20,0x20);return 1;}if(frame[15]!=147)return 0;++irqs;*(volatile uint32_t*)(uintptr_t)0xfee000b0=0;return 1;}
static unsigned submit(nvme_controller *c,uint32_t cmd[16],unsigned io){
    if(c->dead)return B_IO;unsigned *tail=io?&c->tail:&c->atail,*head=io?&c->head:&c->ahead,*phase=io?&c->phase:&c->aphase;uint64_t sq=io?c->sq:c->asq,cq=io?c->cq:c->acq;unsigned qid=io?1:0;
    unsigned cid=++c->cid&65535;cmd[0]=(cmd[0]&65535)|(cid<<16);uint64_t flags;__asm__ volatile("pushfq;popq %0;cli":"=r"(flags)::"memory");memcpy((void*)(uintptr_t)(sq+*tail*64),cmd,64);*tail=(*tail+1)&15;__asm__ volatile("mfence":::"memory");c->registers[(0x1000+(2*qid)*c->stride)/4]=*tail;
    volatile uint32_t *completion=(volatile uint32_t*)(uintptr_t)(cq+*head*16);uint64_t end=m3_clock_ticks()+200;unsigned spins=0,status=B_OK;uint8_t mask=in8(0x21);if(c->irq)out8(0x21,mask&~1u);
    while(((completion[3]>>16)&1)!=*phase){if((c->registers[7]&2)||m3_clock_ticks()>=end||(!c->irq&&++spins==20000000)){status=B_TIMEOUT;break;}if(c->irq){++halts;__asm__ volatile("sti;hlt;cli":::"memory");}else __asm__ volatile("pause");}
    if(!status){__asm__ volatile("lfence":::"memory");if((completion[3]&65535)!=cid||(completion[3]>>17)||(completion[2]>>16)!=qid)status=B_IO;*head=(*head+1)&15;if(!*head)*phase^=1;c->registers[(0x1000+(2*qid+1)*c->stride)/4]=*head;}
    if(c->irq)out8(0x21,mask);if(flags&512)__asm__ volatile("sti":::"memory");if(status){c->dead=1;c->registers[5]&=~1u;(void)ready(c,0);serial_text("M45_NVME_DMA_QUARANTINED\n");}return status;
}
static void prp(uint32_t cmd[16],uint64_t address){cmd[6]=(uint32_t)address;cmd[7]=(uint32_t)(address>>32);}
static unsigned identify(nvme_controller *c,unsigned nsid,unsigned cns){uint32_t cmd[16]={6,nsid};cmd[10]=cns;prp(cmd,c->buffer);memset((void*)(uintptr_t)c->buffer,0,4096);return submit(c,cmd,0);}
static unsigned read_namespace(unsigned context,uint64_t lba,void *buffer){if(context>=namespace_count)return B_DENIED;nvme_namespace *n=&namespaces[context];nvme_controller *c=&controllers[n->controller];uint32_t cmd[16]={2,n->nsid};prp(cmd,c->buffer);cmd[10]=(uint32_t)lba;cmd[11]=(uint32_t)(lba>>32);unsigned s=submit(c,cmd,1);if(!s)memcpy(buffer,(void*)(uintptr_t)c->buffer,n->sector_bytes);return s;}
static unsigned bind(unsigned bdf){
    if(controller_count==2)return B_DENIED;unsigned bar=pci(bdf,16);if(bar&1)return B_UNSUPPORTED;uint64_t physical=bar&~15u;if((bar&6)==4)physical|=(uint64_t)pci(bdf,20)<<32;if(physical<0x10000000||physical>UINT64_C(0x000ffffffff00000))return B_UNSUPPORTED;
    uint64_t pd=page_alloc();if(!pd)return B_IO;uint64_t aligned=physical&~UINT64_C(0x1fffff);((uint64_t*)(uintptr_t)pd)[0]=aligned|UINT64_C(0x800000000000009b);((uint64_t*)(uintptr_t)pd)[1]=(aligned+0x200000)|UINT64_C(0x800000000000009b);unsigned branch=3+controller_count;((uint64_t*)(uintptr_t)kernel_stack_branch())[branch]=pd|3;
    uint64_t va=UINT64_C(0xffff800000000000)+(uint64_t)branch*0x40000000+(physical&0x1fffff);uint64_t cr3;__asm__ volatile("movq %%cr3,%0;movq %0,%%cr3":"=r"(cr3)::"memory");nvme_controller *c=&controllers[controller_count];c->registers=(volatile uint32_t*)(uintptr_t)va;pci_set(bdf,4,(pci(bdf,4)&65535)|6);
    uint64_t cap=(uint64_t)c->registers[0]|((uint64_t)c->registers[1]<<32);if((cap&65535)<15||((cap>>48)&15)||!(cap&(UINT64_C(1)<<37)))return B_UNSUPPORTED;c->stride=4u<<((cap>>32)&15);if(c->stride>4096)return B_UNSUPPORTED;
    c->registers[5]&=~1u;if(ready(c,0))return B_TIMEOUT;uint64_t *pages[5]={&c->asq,&c->acq,&c->sq,&c->cq,&c->buffer};for(unsigned i=0;i<5;++i)if(!(*pages[i]=page_alloc()))return B_IO;
    unsigned ci=controller_count++;c->aphase=c->phase=1;
    unsigned pointer=pci(bdf,0x34)&252,msix=0,msi=0;for(unsigned i=0;pointer&&i<48;++i){unsigned h=pci(bdf,pointer);if((h&255)==17)msix=pointer;if((h&255)==5)msi=pointer;pointer=(h>>8)&252;}
    unsigned target=0xfee00000|((*(volatile uint32_t*)(uintptr_t)0xfee00020>>24)<<12);
    if(msix){unsigned table=pci(bdf,msix+4);if(!(table&7)&&(table&~7u)<0x1fff0){volatile uint32_t *entry=(volatile uint32_t*)(uintptr_t)(va+(table&~7u));entry[3]=1;entry[0]=target;entry[1]=0;entry[2]=147;__asm__ volatile("mfence":::"memory");entry[3]=0;pci_set(bdf,msix,(pci(bdf,msix)|(1u<<31))&~(1u<<30));c->irq=1;}}
    else if(msi){unsigned v=pci(bdf,msi),wide=v&(1u<<23);pci_set(bdf,msi+4,target);if(wide)pci_set(bdf,msi+8,0);pci_set(bdf,msi+(wide?12:8),147);pci_set(bdf,msi,(v&~(7u<<20))|(1u<<16));c->irq=1;}
    if(c->irq)pci_set(bdf,4,(pci(bdf,4)&65535)|0x406);c->registers[9]=15|(15u<<16);register64(c->registers+10,c->asq);register64(c->registers+12,c->acq);c->registers[5]=0x460001;if(ready(c,1))return B_TIMEOUT;
    if(identify(c,0,1))return B_IO;uint32_t cmd[16]={5};prp(cmd,c->cq);cmd[10]=1|(15u<<16);cmd[11]=1|(c->irq?2:0);if(submit(c,cmd,0))return B_IO;memset(cmd,0,sizeof(cmd));cmd[0]=1;prp(cmd,c->sq);cmd[10]=1|(15u<<16);cmd[11]=1|(1u<<16);if(submit(c,cmd,0))return B_IO;
    if(identify(c,0,2))return B_IO;uint32_t ids[STORAGE_MAX];memcpy(ids,(void*)(uintptr_t)c->buffer,sizeof(ids));
    for(unsigned i=0;i<STORAGE_MAX&&ids[i]&&namespace_count<STORAGE_MAX&&storage_count()<STORAGE_MAX;++i){if(identify(c,ids[i],0))return B_IO;const uint8_t *data=(void*)(uintptr_t)c->buffer;uint64_t sectors=r64(data);unsigned format=data[26]&15,bits=data[128+format*4+2];uint16_t metadata;memcpy(&metadata,data+128+format*4,2);unsigned bytes=bits<32?1u<<bits:0,readable=sectors&&!metadata&&(bytes==512||bytes==4096);
        unsigned context=namespace_count++;namespaces[context]=(nvme_namespace){ci,ids[i],bytes};storage_disk d={STORAGE_NVME,bdf,ids[i],bytes,readable,0,sectors,context,0};(void)storage_register(d,read_namespace);}
    serial_text("M45_NVME_CONTROLLER_READY irq=");serial_hex(c->irq);serial_text("\n");return B_OK;
}
unsigned nvme_discover(void){unsigned failures=0;for(unsigned bus=0;bus<256;++bus)for(unsigned dev=0;dev<32;++dev){unsigned first=(bus<<8)|(dev<<3);if((pci(first,0)&65535)==65535)continue;unsigned functions=(pci(first,12)&0x800000)?8:1;for(unsigned f=0;f<functions;++f)if((pci(first|f,8)>>8)==0x010802){unsigned s=bind(first|f);if(s){++failures;serial_text("M45_NVME_BIND_FAILED status=");serial_hex(s);serial_text("\n");}}}return failures?B_IO:B_OK;}
void nvme_diagnostics(void){serial_text("M45_NVME_STATS controllers=");serial_hex(controller_count);serial_text(" namespaces=");serial_hex(namespace_count);serial_text(" irqs=");serial_hex(irqs);serial_text(" wait_halts=");serial_hex(halts);serial_text("\n");}
