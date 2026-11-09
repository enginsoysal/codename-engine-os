#include "runtime.h"
typedef struct {char path[256],name[96];uint8_t*base;uint32_t size,exports,export_size;unsigned dll,loaded;} module8;
static module8 modules8[8];static unsigned module_count8;
uint64_t pe8_main(void){return module_count8?(uint64_t)(uintptr_t)modules8[0].base:0;}
static uint16_t u16(const void*p){uint16_t v;memcpy(&v,p,2);return v;}
static uint32_t u32(const void*p){uint32_t v;memcpy(&v,p,4);return v;}
static uint64_t u64(const void*p){uint64_t v;memcpy(&v,p,8);return v;}
static unsigned range8(uint64_t off,uint64_t size,uint64_t total){return off<=total&&size<=total-off;}
static void *rva8(module8*m,uint32_t rva,unsigned bytes){if(!range8(rva,bytes,m->size))fail8("PE image contains an invalid relative address.");return m->base+rva;}
static const char *name8(module8*m,uint32_t rva){char*s=rva8(m,rva,1);unsigned n=0;while(n<512&&rva+n<m->size){if(!s[n])return s;++n;}fail8("PE image contains an invalid import/export name.");}
uint64_t pe8_module(const char*name){const char*last=name;for(unsigned i=0;name[i];++i)if(name[i]=='\\'||name[i]=='/')last=name+i+1;for(unsigned i=0;i<module_count8;++i)if(!case8(modules8[i].name,last))return (uint64_t)(uintptr_t)modules8[i].base;return 0;}
const char *pe8_path(uint64_t base){for(unsigned i=0;i<module_count8;++i)if((uint64_t)(uintptr_t)modules8[i].base==base)return modules8[i].path;return 0;}
void *pe8_symbol(uint64_t base,const char*name,unsigned ordinal){
 module8*m=0;for(unsigned i=0;i<module_count8;++i)if((uint64_t)(uintptr_t)modules8[i].base==base)m=modules8+i;if(!m||!m->exports)return 0;
 uint8_t*e=rva8(m,m->exports,40);uint32_t start=u32(e+16),functions=u32(e+20),count=u32(e+24),index=UINT32_MAX;
 if(functions>65536||count>65536)return 0;
 if(ordinal){if(ordinal<start||ordinal-start>=functions)return 0;index=ordinal-start;}
 else{uint8_t*names=rva8(m,u32(e+32),count*4),*ordinals=rva8(m,u32(e+36),count*2);for(unsigned i=0;i<count;++i)if(equal8(name8(m,u32(names+i*4)),name)){index=u16(ordinals+i*2);break;}}
 if(index>=functions)return 0;uint32_t result=u32((uint8_t*)rva8(m,u32(e+28),functions*4)+index*4);if(!result)return 0;
 if(result>=m->exports&&result-m->exports<m->export_size){const char*forward=name8(m,result);char dll[96];unsigned n=0;while(forward[n]&&forward[n]!='.'&&n<80){dll[n]=forward[n];++n;}if(forward[n]!='.')return 0;dll[n]=0;append8(dll,".dll",sizeof(dll));return resolve8(dll,forward+n+1,0);}
 return rva8(m,result,1);
}
uint64_t pe8_load(const char*path){
 const char*basename=path;for(unsigned i=0;path[i];++i)if(path[i]=='/'||path[i]=='\\')basename=path+i+1;uint64_t existing=pe8_module(basename);if(existing)return existing;
 if(module_count8==8)fail8("The Windows module limit has been reached.");void*file=0;unsigned bytes=load8(path,&file,8*1024*1024);if(bytes<256)fail8("The Windows executable could not be read.");uint8_t*raw=file;uint32_t at=u32(raw+60);
 if(u16(raw)!=0x5a4d||!range8(at,264,bytes)||u32(raw+at)!=0x4550||u16(raw+at+4)!=0x8664||u16(raw+at+24)!=0x20b)fail8("This runtime requires a 64-bit x86 Windows PE executable.");
 unsigned sections=u16(raw+at+6),opt=u16(raw+at+20),characteristics=u16(raw+at+22);uint8_t*o=raw+at+24;
 if(!sections||sections>32||opt<240||!range8(at+24+opt,(uint64_t)sections*40,bytes)||u32(o+32)!=4096||u32(o+108)<16)fail8("Unsupported or damaged PE section layout.");
 uint32_t size=u32(o+56),headers=u32(o+60),entry=u32(o+16),image_size=(size+4095u)&~4095u;
 if(size<headers||size>8*1024*1024||headers>bytes||entry>=size)fail8("Invalid PE image size or entry point.");
 module8*m=modules8+module_count8;uint64_t address=UINT64_C(0x110000000)+(uint64_t)module_count8*0x1000000;unsigned index=module_count8++;
 memset(m,0,sizeof(*m));copy8(m->path,path,256);copy8(m->name,basename,96);m->size=size;m->dll=(characteristics&0x2000)!=0;m->base=(void*)(uintptr_t)address;
 if(sys8(W8_SYS_MAP,address,image_size,3,0,0))fail8("Insufficient memory for the Windows image.");memcpy(m->base,raw,headers);
 uint8_t*section=raw+at+24+opt;uint32_t previous=(headers+4095)&~4095u;
 for(unsigned i=0;i<sections;++i){uint8_t*s=section+i*40;uint32_t virtual_size=u32(s+8),rva=u32(s+12),raw_size=u32(s+16),offset=u32(s+20),flags=u32(s+36),span=virtual_size>raw_size?virtual_size:raw_size;
  if(!span)continue;if((rva&4095)||rva<previous||!range8(rva,span,size)||!range8(offset,raw_size,bytes)||((flags&0xa0000000u)==0xa0000000u))fail8("Overlapping, writable-executable or invalid PE section.");
  memcpy(m->base+rva,raw+offset,raw_size);previous=(rva+span+4095)&~4095u;
 }
 uint64_t preferred=u64(o+24),delta=address-preferred;uint32_t reloc=u32(o+112+5*8),reloc_size=u32(o+116+5*8);
 if(delta){if(!reloc||!reloc_size)fail8("This executable cannot be relocated away from kernel memory.");uint32_t off=0;
  while(off<reloc_size){uint8_t*b=rva8(m,reloc+off,8);uint32_t page=u32(b),n=u32(b+4);if(n<8||(n&1)||n>reloc_size-off)fail8("Invalid PE relocation block.");b=rva8(m,reloc+off,n);
   for(unsigned j=8;j<n;j+=2){unsigned item=u16(b+j),type=item>>12;uint32_t where=page+(item&4095);if(type==0)continue;if(type!=10)fail8("Unsupported PE relocation type.");uint64_t*p=rva8(m,where,8);*p+=delta;}off+=n;
  }
 }
 m->exports=u32(o+112);m->export_size=u32(o+116);
 uint32_t imports=u32(o+112+8),import_size=u32(o+116+8);if(imports){unsigned off=0,terminated=0;
  while(off+20<=import_size){uint8_t*d=rva8(m,imports+off,20);uint32_t lookup=u32(d),name=u32(d+12),iat=u32(d+16);if(!lookup&&!name&&!iat){terminated=1;break;}if(!lookup)lookup=iat;const char*dll=name8(m,name);
   for(unsigned n=0;n<4096;++n){uint64_t thunk=u64(rva8(m,lookup+n*8,8));if(!thunk)break;unsigned ordinal=(thunk>>63)?(unsigned)(thunk&65535):0;const char*symbol=ordinal?0:name8(m,(uint32_t)thunk+2);if(!ordinal&&(thunk>>32))fail8("Invalid PE import thunk.");
    void*fn=resolve8(dll,symbol,ordinal);if(!fn){print8("Unsupported import: ");print8(dll);print8("!");if(symbol)print8(symbol);else number8(ordinal);print8("\n");fail8("This executable requires a Windows function that is not available yet.");}
    *(uint64_t*)rva8(m,iat+n*8,8)=(uint64_t)(uintptr_t)fn;if(n==4095)fail8("PE import count exceeds the supported bound.");
   }off+=20;
  }if(!terminated)fail8("PE import directory is not terminated.");
 }
 /* TLS callbacks run only after relocation/imports. The data-index path is
    deliberately refused until an actual static TLS allocator is implemented. */
 uint32_t tls=u32(o+112+9*8);if(tls)fail8("Static Windows TLS and TLS callbacks are not implemented for this executable.");
 /* Headers and every section receive final W^X permissions before execution. */
 if(sys8(W8_SYS_PROTECT,address,image_size,1,0,0))fail8("PE image permissions could not be applied.");
 for(unsigned i=0;i<sections;++i){uint8_t*s=section+i*40;uint32_t span=u32(s+8)>u32(s+16)?u32(s+8):u32(s+16),rva=u32(s+12),flags=u32(s+36);if(!span)continue;unsigned permissions=(flags&0x20000000u)?5:(flags&0x80000000u)?3:1;
  if(sys8(W8_SYS_PROTECT,address+rva,(span+4095)&~4095u,permissions,0,0))fail8("A PE section could not be protected.");}
 m->loaded=1;free8(file);if(m->dll&&entry){if(!((int(MS*)(void*,unsigned,void*))(m->base+entry))(m->base,1,0))fail8("The Windows DLL rejected process attachment.");}
 (void)index;return address;
}
int pe8_release(uint64_t base){for(unsigned i=1;i<module_count8;++i){module8*m=modules8+i;if((uint64_t)(uintptr_t)m->base!=base||!m->loaded)continue;uint8_t*o=m->base+u32(m->base+60)+24;uint32_t entry=u32(o+16);if(m->dll&&entry)(void)((int(MS*)(void*,unsigned,void*))(m->base+entry))(m->base,0,0);if(sys8(W8_SYS_UNMAP,base,(m->size+4095)&~4095u,0,0,0))return 0;memset(m,0,sizeof(*m));return 1;}return 0;}
void pe8_run(const char*path,const char*arguments){(void)arguments;uint64_t base=pe8_load(path);module8*m=modules8;uint8_t*o=(uint8_t*)(uintptr_t)base+u32((uint8_t*)(uintptr_t)base+60)+24;
 if(m->dll||u16(o+68)!=3)fail8("This first Windows runtime supports console applications; graphical Windows apps are not qualified.");
 print8("Running the original Windows executable in a protected process.\n\n");((void(MS*)(void))(uintptr_t)(base+u32(o+16)))();exit8(0);
}
