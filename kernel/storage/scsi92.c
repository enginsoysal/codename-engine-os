/* Original bounded SCSI transports. Protocol layouts follow the public MPI v1
   and VMware PVSCSI interfaces. No upstream driver implementation is imported. */
#include <engine/storage.h>
#define ADAPTERS92 4u
typedef struct {volatile uint32_t*regs;unsigned bdf,kind,dead,sequence,bar,irq;uint16_t io;uint64_t request,reply,data,sense,state;uint32_t producer,consumer;uint64_t commands,timeouts,interrupts,halts;} adapter92;
typedef struct {unsigned adapter,target;uint32_t bytes;uint64_t sectors;} disk92;
static volatile unsigned waiting92;
static adapter92 adapters92[ADAPTERS92];static disk92 disks92[STORAGE_MAX];static unsigned adapter_count92,disk_count92;
static uint32_t pci92(unsigned b,unsigned r){uint32_t v,a=0x80000000u|(b<<8)|(r&252);__asm__ volatile("outl %0,%1"::"a"(a),"Nd"((uint16_t)0xcf8));__asm__ volatile("inl %1,%0":"=a"(v):"Nd"((uint16_t)0xcfc));return v;}
static void setpci92(unsigned b,unsigned r,uint32_t v){uint32_t a=0x80000000u|(b<<8)|(r&252);__asm__ volatile("outl %0,%1"::"a"(a),"Nd"((uint16_t)0xcf8));__asm__ volatile("outl %0,%1"::"a"(v),"Nd"((uint16_t)0xcfc));}
static uint32_t get32_92(const void*p){uint32_t v;memcpy(&v,p,4);return v;}
static void put32_92(void*p,uint32_t v){memcpy(p,&v,4);}
static void put64_92(void*p,uint64_t v){memcpy(p,&v,8);}
static uint32_t be32_92(const uint8_t*p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static void beout92(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static unsigned fail92(adapter92*a,unsigned status){a->dead=1;if(a->regs){if(a->kind==1)a->regs[13]=UINT32_MAX;else if(a->kind==2)a->regs[0x2010/4]=0;}setpci92(a->bdf,4,(pci92(a->bdf,4)&65535)&~4u);if(status==B_TIMEOUT)++a->timeouts;serial_text("M92_SCSI_QUARANTINED bdf=");serial_hex(a->bdf);serial_text(" status=");serial_hex(status);serial_text("\n");return status;}
static unsigned wait92(adapter92*a,unsigned offset,uint32_t mask,uint32_t wanted){uint64_t until=m3_clock_ticks()+100;for(unsigned n=0;n<10000000;++n){if((a->regs[offset/4]&mask)==wanted)return B_OK;if(m3_clock_ticks()>=until)break;__asm__ volatile("pause");}return B_TIMEOUT;}
static volatile uint32_t*map92(unsigned bdf,unsigned bar,unsigned branch){
 uint32_t low=pci92(bdf,16+bar*4);if(low&1)return 0;uint64_t address=low&~15u;if((low&6)==4){if(bar==5)return 0;address|=(uint64_t)pci92(bdf,20+bar*4)<<32;}
 if(address<0x10000000||address>UINT64_C(0x000ffffffff00000))return 0;uint64_t*root=(void*)(uintptr_t)kernel_stack_branch();if(root[branch])return 0;uint64_t pd=page_alloc();if(!pd)return 0;
 uint64_t base=address&~UINT64_C(0x1fffff);((uint64_t*)(uintptr_t)pd)[0]=base|UINT64_C(0x800000000000009b);((uint64_t*)(uintptr_t)pd)[1]=(base+0x200000)|UINT64_C(0x800000000000009b);root[branch]=pd|3;
 uint64_t cr3;__asm__ volatile("movq %%cr3,%0;movq %0,%%cr3":"=r"(cr3)::"memory");return (volatile uint32_t*)(uintptr_t)(UINT64_C(0xffff800000000000)+(uint64_t)branch*0x40000000+(address&0x1fffff));
}
static unsigned handshake92(adapter92*a,uint8_t*request,unsigned bytes,uint8_t*reply){
 if(!bytes||(bytes&3)||bytes>128)return B_DENIED;memset(reply,0,128);a->regs[12]=0;a->regs[0]=0x42000000u|((bytes/4)<<16);
 if(wait92(a,0x30,1,1))return B_TIMEOUT;a->regs[12]=0;if(wait92(a,0x30,0x80000000u,0))return B_TIMEOUT;
 for(unsigned i=0;i<bytes;i+=4){a->regs[0]=get32_92(request+i);if(wait92(a,0x30,0x80000000u,0))return B_TIMEOUT;}
 unsigned words=2;for(unsigned i=0;i<words;++i){if(wait92(a,0x30,1,1))return B_TIMEOUT;uint16_t value=(uint16_t)a->regs[0];memcpy(reply+i*2,&value,2);a->regs[12]=0;if(i==1){words=(unsigned)reply[2]*2;if(words<2||words>64)return B_IO;}}
 if(wait92(a,0x30,1,1))return B_TIMEOUT;a->regs[12]=0;if(wait92(a,0,0x08000000,0))return B_TIMEOUT;
 return (get32_92(reply+12)&0x7fff0000u)?B_IO:B_OK;
}
static void idle92(adapter92*a){if(a->irq){++a->halts;__asm__ volatile("sti;hlt;cli":::"memory");}else __asm__ volatile("pause");}
unsigned scsi92_irq(uint64_t*frame){
 if(frame[15]==32&&waiting92){out8(0x20,0x20);return 1;}
 if(frame[15]!=150)return 0;for(unsigned i=0;i<adapter_count92;++i){adapter92*a=&adapters92[i];if(!a->irq||a->dead)continue;
  if(a->kind==1){if(a->regs[12]&8){++a->interrupts;a->regs[13]|=8;}}
  else if(a->kind==2){unsigned status=a->regs[0x100c/4];if(status){++a->interrupts;a->regs[0x100c/4]=status;}}
 }
 *(volatile uint32_t*)(uintptr_t)0xfee000b0=0;return 1;
}
static void arm92(adapter92*a,unsigned index){
 if(a->kind==3)return;unsigned pointer=pci92(a->bdf,0x34)&252,msi=0,msix=0;
 for(unsigned n=0;pointer&&n<48;++n){unsigned cap=pci92(a->bdf,pointer);if((cap&255)==5)msi=pointer;if((cap&255)==17)msix=pointer;pointer=(cap>>8)&252;}
 unsigned address=0xfee00000|((*(volatile uint32_t*)(uintptr_t)0xfee00020>>24)<<12);
 if(msi){unsigned value=pci92(a->bdf,msi),wide=value&(1u<<23);setpci92(a->bdf,msi+4,address);if(wide)setpci92(a->bdf,msi+8,0);setpci92(a->bdf,msi+(wide?12:8),150);setpci92(a->bdf,msi,(value&~(7u<<20))|(1u<<16));a->irq=1;}
 else if(msix){unsigned table=pci92(a->bdf,msix+4),bar=table&7;volatile uint32_t*base=bar==a->bar?a->regs:bar<6?map92(a->bdf,bar,40+index):0;
  if(base&&(table&~7u)<0x10000){volatile uint32_t*entry=base+(table&~7u)/4;setpci92(a->bdf,msix,pci92(a->bdf,msix)|(1u<<30));entry[3]=1;entry[0]=address;entry[1]=0;entry[2]=150;__asm__ volatile("mfence":::"memory");entry[3]=0;setpci92(a->bdf,msix,(pci92(a->bdf,msix)|(1u<<31))&~(1u<<30));a->irq=1;}}
 if(a->irq){if(a->kind==1)a->regs[13]=1;else a->regs[0x2010/4]=3;}
}
static unsigned mpi_start92(adapter92*a){
 a->regs[13]=UINT32_MAX;a->regs[0]=0x40000000;if(wait92(a,0,0xf0000000,0x10000000))return B_TIMEOUT;
 uint8_t request[128]={0},reply[128];request[0]=4;request[3]=2;request[5]=16;request[6]=1;request[12]=128;request[40]=5;request[41]=1;
 unsigned s=handshake92(a,request,44,reply);if(s)return s;
 for(unsigned i=0;i<16;++i)a->regs[17]=(uint32_t)(a->reply+i*128);
 memset(request,0,sizeof(request));request[3]=6;return handshake92(a,request,12,reply);
}
static unsigned mpi_command92(adapter92*a,unsigned target,const uint8_t*cdb,unsigned length,unsigned bytes,unsigned write,unsigned*scsi){
 uint8_t*r=(void*)(uintptr_t)a->request;memset(r,0,128);memset((void*)(uintptr_t)a->sense,0,96);unsigned token=++a->sequence;if(!token||token&0x80000000u)return fail92(a,B_IO);
 r[0]=(uint8_t)target;r[4]=(uint8_t)length;r[5]=96;put32_92(r+8,token);put32_92(r+20,bytes?(write?0x01000000u:0x02000000u):0);memcpy(r+24,cdb,length);put32_92(r+40,bytes);put32_92(r+44,(uint32_t)a->sense);
 put32_92(r+48,0xd1000000u|(write?0x04000000u:0)|bytes);put32_92(r+52,(uint32_t)a->data);__asm__ volatile("mfence":::"memory");if(a->irq)a->regs[13]=1;a->regs[16]=(uint32_t)a->request;
 uint64_t until=m3_clock_ticks()+100;uint32_t result=UINT32_MAX;for(unsigned spin=0;spin<10000000;++spin){result=a->regs[17];if(result!=UINT32_MAX)break;if(m3_clock_ticks()>=until)break;idle92(a);}
 if(result==UINT32_MAX)return fail92(a,B_TIMEOUT);__asm__ volatile("lfence":::"memory");*scsi=0;
 if(!(result&0x80000000u)){(void)a->regs[17];return result==token?B_OK:fail92(a,B_IO);}
 uint64_t address=(uint64_t)(result&0x7fffffffu)<<1;if(address<a->reply||address>=a->reply+2048||((address-a->reply)&127))return fail92(a,B_IO);
 uint8_t*reply=(void*)(uintptr_t)address;unsigned context=get32_92(reply+8),status=(get32_92(reply+12)>>16)&0x7fff,transferred=get32_92(reply+20);*scsi=reply[12];a->regs[17]=(uint32_t)address;(void)a->regs[17];
 if(context!=token)return fail92(a,B_IO);if(status==0x43)return B_ABSENT;if(status&&status!=0x45)return B_IO;if(!*scsi&&transferred!=bytes)return B_IO;return B_OK;
}
static unsigned pv_start92(adapter92*a){
 a->regs[0x2010/4]=0;a->regs[0]=1;
 memset((void*)(uintptr_t)a->state,0,4096);memset((void*)(uintptr_t)a->request,0,4096);memset((void*)(uintptr_t)a->reply,0,4096);
 uint8_t setup[528]={0};put32_92(setup,1);put32_92(setup+4,1);put64_92(setup+8,a->state>>12);put64_92(setup+16,a->request>>12);put64_92(setup+272,a->reply>>12);
 a->regs[0]=3;for(unsigned i=0;i<sizeof(setup);i+=4)a->regs[1]=get32_92(setup+i);
 __asm__ volatile("mfence":::"memory");volatile uint32_t*state=(void*)(uintptr_t)a->state;if(state[2]!=5||state[5]!=7)return B_IO;return B_OK;
}
static unsigned pv_command92(adapter92*a,unsigned target,const uint8_t*cdb,unsigned length,unsigned bytes,unsigned write,unsigned*scsi){
 volatile uint32_t*state=(void*)(uintptr_t)a->state;if(state[2]!=5||state[5]!=7||state[0]-state[1]>=32||state[3]-state[4]>128)return fail92(a,B_IO);
 uint8_t*r=(void*)(uintptr_t)(a->request+(a->producer&31)*128);memset(r,0,128);memset((void*)(uintptr_t)a->sense,0,96);unsigned token=++a->sequence;if(!token)return fail92(a,B_IO);
 put64_92(r,token);put64_92(r+8,a->data);put64_92(r+16,bytes);put64_92(r+24,a->sense);put32_92(r+32,96);put32_92(r+36,bytes?(write?16:8):4);memcpy(r+40,cdb,length);r[56]=(uint8_t)length;r[65]=0x20;r[67]=(uint8_t)target;
 __asm__ volatile("mfence":::"memory");state[0]=++a->producer;__asm__ volatile("mfence":::"memory");a->regs[0x3014/4]=0;
 uint64_t until=m3_clock_ticks()+100;unsigned spin=0;while(state[3]==a->consumer){if(m3_clock_ticks()>=until||(!a->irq&&++spin==10000000))return fail92(a,B_TIMEOUT);idle92(a);}
 __asm__ volatile("lfence":::"memory");if(state[3]-a->consumer!=1)return fail92(a,B_IO);uint8_t*reply=(void*)(uintptr_t)(a->reply+(a->consumer&127)*32);uint64_t context,transferred;memcpy(&context,reply,8);memcpy(&transferred,reply+8,8);unsigned status=get32_92(reply+20);*scsi=status>>16;
 state[4]=++a->consumer;a->regs[0x100c/4]=3;if(context!=token)return fail92(a,B_IO);if((status&65535)==0x11)return B_ABSENT;if((status&65535)&&((status&65535)!=0x0c))return B_IO;if(!*scsi&&transferred!=bytes)return B_IO;return B_OK;
}
static unsigned buslogic_wait92(adapter92*a,unsigned offset,unsigned mask,unsigned value){uint64_t end=m3_clock_ticks()+200;for(unsigned n=0;n<20000000;++n){if((in8(a->io+offset)&mask)==value)return B_OK;if(m3_clock_ticks()>=end)break;__asm__ volatile("pause");}return B_TIMEOUT;}
static unsigned buslogic_start92(adapter92*a){
 out8(a->io,0x40);if(buslogic_wait92(a,0,0xd0,0x10))return B_TIMEOUT;out8(a->io,0x20);
 if(buslogic_wait92(a,0,8,0))return B_TIMEOUT;out8(a->io+1,0x81);uint8_t args[5]={1};put32_92(args+1,(uint32_t)a->state);
 for(unsigned i=0;i<5;++i){if(buslogic_wait92(a,0,8,0))return B_TIMEOUT;out8(a->io+1,args[i]);}
 if(buslogic_wait92(a,2,4,4))return B_TIMEOUT;unsigned bad=in8(a->io)&1;out8(a->io,0x20);return bad?B_UNSUPPORTED:B_OK;
}
static unsigned buslogic_command92(adapter92*a,unsigned target,const uint8_t*cdb,unsigned length,unsigned bytes,unsigned write,unsigned*scsi){
 if(length>12)return B_UNSUPPORTED;uint8_t*r=(void*)(uintptr_t)a->request;memset(r,0,64);memset((void*)(uintptr_t)a->sense,0,96);r[1]=(uint8_t)(bytes?(write?16:8):24);r[2]=(uint8_t)length;r[3]=96;put32_92(r+4,bytes);put32_92(r+8,(uint32_t)a->data);r[16]=(uint8_t)target;memcpy(r+18,cdb,length);put32_92(r+36,(uint32_t)a->sense);
 volatile uint8_t*box=(void*)(uintptr_t)a->state;if(box[7]||box[15])return fail92(a,B_IO);put32_92((void*)box,(uint32_t)a->request);__asm__ volatile("mfence":::"memory");box[7]=1;__asm__ volatile("mfence":::"memory");out8(a->io+1,2);
 uint64_t end=m3_clock_ticks()+200;unsigned spins=0;while(!box[15]){if(m3_clock_ticks()>=end||++spins==20000000)return fail92(a,B_TIMEOUT);__asm__ volatile("pause");}
 __asm__ volatile("lfence":::"memory");unsigned address=get32_92((const void*)(box+8)),host=box[12],completion=box[15];*scsi=box[13];box[15]=0;out8(a->io,0x20);
 if(address!=(uint32_t)a->request||completion>4)return fail92(a,B_IO);if(host==0x11)return B_ABSENT;return host?B_IO:B_OK;
}
static unsigned command92(adapter92*a,unsigned target,const uint8_t*cdb,unsigned length,void*buffer,unsigned bytes,unsigned write){
 if(a->dead)return B_IO;if(length>16||!length||bytes>4096||(bytes&&!buffer))return B_DENIED;
 if(bytes){if(write)memcpy((void*)(uintptr_t)a->data,buffer,bytes);else memset((void*)(uintptr_t)a->data,0,bytes);}
 uint64_t flags=0;uint8_t mask=0;if(a->irq){__asm__ volatile("pushfq;popq %0;cli":"=r"(flags)::"memory");mask=in8(0x21);waiting92=1;out8(0x21,mask&~1u);}
 unsigned status=0,scsi=0;for(unsigned attempt=0;attempt<3;++attempt){++a->commands;status=a->kind==1?mpi_command92(a,target,cdb,length,bytes,write,&scsi):a->kind==2?pv_command92(a,target,cdb,length,bytes,write,&scsi):buslogic_command92(a,target,cdb,length,bytes,write,&scsi);if(status||!scsi)break;uint8_t*sense=(void*)(uintptr_t)a->sense;if(scsi!=2||(sense[2]&15)!=6)break;}
 if(a->irq){out8(0x21,mask);waiting92=0;if(flags&512)__asm__ volatile("sti":::"memory");}
 if(status)return status;if(scsi)return B_IO;if(bytes&&!write)memcpy(buffer,(void*)(uintptr_t)a->data,bytes);return B_OK;
}
static unsigned transfer92(unsigned index,uint64_t lba,void*buffer,unsigned bytes,unsigned write){
 if(index>=disk_count92||!buffer||!bytes||bytes>4096)return B_DENIED;disk92*d=&disks92[index];if(bytes%d->bytes||lba>=d->sectors||bytes/d->bytes>d->sectors-lba||lba>UINT32_MAX)return B_DENIED;
 uint8_t cdb[10]={0};cdb[0]=write?0x2a:0x28;beout92(cdb+2,(uint32_t)lba);unsigned sectors=bytes/d->bytes;cdb[7]=(uint8_t)(sectors>>8);cdb[8]=(uint8_t)sectors;return command92(&adapters92[d->adapter],d->target,cdb,10,buffer,bytes,write);
}
static unsigned read92(unsigned index,uint64_t lba,void*buffer){return index<disk_count92?transfer92(index,lba,buffer,disks92[index].bytes,0):B_DENIED;}
static unsigned flush92(unsigned index){if(index>=disk_count92)return B_DENIED;disk92*d=&disks92[index];uint8_t cdb[10]={0x35};return command92(&adapters92[d->adapter],d->target,cdb,10,0,0,0);}
static unsigned bind92(unsigned bdf,unsigned kind){
 if(adapter_count92==ADAPTERS92)return B_DENIED;unsigned ci=adapter_count92;adapter92*a=&adapters92[ci];a->bdf=bdf;a->kind=kind;
 if(kind==3){for(unsigned bar=0;bar<6;++bar){unsigned value=pci92(bdf,16+bar*4);if((value&1)&&(value&~3u)&&((value&~3u)<65532)){a->io=(uint16_t)(value&~3u);break;}}if(!a->io)return B_UNSUPPORTED;}
 else{for(unsigned bar=0;bar<6;++bar){uint32_t low=pci92(bdf,16+bar*4);if(!low||low==UINT32_MAX||(low&1))continue;a->bar=bar;a->regs=map92(bdf,bar,32+ci);break;}if(!a->regs)return B_UNSUPPORTED;}
 uint64_t*pages[]={&a->request,&a->reply,&a->data,&a->sense,&a->state};for(unsigned i=0;i<5;++i)if(!(*pages[i]=page_alloc()))return fail92(a,B_IO);
 ++adapter_count92;setpci92(bdf,4,(pci92(bdf,4)&65535)|0x407);unsigned status=kind==1?mpi_start92(a):kind==2?pv_start92(a):buslogic_start92(a);if(status)return fail92(a,status);arm92(a,ci);
 for(unsigned target=0;target<16&&disk_count92<STORAGE_MAX&&storage_count()<STORAGE_MAX;++target){uint8_t inquiry[36],cdb[16]={0x12,0,0,0,36};status=command92(a,target,cdb,6,inquiry,36,0);if(status||((inquiry[0]&31)!=0))continue;
  uint8_t capacity[8];memset(cdb,0,sizeof(cdb));cdb[0]=0x25;if(command92(a,target,cdb,10,capacity,8,0))continue;uint32_t last=be32_92(capacity),bytes=be32_92(capacity+4);if((bytes!=512&&bytes!=4096)||last==UINT32_MAX)continue;
  unsigned index=disk_count92++;disks92[index]=(disk92){ci,target,bytes,(uint64_t)last+1};storage_disk disk={STORAGE_SCSI,bdf,target,bytes,1,0,(uint64_t)last+1,index,0};(void)storage_register_io(disk,read92,transfer92,flush92);
  serial_text("M92_SCSI_DISK_READY bdf=");serial_hex(bdf);serial_text(" kind=");serial_hex(kind);serial_text(" target=");serial_hex(target);serial_text(" sectors=");serial_hex(disk.sectors);serial_text("\n");
 }
 serial_text("M92_SCSI_STATS bdf=");serial_hex(bdf);serial_text(" irq=");serial_hex(a->irq);serial_text(" commands=");serial_hex(a->commands);serial_text(" interrupts=");serial_hex(a->interrupts);serial_text(" wait_halts=");serial_hex(a->halts);serial_text("\n");
 return B_OK;
}
unsigned scsi92_discover(void){
 for(unsigned bus=0;bus<256;++bus)for(unsigned dev=0;dev<32;++dev){unsigned first=(bus<<8)|(dev<<3);if((pci92(first,0)&65535)==65535)continue;unsigned functions=pci92(first,12)&0x800000?8:1;
  for(unsigned f=0;f<functions;++f){unsigned bdf=first|f,id=pci92(bdf,0),kind=(id==0x00301000||id==0x00541000||id==0x00581000)?1:id==0x07c015ad?2:id==0x1040104b?3:0;if(!kind)continue;
   unsigned status=bind92(bdf,kind);if(status){serial_text("M92_SCSI_BIND_FAILED id=");serial_hex(id);serial_text(" status=");serial_hex(status);serial_text("\n");}
  }
 }
 return B_OK;
}
