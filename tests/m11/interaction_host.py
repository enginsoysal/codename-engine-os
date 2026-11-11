"""Exercise the production pointer classifier, including cancellation and timing."""
import ctypes,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/build'))
from m1 import pinned_tools,command
work=ROOT/'build/m11/input-host';work.mkdir(parents=True,exist_ok=True);t=pinned_tools()
command([t['clang'],'--target=x86_64-pc-windows-msvc','-std=c17','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin','-I',ROOT/'include','-c',ROOT/'tests/m11/interaction_host.c','-o',work/'input.obj'])
command([t['lld-link'],'/dll','/noentry','/nodefaultlib','/export:interaction11_checks','/out:'+str(work/'input.dll'),work/'input.obj'])
result=ctypes.CDLL(str(work/'input.dll')).interaction11_checks();assert result==0,('Production input assertion',result)
r={'passed':True,'scope':'Production pointer classifier on host; not native integration proof','checks':['Single selection and double-open on matching target','Triple click, other target and timeout do not double-open','Secondary button never activates primary action','Threshold drag and release-outside cancellation','Button activation on release, Escape cancellation and bounded interval'],'source_sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in ['include/engine/desktop/interaction11.h','tests/m11/interaction_host.c']}}
(work/'results.json').write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8');print('M11 production pointer checks PASS')
