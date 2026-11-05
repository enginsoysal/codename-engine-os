"""Independent ext2 inspection, full-volume and interrupted-write recovery tests."""
from pathlib import Path
import json,struct,subprocess,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools'))
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/build'))
from vm import ROOT,arguments,qemu_tools
from data_image import create
from ext2_image import Image,check,put16,BS

def main():
    work=ROOT/'build/m4/filesystem';work.mkdir(parents=True,exist_ok=True);evidence=ROOT/'evidence/M4';results=[]
    def fresh(name):
        p=work/(name+'.img')
        if p.exists():p.unlink()
        create(p);return p
    clean=fresh('clean');data=bytearray(clean.read_bytes());im=Image(data)
    parent=im.mkdir(2,'Full');payload=b'F'*(12*BS);count=0
    while True:
        remaining=sum(not data[2*BS+n//8]&(1<<(n%8)) for n in range(4096))
        if not remaining:break
        im.file(parent,'f'+str(count),payload[:min(12,remaining)*BS]);count+=1
    im.counts();check(data);full=fresh('full');full.write_bytes(data)
    dirty=fresh('dirty');damaged=bytearray(dirty.read_bytes());put16(damaged,1082,2);dirty.write_bytes(damaged)
    repaired=work/'repaired.img'
    if repaired.exists():repaired.unlink()
    subprocess.run([sys.executable,'tools/build/fsck.py',str(dirty),'--repair-copy',str(repaired)],cwd=ROOT,check=True,capture_output=True)
    check(bytearray(repaired.read_bytes()));results.append({'name':'dirty source unchanged; offline repair to an exclusive new copy','passed':dirty.read_bytes()==damaged})
    tools=qemu_tools();iso=ROOT/'build/m4/qualification/EngineOS-M4.iso'
    interrupted=fresh('interrupted');image=interrupted.read_bytes();first_free=next(n for n in range(20,4096) if not image[2*BS+n//8]&(1<<(n%8)))
    argv,serial=arguments(tools,'iso',iso,work/'interrupted',ram=128)
    node={'driver':'raw','node-name':'data','file':{'driver':'blkdebug','image':{'driver':'file','filename':str(interrupted)},'inject-error':[{'event':'write_aio','iotype':'write','errno':5,'sector':first_free*8}]}}
    argv+=['-blockdev',json.dumps(node),'-device','ide-hd,bus=ide.0,drive=data,werror=report']
    p=subprocess.run(argv,capture_output=True,timeout=90,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));log=serial.read_text(encoding='utf-8',errors='replace');(evidence/'interrupted-fs.log').write_bytes(log.encode())
    interrupted_data=bytearray(interrupted.read_bytes());refused=False
    try:check(interrupted_data)
    except ValueError:refused=True
    passed=p.returncode==101 and 'M4_FILESYSTEM_FAIL status=0x0000000000000001' in log and refused and struct.unpack_from('<H',interrupted_data,1082)[0]==2
    results.append({'name':'real kernel I/O failure after durable dirty marker; offline checker refuses clean mount','passed':passed})
    recovered=work/'interrupted-recovered.img'
    if recovered.exists():recovered.unlink()
    subprocess.run([sys.executable,'tools/build/fsck.py',str(interrupted),'--repair-copy',str(recovered)],cwd=ROOT,check=True,capture_output=True)
    check(bytearray(recovered.read_bytes()))
    rejected=[]
    for name in ('unsupported-feature','unsupported-link','duplicate-name'):
        path=fresh(name);bad=bytearray(path.read_bytes());fixture=Image(bad)
        if name=='unsupported-feature':struct.pack_into('<I',bad,1024+92,4)
        elif name=='unsupported-link':
            n=fixture.file(2,'unsupported',b'x');put16(fixture.inode(n),0,0xa1ff);fixture.counts()
        else:
            n=next(n for label,n,typ in fixture.entries(2) if label=='System');fixture.insert(2,'System',n,2)
        path.write_bytes(bad);rejected.append((name,path,101,'M4_FS_MOUNT status=0x0000000000000009' if name=='unsupported-feature' else 'M4_FS_MOUNT status=0x0000000000000002'))
    timeout_disk=fresh('timeout');argv,serial=arguments(tools,'iso',iso,work/'timeout',ram=128)
    delayed={'driver':'raw','node-name':'data','file':{'driver':'blkdebug','image':{'driver':'file','filename':str(timeout_disk)},'inject-error':[{'event':'write_aio','iotype':'write','errno':0,'delay-ns':3000000000}]}}
    argv+=['-blockdev',json.dumps(delayed),'-device','ide-hd,bus=ide.0,drive=data,werror=report']
    p=subprocess.run(argv,capture_output=True,timeout=90,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));log=serial.read_text(encoding='utf-8',errors='replace');(evidence/'timeout-fs.log').write_bytes(log.encode())
    results.append({'name':'delayed write exceeds deadline; controller offlined and DMA pages quarantined; write outcome intentionally unknown','passed':p.returncode==101 and 'M4_AHCI_TIMEOUT_DMA_QUARANTINED' in log and 'M4_FILESYSTEM_FAIL' in log})
    for name,disk,code,marker in [('full',full,101,'M4_FILESYSTEM_FAIL status=0x0000000000000005'),('dirty',dirty,101,'M4_FS_MOUNT status=0x0000000000000007'),('repaired',repaired,33,'M4_EXT2_VFS_PASS'),('interrupted-recovered',recovered,33,'M4_EXT2_VFS_PASS')]+rejected:
        before=disk.read_bytes();argv,serial=arguments(tools,'iso',iso,work/name,ram=128)
        argv+=['-drive',f'if=none,id=data,format=raw,file={disk}','-device','ide-hd,bus=ide.0,drive=data']
        p=subprocess.run(argv,capture_output=True,timeout=90,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));log=serial.read_text(encoding='utf-8',errors='replace');(evidence/(name+'-fs.log')).write_bytes(log.encode())
        passed=p.returncode==code and marker in log
        if code==101:passed=passed and before==disk.read_bytes()
        else:check(bytearray(disk.read_bytes()))
        results.append({'name':name,'passed':passed,'exit':p.returncode});print(name,'PASS' if passed else 'FAIL',flush=True)
    manifest=json.loads((ROOT/'dist/m4-build-manifest.json').read_text(encoding='utf-8'))
    (evidence/'filesystem-report.json').write_bytes((json.dumps({'build_id':manifest['build_id'],'checks':results},indent=2)+'\n').encode())
    if not all(r['passed'] for r in results):raise SystemExit(1)
if __name__=='__main__':main()
