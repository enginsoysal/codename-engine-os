#include <engine/storage.h>
typedef struct {uint16_t io,control;unsigned slave,bytes,lba48;uint64_t sectors;} ide_disk;
static ide_disk disks[8];static unsigned disk_count;static uint16_t channels[8];static unsigned channel_count;
static uint32_t pci(unsigned bdf,unsigned reg){uint32_t v,a=0x80000000u|(bdf<<8)|(reg&252);__asm__ volatile("outl %0,%1"::"a"(a),"Nd"((uint16_t)0xcf8));__asm__ volatile("inl %1,%0":"=a"(v):"Nd"((uint16_t)0xcfc));return v;}
static void pci_set(unsigned bdf,unsigned reg,unsigned value){uint32_t a=0x80000000u|(bdf<<8)|(reg&252);__asm__ volatile("outl %0,%1"::"a"(a),"Nd"((uint16_t)0xcf8));__asm__ volatile("outl %0,%1"::"a"(value),"Nd"((uint16_t)0xcfc));}
static uint16_t word(uint16_t p){uint16_t value;__asm__ volatile("inw %1,%0":"=a"(value):"Nd"(p));return value;}
static void delay(uint16_t control){for(unsigned i=0;i<4;++i)(void)in8(control);}
static unsigned wait(ide_disk *d,unsigned data){uint64_t end=m3_clock_ticks()+100;unsigned spins=0;for(;;){unsigned status=in8(d->control);if(!status||status==255)return B_ABSENT;if(!(status&128)){if(status&0x21)return B_IO;if(!data||(status&8))return B_OK;}if(m3_clock_ticks()>=end||++spins==10000000)return B_TIMEOUT;__asm__ volatile("pause");}}
static void transfer(ide_disk *d,void *buffer,unsigned bytes){uint8_t *p=buffer;for(unsigned i=0;i<bytes;i+=2){uint16_t v=word(d->io);p[i]=(uint8_t)v;p[i+1]=(uint8_t)(v>>8);}delay(d->control);(void)in8(d->io+7);}
static unsigned read_disk(unsigned context,uint64_t lba,void *buffer){
    if(context>=disk_count)return B_DENIED;ide_disk *d=&disks[context];if(lba>=d->sectors)return B_DENIED;out8(d->control,2);out8(d->io+6,(uint8_t)(0xe0|(d->slave<<4)|(d->lba48?0:((lba>>24)&15))));delay(d->control);unsigned s=wait(d,0);if(s)return s;
    if(d->lba48){out8(d->io+2,0);out8(d->io+3,(uint8_t)(lba>>24));out8(d->io+4,(uint8_t)(lba>>32));out8(d->io+5,(uint8_t)(lba>>40));}out8(d->io+2,1);out8(d->io+3,(uint8_t)lba);out8(d->io+4,(uint8_t)(lba>>8));out8(d->io+5,(uint8_t)(lba>>16));out8(d->io+7,d->lba48?0x24:0x20);delay(d->control);if((s=wait(d,1)))return s;transfer(d,buffer,d->bytes);return wait(d,0);
}

#ifdef ENGINE_M92
static unsigned io92(unsigned context,uint64_t lba,void*buffer,unsigned bytes,unsigned write){
 if(context>=disk_count||!buffer||!bytes||bytes>4096)return B_DENIED;ide_disk*d=&disks[context];if(bytes%d->bytes||lba>=d->sectors||bytes/d->bytes>d->sectors-lba)return B_DENIED;
 if(!write){uint8_t*p=buffer;while(bytes){unsigned s=read_disk(context,lba++,p);if(s)return s;p+=d->bytes;bytes-=d->bytes;}return B_OK;}
 out8(d->control,2);out8(d->io+6,(uint8_t)(0xe0|(d->slave<<4)|(d->lba48?0:((lba>>24)&15))));delay(d->control);unsigned status=wait(d,0);if(status)return status;
 unsigned sectors=bytes/d->bytes;if(d->lba48){out8(d->io+2,0);out8(d->io+3,(uint8_t)(lba>>24));out8(d->io+4,(uint8_t)(lba>>32));out8(d->io+5,(uint8_t)(lba>>40));}
 out8(d->io+2,(uint8_t)sectors);out8(d->io+3,(uint8_t)lba);out8(d->io+4,(uint8_t)(lba>>8));out8(d->io+5,(uint8_t)(lba>>16));out8(d->io+7,d->lba48?0x34:0x30);delay(d->control);
 const uint8_t*p=buffer;for(unsigned sector=0;sector<sectors;++sector){if((status=wait(d,1)))return status;for(unsigned i=0;i<d->bytes;i+=2){uint16_t value=(uint16_t)p[i]|((uint16_t)p[i+1]<<8);__asm__ volatile("outw %0,%1"::"a"(value),"Nd"(d->io));}p+=d->bytes;delay(d->control);}
 (void)in8(d->io+7);return wait(d,0);
}
static unsigned flush92(unsigned context){if(context>=disk_count)return B_DENIED;ide_disk*d=&disks[context];out8(d->control,2);out8(d->io+6,(uint8_t)(0xe0|(d->slave<<4)));delay(d->control);unsigned s=wait(d,0);if(s)return s;out8(d->io+7,d->lba48?0xea:0xe7);delay(d->control);return wait(d,0);}
#endif

