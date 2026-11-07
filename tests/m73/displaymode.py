"""Host contracts for the exact original UEFI mode-selector C, not GPU proof."""
import ctypes
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import traceback
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools/build'));from m1 import pinned_tools
WRAPPER=r'''
#include "boot/uefi/uefi.h"
static efi_boot_services services;static efi_boot_services *bs=&services;
#define serial_text host_text
#define serial_hex host_hex
static void host_text(const char*s){(void)s;}static void host_hex(uint64_t n){(void)n;}
#include "boot/uefi/display73.inc"
static uint8_t bytes[128];static struct{uint32_t size;uint8_t*data;} edid;
static efi_gop_info modes[3];static efi_gop_mode state;static efi_gop gop;
static unsigned sets,queries,frees,present,refuse,short_info;
static efi_status locate(efi_guid*g,void*r,void**out){(void)r;static const efi_guid expected={0xbd8c1056,0x9f36,0x44ec,{0x92,0xa8,0xa6,0x33,0x7f,0x81,0x79,0x86}};if(!present||memcmp(g,&expected,16))return UINT64_C(0x800000000000000e);*out=&edid;return 0;}
static efi_status query(efi_gop*g,uint32_t mode,uint64_t*size,efi_gop_info**info){(void)g;++queries;if(mode>=3)return UINT64_C(0x8000000000000002);*size=short_info?8:sizeof(**info);*info=modes+mode;return 0;}
static efi_status release(void*p){(void)p;++frees;return 0;}
static efi_status select_mode(efi_gop*g,uint32_t mode){++sets;if(refuse)return UINT64_C(0x8000000000000007);g->mode->mode=mode;return 0;}
static void reset(void){memset(&services,0,sizeof(services));services.locate_protocol=locate;services.free_pool=(void*)release;sets=queries=frees=refuse=short_info=0;present=1;memset(bytes,0,sizeof(bytes));for(unsigned i=1;i<7;++i)bytes[i]=255;bytes[54]=1;bytes[58]=0xf0;bytes[59]=0x70;bytes[61]=0x80;unsigned sum=0;for(unsigned i=0;i<127;++i)sum+=bytes[i];bytes[127]=(uint8_t)(0-sum);edid.size=128;edid.data=bytes;memset(modes,0,sizeof(modes));modes[0].width=1024;modes[0].height=768;modes[1].width=1920;modes[1].height=1080;modes[2].width=3840;modes[2].height=2160;state=(efi_gop_mode){3,0,modes,sizeof(modes[0]),0,0};gop=(efi_gop){(void*)query,(void*)select_mode,0,&state};}
unsigned display73_contracts(void){unsigned fail=0;
 reset();preferred_display73(&gop);if(sets!=1||state.mode!=2||queries!=6||frees!=6)fail|=1;
 reset();present=0;preferred_display73(&gop);if(sets||state.mode)fail|=2;
 reset();bytes[20]^=1;preferred_display73(&gop);if(sets)fail|=4;
 reset();bytes[0]=1;preferred_display73(&gop);if(sets)fail|=8;
 reset();short_info=1;preferred_display73(&gop);if(sets||queries!=frees)fail|=16;
 reset();modes[2].format=2;preferred_display73(&gop);if(sets)fail|=32;
 reset();refuse=1;preferred_display73(&gop);if(sets!=1||state.mode)fail|=64;
 reset();state.max_mode=257;preferred_display73(&gop);if(sets||queries!=256)fail|=128;
 return fail;
}
'''
def main():
 out=ROOT/'evidence/M7.3/displaymode';out.mkdir(parents=True,exist_ok=True);work=ROOT/'build/m73/displaymode';work.mkdir(parents=True,exist_ok=True);report={'passed':False}
 try:
  tools=pinned_tools();source=work/'probe.c';source.write_bytes(WRAPPER.encode());objects=[]
  for path,name in [(source,'probe'),(ROOT/'runtime/memory.c','memory')]:
   obj=work/(name+'.obj');objects.append(obj);args=[tools['clang'],'--target=x86_64-pc-windows-msvc','-std=c17','-O2','-ffreestanding','-fno-builtin','-fno-stack-protector','-Wall','-Wextra','-Werror','-I',ROOT,'-I',ROOT/'include','-c',path,'-o',obj];r=subprocess.run(list(map(str,args)),capture_output=True,text=True);assert r.returncode==0,r.stderr
  args=[tools['lld-link'],'/dll','/noentry','/nodefaultlib','/export:display73_contracts',*objects,'/out:'+str(work/'probe.dll')];r=subprocess.run(list(map(str,args)),capture_output=True,text=True);assert r.returncode==0,r.stderr
  library=ctypes.CDLL(str(work/'probe.dll'));library.display73_contracts.restype=ctypes.c_uint;assert library.display73_contracts()==0
  report.update(passed=True,source_sha256=hashlib.sha256((ROOT/'boot/uefi/display73.inc').read_bytes()).hexdigest(),scope='host protocol contracts; actual framebuffer proof is in the native resolution tasks',checks=['validated preferred 3840x2160 mode selected; each returned info buffer freed','missing EDID, invalid header/checksum, short GOP info and unsupported pixel format retain current mode','SetMode failure retains current mode','mode enumeration bounded to 256'])
 except Exception:report['error']=traceback.format_exc().replace(str(ROOT),'<project-root>');print(report['error'])
 (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M7.3 display contracts '+str(report['passed']),flush=True)
 if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
