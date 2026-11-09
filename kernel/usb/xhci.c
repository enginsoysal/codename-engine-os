#include <engine/storage.h>
typedef struct {volatile uint64_t parameter;volatile uint32_t status,control;} trb;
typedef struct {trb *base;unsigned index,cycle;} ring;
typedef struct {unsigned slot,port,kind,endpoint,packet,interval,parent,hub_port,route,depth,speed,hub_ports,rejected_children,tt_slot,tt_port,protocol;uint64_t input,output,buffer;ring control,interrupt,bulk_out,uas_status;uint64_t data,sectors;unsigned out_endpoint,bytes,tag,dead,storage_index,generation,uas,status_endpoint,uas_command_complete,uas_abort_data_stopped;uint64_t reads,wait_halts;uint8_t last[8];} device;
#define USB_MAX_DEVICES 8
static device devices[USB_MAX_DEVICES];static unsigned device_count,active,context_size,event_index,event_cycle=1,mouse_dirty,mouse_buttons;static int mouse_x=600,mouse_y=80;
static int mouse_limit_x=1200,mouse_limit_y=650;
#ifdef ENGINE_M9
static unsigned mouse_width9=1215,mouse_height9=674;
#endif
#ifdef ENGINE_M6
static unsigned mouse_press_pending,mouse_press_x,mouse_press_y;
#endif
static uint64_t mouse_events,key_events,irq_count,dma_pages;
static volatile uint32_t *op,*runtime,*doorbell;static uint64_t *dcbaa;static trb *events;static ring commands;
static unsigned controller_selected,initialization_status=B_ABSENT;
static unsigned transfer_remaining,bulk_waiting,controller_bdf,port_count,primary_streams;static unsigned pending_ports,pending_hubs,rejected_ports;
static uint64_t uas_abort_wait;static unsigned uas_abort_done,uas_abort_remaining;
static uint64_t uas_status_wait,uas_data_wait;static unsigned uas_status_done,uas_data_done,uas_status_remaining,uas_data_remaining;
static uint64_t command_wait,transfer_wait;static unsigned command_done,transfer_done,completion_slot;
static inline uint32_t pci(unsigned bdf,unsigned reg){uint32_t v;uint32_t a=0x80000000u|(bdf<<8)|(reg&252);__asm__ volatile("outl %0,%1"::"a"(a),"Nd"((uint16_t)0xcf8));__asm__ volatile("inl %1,%0":"=a"(v):"Nd"((uint16_t)0xcfc));return v;}
static void pci_set(unsigned bdf,unsigned reg,uint32_t v){uint32_t a=0x80000000u|(bdf<<8)|(reg&252);__asm__ volatile("outl %0,%1"::"a"(a),"Nd"((uint16_t)0xcf8));__asm__ volatile("outl %0,%1"::"a"(v),"Nd"((uint16_t)0xcfc));}
static uint64_t page(void){uint64_t p=page_alloc();if(p)++dma_pages;return p;}
static int wait_clear(volatile uint32_t *r,unsigned bits){uint64_t end=m3_clock_ticks()+100;unsigned spins=0;while(*r&bits){if(m3_clock_ticks()>end||++spins==10000000)return 0;__asm__ volatile("pause");}return 1;}
static int wait_set(volatile uint32_t *r,unsigned bits){uint64_t end=m3_clock_ticks()+100;unsigned spins=0;while((*r&bits)!=bits){if(m3_clock_ticks()>end||++spins==10000000)return 0;__asm__ volatile("pause");}return 1;}
static void write64(volatile uint32_t *r,uint64_t value){r[0]=(uint32_t)value;r[1]=(uint32_t)(value>>32);}
static int ring_init(ring *r){r->base=(trb*)(uintptr_t)page();r->cycle=1;return !!r->base;}
static uint64_t push(ring *r,uint64_t parameter,uint32_t status,uint32_t control){
    if(r->index==255){trb *link=&r->base[255];link->parameter=(uint64_t)(uintptr_t)r->base;link->status=0;__asm__ volatile("mfence":::"memory");link->control=(6u<<10)|2|r->cycle;r->index=0;r->cycle^=1;}
    trb *t=&r->base[r->index++];t->parameter=parameter;t->status=status;__asm__ volatile("mfence":::"memory");t->control=control|r->cycle;return (uint64_t)(uintptr_t)t;
}
static char key(unsigned code,unsigned shifted){
    if(code>=4&&code<=29)return (char)((shifted?'A':'a')+code-4);
    if(code>=30&&code<=39)return (shifted?"!@#$%^&*()":"1234567890")[code-30];
#ifdef ENGINE_M7
    if(code==41)return 27;
    if(code==80)return (char)128;if(code==79)return (char)129;if(code==74)return (char)130;
    if(code==77)return (char)131;if(code==76)return (char)132;if(code==82)return (char)133;if(code==81)return (char)134;
#endif
    if(code==40)return 10;if(code==42)return 8;if(code==43)return 9;if(code==44)return ' ';
    if(code>=45&&code<=56)return (shifted?"_+{}|~:\"~<>?":"-=[]\\#;'`,./")[code-45];return 0;
}
static void submit_input(device *d){push(&d->interrupt,d->buffer+512,d->packet,(1u<<10)|(1u<<5)|(1u<<2));__asm__ volatile("mfence":::"memory");doorbell[d->slot]=d->endpoint;}
#ifdef ENGINE_M6
void m6_pointer_report(int dx,int dy,unsigned buttons){
    mouse_x+=dx;mouse_y+=dy;if(mouse_x<0)mouse_x=0;if(mouse_y<0)mouse_y=0;if(mouse_x>mouse_limit_x)mouse_x=mouse_limit_x;if(mouse_y>mouse_limit_y)mouse_y=mouse_limit_y;
    unsigned old_buttons=mouse_buttons;mouse_buttons=buttons&7;
    if((mouse_buttons&1)&&!(old_buttons&1)&&!mouse_press_pending){mouse_press_x=(unsigned)mouse_x;mouse_press_y=(unsigned)mouse_y;mouse_press_pending=1;}
    ++mouse_events;__atomic_store_n(&mouse_dirty,1,__ATOMIC_RELEASE);
}
#ifdef ENGINE_M9
void m9_pointer_absolute(unsigned x,unsigned y,unsigned buttons){
 int target_x=(int)((uint64_t)x*(mouse_width9-1)/65535),target_y=(int)((uint64_t)y*(mouse_height9-1)/65535);
 m6_pointer_report(target_x-mouse_x,target_y-mouse_y,buttons);
}
#endif
#endif
static void input_complete(device *d,unsigned bytes){
    uint8_t *data=(uint8_t*)(uintptr_t)(d->buffer+512);if(d->kind==9){for(unsigned i=0;i<bytes;++i)if(data[i]){__atomic_fetch_or(&pending_hubs,1u<<(unsigned)(d-devices),__ATOMIC_RELEASE);break;}return;}
    if(d->kind==1&&bytes>=8){for(unsigned i=2;i<8;++i)if(data[i]>3){unsigned old=0;for(unsigned j=2;j<8;++j)old|=d->last[j]==data[i];if(!old){char c=key(data[i],data[0]&0x22);
#ifdef ENGINE_M7
if((data[0]&0x11)&&data[i]>=4&&data[i]<=29)c=(char)(data[i]-3);
if(data[i]==43)c=(char)((data[0]&0x44)?135:(data[0]&0x22)?136:9);
if(data[i]==67)c=(char)137;
#endif
if(c){input_feed((uint8_t)c);++key_events;}}}memcpy(d->last,data,8);}
    if(d->kind==2&&bytes>=3){
#ifdef ENGINE_M6
        m6_pointer_report((int8_t)data[1],(int8_t)data[2],data[0]&7);
#else
        mouse_x+=(int8_t)data[1];mouse_y+=(int8_t)data[2];if(mouse_x<0)mouse_x=0;if(mouse_y<0)mouse_y=0;if(mouse_x>mouse_limit_x)mouse_x=mouse_limit_x;if(mouse_y>mouse_limit_y)mouse_y=mouse_limit_y;mouse_buttons=data[0]&7;++mouse_events;__atomic_store_n(&mouse_dirty,1,__ATOMIC_RELEASE);
#endif
    }
}
static void consume(void){
    for(unsigned bounded=0;bounded<256;++bounded){trb *e=&events[event_index];uint32_t ctrl=e->control;if((ctrl&1)!=event_cycle)break;__asm__ volatile("lfence":::"memory");uint64_t p=e->parameter;unsigned status=e->status,kind=(ctrl>>10)&63,cc=status>>24,slot=ctrl>>24;
        if(kind==33&&p==command_wait){command_done=cc;completion_slot=slot;}
        if(kind==32){if(uas_abort_wait&&p==uas_abort_wait){uas_abort_done=cc;uas_abort_remaining=status&0xffffff;}else if(uas_status_wait&&p==uas_status_wait){uas_status_done=cc;uas_status_remaining=status&0xffffff;}else if(uas_data_wait&&p==uas_data_wait){uas_data_done=cc;uas_data_remaining=status&0xffffff;}else if(transfer_wait&&p==transfer_wait){transfer_done=cc;transfer_remaining=status&0xffffff;}else for(unsigned i=0;i<device_count;++i){device *d=&devices[i];if(!d->dead&&d->kind!=3&&d->slot==slot&&((ctrl>>16)&31)==d->endpoint){if((cc==1||cc==13)&&(status&0xffffff)<=d->packet){input_complete(d,d->packet-(status&0xffffff));submit_input(d);}else{serial_text("M4_USB_ENDPOINT_ERROR\n");}break;}}}
        if(kind==34){unsigned n=(unsigned)(p>>24)&255;if(n&&n<=port_count&&n<=32){__atomic_fetch_or(&pending_ports,1u<<(n-1),__ATOMIC_RELEASE);volatile uint32_t *port=op+0x100+(n-1)*4;unsigned state=*port;if(state&(1u<<17))rejected_ports&=~(1u<<(n-1));*port=(state&0x200)|(state&0xfe0000);}}
        if(++event_index==256){event_index=0;event_cycle^=1;}
    }
    write64(runtime+14,(uint64_t)(uintptr_t)&events[event_index]|8);runtime[8]=3;op[1]=8;
}
static unsigned command_ex(uint64_t pointer,uint32_t status,uint32_t flags){command_done=0;command_wait=push(&commands,pointer,status,flags);__asm__ volatile("mfence":::"memory");doorbell[0]=0;uint64_t end=m3_clock_ticks()+100;unsigned spins=0;while(!command_done){consume();if(m3_clock_ticks()>end||++spins==10000000)return 0;__asm__ volatile("pause");}return command_done==1;}
static unsigned command(uint64_t pointer,uint32_t flags){return command_ex(pointer,0,flags);}
static unsigned control(device *d,unsigned type,unsigned request,unsigned value,unsigned index,unsigned size){
    if((type&128)&&size)memset((void*)(uintptr_t)d->buffer,0,size);
    uint64_t setup=(uint64_t)type|((uint64_t)request<<8)|((uint64_t)value<<16)|((uint64_t)index<<32)|((uint64_t)size<<48);
    push(&d->control,setup,8,(2u<<10)|(1u<<6)|((size?(type&128?3u:2u):0u)<<16));
    if(size)push(&d->control,d->buffer,size,(3u<<10)|((type&128)?1u<<16:0));
    transfer_done=0;transfer_wait=push(&d->control,0,0,(4u<<10)|(1u<<5)|((!size||!(type&128))?1u<<16:0));
    __asm__ volatile("mfence":::"memory");doorbell[d->slot]=1;uint64_t end=m3_clock_ticks()+100;unsigned spins=0;while(!transfer_done){consume();if(m3_clock_ticks()>end||++spins==10000000)return 0;__asm__ volatile("pause");}return transfer_done==1||transfer_done==13;
}
static uint32_t *context(uint64_t address,unsigned n){return (uint32_t*)(uintptr_t)(address+(uint64_t)n*context_size);}
static unsigned mass_configure(device*,unsigned,unsigned,unsigned,unsigned);
static unsigned mass_probe(unsigned);
static unsigned uas_configure(device*,const unsigned*,const unsigned*,const unsigned*);
static unsigned uas_scsi(device*,const uint8_t*,unsigned,unsigned);
static void release_device(unsigned);
static unsigned hub_prepare(device*);
static unsigned hub_scan(unsigned);
static unsigned enumerate_path(unsigned port_number,unsigned index,unsigned parent,unsigned hub_port,unsigned child_speed){
    device *d=&devices[index];d->storage_index=STORAGE_MAX;unsigned speed=child_speed;d->parent=parent;d->hub_port=hub_port;
    if(parent==USB_MAX_DEVICES){volatile uint32_t *port=op+0x100+port_number*4;unsigned state=*port;speed=(state>>10)&15;if(!(state&1))return 0;*port=(state&0x200)|16|(0x7fu<<17);if(!wait_clear(port,16)||!(*port&2))return 0;}
    else{device *hub=&devices[parent];if(hub->depth>=5||!hub_port||hub_port>15)return 0;d->route=hub->route|(hub_port<<(hub->depth*4));d->depth=hub->depth+1;if(speed<=2){if(hub->speed==3){d->tt_slot=hub->slot;d->tt_port=hub_port;}else{d->tt_slot=hub->tt_slot;d->tt_port=hub->tt_port;}}}d->speed=speed;
    if(!command(0,9u<<10))return 0;d->slot=completion_slot;if(index>=device_count)device_count=index+1;d->port=port_number+1;d->input=page();d->output=page();d->buffer=page();if(!d->input||!d->output||!d->buffer||!ring_init(&d->control)||!ring_init(&d->interrupt))return 0;
    dcbaa[d->slot]=d->output;uint32_t *input=context(d->input,0),*slot=context(d->input,1),*ep=context(d->input,2);input[1]=3;slot[0]=d->route|(speed<<20)|(1u<<27);slot[1]=(port_number+1)<<16;slot[2]=d->tt_slot|(d->tt_port<<8);
    unsigned packet=speed==3?64:speed>=4?512:8;ep[1]=(packet<<16)|(4u<<3)|(3u<<1);ep[2]=(uint32_t)(uintptr_t)d->control.base|1;ep[4]=8;
    if(!command(d->input,(11u<<10)|(d->slot<<24)))return 0;
    if(!control(d,128,6,0x100,0,18))return 0;uint8_t *data=(uint8_t*)(uintptr_t)d->buffer;d->protocol=data[6];unsigned actual=data[7];if(speed>=4){if(actual!=9)return 0;actual=512;}else if((speed==2&&actual!=8)||(speed==3&&actual!=64)||(speed==1&&actual!=8&&actual!=16&&actual!=32&&actual!=64))return 0;
    if(actual!=packet){input[1]=2;ep[1]=(actual<<16)|(4u<<3)|(3u<<1);if(!command(d->input,(13u<<10)|(d->slot<<24)))return 0;}
    if(!control(d,128,6,0x200,0,9))return 0;unsigned total=(unsigned)data[2]|((unsigned)data[3]<<8),configuration=data[5];if(total<9||total>256||!control(d,128,6,0x200,0,total))return 0;
    unsigned interface=0,alternate=0,kind=0,endpoint=0,maxpacket=0,interval=0,out_endpoint=0,out_packet=0,uas_address[5]={0},uas_packet[5]={0},uas_stream[5]={0},last_address=0,last_packet=0,last_streams=0;
    for(unsigned off=0;off+2<=total;){unsigned len=data[off],type=data[off+1];if(len<2||len>total-off)return 0;if(type==4&&len>=9){interface=data[off+2];alternate=data[off+3];kind=data[off+5]==3&&data[off+6]==1?data[off+7]:data[off+5]==8&&data[off+6]==6&&data[off+7]==80?3:data[off+5]==8&&data[off+6]==6&&data[off+7]==98?4:data[off+5]==9?9:0;}
        if(kind==4&&type==5&&len>=7){if((data[off+3]&3)!=2)return 0;last_address=data[off+2];last_packet=(unsigned)data[off+4]|((unsigned)data[off+5]<<8);last_streams=0;}
        if(kind==4&&type==0x30&&len>=6){last_streams=data[off+3]&31;for(unsigned i=1;i<=4;++i)if(uas_address[i]==last_address)uas_stream[i]=last_streams;}
        if(kind==4&&type==0x24&&len>=4){unsigned pipe=data[off+2];if(!pipe||pipe>4||uas_address[pipe]||!last_address||!last_packet)return 0;uas_address[pipe]=last_address;uas_packet[pipe]=last_packet;uas_stream[pipe]=last_streams;}
        if(type==5&&len>=7&&kind==3&&(data[off+3]&3)==2){unsigned address=data[off+2],packet=(unsigned)data[off+4]|((unsigned)data[off+5]<<8);if(address&128){endpoint=address&15;maxpacket=packet;}else{out_endpoint=address&15;out_packet=packet;}}
        if(type==5&&len>=7&&(kind==1||kind==2||kind==9)&&(data[off+2]&128)&&(data[off+3]&3)==3){endpoint=data[off+2]&15;maxpacket=(unsigned)data[off+4]|((unsigned)data[off+5]<<8);interval=data[off+6];break;}off+=len;}
    if(kind==4){d->kind=3;d->uas=1;if(!control(d,0,9,configuration,0,0)||(alternate&&!control(d,1,11,alternate,interface,0))||!uas_configure(d,uas_address,uas_packet,uas_stream))return 0;serial_text("M45_USB_UAS_ONLINE speed=");serial_hex(d->speed);serial_text(" streams=");serial_hex(d->speed==4);serial_text("\n");return 1;}
    if(kind==3){if(!endpoint||!out_endpoint||!maxpacket||!out_packet||maxpacket>1024||out_packet>1024||!speed||speed==2||speed>4||(speed==1&&(maxpacket>64||out_packet>64))||(speed==3&&(maxpacket>512||out_packet>512)))return 0;d->kind=3;if(!control(d,0,9,configuration,0,0)||!mass_configure(d,endpoint,out_endpoint,maxpacket,out_packet))return 0;if(index>=device_count)device_count=index+1;return 1;}
    if((kind!=1&&kind!=2&&kind!=9)||!endpoint||!maxpacket||maxpacket>64||!interval||(speed>=3&&interval>16))return 0;d->kind=kind;d->endpoint=endpoint*2+1;d->packet=maxpacket;d->interval=interval;
    if(!control(d,0,9,configuration,0,0)||((kind==1||kind==2)&&!control(d,0x21,11,0,interface,0)))return 0;
    /* SET_IDLE is optional for boot mice; interrupt completion still reports changes. */
    if(d->kind==1&&!control(d,0x21,10,0,interface,0))return 0;
    memset((void*)(uintptr_t)d->input,0,4096);input=context(d->input,0);slot=context(d->input,1);memcpy(slot,(void*)(uintptr_t)d->output,context_size);slot[0]=(slot[0]&~(31u<<27))|(d->endpoint<<27);input[1]=1|(1u<<d->endpoint);
    ep=context(d->input,d->endpoint+1);unsigned exponent=0;if(speed<=2){unsigned n=interval;while(n>1){++exponent;n>>=1;}exponent+=3;}else exponent=interval-1;
    ep[0]=exponent<<16;ep[1]=(maxpacket<<16)|(7u<<3)|(3u<<1);ep[2]=(uint32_t)(uintptr_t)d->interrupt.base|1;ep[4]=(maxpacket<<16)|maxpacket;
    if(!command(d->input,(12u<<10)|(d->slot<<24)))return 0;if(index>=device_count)device_count=index+1;if(kind==9){if(!hub_prepare(d))return 0;serial_text("M45_USB_HUB_ONLINE\n");}else serial_text(kind==1?"M4_USB_KEYBOARD_ONLINE\n":"M4_USB_MOUSE_ONLINE\n");return 1;
}
static unsigned enumerate(unsigned root,unsigned index){return enumerate_path(root,index,USB_MAX_DEVICES,0,0);}
static unsigned usb_start(void){
    unsigned bdf=0;uint64_t base=0;for(unsigned b=0;b<256&&!base;++b)for(unsigned d=0;d<32&&!base;++d){unsigned first=(b<<8)|(d<<3);if((pci(first,0)&65535)==65535)continue;unsigned count=(pci(first,12)&0x800000)?8:1;for(unsigned f=0;f<count;++f)if((pci(first|f,8)>>8)==0x0c0330){bdf=first|f;uint32_t bar=pci(bdf,16);if((bar&1))continue;base=bar&~15u;if((bar&6)==4)base|=(uint64_t)pci(bdf,20)<<32;break;}}
    controller_bdf=bdf;controller_selected=!!base;serial_text("M4_XHCI_BAR=");serial_hex(base);serial_text("\n");if(base<0x10000000||base>UINT64_C(0x000ffffffff00000))return B_UNSUPPORTED;
    uint64_t mapping=page();if(!mapping)return B_IO;uint64_t aligned=base&~UINT64_C(0x1fffff);((uint64_t*)(uintptr_t)mapping)[0]=aligned|UINT64_C(0x800000000000009b);((uint64_t*)(uintptr_t)mapping)[1]=(aligned+0x200000)|UINT64_C(0x800000000000009b);
    ((uint64_t*)(uintptr_t)kernel_stack_branch())[1]=mapping|3;base=UINT64_C(0xffff800040000000)+(base&0x1fffff);
    uint64_t cr3;__asm__ volatile("movq %%cr3,%0;movq %0,%%cr3":"=r"(cr3)::"memory");pci_set(bdf,4,(pci(bdf,4)&65535)|6);
    volatile uint32_t *cap=(volatile uint32_t*)(uintptr_t)base;op=(volatile uint32_t*)(uintptr_t)(base+(cap[0]&255));runtime=(volatile uint32_t*)(uintptr_t)(base+(cap[6]&~31u));doorbell=(volatile uint32_t*)(uintptr_t)(base+(cap[5]&~3u));context_size=(cap[4]&4)?64:32;primary_streams=(cap[4]>>12)&15;
    unsigned ext=(cap[4]>>16)*4;for(unsigned n=0;ext&&n<64;++n){if(ext>0xfff0)return B_UNSUPPORTED;volatile uint32_t *x=(volatile uint32_t*)(uintptr_t)(base+ext);unsigned header=*x;if((header&255)==1){*x=header|(1u<<24);if(!wait_clear(x,1u<<16))return B_TIMEOUT;x[1]=0;}
        unsigned next=(header>>8)&255;if(!next)break;ext+=next*4;}
    op[0]&=~1u;if(!wait_set(op+1,1))return B_TIMEOUT;op[0]=2;if(!wait_clear(op,2)||!wait_clear(op+1,1u<<11))return B_TIMEOUT;
    unsigned cap_pointer=pci(bdf,0x34)&252,msi=0,msix=0;for(unsigned n=0;cap_pointer&&n<48;++n){unsigned h=pci(bdf,cap_pointer);if((h&255)==5)msi=cap_pointer;if((h&255)==17)msix=cap_pointer;cap_pointer=(h>>8)&252;}
    volatile uint32_t *lapic=(volatile uint32_t*)(uintptr_t)0xfee00000;unsigned message=0xfee00000|((lapic[8]>>24)<<12);
    serial_text("M4_XHCI_MSI=");serial_hex(msi);serial_text(" msix=");serial_hex(msix);serial_text("\n");
    if(msix){unsigned table=pci(bdf,msix+4);serial_text("M4_XHCI_MSIX_TABLE=");serial_hex(table);serial_text("\n");if((table&7)||(table&~7u)>0xfff0)return B_UNSUPPORTED;volatile uint32_t *entry=(volatile uint32_t*)(uintptr_t)(base+(table&~7u));entry[3]=1;entry[0]=message;entry[1]=0;entry[2]=145;__asm__ volatile("mfence":::"memory");entry[3]=0;pci_set(bdf,msix,(pci(bdf,msix)|(1u<<31))&~(1u<<30));}
    else if(msi){unsigned m=pci(bdf,msi),wide=m&(1u<<23);pci_set(bdf,msi+4,message);if(wide)pci_set(bdf,msi+8,0);pci_set(bdf,msi+(wide?12:8),145);pci_set(bdf,msi,(m&~(7u<<20))|(1u<<16));}
    else return B_UNSUPPORTED;pci_set(bdf,4,(pci(bdf,4)&65535)|0x406);
    dcbaa=(uint64_t*)(uintptr_t)page();events=(trb*)(uintptr_t)page();uint64_t erst=page();if(!dcbaa||!events||!erst||!ring_init(&commands))return B_IO;
    unsigned scratchpads=((cap[2]>>27)&31)|((cap[2]>>16)&0x3e0);if(scratchpads>8)return B_UNSUPPORTED;if(scratchpads){uint64_t *p=(uint64_t*)(uintptr_t)page();if(!p)return B_IO;dcbaa[0]=(uint64_t)(uintptr_t)p;for(unsigned i=0;i<scratchpads;++i)if(!(p[i]=page()))return B_IO;}
    ((uint64_t*)(uintptr_t)erst)[0]=(uint64_t)(uintptr_t)events;((uint32_t*)(uintptr_t)erst)[2]=256;
    write64(op+12,(uint64_t)(uintptr_t)dcbaa);write64(op+6,(uint64_t)(uintptr_t)commands.base|1);op[14]=cap[1]&255;runtime[10]=1;write64(runtime+12,erst);write64(runtime+14,(uint64_t)(uintptr_t)events);runtime[9]=4000;runtime[8]=1;op[0]=1;
    if(!wait_clear(op+1,1))return B_TIMEOUT;unsigned ports=cap[1]>>24;port_count=ports>32?32:ports;
    for(unsigned i=0,attempts=0;i<port_count&&attempts<2*USB_MAX_DEVICES;++i)if(op[0x100+i*4]&1){++attempts;unsigned before=0;while(before<USB_MAX_DEVICES&&(devices[before].slot||devices[before].generation>=0xffffff))++before;if(before==USB_MAX_DEVICES)break;if(!enumerate(i,before)){rejected_ports|=1u<<i;serial_text("M4_USB_PORT_UNSUPPORTED port=");serial_hex(i+1);serial_text("\n");release_device(before);}}
    for(unsigned i=0;i<device_count;++i)if(devices[i].kind==9)(void)hub_scan(i);
    active=1;for(unsigned i=0;i<device_count;++i)if(devices[i].slot&&!devices[i].dead&&(devices[i].kind==1||devices[i].kind==2||devices[i].kind==9))submit_input(&devices[i]);runtime[8]=3;op[0]=5;for(unsigned i=0;i<device_count;++i)if(devices[i].kind==3)(void)mass_probe(i);unsigned online=0;for(unsigned i=0;i<device_count;++i)online+=devices[i].slot&&!devices[i].dead;return online?B_OK:B_ABSENT;
}
unsigned usb_init(void){initialization_status=usb_start();return initialization_status;}
void usb_hardware_inventory(unsigned bdf){
    if(!controller_selected||bdf!=controller_bdf){console_text("USB controller unbound: one-controller profile or unavailable BAR\n");return;}
    console_text("USB controller initialization status: ");console_decimal(initialization_status);console_text("; IRQ/event engine: ");console_text(active?"active\n":"inactive\n");console_text("USB rejected root-port mask: ");console_hex(rejected_ports);console_text("\n");
    for(unsigned i=0;i<device_count;++i){device *d=&devices[i];if(!d->slot)continue;console_text("USB device ");console_decimal(i);console_text(" root port ");console_decimal(d->port);console_text(" route ");console_hex(d->route);console_text(d->kind==9?" hub":d->kind==1?" keyboard":d->kind==2?" mouse":d->kind==3?(d->uas?" UAS":" BOT"):" unclassified");
        const storage_disk *disk=d->storage_index<STORAGE_MAX?storage_get(d->storage_index):0;console_text(d->dead?" - quarantined; DMA retained\n":d->kind==3?(disk&&disk->readable?" - read-only transfer verified\n":" - storage unavailable\n"):" - driver bound\n");if(d->kind==9){console_text("USB rejected hub-port mask: ");console_hex(d->rejected_children);console_text("\n");}}
}
unsigned usb_irq(uint64_t *frame){if(active&&bulk_waiting&&frame[15]==32){out8(0x20,0x20);return 1;}if(!active||frame[15]!=145)return 0;++irq_count;consume();*(volatile uint32_t*)(uintptr_t)0xfee000b0=0;return 1;}
unsigned usb_mouse_pending(void){return __atomic_load_n(&mouse_dirty,__ATOMIC_ACQUIRE);}
void usb_mouse_draw(void){
#ifndef ENGINE_M6
__atomic_store_n(&mouse_dirty,0,__ATOMIC_RELEASE);
#endif
extern void display_mouse(unsigned,unsigned);display_mouse((unsigned)mouse_x,(unsigned)mouse_y);}
void usb_info(void){unsigned online=0;for(unsigned i=0;i<device_count;++i)online+=devices[i].slot&&!devices[i].dead;console_text("USB devices=");console_hex(online);console_text(" driver pages=");console_hex(dma_pages);console_text(" IRQs=");console_hex(irq_count);console_text("\nUSB keys=");console_hex(key_events);console_text(" mouse events=");console_hex(mouse_events);console_text(" buttons=");console_hex(mouse_buttons);console_text("\n");page_stats memory=pages_stats();console_text("Managed current bytes=");console_hex(memory.current_pages*4096);console_text(" peak bytes=");console_hex(memory.peak_pages*4096);console_text("\n");}