static void channel(unsigned bdf,uint16_t io,uint16_t control,unsigned channel_number){
    if(!io||!control||channel_count==8)return;for(unsigned i=0;i<channel_count;++i)if(channels[i]==io)return;channels[channel_count++]=io;out8(control,6);delay(control);out8(control,2);delay(control);
    for(unsigned slave=0;slave<2&&disk_count<8&&storage_count()<STORAGE_MAX;++slave){ide_disk d={io,control,slave,512,0,0};out8(io+6,(uint8_t)(0xa0|(slave<<4)));delay(control);unsigned status=in8(io+7);if(!status||status==255)continue;
        out8(io+2,0);out8(io+3,0);out8(io+4,0);out8(io+5,0);out8(io+7,0xec);delay(control);if(wait(&d,1))continue;uint16_t id[256];transfer(&d,id,512);
        d.lba48=!!(id[83]&(1u<<10));d.sectors=d.lba48?((uint64_t)id[100]|((uint64_t)id[101]<<16)|((uint64_t)id[102]<<32)|((uint64_t)id[103]<<48)):((uint64_t)id[60]|((uint64_t)id[61]<<16));
        if((id[106]&0xd000)==0x5000){uint64_t words=(uint64_t)id[117]|((uint64_t)id[118]<<16);d.bytes=words<=UINT32_MAX/2?(unsigned)words*2:0;}
        unsigned readable=(id[49]&(1u<<9))&&d.sectors&&d.sectors<=(d.lba48?UINT64_C(0x1000000000000):UINT64_C(0x10000000))&&(d.bytes==512||d.bytes==4096);unsigned n=disk_count++;disks[n]=d;storage_disk item={STORAGE_IDE,bdf,channel_number*2+slave,d.bytes,readable,0,d.sectors,n,0};
#ifdef ENGINE_M92
        (void)storage_register_io(item,read_disk,io92,flush92);
#else
        (void)storage_register(item,read_disk);
#endif
    }
}
unsigned ide_discover(void){
    for(unsigned bus=0;bus<256;++bus)for(unsigned dev=0;dev<32;++dev){unsigned first=(bus<<8)|(dev<<3);if((pci(first,0)&65535)==65535)continue;unsigned functions=pci(first,12)&0x800000?8:1;for(unsigned f=0;f<functions;++f){unsigned bdf=first|f,cls=pci(bdf,8)>>8;if((cls>>8)!=0x0101)continue;pci_set(bdf,4,((pci(bdf,4)&65535)|1)&~4u);
        for(unsigned ch=0;ch<2;++ch){unsigned native=cls&(1u<<(ch*2));uint32_t bar=pci(bdf,16+ch*8),ctl=pci(bdf,20+ch*8);if(native&&(!(bar&1)||!(ctl&1)||(bar&~3u)>65528||(ctl&~3u)>65532))continue;uint16_t io=native?(uint16_t)(bar&~3u):(uint16_t)(ch?0x170:0x1f0),control=native?(uint16_t)((ctl&~3u)+2):(uint16_t)(ch?0x376:0x3f6);channel(bdf,io,control,ch);}
    }}
    return B_OK;
}
