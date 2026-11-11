"""Production Start record and static virtual ACPI-package validation; not guest proof."""
import ctypes,hashlib,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools/build'))
from m1 import command,pinned_tools
work=ROOT/'build/m11/start-host';work.mkdir(parents=True,exist_ok=True);t=pinned_tools()
command([t['clang'],'--target=x86_64-pc-windows-msvc','-std=c17','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin','-I',ROOT/'include','-c',ROOT/'tests/m11/start_host.c','-o',work/'checks.obj'])
command([t['lld-link'],'/dll','/noentry','/nodefaultlib','/export:start11_checks','/out:'+str(work/'checks.dll'),work/'checks.obj'])
value=ctypes.CDLL(str(work/'checks.dll')).start11_checks();assert value==0,('Production Start assertion',value)
inputs=['include/engine/desktop/start11.h','include/engine/desktop/power11.h','tests/m11/start_host.c','tests/m11/start_host.py']
report={'passed':True,'scope':'Production validation on host; native launch/preferences/power checks remain required','checks':['Version/magic/count/view/tab/alignment bounds','Category permutation, unique case-folded names and bounded text','App/folder pin identity, duplicate and path bounds','Taskbar identity/count/duplicate bounds','Static S5 package integer, length, truncation, ambiguity and state bounds'],'source_sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in inputs}}
(work/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');print('M11 production Start/power validation PASS')
