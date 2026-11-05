"""Two AHCI persistence boots in one owned VMware VM; preserve all existing VMs."""
from pathlib import Path
import hashlib,json,re,struct,subprocess,sys,time,zlib
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/build'))
from m1 import ROOT
from media import package
from data_image import create

def main():
    vmrun=Path('C:/Program Files/VMware/VMware Workstation/vmrun.exe')
    work=ROOT/'build/m4/vmware';work.mkdir(parents=True,exist_ok=True)
    vmx=work/'EngineOS-M4-test.vmx';disk=work/'EngineOS-M4-data-template.img';serial=work/'serial.log'
    evidence=ROOT/'evidence/M4';evidence.mkdir(parents=True,exist_ok=True)
    def run(*a):return subprocess.run([str(vmrun),'-T','ws',*map(str,a)],capture_output=True,text=True,timeout=60,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    def inventory():
        p=run('list')
        if p.returncode:raise RuntimeError('VMware inventory unavailable')
        return {s.strip() for s in p.stdout.splitlines()[1:] if s.strip()}
    before=inventory()
    if str(vmx) in before:raise RuntimeError('Owned test VM already running; leaving it untouched')
    if disk.exists():disk.unlink()
    create(disk)
    descriptor=work/'EngineOS-M4-data-template.vmdk';descriptor.write_bytes((ROOT/'dist/EngineOS-M4-data-template.vmdk').read_bytes())
    build=ROOT/'build/m4';manifest=json.loads((ROOT/'dist/m4-build-manifest.json').read_text(encoding='utf-8'))
    package(work,(build/'BOOTX64.EFI').read_bytes(),(build/'KERNEL.ELF').read_bytes(),struct.pack('<8sII32s',b'ENGINIT1',48,1,manifest['build_id'].encode()),'M4')
    config={'.encoding':'UTF-8','config.version':'8','virtualHW.version':'20','displayName':'Engine OS M4 isolated SATA test','guestOS':'freebsd-64','firmware':'efi','uefi.secureBoot.enabled':'FALSE','memsize':'128','numvcpus':'2','cpuid.coresPerSocket':'2','hpet0.present':'TRUE','ethernet0.present':'FALSE','floppy0.present':'FALSE','usb.present':'FALSE','mks.enable3d':'FALSE','ide1:0.present':'TRUE','ide1:0.deviceType':'cdrom-image','ide1:0.fileName':str(work/'EngineOS-M4.iso'),'ide1:0.startConnected':'TRUE','sata0.present':'TRUE','sata0:0.present':'TRUE','sata0:0.fileName':str(descriptor),'serial0.present':'TRUE','serial0.fileType':'file','serial0.fileName':str(serial),'serial0.startConnected':'TRUE','serial0.yieldOnMsrRead':'TRUE','uuid.action':'create','msg.autoAnswer':'TRUE','bios.bootOrder':'cdrom'}
    vmx.write_bytes(''.join(f'{k} = "{v}"\n' for k,v in config.items()).encode())
    result={'build_id':manifest['build_id'],'passed':False,'scope':'VMware EFI FreeBSD-64, 2 CPUs, 128 MiB, SATA AHCI, two smoke boots; no graphical/input qualification','checks':[]}
    owned=False
    try:
        for generation in (1,2):
            if serial.exists():serial.unlink()
            p=run('start',vmx,'nogui')
            if p.returncode:raise RuntimeError('Owned VMware launch failed')
            owned=True;end=time.monotonic()+60
            while time.monotonic()<end:
                log=serial.read_text(encoding='utf-8',errors='replace') if serial.exists() else ''
                if any(s in log for s in ('M4_STORAGE_PASS','M4_STORAGE_UNAVAILABLE','M4_STORAGE_IO_ERROR','_FAIL','KERNEL_PANIC')):break
                time.sleep(.1)
            else:raise RuntimeError('M4 VMware storage timeout')
            (evidence/f'vmware-boot{generation}.log').write_bytes(re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]','',log).encode())
            p=run('stop',vmx,'hard')
            if p.returncode:raise RuntimeError('Owned VM stop failed')
            owned=False;record=disk.read_bytes()[512:1024]
            passed=f'M4_STORAGE_PASS generation=0x{generation:016x}' in log and 'M3_COMPLETE_PASS' in log and struct.unpack_from('<Q',record,8)[0]==generation and struct.unpack_from('<I',record,508)[0]==zlib.crc32(record[:508])
            result['checks'].append({'name':f'boot {generation} real AHCI and independently checked persisted record','passed':passed})
        vm_log=(work/'vmware.log').read_text(encoding='utf-8',errors='replace')
        version=re.search(r'VMware Workstation[^\r\n]*version=([\d.]+) build=(\d+)',vm_log)
        if version:result['vmware_version']=version.group(1);result['vmware_build']=version.group(2)
        result['passed']=all(c['passed'] for c in result['checks'])
    finally:
        if owned:
            if run('stop',vmx,'hard').returncode:result['passed']=False
        result['existing_vm_inventory_unchanged']=inventory()==before
        result['passed']=result['passed'] and result['existing_vm_inventory_unchanged']
        (evidence/'vmware-report.json').write_bytes((json.dumps(result,indent=2)+'\n').encode())
    print(json.dumps(result,indent=2));raise SystemExit(0 if result['passed'] else 1)
if __name__=='__main__':main()
