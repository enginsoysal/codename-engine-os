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
    cases=[('iso-clock-deterministic','iso',1,128,1,33,'ENGINE_OS_KERNEL_ONLINE'),
           ('iso-smoke','iso',1,512,1,33,'ENGINE_OS_KERNEL_ONLINE'),
           ('usb-smoke','usb',1,128,2,33,'ENGINE_OS_KERNEL_ONLINE'),
           ('iso-lowram','iso',1,64,1,33,'ENGINE_OS_KERNEL_ONLINE'),
           ('iso-smp2','iso',1,64,2,33,'ENGINE_OS_KERNEL_ONLINE'),
           ('iso-smp4','iso',1,64,4,33,'ENGINE_OS_KERNEL_ONLINE'),
           ('iso-smp4-repeat1','iso',1,128,4,33,'ENGINE_OS_KERNEL_ONLINE'),
           ('iso-smp4-repeat2','iso',1,128,4,33,'ENGINE_OS_KERNEL_ONLINE'),
           ('iso-large-guest','iso',1,2048,2,33,'ENGINE_OS_KERNEL_ONLINE'),
           ('iso-retry','iso',4,128,4,33,'EXIT_BOOT_SERVICES_RETRY'),
           ('iso-tlb-negative','iso',1,128,2,99,'M3_SMP_FAIL TLB or CPU state'),
           ('iso-panic','iso',2,128,2,67,'OWN_IDT_EXCEPTION_HANDLED'),
           ('iso-badboot','iso',3,128,2,71,'KERNEL_REJECT: boot header'),
           ('iso-badelf','iso',1,128,2,69,'LOADER_REJECT: ELF validation')]
    results={'scope':'M3 native VM qualification: preemption, threads, waits, COW/pressure and active SMP', 'build_id':build_id,'checks':[]}
    for name,media,mode,ram,cpus,exit_code,marker in cases:
        work=build/'tests'/name
        init=struct.pack('<8sII32s',b'ENGINIT1',48,mode,build_id.encode())
        image_kernel=kernel if name!='iso-badelf' else bytes(64)
        if name=='iso-tlb-negative':
            import shutil
            nm=Path(shutil.which('clang')).parent/'llvm-nm.exe'
            symbols=subprocess.check_output([str(nm),'--defined-only',str(build/'KERNEL.ELF')],text=True)
            symbol=re.search(r'^([0-9a-fA-F]+)\s+\w\s+m3_invalidate_shared$',symbols,re.M)
            if not symbol:raise RuntimeError('TLB injection symbol missing')
            address=int(symbol.group(1),16);modified=bytearray(kernel);phoff=struct.unpack_from('<Q',kernel,32)[0]
            entsize,count=struct.unpack_from('<HH',kernel,54);patched=False
            for i in range(count):
                typ,flags,offset,va,pa,filesz,memsz,align=struct.unpack_from('<IIQQQQQQ',kernel,phoff+i*entsize)
                if typ==1 and va<=address<va+filesz:
                    location=offset+address-va
                    if modified[location:location+2]!=b'\x48\xb8':raise RuntimeError('Unexpected invalidation entry bytes')
                    modified[location]=0xc3;patched=True;break
            if not patched:raise RuntimeError('TLB injection outside loadable text')
            image_kernel=bytes(modified)
        package(work,efi,image_kernel,init,'M3')
        image=work/('EngineOS-M3.iso' if media=='iso' else 'EngineOS-M3-usb.img')
        argv,serial=arguments(files,media,image,work,ram=ram)
        argv[argv.index('-smp')+1]=str(cpus)
        if name=='iso-clock-deterministic':
            argv[argv.index('-accel')+1]='tcg,thread=single'
            argv+=['-icount','shift=3,align=off,sleep=on']
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
        if exit_code==33:
            match=re.search(r'M2_STATS (.*)',log)
            stats={key:int(value,16) for key,value in re.findall(r'(\w+)=(0x[0-9a-f]+)',match.group(1))} if match else {}
            reservation=re.search(r'M2_BOOT_RESERVED_BYTES=(0x[0-9a-f]+)',log)
            stats['boot_reserved_bytes']=int(reservation.group(1),16) if reservation else 2**64
            passed=passed and 'M2_ISOLATION_PASS' in log and 'M3_PREEMPTION_PASS' in log and stats.get('starts')==47 and stats.get('isolated_faults')==10 and stats.get('failures')==0
            passed=passed and stats.get('peak_owned_bytes',2**64)<=512*1024 and stats.get('retained_owned_bytes')==(15+8*(cpus-1))*4096
            passed=passed and stats['boot_reserved_bytes']<=1024*1024 and stats['boot_reserved_bytes']+stats.get('peak_owned_bytes',2**64)<=2*1024*1024
            passed=passed and all(marker in log for marker in ('HANDLE_RIGHTS_STALE_AND_REUSE_OK','HANDLE_QUOTA_AND_CLEANUP_OK','IPC_QUEUE_BOUND_OK','IPC_AND_EXPLICIT_SHARED_MEMORY_OK','PRODUCER_SURVIVED_OTHER_FAULTS','USER_RETURN_REJECTED','INVALID_POINTERS_AND_SYSCALL_OK'))
            preemptions=re.search(r'M3_PREEMPTIONS=(0x[0-9a-f]+)',log)
            stats['preemptions']=int(preemptions.group(1),16) if preemptions else 0
            passed=passed and stats['preemptions']>=16 and stats.get('exits')==37
            passed=passed and all(m in log for m in ('NON_YIELDING_HOG_RESUMED_OK','NON_YIELDING_PEER_PROGRESS_OK'))
            def parsed(prefix):
                match=re.search(re.escape(prefix)+r'(.*)',log)
                return {k:int(v,16) for k,v in re.findall(r'(\w+)=(0x[0-9a-f]+)',match.group(1))} if match else {}
            runtime=parsed('M3_RUNTIME_STATS ');smp=parsed('M3_SMP_STATS ');dispatch=parsed('M3_DISPATCH_TSC ')
            roundtrip=parsed('M3_ROUNDTRIP_TSC ');roundtrip.update(parsed('M3_ROUNDTRIP_PERCENTILES '))
            passed=passed and roundtrip.get('samples')==64 and 0<roundtrip.get('min',0)<=roundtrip.get('p50',0)<=roundtrip.get('p95',0)<=roundtrip.get('max',0)
            cpu_mask=re.search(r'M3_USER_CPU_MASK=(0x[0-9a-f]+)',log)
            state_mask=re.search(r'M3_CPU_STATE_MASK=(0x[0-9a-f]+)',log)
            passed=passed and state_mask is not None and int(state_mask.group(1),16)==(1<<cpus)-1
            passed=passed and cpu_mask is not None and int(cpu_mask.group(1),16)==(1<<cpus)-1
            passed=passed and smp.get('cpus')==cpus and smp.get('concurrent_allocations')==cpus*512 and smp.get('pressure_checks')==5
            passed=passed and all(runtime.get(k)==v for k,v in {'threads':85,'exits':73,'cancellations':10,'isolated_faults':2,'cow_faults':2,'copy_cow':1,'oom_faults':1}.items())
            passed=passed and runtime.get('idle_halts',0)>0 and runtime.get('switches',0)>=400
            passed=passed and runtime.get('thread_metadata_bytes',2**64)<=1024 and runtime.get('thread_resident_bytes',2**64)<=10240
            passed=passed and runtime.get('runtime_peak_owned_bytes',2**64)<=512*1024
            passed=passed and dispatch.get('samples')==256 and 0<dispatch.get('min',0)<=dispatch.get('p50',0)<=dispatch.get('p95',0)<=dispatch.get('max',0)
            passed=passed and all(m in log for m in ('M3_COMPLETE_PASS','M3_RUNTIME_PASS','M3_SMP_ALLOCATOR_TLB_TIMER_PASS','M3_TRANSFER_IPC_SHARED_LIFETIME_OK','M3_MONOTONIC_CLOCK_CATCHUP_OK','M3_HANDLE_RIGHTS_QUOTA_AND_MAPPING_LIFETIME_OK','SSE_X87_MXCSR_TLS_ISOLATION_OK','AP_SIMD_CONTEXT_OK','EVENT_ATOMIC_WAIT_WAKE_REUSE_OK','DEADLINE_CANCEL_CLOSE_AND_STALE_THREAD_OK','THREAD_QUOTA_NOMEM_ROLLBACK_AND_REUSE_OK','BLOCKING_MAILBOX_OK','COW_CHILD_PRIVATE_WRITE_OK','VM_RESERVE_COMMIT_COW_PRESSURE_RECOVERY_OK','ADVANCED_FAULT_AND_RETURN_ISOLATION_OK','PROCESS_EXIT_CANCELS_BLOCKED_THREADS_OK'))
            if name=='iso-clock-deterministic':passed=passed and runtime.get('deadline_late_ticks',2**64)<=5
            results.setdefault('runtime_measurements',{})[name]={'runtime':runtime,'smp':smp,'dispatch_tsc':dispatch,'roundtrip_tsc':roundtrip,'user_cpu_mask':int(cpu_mask.group(1),16) if cpu_mask else 0}
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
        results['checks'].append({'name':name,'passed':passed,'media':media,'ram_mib':ram,'active_cpus':cpus,'deterministic_clock':name=='iso-clock-deterministic','expected_exit':exit_code,'actual_exit':actual,
                                  'expected_marker':marker,'stderr':error,'log_sha256':hashlib.sha256(log.encode()).hexdigest()})
        print(f'{name}: {"PASS" if passed else "FAIL"} (exit {actual})',flush=True)
        if not passed:print(log[-4000:])
    results['passed']=all(c['passed'] for c in results['checks'])
    (evidence/'boot-results.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')
    if not results['passed']:raise SystemExit(1)
if __name__=='__main__':main()
