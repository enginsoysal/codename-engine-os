"""Dual-firmware VMware boots and ordinary first-sector reads in owned VMs only."""
from pathlib import Path
import hashlib,json,re,subprocess,time
ROOT=Path(__file__).resolve().parents[2]

def main():
    vmrun=Path('C:/Program Files/VMware/VMware Workstation/vmrun.exe');work=ROOT/'build/m45/vmware';work.mkdir(parents=True,exist_ok=True);out=ROOT/'evidence/M4.5/vmware';out.mkdir(parents=True,exist_ok=True)
    manifest=json.loads((ROOT/'dist/m45-build-manifest.json').read_text(encoding='utf-8'));iso=ROOT/'dist/EngineOS-M45.iso';iso_sha256=hashlib.sha256(iso.read_bytes()).hexdigest();assert iso_sha256==manifest['artifacts'][iso.name]['sha256'];checks=[];owned=None
    def run(*args):return subprocess.run([str(vmrun),'-T','ws',*map(str,args)],capture_output=True,text=True,timeout=60,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    def inventory():
        p=run('list')
        if p.returncode:raise RuntimeError('VMware inventory unavailable')
        return {line.strip() for line in p.stdout.splitlines()[1:] if line.strip()}
    before=inventory()
    try:
        for firmware,transport in [('efi','sata'),('bios','sata'),('efi','ide'),('bios','ide'),('efi','nvme')]:
            name=firmware+'-'+transport;folder=work/name;folder.mkdir(parents=True,exist_ok=True);vmx=folder/'EngineOS-M45-test.vmx';serial=folder/'serial.log';disk=folder/'ordinary-flat.img';descriptor=folder/'ordinary.vmdk'
            if str(vmx) in inventory():raise RuntimeError('Owned test VM already running; leaving untouched')
            with disk.open('wb') as f:f.truncate(64*1024*1024);f.write(b'ENGINE-ORDINARY-READ-ONLY')
            before_hash=hashlib.sha256(disk.read_bytes()).hexdigest()
            descriptor.write_bytes(b'version=1\nencoding="UTF-8"\nCID=fffffffe\nparentCID=ffffffff\ncreateType="monolithicFlat"\nRW 131072 FLAT "ordinary-flat.img" 0\nddb.adapterType = "ide"\nddb.geometry.cylinders = "130"\nddb.geometry.heads = "16"\nddb.geometry.sectors = "63"\n')
            if serial.exists():serial.unlink()
            controller='ide0' if transport=='ide' else transport+'0';prefix=controller+':0'
            config={'.encoding':'UTF-8','config.version':'8','virtualHW.version':'20','displayName':'Engine OS M45 isolated '+name,'guestOS':'freebsd-64','firmware':firmware,'uefi.secureBoot.enabled':'FALSE','memsize':'128','numvcpus':'2','cpuid.coresPerSocket':'2','hpet0.present':'TRUE','ethernet0.present':'FALSE','floppy0.present':'FALSE','usb.present':'FALSE','mks.enable3d':'FALSE','ide1:0.present':'TRUE','ide1:0.deviceType':'cdrom-image','ide1:0.fileName':str(ROOT/'dist/EngineOS-M45.iso'),'ide1:0.startConnected':'TRUE',prefix+'.present':'TRUE',prefix+'.fileName':str(descriptor),'serial0.present':'TRUE','serial0.fileType':'file','serial0.fileName':str(serial),'serial0.startConnected':'TRUE','serial0.yieldOnMsrRead':'TRUE','uuid.action':'create','msg.autoAnswer':'TRUE','bios.bootOrder':'cdrom'}
            if transport!='ide':config[controller+'.present']='TRUE'
            if transport=='nvme':config.update({'pciBridge0.present':'TRUE','pciBridge4.present':'TRUE','pciBridge4.virtualDev':'pcieRootPort','pciBridge4.functions':'8'})
            vmx.write_bytes(''.join(f'{key} = "{value}"\n' for key,value in config.items()).encode());passed=False;error=''
            try:
                p=run('start',vmx,'nogui')
                if p.returncode:raise RuntimeError('Owned VMware launch failed: '+(p.stdout+p.stderr).replace(str(ROOT),'<project-root>'))
                owned=vmx;deadline=time.monotonic()+75
                while time.monotonic()<deadline:
                    log=serial.read_text(encoding='utf-8',errors='replace') if serial.exists() else ''
                    if any(s in log for s in ('M4_CONSOLE_READY','_FAIL','KERNEL_PANIC','BIOS_REJECT:','KERNEL_REJECT:')):break
                    time.sleep(.1)
                else:raise RuntimeError('VMware boot deadline')
                expected={'sata':'SATA/AHCI','ide':'ATA/PIO','nvme':'NVMe'}[transport];passed=all(s in log for s in ('M3_COMPLETE_PASS','M4_CONSOLE_READY','ENGINE_OS_KERNEL_ENTRY build='+manifest['build_id'],f'Disk 0 {expected} - 64 MiB - 512 bytes/sector - readable'))
                passed=passed and ('BIOS_KERNEL_HANDOFF' in log if firmware=='bios' else 'EXIT_BOOT_SERVICES_OK' in log)
            except Exception as exception:error=str(exception).replace(str(ROOT),'<project-root>')
            finally:
                if owned:
                    if run('stop',owned,'hard').returncode:raise RuntimeError('Owned VM stop failed')
                    owned=None
            unchanged=hashlib.sha256(disk.read_bytes()).hexdigest()==before_hash;passed=passed and unchanged;log=serial.read_text(encoding='utf-8',errors='replace') if serial.exists() else '';log=re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]','',log);(out/(name+'.log')).write_bytes(('\n'.join(line.rstrip() for line in log.splitlines())+'\n').encode())
            version=re.search(r'VMware Workstation[^\r\n]*version=([\d.]+) build=(\d+)',(folder/'vmware.log').read_text(encoding='utf-8',errors='replace')) if (folder/'vmware.log').exists() else None
            checks.append({'name':name,'passed':passed,'backing_unchanged':unchanged,'scope':'normal dual-firmware ISO; native completion and ordinary first-sector transfer only','error':error,'vmware_version':version.group(1) if version else None,'vmware_build':version.group(2) if version else None});print(name,'PASS' if passed else 'FAIL',flush=True)
    finally:
        if owned:run('stop',owned,'hard')
        unchanged_inventory=inventory()==before;assert hashlib.sha256(iso.read_bytes()).hexdigest()==iso_sha256;(out/'results.json').write_bytes((json.dumps({'build_id':manifest['build_id'],'full_m45_complete':False,'iso_sha256':iso_sha256,'existing_vm_inventory_unchanged':unchanged_inventory,'checks':checks},indent=2)+'\n').encode())
    if not unchanged_inventory or len(checks)!=5 or not all(c['passed'] for c in checks):raise SystemExit(1)
if __name__=='__main__':main()
