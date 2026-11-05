"""Optional isolated VMware Workstation smoke test; never operates an existing VM."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
import time
ROOT=Path(__file__).resolve().parents[2]
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--vmrun',type=Path,default=Path('C:/Program Files/VMware/VMware Workstation/vmrun.exe'))
    parser.add_argument('--cpus',type=int,choices=(1,2,4),default=2)
    args=parser.parse_args();vmrun=args.vmrun
    if not vmrun.is_file():raise SystemExit('VMware vmrun is not installed; optional qualification unavailable')
    work=(ROOT/'build/m3/vmware'/f'{args.cpus}cpu').resolve();work.mkdir(parents=True,exist_ok=True)
    if not work.is_relative_to((ROOT/'build/m3').resolve()):raise RuntimeError('VM workspace escaped project')
    vmx=work/'EngineOS-M3-test.vmx';serial=work/'serial.log';image=(ROOT/'dist/EngineOS-M3.iso').resolve()
    evidence=ROOT/'evidence/M3/vmware';evidence.mkdir(parents=True,exist_ok=True);report=evidence/f'{args.cpus}cpu-results.json'
    def run(*a,timeout=35):return subprocess.run([str(vmrun),'-T','ws',*map(str,a)],capture_output=True,text=True,timeout=timeout,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    inventory=run('list')
    if inventory.returncode:raise RuntimeError('VMware inventory failed')
    before={p.strip() for p in inventory.stdout.splitlines()[1:] if p.strip()}
    if str(vmx) in before:raise RuntimeError('Owned test VM already running; leave it untouched')
    if serial.exists():serial.unlink()
    config={'.encoding':'UTF-8','config.version':'8','virtualHW.version':'20','displayName':'Engine OS M3 isolated test',
            'guestOS':'freebsd-64','firmware':'efi','uefi.secureBoot.enabled':'FALSE','memsize':'128','numvcpus':str(args.cpus),
            'cpuid.coresPerSocket':str(args.cpus),'hpet0.present':'TRUE','ethernet0.present':'FALSE','floppy0.present':'FALSE',
            'usb.present':'FALSE','mks.enable3d':'FALSE','ide1:0.present':'TRUE','ide1:0.deviceType':'cdrom-image',
            'ide1:0.fileName':str(image),'ide1:0.startConnected':'TRUE','serial0.present':'TRUE','serial0.fileType':'file',
            'serial0.fileName':str(serial),'serial0.startConnected':'TRUE','serial0.yieldOnMsrRead':'TRUE',
            'uuid.action':'create','msg.autoAnswer':'TRUE','bios.bootOrder':'cdrom'}
    vmx.write_text(''.join(f'{k} = "{v}"\n' for k,v in config.items()).replace('\\n','\n'),encoding='utf-8')
    result={'passed':False,'scope':'one isolated VMware Workstation EFI/FreeBSD-64 profile; no disk, network or guest tools',
            'ram_mib':128,'cpus':args.cpus,'vmrun_sha256':hashlib.sha256(vmrun.read_bytes()).hexdigest(),
            'iso_sha256':hashlib.sha256(image.read_bytes()).hexdigest(),'checks':[]}
    report.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    owned=False
    try:
        start=run('start',vmx,'nogui',timeout=50)
        if start.returncode:raise RuntimeError('VMware start: '+start.stdout+start.stderr)
        owned=True;deadline=time.monotonic()+60
        while time.monotonic()<deadline:
            log=serial.read_text(encoding='utf-8',errors='replace') if serial.exists() else ''
            if 'M2_IDLE_HLT' in log or '_FAIL' in log or 'LOADER_REJECT:' in log or 'KERNEL_PANIC' in log:break
            time.sleep(.2)
        else:raise RuntimeError('VMware boot timed out')
        log=re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]','',log)
        (evidence/f'{args.cpus}cpu-serial.log').write_text(log,encoding='utf-8')
        manifest=json.loads((ROOT/'dist/m3-build-manifest.json').read_text(encoding='utf-8'))
        result['build_id']=manifest['build_id']
        result['checks']=[{'name':'normal M3 runtime reaches complete/idle','passed':all(m in log for m in ('M3_COMPLETE_PASS','M3_RUNTIME_PASS','M3_SMP_ALLOCATOR_TLB_TIMER_PASS','M2_IDLE_HLT'))},
                          {'name':'native execution on every configured CPU','passed':f'M3_USER_CPU_MASK=0x{((1<<args.cpus)-1):016x}' in log},
                          {'name':'tested source identity','passed':'ENGINE_OS_KERNEL_ENTRY build='+manifest['build_id'] in log}]
        def parsed(prefix):
            match=re.search(re.escape(prefix)+r'(.*)',log)
            return {k:int(v,16) for k,v in re.findall(r'(\w+)=(0x[0-9a-f]+)',match.group(1))} if match else {}
        result['roundtrip_tsc']=parsed('M3_ROUNDTRIP_TSC ');result['roundtrip_tsc'].update(parsed('M3_ROUNDTRIP_PERCENTILES '));result['smp']=parsed('M3_SMP_STATS ');result['memory']=parsed('M2_STATS ');result['runtime']=parsed('M3_RUNTIME_STATS ');result['dispatch_tsc']=parsed('M3_DISPATCH_TSC ')
        reservation=re.search(r'M2_BOOT_RESERVED_BYTES=(0x[0-9a-f]+)',log)
        result['boot_reserved_bytes']=int(reservation.group(1),16) if reservation else None
        result['checks'].append({'name':'runtime counters and managed/boot memory budgets','passed':
            all(result['runtime'].get(k)==v for k,v in {'threads':85,'exits':73,'cancellations':10,'isolated_faults':2,'cow_faults':2,'copy_cow':1,'oom_faults':1}.items())
            and result['memory'].get('failures')==0 and result['memory'].get('peak_owned_bytes',2**64)<=512*1024
            and result['memory'].get('retained_owned_bytes')==(15+8*(args.cpus-1))*4096
            and result['boot_reserved_bytes'] is not None and result['boot_reserved_bytes']<=1024*1024})
        vm_log=(work/'vmware.log').read_text(encoding='utf-8',errors='replace') if (work/'vmware.log').exists() else ''
        version=re.search(r'VMware Workstation[^\r\n]*version=([\d.]+) build=(\d+)',vm_log)
        if version:result['vmware_version']=version.group(1);result['vmware_build']=version.group(2)
        capture=run('captureScreen',vmx,evidence/f'{args.cpus}cpu-screen.png')
        result['screenshot_captured']=capture.returncode==0 and (evidence/f'{args.cpus}cpu-screen.png').is_file()
        result['passed']=all(c['passed'] for c in result['checks'])
    except Exception as error:
        result['error']=str(error).replace(str(ROOT),'<project-root>')
    finally:
        if owned:
            stopped=run('stop',vmx,'hard');result['owned_vm_stopped']=stopped.returncode==0
            if stopped.returncode:result['passed']=False
        after=run('list');remaining={p.strip() for p in after.stdout.splitlines()[1:] if p.strip()}
        result['existing_vm_inventory_unchanged']=remaining==before
        if not result['existing_vm_inventory_unchanged']:result['passed']=False
        report.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)
if __name__=='__main__':main()
