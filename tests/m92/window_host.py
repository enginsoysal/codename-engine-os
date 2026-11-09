"""Execute actual C window constraints, damage clipping and token lifetime."""
import ctypes,hashlib,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools/build'))
from m1 import pinned_tools,command
def main():
 work=ROOT/'build/m92/window-host';work.mkdir(parents=True,exist_ok=True);tools=pinned_tools();objects=[];inputs=['tests/m92/window_host.c','kernel/desktop/window.c','runtime/memory.c']
 for source in inputs:
  obj=work/(Path(source).stem+'.obj');command([tools['clang'],'--target=x86_64-pc-windows-msvc','-std=c17','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin','-fno-stack-protector','-DENGINE_M9','-DENGINE_M92','-I',ROOT/'include','-c',ROOT/source,'-o',obj]);objects.append(obj)
 command([tools['lld-link'],'/dll','/noentry','/nodefaultlib','/timestamp:0','/export:window92_cases','/out:'+str(work/'window.dll'),*objects]);lib=ctypes.CDLL(str(work/'window.dll'));assert lib.window92_cases()==0
 report={'passed':True,'scope':'Actual C model host execution; native graphics proof remains separate','checks':['Partial side/bottom overlap with reachable title-bar grip','Extreme signed origins and dimensions remain bounded','Viewport-clipped damage and stale-token refusal'],'source_sha256':{x:hashlib.sha256((ROOT/x).read_bytes()).hexdigest() for x in inputs}}
 (work/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M92 actual C window model PASS')
if __name__=='__main__':main()
