#ifndef ENGINE_ELF_H
#define ENGINE_ELF_H
#include <engine/base.h>
typedef struct {uint8_t ident[16];uint16_t type,machine;uint32_t version;uint64_t entry,phoff,shoff;
    uint32_t flags;uint16_t ehsize,phentsize,phnum,shentsize,shnum,shstrndx;} elf_header;
typedef struct {uint32_t type,flags;uint64_t offset,vaddr,paddr,filesz,memsz,align;} elf_segment;
typedef struct {uint64_t low,high,entry;uint16_t count;} elf_plan;
_Static_assert(sizeof(elf_header)==64 && sizeof(elf_segment)==56,"ELF layouts");
/* Caller must supply aligned readable bytes. Copies into locals avoid unaligned struct reads. */
static inline int elf_validate(const uint8_t *data,uint64_t size,elf_plan *plan) {
    if(!data || !plan || size<64)return 1;
    elf_header h;memcpy(&h,data,sizeof(h));
    if(h.ident[0]!=0x7f || h.ident[1]!='E' || h.ident[2]!='L' || h.ident[3]!='F' ||
       h.ident[4]!=2 || h.ident[5]!=1 || h.ident[6]!=1 || h.type!=2 || h.machine!=62 ||
       h.version!=1 || h.ehsize!=64 || h.phentsize!=56 || !h.phnum || h.phnum>32 ||
       h.phoff>size || (uint64_t)h.phnum*56>size-h.phoff)return 2;
    uint64_t low=UINT64_MAX,high=0;uint16_t count=0;int entry_ok=0;
    elf_segment loads[32];
    for(uint16_t i=0;i<h.phnum;++i) {
        elf_segment p;memcpy(&p,data+h.phoff+(uint64_t)i*56,sizeof(p));
        if(p.type==2 || p.type==3)return 3;
        if(p.type!=1)continue;
        if(!p.memsz || p.filesz>p.memsz || p.offset>size || p.filesz>size-p.offset ||
           p.vaddr!=p.paddr || p.paddr<0x200000 || p.paddr>=0x1000000 || p.memsz>0x1000000-p.paddr ||
           p.flags>7 || (p.flags&3)==3 || p.align<4096 || p.align>0x200000 || (p.align&(p.align-1)) ||
           (p.vaddr%p.align)!=(p.offset%p.align))return 4;
        uint64_t start=p.paddr&~UINT64_C(4095),end=(p.paddr+p.memsz+4095)&~UINT64_C(4095);
        for(uint16_t j=0;j<count;++j) {
            uint64_t a=loads[j].paddr&~UINT64_C(4095),b=(loads[j].paddr+loads[j].memsz+4095)&~UINT64_C(4095);
            if(start<b && a<end)return 5;
        }
        loads[count++]=p;
        if(start<low)low=start;if(end>high)high=end;
        if((p.flags&1) && h.entry>=p.paddr && h.entry-p.paddr<p.filesz)entry_ok=1;
    }
    if(!count || !entry_ok)return 6;
    plan->low=low;plan->high=high;plan->entry=h.entry;plan->count=count;return 0;
}
#endif
