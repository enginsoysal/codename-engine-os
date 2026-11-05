"""Exercise real AHCI persistence and rejection paths on disposable files only."""
from pathlib import Path
import argparse,hashlib,json,re,struct,subprocess,sys,zlib
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools'))
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/build'))
from media import package
from data_image import create
from ext2_image import check
from vm import ROOT,arguments,qemu_tools

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--case');options=parser.parse_args()
    build=ROOT/'build/m4'; evidence=ROOT/'evidence/M4';evidence.mkdir(parents=True,exist_ok=True)
    manifest=json.loads((ROOT/'dist/m4-build-manifest.json').read_text(encoding='utf-8'))
    init=struct.pack('<8sII32s',b'ENGINIT1',48,1,manifest['build_id'].encode())
    work=build/'qualification';package(work,(build/'BOOTX64.EFI').read_bytes(),(build/'KERNEL.ELF').read_bytes(),init,'M4')
    files=qemu_tools();results=[]
    disk=work/'persistent.img'
    if disk.exists():disk.unlink() # Owned generated test fixture; never a supplied path.
    create(disk);original=disk.read_bytes()
    cases=[('persist-first',disk,False,33,1),('persist-reboot',disk,False,33,2),('write-error',disk,True,101,2),('absent',None,False,101,None)]
    for kind in ('bad-header','bad-record','wrong-capacity'):
        path=work/(kind+'.img')
        if path.exists():path.unlink()
        create(path)
        with path.open('r+b') as f:
            if kind=='wrong-capacity':f.truncate(8*1024*1024)
            else:f.seek(0 if kind=='bad-header' else 512);f.write(b'!')
        cases.append((kind,path,False,101,None))
    for name in ('lowram','usb-storage'):
        p=work/(name+'.img')
        if p.exists():p.unlink()
        create(p);cases.append((name,p,False,33,1))
    for name,path,readonly,expected,generation in cases:
        if options.case and name!=options.case:continue
        before=path.read_bytes() if path else None
        media='usb' if name=='usb-storage' else 'iso';image=work/('EngineOS-M4-usb.img' if media=='usb' else 'EngineOS-M4.iso')
        argv,serial=arguments(files,media,image,work/name,ram=64 if name=='lowram' else 128)
        if name=='lowram':argv[argv.index('-smp')+1]='1'
        if path:
            backend={'driver':'file','filename':str(path)}
            if readonly:backend={'driver':'blkdebug','image':backend,'inject-error':[{'event':'write_aio','iotype':'write','errno':5}]}
            node={'driver':'raw','file':backend}
            node['node-name']='data';argv+=['-blockdev',json.dumps(node),'-device','ide-hd,bus=ide.0,drive=data,werror=report,rerror=report']
        proc=subprocess.run(argv,capture_output=True,timeout=60,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
        log=serial.read_text(encoding='utf-8',errors='replace');(evidence/(name+'.log')).write_bytes(log.encode())
        passed=proc.returncode==expected and 'M3_COMPLETE_PASS' in log
        if expected==33:
            data=path.read_bytes();record=data[512:1024]
            passed=passed and 'M4_STORAGE_PASS' in log and 'dma_bytes=0x0000000000004000' in log
            passed=passed and struct.unpack_from('<Q',record,8)[0]==generation and struct.unpack_from('<I',record,508)[0]==zlib.crc32(record[:508])
            payload=b'ENGINE OS M4 DURABLE RECORD'
            passed=passed and record[24:24+len(payload)]==payload and data[:512]==original[:512] and 'M4_EXT2_VFS_PASS' in log
            passed=passed and 'M4_FS_TEST_BUFFERS_RECLAIMED' in log
            irq=re.search(r'irqs=(0x[0-9a-f]+) wait_halts=(0x[0-9a-f]+)',log)
            passed=passed and bool(irq) and int(irq.group(1),16)>0 and int(irq.group(2),16)>0
            try:check(bytearray(data))
            except ValueError:passed=False
        else:passed=passed and 'M4_STORAGE_' in log and (path is None or path.read_bytes()==before)
        results.append({'case':name,'passed':passed,'exit':proc.returncode,'generation':generation,'unchanged_on_rejection':expected!=33 and (path is None or path.read_bytes()==before)})
        print(name,'PASS' if passed else 'FAIL',proc.returncode,flush=True)
    report={'build_id':manifest['build_id'],'scope':'AHCI/ext2 reboot, rejection, IRQ wait, low-RAM and ISO/USB qualification','checks':results}
    (evidence/('selected-storage-report.json' if options.case else 'storage-report.json')).write_bytes((json.dumps(report,indent=2)+'\n').encode())
    if not results or not all(r['passed'] for r in results):raise SystemExit(1)
if __name__=='__main__':main()
