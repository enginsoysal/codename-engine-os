"""Execute production layout/wheel code and bounded HID descriptor admission."""
import ctypes as c,hashlib,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/build'))
from m1 import pinned_tools,command
def main():
    work=ROOT/'build/m10/input-host';work.mkdir(parents=True,exist_ok=True);tools=pinned_tools();objects=[]
    inputs=['tests/m10/input_host.c','kernel/input/layout10.c']
    for name in inputs:
        obj=work/(Path(name).stem+'.obj');command([tools['clang'],'--target=x86_64-pc-windows-msvc','-std=c17','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin','-I',ROOT/'include','-c',ROOT/name,'-o',obj]);objects.append(obj)
    exports=['wheel_events10','descriptor10','input10_hid','input10_toggle_caps','input10_set_layout','input10_layout','input10_wheel','input10_take_wheel']
    command([tools['lld-link'],'/dll','/noentry','/nodefaultlib',*['/export:'+n for n in exports],'/out:'+str(work/'input.dll'),*objects]);lib=c.CDLL(str(work/'input.dll'))
    lib.input10_take_wheel.restype=c.c_int
    for code in range(4,30):
        assert lib.input10_hid(code,0)==ord('a')+code-4
        assert lib.input10_hid(code,1)==ord('A')+code-4
    lib.input10_toggle_caps();assert lib.input10_hid(4,0)==ord('A') and lib.input10_hid(4,1)==ord('a')
    assert lib.input10_hid(30,0)==ord('1') and lib.input10_hid(30,1)==ord('!')
    lib.input10_toggle_caps();lib.input10_set_layout(1);assert lib.input10_hid(20,0)==ord("'") and lib.input10_hid(54,0)==ord('w')
    lib.input10_set_layout(99);assert lib.input10_layout()==1;lib.input10_set_layout(0)
    for _ in range(100):lib.input10_wheel(1)
    assert lib.wheel_events10()==1 and lib.input10_take_wheel()==32 and lib.input10_take_wheel()==0
    for _ in range(100):lib.input10_wheel(-1)
    assert lib.wheel_events10()==2 and lib.input10_take_wheel()==-32
    descriptor=bytes.fromhex('05010902a1010901a1000509190129031500250195037501810295017505810105010930093109381581257f750895038106c0c0')
    lib.descriptor10.argtypes=[c.c_char_p,c.c_uint]
    assert lib.descriptor10(descriptor,len(descriptor))==1
    for bad in (b'',b'\xfe',b'\x85\x01'+descriptor,descriptor[:-6],descriptor.replace(b'\x81\x06',b'\x81\x02'),descriptor*6):
        assert lib.descriptor10(bad,len(bad))==0
    report={'passed':True,'scope':'Production input logic on host; native controller/input tasks tested separately','checks':['US QWERTY and Dvorak, Shift/Caps Lock, invalid layout refusal','Bounded wheel accumulation and transition-only wake events','Descriptor-proven wheel admission; malformed, report-ID, oversized and absolute-axis refusals'],'source_sha256':{n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in inputs+['include/engine/hid10.h']}}
    (work/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');print('M10 input host PASS')
if __name__=='__main__':main()
