#include <engine/audio74.h>
#include <engine/install/m6.h>
#include <engine/m3.h>

#define AUDIO_BRANCH74 25u
#define AUDIO_VECTOR74 150u
#define AUDIO_BUFFERS74 16u
#define AUDIO_RING74 (AUDIO_BUFFERS74*4096u)
#define NX74 (UINT64_C(1)<<63)
typedef struct {uint64_t address;uint32_t bytes,flags;} hda_bdl74;
static volatile uint8_t *hda74;
static uint64_t pages74[24],buffers74[AUDIO_BUFFERS74],scratch74,corb74,rirb74,bdl74;
static unsigned page_count74,bdf74,msi_cap74,corb_mask74,rirb_mask74,corb_wp74,rirb_rp74,codec74,group74,pin74,dac74,stream74,sd74;
static unsigned route74[8],route_index74[8],route_count74,pending74,last_position74,refill74,quarantine74;
static uint64_t played74,last_pump74,data_first74,data_bytes74,data_read74;
static char path74[256];static uint32_t source_crc74;static uint64_t source_size74;
static unsigned wave_ready74;
static audio74_info info74={AUDIO74_STOPPED,0,0,0,100,0,0,0,0,"Choose a 16-bit PCM WAV file to play."};
static uint8_t r8_74(unsigned r){return hda74[r];}
static uint16_t r16_74(unsigned r){return *(volatile uint16_t *)(hda74+r);}
static uint32_t r32_74(unsigned r){return *(volatile uint32_t *)(hda74+r);}
static void w8_74(unsigned r,uint8_t v){hda74[r]=v;}
static void w16_74(unsigned r,uint16_t v){*(volatile uint16_t *)(hda74+r)=v;}
static void w32_74(unsigned r,uint32_t v){*(volatile uint32_t *)(hda74+r)=v;}
static uint32_t pci74(unsigned bdf,unsigned reg){uint32_t address=0x80000000u|(bdf<<8)|(reg&252),value;
    __asm__ volatile("outl %0,%1"::"a"(address),"Nd"((uint16_t)0xcf8));__asm__ volatile("inl %1,%0":"=a"(value):"Nd"((uint16_t)0xcfc));return value;}
static void pci_write74(unsigned bdf,unsigned reg,uint32_t value){uint32_t address=0x80000000u|(bdf<<8)|(reg&252);
    __asm__ volatile("outl %0,%1"::"a"(address),"Nd"((uint16_t)0xcf8));__asm__ volatile("outl %0,%1"::"a"(value),"Nd"((uint16_t)0xcfc));}
static void state74(unsigned state,const char *message){info74.state=state;info74.message=message;++info74.revision;}
static uint64_t allocate74(void){if(page_count74==24)return 0;uint64_t p=page_alloc();if(p)pages74[page_count74++]=p;info74.owned_bytes=page_count74*4096;return p;}
static unsigned wait32_74(unsigned reg,uint32_t mask,unsigned set){uint64_t until=m3_clock_ticks()+10;
    for(unsigned n=0;n<10000000&&m3_clock_ticks()<until;++n){if(!!(r32_74(reg)&mask)==set)return 0;__asm__ volatile("pause");}return 1;}
