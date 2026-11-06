"""Prove the native live-memory cap, denial and complete test-page reclamation."""
from pathlib import Path
import json,struct,subprocess,sys
ROOT=Path(__file__).resolve().parents[2];sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/build')]
from vm import arguments,qemu_tools
from media import package

def main():
    work=ROOT/'build/m51/overlay';work.mkdir(parents=True,exist_ok=True);out=ROOT/'evidence/M5.1/resources';out.mkdir(parents=True,exist_ok=True);build=ROOT/'build/m5';manifest=json.loads((ROOT/'dist/m5-build-manifest.json').read_text(encoding='utf-8'))
    package(work,(build/'BOOTX64.EFI').read_bytes(),(build/'KERNEL.ELF').read_bytes(),struct.pack('<8sII32s',b'ENGINIT1',48,7,manifest['build_id'].encode()),'M5')
    argv,serial=arguments(qemu_tools(),'iso',work/'EngineOS-M5.iso',work/'guest',ram=64);p=subprocess.run(argv,capture_output=True,timeout=90,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));log=serial.read_text(encoding='utf-8',errors='replace');passed=p.returncode==33 and all(s in log for s in ['M3_COMPLETE_PASS','M5_OVERLAY_PASS','M5_OVERLAY_RELEASE_BASELINE_RESTORED'])
    (out/'serial.log').write_bytes(log.encode());(out/'results.json').write_text(json.dumps({'passed':passed,'build_id':manifest['build_id'],'guest_ram_mib':64,'overlay_cap_bytes':524288,'checks':[{'name':'128-page cap, 129th-page denial, reuse without allocation and exact allocator baseline after cleanup','passed':passed}]},indent=2)+'\n',encoding='utf-8');print('Native overlay', 'PASS' if passed else 'FAIL',flush=True)
    if not passed:raise SystemExit(1)
if __name__=='__main__':main()


def menu_resources():
    import importlib.util,re,time,ctypes
    from ctypes import wintypes as w
    spec=importlib.util.spec_from_file_location('m51_resource_guest',ROOT/'tests/m5/vm.py');q=importlib.util.module_from_spec(spec);spec.loader.exec_module(q)
    work=ROOT/'build/m51/menu-resources'/str(time.time_ns());work.mkdir(parents=True);disk=work/'blank.img'
    with disk.open('xb') as f:f.truncate(256*1024**2)
    with q.Guest(work/'guest',disk,ram=64) as g:
        g.wait('M51_HOME_READY');g.command('4','engine> ');g.command('info','engine> ')
        before=int(re.findall(r'Managed current bytes=(0x[0-9a-f]+)',g.log())[-1],16);g.command('menu','Choice> ')
        for i in range(8):g.command('5','Choice> ');g.command('0','Choice> ')
        g.command('4','engine> ');g.command('info','engine> ');after=int(re.findall(r'Managed current bytes=(0x[0-9a-f]+)',g.log())[-1],16);assert before==after
        g.command('menu','Choice> ');time.sleep(.2);regs=g.qmp('human-monitor-command',{'command-line':'info registers -a'});assert regs.count('HLT=1')==2
        api=ctypes.WinDLL('kernel32',use_last_error=True);api.GetProcessTimes.argtypes=[w.HANDLE]+[ctypes.POINTER(w.FILETIME)]*4;api.GetProcessTimes.restype=w.BOOL
        def cpu():
            values=[w.FILETIME() for _ in range(4)];assert api.GetProcessTimes(int(g.process._handle),*[ctypes.byref(v) for v in values]);return sum((v.dwHighDateTime<<32)|v.dwLowDateTime for v in values[2:])/1e7
        used=cpu();start=time.monotonic();time.sleep(2);elapsed=time.monotonic()-start;used=cpu()-used;assert used/elapsed<.35
        out=ROOT/'evidence/M5.1/resources';(out/'menu.log').write_bytes(g.log().encode());r=json.loads((out/'results.json').read_text(encoding='utf-8'));r['checks']+=[{'name':'repeated menu/details/terminal transitions allocate no additional managed pages','passed':True,'managed_before_bytes':before,'managed_after_bytes':after},{'name':'both CPUs halted at menu idle; two-second host-QEMU CPU observation','passed':True,'wall_seconds':elapsed,'host_cpu_seconds':used,'one_cpu_percent':used/elapsed*100}];(out/'results.json').write_bytes((json.dumps(r,indent=2)+'\n').encode());print('Menu resources PASS',flush=True)
if __name__=='__main__':menu_resources()
