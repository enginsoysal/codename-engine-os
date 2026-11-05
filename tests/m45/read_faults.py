"""Real device read EIO injection after discovery; never report an unreadable disk as readable."""
from pathlib import Path
import hashlib,json,struct,subprocess,sys
sys.path[:0]=[str(Path(__file__).resolve().parents[2]/'tools'),str(Path(__file__).resolve().parents[2]/'tools/build')]
from vm import ROOT,arguments,qemu_tools
from media import package

def main():
    work=ROOT/'build/m45/read-faults';work.mkdir(parents=True,exist_ok=True);out=ROOT/'evidence/M4.5/faults';out.mkdir(parents=True,exist_ok=True);build=ROOT/'build/m45';manifest=json.loads((ROOT/'dist/m45-build-manifest.json').read_text(encoding='utf-8'));package(work,(build/'BOOTX64.EFI').read_bytes(),(build/'KERNEL.ELF').read_bytes(),struct.pack('<8sII32s',b'ENGINIT1',48,1,manifest['build_id'].encode()),'M45');checks=[]
    for name,device,label in [('ahci','ide-hd,bus=ide.0','SATA/AHCI'),('nvme','nvme,serial=read-fault','NVMe'),('virtio','virtio-blk-pci,disable-legacy=on','VirtIO')]:
        disk=work/(name+'.img')
        with disk.open('wb') as f:f.truncate(64*1024*1024)
        before=hashlib.sha256(disk.read_bytes()).hexdigest();node={'driver':'raw','node-name':'data','file':{'driver':'blkdebug','image':{'driver':'file','filename':str(disk)},'inject-error':[{'event':'read_aio','iotype':'read','errno':5,'sector':0,'once':False}]}};args,serial=arguments(qemu_tools(),'iso',work/'EngineOS-M45.iso',work/name,ram=128);args+=['-blockdev',json.dumps(node),'-device',device+',drive=data'+('' if name=='nvme' else ',rerror=report,werror=report')];p=subprocess.run(args,capture_output=True,timeout=100,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));log=serial.read_text(encoding='utf-8',errors='replace') if serial.exists() else '';(out/(name+'.log')).write_bytes(('\n'.join(line.rstrip() for line in log.splitlines())+'\n').encode());passed=p.returncode==33 and f'Disk 0 {label} - 64 MiB - 512 bytes/sector - read error 3' in log and 'M45_DISCOVERY_PASS' in log and 'KERNEL_PANIC' not in log and hashlib.sha256(disk.read_bytes()).hexdigest()==before;checks.append({'name':name,'passed':passed,'exit':p.returncode,'backing_unchanged':hashlib.sha256(disk.read_bytes()).hexdigest()==before,'error':p.stderr.decode(errors='replace').replace(str(ROOT),'<project-root>') if p.returncode!=33 else ''});print(name,'PASS' if passed else 'FAIL',flush=True)
    (out/'results.json').write_bytes((json.dumps({'build_id':manifest['build_id'],'full_m45_complete':False,'checks':checks,'scope':'Persistent sector-zero read EIO; firmware can see the same failure. Native first-sector transfer must still fail explicitly.'},indent=2)+'\n').encode())
    if not all(c['passed'] for c in checks):raise SystemExit(1)
if __name__=='__main__':main()