static void delay74(unsigned ticks){uint64_t until=m3_clock_ticks()+ticks;for(unsigned n=0;n<10000000&&m3_clock_ticks()<until;++n)__asm__ volatile("pause");}
static unsigned command74(unsigned node,unsigned verb,unsigned payload,uint32_t *answer){
    if(!hda74||!corb74||!rirb74||node>255)return 1;
    corb_wp74=(corb_wp74+1)&corb_mask74;((volatile uint32_t *)(uintptr_t)corb74)[corb_wp74]=(codec74<<28)|(node<<20)|(verb<<8)|payload;
    __asm__ volatile("mfence":::"memory");w16_74(0x48,(uint16_t)corb_wp74);uint64_t until=m3_clock_ticks()+5;
    for(unsigned n=0;n<10000000&&m3_clock_ticks()<until;++n){unsigned wp=r16_74(0x58)&rirb_mask74;
        while(rirb_rp74!=wp){rirb_rp74=(rirb_rp74+1)&rirb_mask74;__asm__ volatile("mfence":::"memory");
            const volatile uint32_t *r=(const volatile uint32_t *)(uintptr_t)rirb74+rirb_rp74*2;
            if(!(r[1]&16)&&(r[1]&15)==codec74){*answer=r[0];w8_74(0x5d,5);return 0;}}
        __asm__ volatile("pause");
    }return 1;
}
static uint32_t parameter74(unsigned node,unsigned parameter){uint32_t value=UINT32_MAX;if(command74(node,0xf00,parameter,&value))return UINT32_MAX;return value;}
static void verb74(unsigned node,unsigned verb,unsigned payload){uint32_t ignored;(void)command74(node,verb,payload,&ignored);}
static unsigned enable_msi74(void){
    if(!(pci74(bdf74,4)&(1u<<20)))return 1;unsigned cap=pci74(bdf74,0x34)&255;uint64_t visited=0;
    for(unsigned n=0;n<48&&cap>=0x40&&cap<=0xfc&&!(cap&3);++n){uint64_t bit=UINT64_C(1)<<(cap/4);if(visited&bit)break;visited|=bit;
        uint32_t header=pci74(bdf74,cap);unsigned control=header>>16;
        if((header&255)==5){unsigned wide=(control>>7)&1,offset=wide?12:8;if(cap+offset>0xfc)return 1;
            unsigned destination=*(volatile uint32_t *)(uintptr_t)0xfee00020>>24;pci_write74(bdf74,cap+4,0xfee00000u|(destination<<12));
            if(wide)pci_write74(bdf74,cap+8,0);pci_write74(bdf74,cap+offset,(pci74(bdf74,cap+offset)&0xffff0000u)|AUDIO_VECTOR74);
            if(control&0x100){if(cap+offset+4>0xfc)return 1;pci_write74(bdf74,cap+offset+4,0);}
            pci_write74(bdf74,cap,(header&65535)|(((control&~0x70u)|1)<<16));msi_cap74=cap;return 0;
        }cap=(header>>8)&255;
    }return 1;
}
static unsigned map74(void){
    bdf74=UINT32_MAX;
    for(unsigned bus=0;bus<256&&bdf74==UINT32_MAX;++bus)for(unsigned device=0;device<32&&bdf74==UINT32_MAX;++device){unsigned base=(bus<<8)|(device<<3);
        if((pci74(base,0)&65535)==65535)continue;unsigned functions=(pci74(base,12)&0x800000)?8:1;
        for(unsigned f=0;f<functions;++f){unsigned bdf=base|f;if((pci74(bdf,0)&65535)!=65535&&(pci74(bdf,8)>>8)==0x040300){bdf74=bdf;break;}}}
    if(bdf74==UINT32_MAX)return 1;uint32_t low=pci74(bdf74,16);if((low&1)||((low&6)!=0&&(low&6)!=4))return 1;
    uint64_t physical=low&~15u;if((low&6)==4)physical|=(uint64_t)pci74(bdf74,20)<<32;
    if(physical<0x10000000||physical>UINT64_C(0x000ffffffff00000))return 1;
    uint64_t page=allocate74();if(!page)return 1;uint64_t base=physical&~UINT64_C(0x1fffff);
    ((uint64_t *)(uintptr_t)page)[0]=base|0x9b|NX74;((uint64_t *)(uintptr_t)page)[1]=(base+0x200000)|0x9b|NX74;
    ((uint64_t *)(uintptr_t)kernel_stack_branch())[AUDIO_BRANCH74]=page|3;
    uint64_t cr3;__asm__ volatile("movq %%cr3,%0;movq %0,%%cr3":"=r"(cr3)::"memory");
    hda74=(volatile uint8_t *)(uintptr_t)(UINT64_C(0xffff800000000000)+(uint64_t)AUDIO_BRANCH74*0x40000000+(physical&0x1fffff));
    pci_write74(bdf74,4,(pci74(bdf74,4)&65535)|6|0x400);return r8_74(3)!=1;
}
static unsigned rings74(void){
    w32_74(0x20,0);w8_74(0x4c,0);w8_74(0x5c,0);w32_74(8,0);if(wait32_74(8,1,0))return 1;w32_74(8,1);if(wait32_74(8,1,1))return 1;delay74(2);
    unsigned output=(r16_74(0)>>12)&15,input=(r16_74(0)>>8)&15;if(!output||input>15)return 1;stream74=input;sd74=0x80+input*32;
    corb74=allocate74();rirb74=allocate74();bdl74=allocate74();scratch74=allocate74();if(!corb74||!rirb74||!bdl74||!scratch74)return 1;
    unsigned cs=r8_74(0x4e),rs=r8_74(0x5e),c=cs&64?2:cs&32?1:0,r=rs&64?2:rs&32?1:0;
    if(!(cs&(16u<<c))||!(rs&(16u<<r)))return 1;corb_mask74=c==2?255:c==1?15:1;rirb_mask74=r==2?255:r==1?15:1;
    w8_74(0x4e,(uint8_t)c);w8_74(0x5e,(uint8_t)r);w32_74(0x40,(uint32_t)corb74);w32_74(0x44,0);w32_74(0x50,(uint32_t)rirb74);w32_74(0x54,0);
    w16_74(0x48,0);w16_74(0x4a,0x8000);w16_74(0x4a,0);w16_74(0x58,0x8000);w16_74(0x5a,1);corb_wp74=rirb_rp74=0;
    w8_74(0x4d,1);w8_74(0x5d,5);w8_74(0x5c,3);w8_74(0x4c,2);return enable_msi74();
}
static unsigned connections74(unsigned node,unsigned *out){
    uint32_t length=parameter74(node,0x0e);if(length==UINT32_MAX)return 0;unsigned count=length&127,wide=length&128,seen=0,previous=0;
    if(count>32)return 0;
    for(unsigned i=0;i<count;){uint32_t entries;if(command74(node,0xf02,i,&entries))return 0;unsigned batch=wide?2:4;
        for(unsigned k=0;k<batch&&i<count;++k,++i){unsigned value=wide?(entries>>(k*16))&65535:(entries>>(k*8))&255,range=value&(wide?0x8000:0x80),last=value&(wide?0x7fff:0x7f);
            if(last>255)return 0;unsigned first=range?previous+1:last;if(first>last)return 0;
            for(unsigned n=first;n<=last;++n){if(seen==32)return 0;out[seen++]=n;}previous=last;
        }
    }return seen;
}
static unsigned route_find74(unsigned node,unsigned depth,uint64_t *visited){
    if(node>255||depth==8||(visited[node/64]&(UINT64_C(1)<<(node%64))))return 0;
    visited[node/64]|=UINT64_C(1)<<(node%64);uint32_t caps=parameter74(node,9);if(caps==UINT32_MAX||(caps&512))return 0;
    route74[depth]=node;unsigned type=(caps>>20)&15;
    if(type==0){uint32_t pcm=parameter74((caps&16)?node:group74,0x0a),formats=parameter74((caps&16)?node:group74,0x0b);
        serial_text("M74_AUDIO_DAC ");serial_hex(node);serial_text(" pcm=");serial_hex(pcm);serial_text(" formats=");serial_hex(formats);serial_text("\n");if(pcm==UINT32_MAX||formats==UINT32_MAX||!(caps&1)||!(pcm&(1u<<(info74.rate==44100?5:6)))||!(pcm&(1u<<17))||!(formats&1))return 0;
        dac74=node;route_count74=depth+1;return 1;
    }
    unsigned list[32],count=connections74(node,list);
    for(unsigned i=0;i<count;++i){if((caps&2)&&i>15)continue;route_index74[depth]=i;if(route_find74(list[i],depth+1,visited))return 1;}
    return 0;
}
static unsigned codec_route74(void){
    unsigned present=r16_74(0x0e)&0x7fff;serial_text("M74_AUDIO_CODECS ");serial_hex(present);serial_text("\n");
    for(codec74=0;codec74<15;++codec74){if(!(present&(1u<<codec74)))continue;uint32_t root=parameter74(0,4);serial_text("M74_AUDIO_ROOT ");serial_hex(root);serial_text("\n");unsigned first=(root>>16)&255,count=root&255;if(count>32||first+count>256)continue;
        for(unsigned g=first;g<first+count;++g){uint32_t type=parameter74(g,5);serial_text("M74_AUDIO_GROUP ");serial_hex(type);serial_text("\n");if((type&255)!=1)continue;group74=g;verb74(g,0x705,0);uint32_t widgets=parameter74(g,4);serial_text("M74_AUDIO_WIDGETS ");serial_hex(widgets);serial_text("\n");unsigned start=(widgets>>16)&255,nodes=widgets&255;if(nodes>128||start+nodes>256)continue;
            for(unsigned pass=0;pass<3;++pass)for(unsigned node=start;node<start+nodes;++node){uint32_t caps=parameter74(node,9);if(pass==0){serial_text("M74_AUDIO_WIDGET ");serial_hex(node);serial_text(" caps=");serial_hex(caps);serial_text("\n");}if(((caps>>20)&15)!=4||(caps&512))continue;
                uint32_t pin_caps=parameter74(node,0x0c),config;if(!(pin_caps&16)||command74(node,0xf1c,0,&config))continue;
                serial_text("M74_AUDIO_PIN ");serial_hex(node);serial_text(" caps=");serial_hex(pin_caps);serial_text(" config=");serial_hex(config);serial_text("\n");unsigned kind=(config>>20)&15,preferred=pass==0?1:pass==1?0:2;if(kind!=preferred||(config>>30)==1)continue;
                uint64_t visited[4]={0};if(route_find74(node,0,visited)){pin74=node;return 0;}
            }
        }
    }return 1;
}
static unsigned format74(void){
    uint32_t response;unsigned format=0x11|(info74.rate==44100?0x4000:0);
    for(unsigned i=0;i<route_count74;++i){unsigned node=route74[i];uint32_t caps=parameter74(node,9);if(caps&1024)verb74(node,0x705,0);
        if(i+1<route_count74)verb74(node,0x701,route_index74[i]);
        if(caps&4){uint32_t amp=parameter74((caps&8)?node:group74,0x12);verb74(node,0x300,0xb000|(amp&127));}
        if(caps&2){uint32_t amp=parameter74((caps&8)?node:group74,0x0d);verb74(node,0x300,0x7000|((route_index74[i]&15)<<8)|(amp&127));}
    }
    uint32_t pin_caps=parameter74(pin74,0x0c);verb74(pin74,0x707,0x40|((pin_caps&8)?0x80:0));if(pin_caps&(1u<<16))verb74(pin74,0x70c,2);
    verb74(dac74,0x200,format);verb74(dac74,0x706,0x10);
    if(command74(dac74,0xa00,0,&response)||(response&65535)!=format)return 1;
    return 0;
}
static unsigned same_path74(const char *a,const char *b){unsigned i=0;while(a[i]&&a[i]==b[i])++i;return !a[i]&&!b[i];}
static const v6_entry *source74(void){v6_volume *v=m6_volume();if(!v)return 0;for(unsigned i=0;i<V6_ENTRIES;++i)if(v->entries[i].kind==1&&same_path74(v->entries[i].path,path74))return &v->entries[i];return 0;}
static unsigned file_read74(uint64_t offset,void *out,unsigned bytes){unsigned actual=0;return v6_read(m6_volume(),path74,offset,out,bytes,&actual)||actual!=bytes;}
static uint16_t little16_74(const uint8_t *p){return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t little32_74(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned wave74(const char *path){
    wave_ready74=0;info74.frames=info74.total_frames=0;
    if(!path)return 1;unsigned n=0;while(path[n]&&n<255){path74[n]=path[n];++n;}path74[n]=0;if(path[n])return 1;
    const v6_entry *e=source74();if(!e||e->size<44||e->size>64u*1024*1024||v6_verify(m6_volume(),path74))return 1;
    source_crc74=e->crc;source_size74=e->size;uint8_t header[64];if(file_read74(0,header,12)||memcmp(header,"RIFF",4)||memcmp(header+8,"WAVE",4))return 1;
    uint64_t end=(uint64_t)little32_74(header+4)+8;if(end>e->size||end<12)return 1;unsigned have_format=0,have_data=0;data_read74=0;
    for(uint64_t at=12,count=0;at+8<=end&&count<128;++count){if(file_read74(at,header,8))return 1;unsigned bytes=little32_74(header+4);uint64_t next=at+8+bytes+(bytes&1);if(next>end)return 1;
        if(!memcmp(header,"fmt ",4)){if(have_format||bytes<16||bytes>64||file_read74(at+8,header,bytes))return 1;
            if(little16_74(header)!=1||little16_74(header+14)!=16)return 1;info74.channels=little16_74(header+2);info74.rate=little32_74(header+4);
            if((info74.channels!=1&&info74.channels!=2)||(info74.rate!=44100&&info74.rate!=48000)||little16_74(header+12)!=info74.channels*2||little32_74(header+8)!=info74.rate*info74.channels*2)return 1;have_format=1;
        }else if(!memcmp(header,"data",4)){if(have_data)return 1;data_first74=at+8;data_bytes74=bytes;have_data=1;}
        at=next;
    }
    if(!have_format||!have_data||!data_bytes74||data_bytes74%(info74.channels*2))return 1;info74.total_frames=data_bytes74/(info74.channels*2);info74.frames=0;wave_ready74=1;return 0;
}
static unsigned fill74(unsigned index){
    uint8_t *out=(uint8_t *)(uintptr_t)buffers74[index];memset(out,0,4096);if(data_read74==data_bytes74)return 0;
    const v6_entry *e=source74();if(!e||e->crc!=source_crc74||e->size!=source_size74)return 1;
    unsigned bytes=info74.channels==1?2048:4096;if(bytes>data_bytes74-data_read74)bytes=(unsigned)(data_bytes74-data_read74);unsigned actual=0;
    if(v6_read_scratch(m6_volume(),path74,data_first74+data_read74,out,bytes,&actual,(void *)(uintptr_t)scratch74)||actual!=bytes)return 1;
    unsigned frames=bytes/(info74.channels*2);
    for(unsigned i=frames;i;){--i;int16_t *samples=(int16_t *)out;int left=samples[i*info74.channels],right=info74.channels==1?left:samples[i*2+1];
        samples[i*2]=(int16_t)(left*(int)info74.volume/100);samples[i*2+1]=(int16_t)(right*(int)info74.volume/100);}
    data_read74+=bytes;return 0;
}
void audio74_stop(void){
    if(quarantine74){state74(AUDIO74_FAILED,"Audio DMA is quarantined. Restart before retrying; its pages remain reserved.");return;}
    if(!hda74){if(!quarantine74)state74(AUDIO74_STOPPED,"Playback stopped.");return;}
    uint64_t flags;__asm__ volatile("pushfq;popq %0;cli":"=r"(flags)::"memory");
    w32_74(0x20,0);if(sd74)w8_74(sd74,r8_74(sd74)&~2u);w8_74(0x4c,0);w8_74(0x5c,0);
    if(msi_cap74){uint32_t cap=pci74(bdf74,msi_cap74);pci_write74(bdf74,msi_cap74,cap&~(1u<<16));}
    pci_write74(bdf74,4,(pci74(bdf74,4)&65535)&~4u);
    unsigned stopped=(!sd74||!wait32_74(sd74,2,0))&&!(r8_74(0x4c)&2)&&!(r8_74(0x5c)&2);
    if(!stopped){quarantine74=1;state74(AUDIO74_FAILED,"Audio DMA did not stop. Its pages remain reserved; restart before retrying.");}
    else{((uint64_t *)(uintptr_t)kernel_stack_branch())[AUDIO_BRANCH74]=0;uint64_t cr3;__asm__ volatile("movq %%cr3,%0;movq %0,%%cr3":"=r"(cr3)::"memory");hda74=0;
        while(page_count74)page_free(pages74[--page_count74]);memset(buffers74,0,sizeof(buffers74));scratch74=corb74=rirb74=bdl74=0;info74.owned_bytes=0;sd74=msi_cap74=0;state74(AUDIO74_STOPPED,"Playback stopped.");}
    __atomic_store_n(&pending74,0,__ATOMIC_RELEASE);if(flags&512)__asm__ volatile("sti":::"memory");
}
unsigned audio74_inspect(const char *path){
    audio74_stop();if(quarantine74)return 1;
    if(wave74(path)){state74(AUDIO74_FAILED,"Choose a WAV file: PCM 16-bit, mono/stereo, 44.1 or 48 kHz, up to 64 MiB.");return 1;}
    state74(AUDIO74_STOPPED,"Ready to play this PCM WAV file.");return 0;
}
unsigned audio74_play(const char *path){
    audio74_stop();if(quarantine74)return 1;info74.interrupts=0;
    const v6_entry *source=source74();
    if(!wave_ready74||!same_path74(path,path74)||!source||source->crc!=source_crc74||source->size!=source_size74){
        if(wave74(path)){state74(AUDIO74_FAILED,"Choose a WAV file: PCM 16-bit, mono/stereo, 44.1 or 48 kHz, up to 64 MiB.");return 1;}}
    data_read74=0;info74.frames=0;
    unsigned stage=map74()?1:rings74()?2:codec_route74()?3:format74()?4:0;
    if(stage){serial_text("M74_AUDIO_START_FAILED stage=");serial_hex(stage);serial_text("\n");audio74_stop();state74(AUDIO74_FAILED,"No supported analog HDA output with MSI is available.");return 1;}
    for(unsigned i=0;i<AUDIO_BUFFERS74;++i){buffers74[i]=allocate74();if(!buffers74[i]||fill74(i)){audio74_stop();state74(AUDIO74_FAILED,"Audio buffers could not be prepared. Your original file is unchanged.");return 1;}
        ((hda_bdl74 *)(uintptr_t)bdl74)[i]=(hda_bdl74){buffers74[i],4096,1};}
    w8_74(sd74,0);w8_74(sd74,1);if(wait32_74(sd74,1,1)){audio74_stop();state74(AUDIO74_FAILED,"The HDA stream did not enter reset.");return 1;}w8_74(sd74,0);if(wait32_74(sd74,1,0)){audio74_stop();state74(AUDIO74_FAILED,"The HDA stream did not leave reset.");return 1;}
    w8_74(sd74+3,0x1c);w8_74(sd74+2,0x10);w32_74(sd74+8,AUDIO_RING74);w16_74(sd74+12,AUDIO_BUFFERS74-1);
    w16_74(sd74+18,(uint16_t)(0x11|(info74.rate==44100?0x4000:0)));w32_74(sd74+24,(uint32_t)bdl74);w32_74(sd74+28,0);
    played74=last_position74=refill74=0;last_pump74=m3_clock_ticks();__atomic_store_n(&pending74,0,__ATOMIC_RELEASE);
    __asm__ volatile("mfence":::"memory");state74(AUDIO74_PLAYING,"Playing PCM audio through the HDA device.");w32_74(0x20,0x80000000u|(1u<<stream74));w8_74(sd74,0x1e);
    serial_text("M74_AUDIO_HDA_PLAYING dma_bytes=");serial_hex(info74.owned_bytes);serial_text("\n");return 0;
}
void audio74_pause(void){if(!hda74)return;if(info74.state==AUDIO74_PLAYING){w8_74(sd74,r8_74(sd74)&~2u);state74(AUDIO74_PAUSED,"Playback paused.");}
    else if(info74.state==AUDIO74_PAUSED){last_pump74=m3_clock_ticks();w8_74(sd74,r8_74(sd74)|2);state74(AUDIO74_PLAYING,"Playing PCM audio through the HDA device.");}}
void audio74_volume(unsigned volume){if(volume>100)volume=100;info74.volume=volume;++info74.revision;}
unsigned audio74_irq(uint64_t *frame){
    if(frame[15]!=AUDIO_VECTOR74)return 0;
    if(hda74){unsigned status=r8_74(sd74+3);if(status&0x1c){w8_74(sd74+3,(uint8_t)(status&0x1c));++info74.interrupts;__atomic_fetch_or(&pending74,(status&0x18)?2:1,__ATOMIC_RELEASE);}}
    *(volatile uint32_t *)(uintptr_t)0xfee000b0=0;return 1;
}
unsigned audio74_pending(void){return __atomic_load_n(&pending74,__ATOMIC_ACQUIRE);}
void audio74_pump(void){
    unsigned pending=__atomic_exchange_n(&pending74,0,__ATOMIC_ACQ_REL);if(!hda74||info74.state!=AUDIO74_PLAYING)return;
    uint64_t ticks=m3_clock_ticks();unsigned position=r32_74(sd74+4);if(position>=AUDIO_RING74){audio74_stop();state74(AUDIO74_FAILED,"The audio device reported an invalid position.");return;}
    if((pending&2)||ticks-last_pump74>(uint64_t)AUDIO_RING74*100/(info74.rate*4)){
        audio74_stop();state74(AUDIO74_FAILED,"Playback could not keep up. Stop other work and try again.");return;}
    played74+=(position-last_position74)&(AUDIO_RING74-1);last_position74=position;last_pump74=ticks;
    info74.frames=played74/4;if(info74.frames>info74.total_frames)info74.frames=info74.total_frames;
    if(played74>=info74.total_frames*4+(uint64_t)info74.rate*4/5){audio74_stop();state74(AUDIO74_STOPPED,"Playback finished.");serial_text("M74_AUDIO_FINISHED\n");return;}
    unsigned current=position/4096;
    for(unsigned budget=0;refill74!=current&&budget<AUDIO_BUFFERS74;++budget){if(fill74(refill74)){audio74_stop();state74(AUDIO74_FAILED,"The audio file changed or could not be read. Playback stopped.");return;}refill74=(refill74+1)%AUDIO_BUFFERS74;}
    __asm__ volatile("mfence":::"memory");
}
audio74_info audio74_current(void){return info74;}