static unsigned mass_configure(device *d,unsigned in,unsigned out,unsigned in_packet,unsigned out_packet){
    d->endpoint=in*2+1;d->out_endpoint=out*2;d->storage_index=STORAGE_MAX;d->data=page();if(!d->data||!ring_init(&d->bulk_out))return 0;
    memset((void*)(uintptr_t)d->input,0,4096);uint32_t *input=context(d->input,0),*slot=context(d->input,1);memcpy(slot,(void*)(uintptr_t)d->output,context_size);unsigned top=d->endpoint>d->out_endpoint?d->endpoint:d->out_endpoint;slot[0]=(slot[0]&~(31u<<27))|(top<<27);input[1]=1|(1u<<d->endpoint)|(1u<<d->out_endpoint);
    for(unsigned i=0;i<2;++i){unsigned endpoint=i?d->out_endpoint:d->endpoint,packet=i?out_packet:in_packet;uint32_t *ep=context(d->input,endpoint+1);ring *r=i?&d->bulk_out:&d->interrupt;ep[1]=(packet<<16)|((i?2u:6u)<<3)|(3u<<1);ep[2]=(uint32_t)(uintptr_t)r->base|1;ep[4]=1024;}
    return command(d->input,(12u<<10)|(d->slot<<24));
}
static unsigned bulk_endpoint(device *d,ring *r,unsigned endpoint,uint64_t address,unsigned length,unsigned allow_short,unsigned *actual){
    uint64_t flags;__asm__ volatile("pushfq;popq %0;cli":"=r"(flags)::"memory");transfer_done=0;transfer_remaining=0;transfer_wait=push(r,address,length,(1u<<10)|(1u<<5)|(1u<<2));bulk_waiting=1;
    uint8_t mask=in8(0x21);out8(0x21,mask&~1u);__asm__ volatile("mfence":::"memory");doorbell[d->slot]=endpoint;uint64_t end=m3_clock_ticks()+200;unsigned spins=0,status=B_OK;
    while(!transfer_done){if(m3_clock_ticks()>=end||++spins==10000000){status=B_TIMEOUT;break;}if(active){++d->wait_halts;__asm__ volatile("sti;hlt;cli":::"memory");}else{consume();__asm__ volatile("pause");}}
    if(!status&&((transfer_done!=1&&transfer_done!=13)||transfer_remaining>length||(!allow_short&&transfer_remaining)))status=B_IO;if(status){serial_text("M45_USB_TRANSFER_ERROR completion=");serial_hex(transfer_done);serial_text(" remaining=");serial_hex(transfer_remaining);serial_text(" expected_bytes=");serial_hex(length);serial_text(" endpoint=");serial_hex(endpoint);serial_text("\n");}transfer_wait=0;bulk_waiting=0;out8(0x21,mask);if(flags&512)__asm__ volatile("sti":::"memory");if(!status&&actual)*actual=length-transfer_remaining;return status;
}
static unsigned bulk(device *d,unsigned input,uint64_t address,unsigned length){return bulk_endpoint(d,input?&d->interrupt:&d->bulk_out,input?d->endpoint:d->out_endpoint,address,length,0,0);}
static void be32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(v>>(24-i*8));}
static void be64(uint8_t *p,uint64_t v){for(unsigned i=0;i<8;++i)p[i]=(uint8_t)(v>>(56-i*8));}
static uint32_t from_be32(const uint8_t *p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static unsigned scsi(device *d,const uint8_t *cdb,unsigned cdb_bytes,unsigned length){
    if(d->dead||!cdb_bytes||cdb_bytes>16||length>4096)return B_DENIED;if(d->uas)return uas_scsi(d,cdb,cdb_bytes,length);uint8_t *cbw=(void*)(uintptr_t)(d->buffer+512),*csw=(void*)(uintptr_t)(d->buffer+576);memset(cbw,0,31);memset(csw,0,13);uint32_t signature=0x43425355,tag=++d->tag;memcpy(cbw,&signature,4);memcpy(cbw+4,&tag,4);memcpy(cbw+8,&length,4);cbw[12]=length?128:0;cbw[14]=(uint8_t)cdb_bytes;memcpy(cbw+15,cdb,cdb_bytes);
    unsigned status=bulk(d,0,d->buffer+512,31);if(!status&&length)status=bulk(d,1,d->data,length);if(!status)status=bulk(d,1,d->buffer+576,13);
    uint32_t actual_signature,actual_tag,residue;memcpy(&actual_signature,csw,4);memcpy(&actual_tag,csw+4,4);memcpy(&residue,csw+8,4);if(!status&&csw[12]){serial_text("M45_USB_SCSI_STATUS=");serial_hex(csw[12]);serial_text(" opcode=");serial_hex(cdb[0]);serial_text("\n");}if(!status){if(actual_signature!=0x53425355||actual_tag!=tag||residue>length||csw[12]>2)status=B_IO;else if(csw[12]==1)return B_ABSENT;else if(residue||csw[12])status=B_IO;}
    if(status){d->dead=1;serial_text("M45_USB_BOT_QUARANTINED status=");serial_hex(status);serial_text("\n");}return status;
}
static unsigned scsi_retry(device *d,const uint8_t *cdb,unsigned cdb_bytes,unsigned length){
    for(unsigned attempt=0;attempt<3;++attempt){unsigned status=scsi(d,cdb,cdb_bytes,length);if(status!=B_ABSENT)return status;uint8_t sense[6]={3,0,0,0,18,0};if(scsi(d,sense,6,18))return B_IO;const uint8_t *data=(void*)(uintptr_t)d->data;unsigned response=data[0]&127,key=response==0x70||response==0x71?data[2]&15:response==0x72||response==0x73?data[1]&15:255;serial_text("M45_USB_SCSI_SENSE key=");serial_hex(key);serial_text("\n");if(key!=6)return B_IO;}
    return B_IO;
}
static unsigned mass_read(unsigned context_index,uint64_t lba,void *buffer){
    unsigned index=context_index&255,generation=context_index>>8;if(index>=device_count)return B_DENIED;device *d=&devices[index];if(d->generation!=generation)return B_DENIED;if(d->kind!=3||d->dead||lba>=d->sectors)return B_DENIED;uint8_t cdb[16]={0};unsigned length=10;
    if(lba<=UINT32_MAX){cdb[0]=0x28;be32(cdb+2,(uint32_t)lba);cdb[8]=1;}else{cdb[0]=0x88;be64(cdb+2,lba);be32(cdb+10,1);length=16;}unsigned status=scsi_retry(d,cdb,length,d->bytes);if(!status){memcpy(buffer,(void*)(uintptr_t)d->data,d->bytes);++d->reads;}return status;
}
static unsigned mass_probe(unsigned index){
    device *d=&devices[index];uint8_t cdb[16]={0x12,0,0,0,36};unsigned status=scsi_retry(d,cdb,6,36);if(status)return status;uint8_t *data=(void*)(uintptr_t)d->data;if((data[0]&31)!=0)return B_UNSUPPORTED;
    memset(cdb,0,16);cdb[0]=0x25;if((status=scsi_retry(d,cdb,10,8)))return status;uint64_t last=from_be32(data);d->bytes=from_be32(data+4);
    if(last==UINT32_MAX){memset(cdb,0,16);cdb[0]=0x9e;cdb[1]=0x10;be32(cdb+10,32);if((status=scsi_retry(d,cdb,16,32)))return status;last=((uint64_t)from_be32(data)<<32)|from_be32(data+4);d->bytes=from_be32(data+8);}
    if(last==UINT64_MAX||(d->bytes!=512&&d->bytes!=4096))return B_UNSUPPORTED;d->sectors=last+1;d->storage_index=storage_count();storage_disk item={STORAGE_USB,controller_bdf,d->port,d->bytes,1,0,d->sectors,(d->generation<<8)|index,0};status=storage_register(item,mass_read);serial_text("M45_USB_MASS_STORAGE_READY status=");serial_hex(status);serial_text("\n");return status;
}

static void release_device(unsigned index){
    device *d=&devices[index];d->dead=1;if(d->kind==3&&d->storage_index<STORAGE_MAX)storage_unavailable(d->storage_index);
    if(d->slot){if(!command(0,(10u<<10)|(d->slot<<24))){serial_text("M45_USB_DISABLE_FAILED_DMA_RETAINED\n");return;}consume();dcbaa[d->slot]=0;__asm__ volatile("mfence":::"memory");}
    uint64_t pages[]={d->input,d->output,d->buffer,(uint64_t)(uintptr_t)d->control.base,(uint64_t)(uintptr_t)d->interrupt.base,(uint64_t)(uintptr_t)d->bulk_out.base,d->data,(uint64_t)(uintptr_t)d->uas_status.base};for(unsigned i=0;i<8;++i)if(pages[i]){page_free(pages[i]);--dma_pages;}
    unsigned generation=d->generation+1;memset(d,0,sizeof(*d));d->generation=generation;d->storage_index=STORAGE_MAX;
}
unsigned usb_work_pending(void){return active&&(__atomic_load_n(&pending_ports,__ATOMIC_ACQUIRE)||__atomic_load_n(&pending_hubs,__ATOMIC_ACQUIRE));}
static unsigned descendant(unsigned child,unsigned parent){for(unsigned n=0;n<5&&devices[child].parent<USB_MAX_DEVICES;++n){child=devices[child].parent;if(child==parent)return 1;}return 0;}
static void release_tree(unsigned index){
    for(int depth=5;depth>=0;--depth)for(unsigned i=0;i<device_count;++i)if(devices[i].slot&&devices[i].depth==(unsigned)depth&&(i==index||descendant(i,index))){unsigned child_left=0;for(unsigned j=0;j<device_count;++j)child_left|=devices[j].slot&&j!=i&&descendant(j,i);if(child_left){devices[i].dead=1;serial_text("M45_USB_TREE_DISABLE_FAILED_DMA_RETAINED\n");}else release_device(i);}
}
void usb_process_changes(void){
    uint64_t flags;__asm__ volatile("pushfq;popq %0;cli":"=r"(flags)::"memory");unsigned pending=__atomic_exchange_n(&pending_ports,0,__ATOMIC_ACQ_REL),hubs=__atomic_exchange_n(&pending_hubs,0,__ATOMIC_ACQ_REL),added=0;op[0]&=~4u;active=0;
    for(unsigned port=0;port<port_count&&port<32;++port)if(pending&(1u<<port)){
        unsigned present=op[0x100+port*4]&1,found=0;for(unsigned i=0;i<device_count;++i)if(devices[i].slot&&devices[i].port==port+1&&devices[i].parent==USB_MAX_DEVICES){found=1;if(!present){release_tree(i);serial_text("M45_USB_DISCONNECTED port=");serial_hex(port+1);serial_text("\n");}}
        if(present&&!found&&!(rejected_ports&(1u<<port))){unsigned index=0;while(index<USB_MAX_DEVICES&&(devices[index].slot||devices[index].generation>=0xffffff))++index;if(index==USB_MAX_DEVICES){serial_text("M45_USB_DEVICE_QUOTA\n");continue;}
            if(enumerate(port,index)){added|=1u<<index;if(devices[index].kind==9)added|=hub_scan(index);serial_text("M45_USB_CONNECTED port=");serial_hex(port+1);serial_text("\n");}else{rejected_ports|=1u<<port;serial_text("M45_USB_ATTACH_UNSUPPORTED\n");release_device(index);}}
    }
    for(unsigned i=0;i<device_count;++i)if((hubs&(1u<<i))&&devices[i].slot&&!devices[i].dead&&devices[i].kind==9)added|=hub_scan(i);
    active=1;runtime[8]=3;op[0]|=4;for(unsigned i=0;i<device_count;++i)if((added&(1u<<i))&&devices[i].slot&&!devices[i].dead){if(devices[i].kind==3)(void)mass_probe(i);else submit_input(&devices[i]);}if(flags&512)__asm__ volatile("sti":::"memory");
}
void usb_storage_diagnostics(void){for(unsigned i=0;i<device_count;++i)if(devices[i].kind==3){serial_text("M45_USB_STORAGE_STATS reads=");serial_hex(devices[i].reads);serial_text(" wait_halts=");serial_hex(devices[i].wait_halts);serial_text(" driver_pages=");serial_hex(dma_pages);serial_text("\n");}}

static unsigned hub_prepare(device *d){
    if(d->speed>3||!control(d,0xa0,6,0x2900,0,7))return 0;uint8_t *bytes=(void*)(uintptr_t)d->buffer;if(bytes[0]<7||bytes[1]!=0x29||!bytes[2]||bytes[2]>15)return 0;d->hub_ports=bytes[2];unsigned power_ticks=((unsigned)bytes[5]*2+9)/10+1,think=(bytes[3]>>5)&3;
    memset((void*)(uintptr_t)d->input,0,4096);uint32_t *input=context(d->input,0),*slot=context(d->input,1);input[1]=1;memcpy(slot,(void*)(uintptr_t)d->output,context_size);slot[0]|=1u<<26;if(d->speed==3&&d->protocol==2)slot[0]|=1u<<25;slot[1]=(slot[1]&0xffffff)|(d->hub_ports<<24);slot[2]=(slot[2]&~(3u<<16))|(think<<16);if(!command(d->input,(13u<<10)|(d->slot<<24)))return 0;
    for(unsigned port=1;port<=d->hub_ports;++port)if(!control(d,0x23,3,8,port,0))return 0;uint64_t until=m3_clock_ticks()+power_ticks;unsigned spins=0;while(m3_clock_ticks()<until&&++spins<20000000)__asm__ volatile("pause");return m3_clock_ticks()>=until;
}
static unsigned hub_scan(unsigned index){
    device *hub=&devices[index];unsigned added=0;
    if(!control(hub,0xa0,0,0,0,4)){serial_text("M45_USB_HUB_STATUS_FAILED\n");return 0;}uint8_t *hub_data=(void*)(uintptr_t)hub->buffer;unsigned hub_status=(unsigned)hub_data[0]|((unsigned)hub_data[1]<<8),hub_changes=(unsigned)hub_data[2]|((unsigned)hub_data[3]<<8);for(unsigned bit=0;bit<2;++bit)if(hub_changes&(1u<<bit))(void)control(hub,0x20,1,bit,0,0);if(hub_status&3){for(unsigned i=0;i<device_count;++i)if(devices[i].slot&&devices[i].parent==index)release_tree(i);serial_text("M45_USB_HUB_POWER_OR_OVERCURRENT_FAULT\n");return 0;}

    for(unsigned port=1;port<=hub->hub_ports;++port){if(!control(hub,0xa3,0,0,port,4)){serial_text("M45_USB_HUB_STATUS_FAILED\n");continue;}uint8_t *data=(void*)(uintptr_t)hub->buffer;unsigned status=(unsigned)data[0]|((unsigned)data[1]<<8),changes=(unsigned)data[2]|((unsigned)data[3]<<8),found=0;
        for(unsigned bit=0;bit<5;++bit)if(changes&(1u<<bit))(void)control(hub,0x23,1,16+bit,port,0);
        for(unsigned i=0;i<device_count;++i)if(devices[i].slot&&devices[i].parent==index&&devices[i].hub_port==port){found=1;if(!(status&1)){release_tree(i);serial_text("M45_USB_HUB_CHILD_DISCONNECTED\n");}}
        if(changes&1)hub->rejected_children&=~(1u<<port);if(!(status&1)||found||(hub->rejected_children&(1u<<port)))continue;unsigned child=0;while(child<USB_MAX_DEVICES&&(devices[child].slot||devices[child].generation>=0xffffff))++child;if(child==USB_MAX_DEVICES){serial_text("M45_USB_DEVICE_QUOTA\n");continue;}if(hub->depth>=5||!control(hub,0x23,3,4,port,0))continue;
        uint64_t end=m3_clock_ticks()+100;unsigned spins=0;do{if(!control(hub,0xa3,0,0,port,4))break;data=(void*)(uintptr_t)hub->buffer;status=(unsigned)data[0]|((unsigned)data[1]<<8);if(!(status&16))break;__asm__ volatile("pause");}while(m3_clock_ticks()<end&&++spins<1000000);
        if((status&19)!=3){serial_text("M45_USB_HUB_RESET_FAILED\n");continue;}(void)control(hub,0x23,1,20,port,0);unsigned speed=(status&0x200)?2:(status&0x400)?3:1;
        if(enumerate_path(hub->port-1,child,index,port,speed)){added|=1u<<child;if(devices[child].kind==9)added|=hub_scan(child);serial_text("M45_USB_HUB_CHILD_CONNECTED\n");}else{hub->rejected_children|=1u<<port;serial_text("M45_USB_HUB_CHILD_UNSUPPORTED\n");release_device(child);}
    }return added;
}

/* One read command at a time. Protocol buffer regions: CDB 512, Sense 1024,
 * abort response 1536, stream contexts 2048/2112, abort TRB ring 3072.
 * No region is reused while a controller can still own its transfer. */
static unsigned uas_configure(device *d,const unsigned *address,const unsigned *packet,const unsigned *streams){
    if(d->speed!=3&&d->speed!=4)return 0;if(d->speed==4&&(!primary_streams||!streams[2]||!streams[3]))return 0;unsigned seen=0;
    for(unsigned i=1;i<=4;++i){if(!(address[i]&15)||(address[i]&~143u)||packet[i]!=(d->speed==4?1024u:512u)||((i==2||i==3)!=!!(address[i]&128)))return 0;unsigned endpoint=(address[i]&15)*2+!!(address[i]&128);if(seen&(1u<<endpoint))return 0;seen|=1u<<endpoint;}
    d->out_endpoint=(address[1]&15)*2;d->status_endpoint=(address[2]&15)*2+1;d->endpoint=(address[3]&15)*2+1;d->data=page();if(!d->data||!ring_init(&d->bulk_out)||!ring_init(&d->uas_status))return 0;
    memset((void*)(uintptr_t)d->input,0,4096);uint32_t *input=context(d->input,0),*slot=context(d->input,1);memcpy(slot,(void*)(uintptr_t)d->output,context_size);unsigned top=0;unsigned endpoints[]={d->out_endpoint,d->status_endpoint,d->endpoint};ring *rings[]={&d->bulk_out,&d->uas_status,&d->interrupt};input[1]=1;
    for(unsigned i=0;i<3;++i){unsigned endpoint=endpoints[i];if(endpoint>top)top=endpoint;input[1]|=1u<<endpoint;uint32_t *ep=context(d->input,endpoint+1);ep[1]=((d->speed==4?1024u:512u)<<16)|((i?6u:2u)<<3)|(3u<<1);ep[2]=(uint32_t)(uintptr_t)rings[i]->base|1;ep[4]=1024;if(d->speed==4&&i){uint64_t array=d->buffer+2048+(i-1)*64;memset((void*)(uintptr_t)array,0,64);((uint64_t*)(uintptr_t)array)[2]=(uint64_t)(uintptr_t)rings[i]->base|3;if(i==1)((uint64_t*)(uintptr_t)array)[4]=(d->buffer+3072)|3;ep[0]=(1u<<10)|(1u<<15);ep[2]=(uint32_t)array;}}
    slot[0]=(slot[0]&~(31u<<27))|(top<<27);return command(d->input,(12u<<10)|(d->slot<<24));
}
static unsigned uas_scsi_ss(device*,unsigned,unsigned*,unsigned*);
static unsigned uas_abort_task(device*);
static unsigned uas_quiesce(device*);
static unsigned uas_scsi(device *d,const uint8_t *cdb,unsigned cdb_bytes,unsigned length){
    uint8_t *iu=(void*)(uintptr_t)(d->buffer+512),*status=(void*)(uintptr_t)(d->buffer+1024);
    for(unsigned attempt=0;attempt<3;++attempt){d->uas_command_complete=0;memset(iu,0,32);iu[0]=1;iu[3]=1;memcpy(iu+16,cdb,cdb_bytes);unsigned actual=0,data_received=!length,s=0;if(d->speed==4)s=uas_scsi_ss(d,length,&actual,&data_received);else{s=bulk_endpoint(d,&d->bulk_out,d->out_endpoint,d->buffer+512,32,0,0);if(!s)d->uas_command_complete=1;}
        if(!s&&d->speed!=4){memset(status,0,512);s=bulk_endpoint(d,&d->uas_status,d->status_endpoint,d->buffer+1024,512,1,&actual);}
        if(!s&&d->speed!=4&&actual>=4&&status[0]==6){if(actual!=4||status[1]||status[2]||status[3]!=1||!length)s=B_IO;else{s=bulk_endpoint(d,&d->interrupt,d->endpoint,d->data,length,0,0);if(!s){data_received=1;memset(status,0,512);s=bulk_endpoint(d,&d->uas_status,d->status_endpoint,d->buffer+1024,512,1,&actual);}}}
        if(s){if(s==B_TIMEOUT&&d->uas_command_complete){unsigned recovery=uas_abort_task(d);serial_text(recovery?"M45_USB_UAS_ABORT_FAILED\n":"M45_USB_UAS_ABORT_COMPLETED\n");}unsigned stopped=uas_quiesce(d);serial_text(stopped?"M45_USB_UAS_STOP_FAILED_DMA_RETAINED\n":"M45_USB_UAS_ENDPOINTS_STOPPED\n");d->dead=1;serial_text("M45_USB_UAS_TRANSPORT_QUARANTINED status=");serial_hex(s);serial_text("\n");return s;}
        if(actual<16||status[0]!=3||status[1]||status[2]||status[3]!=1||status[4]||status[5]){d->dead=1;return B_IO;}unsigned sense_bytes=((unsigned)status[14]<<8)|status[15];if(sense_bytes>actual-16){d->dead=1;return B_IO;}
        if(!status[6]){if(!data_received){d->dead=1;return B_IO;}return B_OK;}if(status[6]!=2||sense_bytes<3)return B_IO;unsigned response=status[16]&127,key=response==0x70||response==0x71?status[18]&15:response==0x72||response==0x73?status[17]&15:255;serial_text("M45_USB_UAS_SENSE key=");serial_hex(key);serial_text("\n");if(key!=6)return B_IO;
    }return B_IO;
}

static unsigned uas_cancel_data(device *d,unsigned completion){
    unsigned opcode=completion==6?14:15;if(!command(0,(opcode<<10)|(d->endpoint<<16)|(d->slot<<24)))return B_IO;
    ring *r=&d->interrupt;if(r->index==255){r->index=0;r->cycle^=1;}uint64_t next=(uint64_t)(uintptr_t)&r->base[r->index]|2|r->cycle;
    if(!command_ex(next,1u<<16,(16u<<10)|(d->endpoint<<16)|(d->slot<<24)))return B_IO;serial_text("M45_USB_UAS_PENDING_DATA_CANCELLED\n");return B_OK;
}
static unsigned uas_scsi_ss(device *d,unsigned length,unsigned *actual,unsigned *received){
    uint64_t flags;__asm__ volatile("pushfq;popq %0;cli":"=r"(flags)::"memory");uint8_t *sense=(void*)(uintptr_t)(d->buffer+1024);memset(sense,0,512);uas_status_done=uas_data_done=uas_status_remaining=uas_data_remaining=0;
    uas_status_wait=push(&d->uas_status,d->buffer+1024,512,(1u<<10)|(1u<<5)|(1u<<2));uas_data_wait=length?push(&d->interrupt,d->data,length,(1u<<10)|(1u<<5)|(1u<<2)):0;__asm__ volatile("mfence":::"memory");doorbell[d->slot]=(1u<<16)|d->status_endpoint;if(length)doorbell[d->slot]=(1u<<16)|d->endpoint;
    uint8_t mask=in8(0x21);out8(0x21,mask&~1u);unsigned status=bulk_endpoint(d,&d->bulk_out,d->out_endpoint,d->buffer+512,32,0,0);if(!status)d->uas_command_complete=1;bulk_waiting=1;uint64_t end=m3_clock_ticks()+200;
    while(!status){
        if(uas_status_done){__asm__ volatile("lfence":::"memory");if((uas_status_done!=1&&uas_status_done!=13)||uas_status_remaining>512){status=B_IO;break;}*actual=512-uas_status_remaining;
            if(*actual<16||sense[0]!=3||sense[1]||sense[2]||sense[3]!=1||sense[4]||sense[5]){status=B_IO;break;}
            if(sense[6]||!length||uas_data_done)break;
        }
        if(uas_data_done&&uas_data_done!=1&&uas_data_done!=13&&uas_data_done!=6){status=B_IO;break;}
        if(m3_clock_ticks()>=end){status=B_TIMEOUT;break;}++d->wait_halts;__asm__ volatile("sti;hlt;cli":::"memory");
    }
    if(!status&&length){
        if(sense[6]){if(!uas_data_done||uas_data_done==6)status=uas_cancel_data(d,uas_data_done);}
        else if((uas_data_done!=1&&uas_data_done!=13)||uas_data_remaining)status=B_IO;
        else *received=1;
    }
    uas_status_wait=uas_data_wait=0;bulk_waiting=0;out8(0x21,mask);if(flags&512)__asm__ volatile("sti":::"memory");return status;
}

static unsigned uas_abort_task(device *d){
    uint64_t flags;__asm__ volatile("pushfq;popq %0;cli":"=r"(flags)::"memory");uint8_t *iu=(void*)(uintptr_t)(d->buffer+512),*response=(void*)(uintptr_t)(d->buffer+1536);memset(iu,0,16);memset(response,0,512);iu[0]=5;iu[3]=2;iu[4]=1;iu[7]=1;unsigned status=B_OK,actual=0;
    if(d->speed==4){
        ring *rings[]={&d->interrupt,&d->uas_status};unsigned endpoints[]={d->endpoint,d->status_endpoint};
        for(unsigned i=0;i<2;++i){ring *r=rings[i];if(!command(0,(15u<<10)|(endpoints[i]<<16)|(d->slot<<24))){status=B_IO;break;}if(!i)d->uas_abort_data_stopped=1;if(r->index==255){r->index=0;r->cycle^=1;}uint64_t next=(uint64_t)(uintptr_t)&r->base[r->index]|2|r->cycle;if(!command_ex(next,1u<<16,(16u<<10)|(endpoints[i]<<16)|(d->slot<<24))){status=B_IO;break;}}
        if(!status&&!command_ex((d->buffer+3072)|3,2u<<16,(16u<<10)|(d->status_endpoint<<16)|(d->slot<<24)))status=B_IO;
        ring abort_ring={(void*)(uintptr_t)(d->buffer+3072),0,1};uas_abort_done=uas_abort_remaining=0;if(!status)uas_abort_wait=push(&abort_ring,d->buffer+1536,512,(1u<<10)|(1u<<5)|(1u<<2));__asm__ volatile("mfence":::"memory");if(!status)doorbell[d->slot]=(2u<<16)|d->status_endpoint;
    }else{
        ring *rings[]={&d->uas_status,&d->interrupt};unsigned endpoints[]={d->status_endpoint,d->endpoint};
        for(unsigned i=0;i<2&&!status;++i){ring *r=rings[i];if(!command(0,(15u<<10)|(endpoints[i]<<16)|(d->slot<<24))){status=B_IO;break;}if(i)d->uas_abort_data_stopped=1;if(r->index==255){r->index=0;r->cycle^=1;}uint64_t next=(uint64_t)(uintptr_t)&r->base[r->index]|r->cycle;if(!command_ex(next,0,(16u<<10)|(endpoints[i]<<16)|(d->slot<<24)))status=B_IO;}
    }
    uint8_t mask=in8(0x21);out8(0x21,mask&~1u);if(!status)status=bulk_endpoint(d,&d->bulk_out,d->out_endpoint,d->buffer+512,16,0,0);bulk_waiting=1;
    if(d->speed==4){uint64_t end=m3_clock_ticks()+200;while(!status&&!uas_abort_done){if(m3_clock_ticks()>=end){status=B_TIMEOUT;break;}++d->wait_halts;__asm__ volatile("sti;hlt;cli":::"memory");}if(!status){if((uas_abort_done!=1&&uas_abort_done!=13)||uas_abort_remaining>512)status=B_IO;else actual=512-uas_abort_remaining;}}
    else for(unsigned drain=0;!status&&drain<3;++drain){memset(response,0,512);status=bulk_endpoint(d,&d->uas_status,d->status_endpoint,d->buffer+1536,512,1,&actual);bulk_waiting=1;if(status||(actual>=4&&response[2]==0&&response[3]==2))break;}
    if(!status&&(actual!=8||response[0]!=4||response[1]||response[2]||response[3]!=2||(response[7]!=0&&response[7]!=8)))status=B_IO;
    uas_abort_wait=0;bulk_waiting=0;out8(0x21,mask);if(flags&512)__asm__ volatile("sti":::"memory");return status;
}

static unsigned uas_quiesce(device *d){uint64_t flags;__asm__ volatile("pushfq;popq %0;cli":"=r"(flags)::"memory");unsigned endpoints[]={d->endpoint,d->status_endpoint,d->out_endpoint},status=B_OK;for(unsigned i=0;i<3;++i)if(!(i==0&&d->uas_abort_data_stopped)&&!command(0,(15u<<10)|(endpoints[i]<<16)|(d->slot<<24)))status=B_IO;if(flags&512)__asm__ volatile("sti":::"memory");return status;}

#ifdef ENGINE_M6
void usb_mouse_limits(unsigned w,unsigned h){
#ifdef ENGINE_M9
 if(w&&h){mouse_width9=w;mouse_height9=h;}
#endif
#ifdef ENGINE_M73
 extern unsigned ui73_scale(void);unsigned cw=15*ui73_scale()/2,ch=24*ui73_scale()/2;mouse_limit_x=w>cw?(int)(w-cw):0;mouse_limit_y=h>ch?(int)(h-ch):0;
#else
 mouse_limit_x=w>9?(int)w-9:0;mouse_limit_y=h>15?(int)h-15:0;
#endif
}
void usb_mouse_state(unsigned*x,unsigned*y,unsigned*b){
    /* Snapshot the BSP-routed IRQ state without losing a press/release during repaint. */
    uint64_t flags;__asm__ volatile("pushfq; popq %0; cli":"=r"(flags)::"memory");
    if(mouse_press_pending){*x=mouse_press_x;*y=mouse_press_y;*b=mouse_buttons|1u;mouse_press_pending=0;__atomic_store_n(&mouse_dirty,1,__ATOMIC_RELEASE);}
    else{*x=(unsigned)mouse_x;*y=(unsigned)mouse_y;*b=mouse_buttons;__atomic_store_n(&mouse_dirty,0,__ATOMIC_RELEASE);}
#ifdef ENGINE_M73
    extern unsigned ui73_scale(void);unsigned factor=ui73_scale();*x=*x*2/factor;*y=*y*2/factor;
#endif
    if(flags&512)__asm__ volatile("sti":::"memory");
}
#endif
