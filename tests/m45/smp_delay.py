"""Pause only the owned QEMU AP thread; distinguish temporary delay from a dead CPU."""
from pathlib import Path
import argparse,ctypes,json,re,shutil,struct,subprocess,sys,time
from ctypes import wintypes as w
sys.path[:0]=[str(Path(__file__).resolve().parents[2]/'tools'),str(Path(__file__).resolve().parents[2]/'tools/build'),str(Path(__file__).resolve().parents[1]/'m3')]
from vm import ROOT,arguments,qemu_tools
from media import package
from visual import free_port,connect

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--delay-ms',type=int,default=150);parser.add_argument('--expected-exit',type=int,default=33);parser.add_argument('--label',default='short-delay');parser.add_argument('--legacy-check',action='store_true');options=parser.parse_args()
    if not 50<=options.delay_ms<=3000 or options.expected_exit not in (33,99) or (options.legacy_check and options.expected_exit!=99) or not re.fullmatch('[a-z0-9-]+',options.label):raise ValueError('Bounded arguments required')
    work=ROOT/'build/m45/smp-delay'/options.label;work.mkdir(parents=True,exist_ok=True);out=ROOT/'evidence/M4.5/smp-delay'/options.label;out.mkdir(parents=True,exist_ok=True);build=ROOT/'build/m45';manifest=json.loads((ROOT/'dist/m45-build-manifest.json').read_text(encoding='utf-8'));package(work,(build/'BOOTX64.EFI').read_bytes(),(build/'KERNEL.ELF').read_bytes(),struct.pack('<8sII32s',b'ENGINIT1',48,1,manifest['build_id'].encode()),'M45');args,serial=arguments(qemu_tools(),'iso',work/'EngineOS-M45.iso',work/'vm',ram=128);port=free_port();gdb_port=free_port();args+=['-qmp',f'tcp:127.0.0.1:{port},server=on,wait=off','-gdb',f'tcp:127.0.0.1:{gdb_port}','-S'];process=subprocess.Popen(args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));error='';thread=None;suspended=False;paused=False;duration=0;address=0
    api=ctypes.WinDLL('kernel32',use_last_error=True);api.OpenThread.argtypes=[w.DWORD,w.BOOL,w.DWORD];api.OpenThread.restype=w.HANDLE;api.GetProcessIdOfThread.argtypes=[w.HANDLE];api.GetProcessIdOfThread.restype=w.DWORD;api.SuspendThread.argtypes=[w.HANDLE];api.SuspendThread.restype=w.DWORD;api.ResumeThread.argtypes=[w.HANDLE];api.ResumeThread.restype=w.DWORD;api.CloseHandle.argtypes=[w.HANDLE]
    def log():return serial.read_text(encoding='utf-8',errors='replace') if serial.exists() else ''
    def wait(marker):
        end=time.monotonic()+45
        while time.monotonic()<end:
            if marker in log():return
            if process.poll() is not None:break
            time.sleep(.001)
        raise RuntimeError('Missing delay gate: '+marker)
    try:
        with connect(port,process) as sock,connect(gdb_port,process) as debugger:
            debugger.settimeout(45)
            def packet(command,reply=True):
                payload=command.encode();debugger.sendall(b'$'+payload+b'#'+f'{sum(payload)%256:02x}'.encode())
                if not reply:return b''
                def receive(count):
                    data=b''
                    while len(data)<count:
                        chunk=debugger.recv(count-len(data))
                        if not chunk:raise RuntimeError('GDB endpoint closed')
                        data+=chunk
                    return data
                while receive(1)!=b'$':pass
                raw=b''
                while True:
                    byte=receive(1)
                    if not byte:raise RuntimeError('GDB endpoint closed')
                    if byte==b'#':break
                    raw+=byte
                checksum=receive(2)
                if len(checksum)!=2 or int(checksum,16)!=sum(raw)%256:raise RuntimeError('GDB response checksum mismatch')
                debugger.sendall(b'+');decoded=bytearray();i=0
                while i<len(raw):
                    value=raw[i];i+=1
                    if value==125:
                        value=raw[i]^32;i+=1;decoded.append(value)
                    elif value==42:
                        decoded.extend([decoded[-1]]*(raw[i]-29));i+=1
                    else:decoded.append(value)
                return bytes(decoded)
            nm=Path(shutil.which('clang')).parent/'llvm-nm.exe';symbols=subprocess.check_output([str(nm),'-n',str(build/'KERNEL.ELF')],text=True);address=int(re.search(r'^([0-9a-f]+) T m3_smp_pressure$',symbols,re.M).group(1),16)
            packet('qSupported')
            if packet(f'Z0,{address:x},1')!=b'OK':raise RuntimeError('Cannot arm pressure-entry breakpoint')
            stream=sock.makefile('rwb');stream.readline()
            def qmp(command):
                stream.write((json.dumps({'execute':command})+'\n').encode());stream.flush()
                while True:
                    response=json.loads(stream.readline())
                    if 'return' in response:return response['return']
                    if 'error' in response:raise RuntimeError(str(response['error']))
            qmp('qmp_capabilities');stop=packet('c')
            if not stop.startswith((b'T05',b'S05')) or 'M3_CPUS_ACTIVE=' not in log():raise RuntimeError('Pressure-entry breakpoint was not reached in the native runtime')
            if int.from_bytes(bytes.fromhex(packet('p10').decode()),'little')!=address or int.from_bytes(bytes.fromhex(packet('p5').decode()),'little')!=1:raise RuntimeError('Breakpoint is not the first pressure-begin call')
            if packet(f'z0,{address:x},1')!=b'OK':raise RuntimeError('Cannot remove pressure-entry breakpoint')
            cpu=next(c for c in qmp('query-cpus-fast') if c['cpu-index']==1);thread=api.OpenThread(0x802,False,cpu['thread-id'])
            if not thread or api.GetProcessIdOfThread(thread)!=process.pid:raise RuntimeError('AP thread does not belong to the owned test VM')
            if process.poll() is not None:raise RuntimeError('Owned test VM already exited')
            if api.SuspendThread(thread)==0xffffffff:raise ctypes.WinError(ctypes.get_last_error())
            suspended=True;paused=True;start=time.monotonic();packet('c',False);wait(('M45' if options.legacy_check else 'M3')+'_PRESSURE_WINDOW_BEGIN phase=0x0000000000000000');time.sleep(options.delay_ms/1000);duration=time.monotonic()-start;api.ResumeThread(thread);suspended=False
        process.communicate(timeout=60)
    except Exception as exception:error=str(exception).replace(str(ROOT),'<project-root>')
    finally:
        if suspended and thread:api.ResumeThread(thread)
        if thread:api.CloseHandle(thread)
        if process.poll() is None:process.terminate();process.wait(timeout=5)
    text=re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]','',log());(out/'serial.log').write_bytes(('\n'.join(line.rstrip() for line in text.splitlines())+'\n').encode());waits=re.search(r'M3_PRESSURE_WAIT_HALTS=(0x[0-9a-f]+)',text);passed=not error and paused and process.returncode==options.expected_exit and ('M45_PRESSURE_MISSING_TICK' if options.legacy_check else 'M3_PRESSURE_WAIT cpu=') in text and ('M3_COMPLETE_PASS' in text and bool(waits) and int(waits.group(1),16)>0 if options.expected_exit==33 else 'M3_SMP_FAIL CPU unresponsive under pressure' in text);result={'build_id':manifest['build_id'],'full_m45_complete':False,'checks':[{'name':'owned AP thread delay at native pressure boundary','passed':passed,'delay_seconds':duration,'requested_hold_after_baseline_ms':options.delay_ms,'exit':process.returncode,'expected_exit':options.expected_exit,'wait_halts':int(waits.group(1),16) if waits else None,'legacy_assertion_reproduction':options.legacy_check,'injection_gate':'GDB all-stop before first pressure baseline; AP thread ownership checked','breakpoint_address':hex(address)}],'error':error};(out/'results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print(json.dumps(result,indent=2));raise SystemExit(0 if passed else 1)
if __name__=='__main__':main()
