#include <engine/hardware10.h>
#include <engine/ui/graphics9.h>
#include <engine/network/nic.h>
/* A fixed inventory, no allocation or periodic rescanning. Refresh observes
 * actual providers; discovery never authorizes an I/O operation. */
static hw10_device inventory[128];static unsigned count,initialized;
static uint32_t pci10(unsigned bdf,unsigned reg){uint32_t value,address=0x80000000u|(bdf<<8)|(reg&252);__asm__ volatile("outl %0,%1"::"a"(address),"Nd"((uint16_t)0xcf8));__asm__ volatile("inl %1,%0":"=a"(value):"Nd"((uint16_t)0xcfc));return value;}
void hw10_refresh(void){
 if(!initialized){for(unsigned bus=0;bus<256&&count<128;++bus)for(unsigned dev=0;dev<32&&count<128;++dev){unsigned first=(bus<<8)|(dev<<3);if((pci10(first,0)&65535)==65535)continue;unsigned functions=pci10(first,12)&0x800000?8:1;for(unsigned f=0;f<functions&&count<128;++f){unsigned bdf=first|f,id=pci10(bdf,0);if((id&65535)==65535)continue;inventory[count++]=(hw10_device){bdf,id&65535,id>>16,pci10(bdf,8)>>8,0,0,"PCI device","Discovered; no active function is claimed."};}}initialized=1;}
 usb10_info usb=usb10_current();net7_nic_info nic=net7_nic_status();const graphics9_info*g=graphics9_current();
 for(unsigned i=0;i<count;++i){hw10_device*d=&inventory[i];unsigned cls=d->class_code>>16;d->state=HW10_DISCOVERED;d->capabilities=0;d->name=cls==1?"Storage controller":cls==2?"Network adapter":cls==3?"Display adapter":cls==4?"Audio controller":cls==6?"Platform bridge":d->class_code==0x0c0330?"USB xHCI controller":"PCI device";d->detail=cls==6?"Platform infrastructure; not an application device.":"No usable function has been verified by a provider.";
  if(cls==4){d->state=HW10_DISCOVERED;d->detail="Audio is opened on demand; no active playback claim.";}
  if(g&&g->adapter_bdf==d->bdf&&g->adapter_id){d->state=HW10_LIMITED;d->capabilities=HW10_DISPLAY;d->detail=g->backend?"VMware 2D presentation; no 3D acceleration.":"Firmware framebuffer; software 2D fallback.";}
  if(nic.present&&nic.bdf==d->bdf){d->state=nic.online?HW10_READY:HW10_LIMITED;d->capabilities=HW10_NETWORK;d->detail=nic.online?"Ethernet link available; connection status is shown in Downloads.":"Driver bound; Ethernet link is unavailable.";}
  if(usb.active&&usb.bdf==d->bdf){d->state=HW10_READY;d->capabilities=HW10_INPUT;d->detail="Supported HID, hub and storage protocols bind automatically.";}
  for(unsigned n=0;n<storage_count();++n){const storage_disk*s=storage_get(n);if(s->bdf!=d->bdf)continue;if(s->readable){d->state=HW10_READY;d->capabilities|=HW10_READ;d->detail="Native storage reads available.";if(storage_writable(s)){d->capabilities|=HW10_WRITE|HW10_FLUSH;d->detail="Native read/write/flush available.";}}else if(!d->capabilities){d->state=HW10_UNAVAILABLE;d->detail="No accessible disk; disconnected, refused or I/O failed.";}}
 }
}
unsigned hw10_count(void){return count;}
const hw10_device *hw10_get(unsigned index){return index<count?&inventory[index]:0;}
