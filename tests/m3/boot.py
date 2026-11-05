"""End-to-end real kernel firmware handoff and failure paths, ISO + actual USB device."""
from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools'))
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/build'))
from media import package
from vm import ROOT,arguments,qemu_tools

def main():
    files=qemu_tools();build=ROOT/'build/m3';evidence=ROOT/'evidence/M3';evidence.mkdir(parents=True,exist_ok=True)
    manifest=json.loads((ROOT/'dist/m3-build-manifest.json').read_text(encoding='utf-8'))
    build_id=manifest['build_id'];efi=(build/'BOOTX64.EFI').read_bytes();kernel=(build/'KERNEL.ELF').read_bytes()
    cases=[('iso-smoke','iso',1,512,33,'ENGINE_OS_KERNEL_ONLINE'),
           ('usb-smoke','usb',1,512,33,'ENGINE_OS_KERNEL_ONLINE'),
           ('iso-lowram','iso',1,64,33,'ENGINE_OS_KERNEL_ONLINE'),
           ('iso-large-guest','iso',1,2048,33,'ENGINE_OS_KERNEL_ONLINE'),
           ('iso-retry','iso',4,512,33,'EXIT_BOOT_SERVICES_RETRY'),
           ('iso-panic','iso',2,512,67,'OWN_IDT_EXCEPTION_HANDLED'),
           ('iso-badboot','iso',3,512,71,'KERNEL_REJECT: boot header'),
           ('iso-badelf','iso',1,512,69,'LOADER_REJECT: ELF validation')]
    results={'scope':'M3 first increment: single-CPU timer preemption; remaining M3 gates pending', 'build_id':build_id,'checks':[]}
    for name,media,mode,ram,exit_code,marker in cases:
        work=build/'tests'/name
        init=struct.pack('<8sII32s',b'ENGINIT1',48,mode,build_id.encode())
        image_kernel=kernel if name!='iso-badelf' else bytes(64)
        package(work,efi,image_kernel,init,'M3')
        image=work/('EngineOS-M3.iso' if media=='iso' else 'EngineOS-M3-usb.img')
        argv,serial=arguments(files,media,image,work,ram=ram)
        argv[argv.index('-smp')+1]='1'
        error=''
        try:
            proc=subprocess.run(argv,capture_output=True,text=True,timeout=45,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
            actual=proc.returncode;error=proc.stderr.replace(str(ROOT),'<project-root>')
        except subprocess.TimeoutExpired:
            actual=None;error='timeout after 45 seconds'
        log=serial.read_text(encoding='utf-8',errors='replace') if serial.exists() else ''
        log=re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]','',log)
        (evidence/f'{name}.log').write_text(log,encoding='utf-8')
        passed=actual==exit_code and marker in log and f'ENGINE_OS_LOADER build={build_id}' in log
        if name!='iso-badelf':passed=passed and 'EXIT_BOOT_SERVICES_OK' in log and f'ENGINE_OS_KERNEL_ENTRY build={build_id}' in log
        if 'smoke' in name or name=='iso-lowram':passed=passed and 'BOOTINFO_AND_MEMORY_OWNERSHIP_OK' in log and 'OWN_GDT_TSS_IDT_OK' in log
        if name=='iso-panic':passed=passed and 'symbol=kernel_fault_probe+' in log and 'exception=0x0000000000000006' in log
        if name in ('iso-smoke','usb-smoke','iso-lowram','iso-large-guest','iso-retry'):
            match=re.search(r'M2_STATS (.*)',log)
            stats={key:int(value,16) for key,value in re.findall(r'(\w+)=(0x[0-9a-f]+)',match.group(1))} if match else {}
            reservation=re.search(r'M2_BOOT_RESERVED_BYTES=(0x[0-9a-f]+)',log)
            stats['boot_reserved_bytes']=int(reservation.group(1),16) if reservation else 2**64
            passed=passed and 'M2_ISOLATION_PASS' in log and 'M3_PREEMPTION_PASS' in log and stats.get('starts')==47 and stats.get('isolated_faults')==10 and stats.get('failures')==0
            passed=passed and stats.get('peak_owned_bytes',2**64)<=512*1024 and stats.get('retained_owned_bytes',2**64)<=64*1024
            passed=passed and stats['boot_reserved_bytes']<=1024*1024 and stats['boot_reserved_bytes']+stats.get('peak_owned_bytes',2**64)<=2*1024*1024
            passed=passed and all(marker in log for marker in ('HANDLE_RIGHTS_STALE_AND_REUSE_OK','HANDLE_QUOTA_AND_CLEANUP_OK','IPC_QUEUE_BOUND_OK','IPC_AND_EXPLICIT_SHARED_MEMORY_OK','PRODUCER_SURVIVED_OTHER_FAULTS','USER_RETURN_REJECTED','INVALID_POINTERS_AND_SYSCALL_OK'))
            preemptions=re.search(r'M3_PREEMPTIONS=(0x[0-9a-f]+)',log)
            stats['preemptions']=int(preemptions.group(1),16) if preemptions else 0
            passed=passed and stats['preemptions']>=16 and stats.get('exits')==37
            passed=passed and all(m in log for m in ('NON_YIELDING_HOG_RESUMED_OK','NON_YIELDING_PEER_PROGRESS_OK'))
            results.setdefault('memory_measurements',{})[name]=stats
        if name=='iso-panic':
            import shutil
            rip=re.search(r' rip=(0x[0-9a-f]+)',log)
            symbolizer=Path(shutil.which('clang')).parent/'llvm-symbolizer.exe'
            symbolized=subprocess.run([str(symbolizer),'--obj='+str(build/'KERNEL.ELF'),rip.group(1) if rip else '0'],
                                      capture_output=True,text=True,timeout=10)
            text=symbolized.stdout
            if re.search(r'[Cc]:[\\/]+Users[\\/]+',text):raise RuntimeError('Host path in debug symbols')
            passed=passed and symbolized.returncode==0 and 'kernel_fault_probe' in text and 'fault.S:5:' in text
            (evidence/'panic-symbol.txt').write_text(text.rstrip()+'\n',encoding='utf-8')
        results['checks'].append({'name':name,'passed':passed,'media':media,'ram_mib':ram,'expected_exit':exit_code,'actual_exit':actual,
                                  'expected_marker':marker,'stderr':error,'log_sha256':hashlib.sha256(log.encode()).hexdigest()})
        print(f'{name}: {"PASS" if passed else "FAIL"} (exit {actual})',flush=True)
        if not passed:print(log[-4000:])
    results['passed']=all(c['passed'] for c in results['checks'])
    (evidence/'boot-results.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')
    if not results['passed']:raise SystemExit(1)
if __name__=='__main__':main()
