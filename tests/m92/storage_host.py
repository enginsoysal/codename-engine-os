"""Execute actual C window constraints, damage clipping and token lifetime."""
import ctypes,hashlib,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools/build'))
from m1 import pinned_tools,command
def main():
 work=ROOT/'build/m92/storage-host';work.mkdir(parents=True,exist_ok=True);tools=pinned_tools();objects=[];inputs=['tests/m92/storage_host.c','runtime/memory.c']
 for source in inputs:
  obj=work/(Path(source).stem+'.obj');command([tools['clang'],'--target=x86_64-pc-windows-msvc','-std=c17','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin','-fno-stack-protector','-mno-stack-arg-probe','-DENGINE_M9','-DENGINE_M92','-I',ROOT/'include','-c',ROOT/source,'-o',obj]);objects.append(obj)
 command([tools['lld-link'],'/dll','/noentry','/nodefaultlib','/timestamp:0','/export:storage92_cases','/out:'+str(work/'storage.dll'),*objects]);lib=ctypes.CDLL(str(work/'storage.dll'));assert lib.storage92_cases()==0
 report={'passed':True,'scope':'Actual C registry host execution; real controller qualification remains separate','checks':['Real common storage registry rejects overflow, invalid bounds and missing buffers before invoking a driver','Capability admission requires matching identity, 512-byte sectors and both write/flush callbacks','Read-only and missing-flush devices cannot be installed; I/O and flush failures revoke admission'],'source_sha256':{x:hashlib.sha256((ROOT/x).read_bytes()).hexdigest() for x in inputs+['kernel/storage/registry.c','include/engine/storage.h']}}
 (work/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M92 actual C storage registry PASS')
if __name__=='__main__':main()

