"""Real ordinary-disk reads and GPT/MBR validation; verify every backing file unchanged."""
from pathlib import Path
import argparse,hashlib,json,re,struct,subprocess,sys,zlib
sys.path[:0]=[str(Path(__file__).resolve().parents[2]/'tools'),str(Path(__file__).resolve().parents[2]/'tools/build')]
from vm import ROOT,arguments,qemu_tools
from media import package
from data_image import create
from ext2_image import check

def image(path,mib,kind='blank',sector=512):
    if path.exists():path.unlink()
    with path.open('xb') as f:
        f.truncate(mib*1024*1024)
        if kind.startswith('mbr'):
            boot=bytearray(sector);boot[510:512]=b'\x55\xaa';struct.pack_into('<B3sB3sII',boot,446,0,b'\0'*3,7,b'\0'*3,2048,1000)
            if kind=='mbr-overlap':struct.pack_into('<B3sB3sII',boot,462,0,b'\0'*3,7,b'\0'*3,2500,1000)
            f.seek(0);f.write(boot);f.seek(2048*sector+3);f.write(b'NTFS    ')
        elif kind.startswith('gpt'):
            count=mib*1024*1024//sector;entries=bytearray(128*128);entries[:16]=bytes(range(1,17));entries[16:32]=bytes(range(17,33));struct.pack_into('<QQ',entries,32,2048,4095)
            table_blocks=(len(entries)+sector-1)//sector;first=2+table_blocks;last=count-2-table_blocks
            def header(lba,other,table):
                h=bytearray(sector);struct.pack_into('<8sIIIIQQQQ16sQIII',h,0,b'EFI PART',0x10000,92,0,0,lba,other,first,last,b'ENGINE-GPT-TEST01',table,128,128,zlib.crc32(entries));struct.pack_into('<I',h,16,zlib.crc32(h[:92]));return h
            boot=bytearray(sector);boot[510:512]=b'\x55\xaa';boot[450]=0xee;struct.pack_into('<II',boot,454,1,count-1);f.seek(0);f.write(boot)
            primary=header(1,count-1,2)
            if kind=='gpt-bad-crc':primary[16]^=1
            backup=header(count-1,1,count-1-table_blocks)
            if kind=='gpt-bad-crc':backup[16]^=1 # Prevent firmware from silently repairing the primary before our kernel runs.
            f.seek(sector);f.write(primary);f.seek(2*sector);f.write(entries);f.seek((count-1-table_blocks)*sector);f.write(entries);f.seek((count-1)*sector);f.write(backup)
            f.seek(2048*sector+3);f.write(b'EXFAT   ')
    return path

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--case',action='append');args=parser.parse_args()
    work=ROOT/'build/m45/storage-tests';work.mkdir(parents=True,exist_ok=True);out=ROOT/'evidence/M4.5';out.mkdir(parents=True,exist_ok=True)
    build=ROOT/'build/m45';manifest=json.loads((ROOT/'dist/m45-build-manifest.json').read_text(encoding='utf-8'));init=struct.pack('<8sII32s',b'ENGINIT1',48,1,manifest['build_id'].encode());package(work,(build/'BOOTX64.EFI').read_bytes(),(build/'KERNEL.ELF').read_bytes(),init,'M45')
    cases=[('uas-ss-512',64,'gpt',512,0),('uas-ss-4k',64,'gpt',4096,0),('uas-hs-512',64,'gpt',512,0),('uas-hs-4k',64,'gpt',4096,0),('usb-512',64,'gpt',512,0),('usb-4k',64,'gpt',4096,0),('ide-pio',64,'mbr',512,0),('virtio-512',64,'gpt',512,0),('virtio-4k',64,'gpt',4096,0),('ordinary-64',64,'blank',512,0),('ordinary-513',513,'blank',512,0),('native-4k',64,'blank',4096,0),('nvme-512',128,'blank',512,0),('mbr-ntfs',64,'mbr',512,0),('mbr-overlap',64,'mbr-overlap',512,2),('gpt-exfat',64,'gpt',512,0),('gpt-4k',64,'gpt',4096,0),('gpt-bad-crc',64,'gpt-bad-crc',512,2)]
    results=[];tools=qemu_tools()
    if args.case and not set(args.case)<={case[0] for case in cases}:raise ValueError('Unknown case')
    for name,mib,kind,sector,status in cases:
        if args.case and name not in args.case:continue
        disk=image(work/(name+'.img'),mib,kind,sector);before=hashlib.sha256(disk.read_bytes()).hexdigest();argv,serial=arguments(tools,'iso',work/'EngineOS-M45.iso',work/name,ram=128)
        nvme=(sector==4096 or name=='nvme-512') and not name.startswith(('virtio','usb','uas'));device=f'nvme,drive=data,serial=engine-test,logical_block_size={sector},physical_block_size={sector}' if nvme else 'ide-hd,bus=ide.0,drive=data'
        if name=='ide-pio':argv+=['-device','piix3-ide,id=legacy'];device='ide-hd,bus=legacy.0,drive=data'
        if name.startswith('virtio'):device=f'virtio-blk-pci,disable-legacy=on,drive=data,logical_block_size={sector},physical_block_size={sector}'
        if name.startswith('usb'):device=f'usb-storage,bus=xhci.0,drive=data,logical_block_size={sector},physical_block_size={sector}'
        if name.startswith('uas'):
            if name.startswith('uas-hs'):argv[argv.index('qemu-xhci,id=xhci')]='qemu-xhci,id=xhci,p3=0'
            argv+=['-device','usb-uas,id=uas,bus=xhci.0'];device=f'scsi-hd,bus=uas.0,drive=data,logical_block_size={sector},physical_block_size={sector}'
        argv+=['-drive',f'if=none,id=data,format=raw,file={disk}','-device',device]
        p=subprocess.run(argv,capture_output=True,timeout=75,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));log=serial.read_text(encoding='utf-8',errors='replace');(out/(name+'.log')).write_bytes(log.encode())
        transport='USB' if name.startswith(('usb','uas')) else 'VirtIO' if name.startswith('virtio') else 'ATA/PIO' if name=='ide-pio' else 'NVMe' if nvme else 'SATA/AHCI'
        passed=p.returncode==33 and 'M45_DISCOVERY_PASS' in log and f'Disk 0 {transport} - {mib} MiB - {sector} bytes/sector - readable' in log and f'Partition status: {status}\n' in log and before==hashlib.sha256(disk.read_bytes()).hexdigest()
        if kind=='mbr':passed=passed and 'NTFS - recognized' in log
        if kind=='gpt':passed=passed and 'exFAT - recognized' in log
        if nvme:
            timing=re.search(r'M45_NVME_STATS .*irqs=(0x[0-9a-f]+) wait_halts=(0x[0-9a-f]+)',log)
            passed=passed and bool(timing) and int(timing.group(1),16)>0 and int(timing.group(2),16)>0
        if name.startswith(('usb','uas')):
            timing=re.search(r'M45_USB_STORAGE_STATS reads=(0x[0-9a-f]+) wait_halts=(0x[0-9a-f]+)',log)
            passed=passed and bool(timing) and int(timing.group(1),16)>=143 and int(timing.group(2),16)>0
        if name.startswith('virtio'):
            timing=re.search(r'M45_VIRTIO_STATS reads=(0x[0-9a-f]+) irqs=(0x[0-9a-f]+) wait_halts=(0x[0-9a-f]+)',log)
            passed=passed and bool(timing) and int(timing.group(1),16)>=67 and int(timing.group(2),16)>0 and int(timing.group(3),16)>0
        if name.startswith('uas'):passed=passed and f'M45_USB_UAS_ONLINE speed=0x{4 if name.startswith("uas-ss") else 3:016x} streams=0x{1 if name.startswith("uas-ss") else 0:016x}' in log
        results.append({'name':name,'transport':transport,'passed':passed,'exit':p.returncode,'size_mib':mib,'logical_sector_bytes':sector,'partition_status':status,'backing_unchanged':before==hashlib.sha256(disk.read_bytes()).hexdigest()});print(name,'PASS' if passed else 'FAIL',flush=True)
    if args.case:
        (out/'selected-storage-results.json').write_bytes((json.dumps({'build_id':manifest['build_id'],'full_m45_complete':False,'checks':results},indent=2)+'\n').encode())
        if not all(r['passed'] for r in results):raise SystemExit(1)
        return
    marked=work/'mixed-marked.img'
    if marked.exists():marked.unlink()
    create(marked);ordinary=image(work/'mixed-ordinary.img',96);gpt_disk=image(work/'mixed-gpt.img',64,'gpt');nvme_disk=image(work/'mixed-nvme.img',128)
    before={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in (ordinary,gpt_disk,nvme_disk)};argv,serial=arguments(tools,'iso',work/'EngineOS-M45.iso',work/'mixed',ram=128)
    for identifier,path,device in [('marked',marked,'ide-hd,bus=ide.0'),('ordinary',ordinary,'ide-hd,bus=ide.1'),('gpt',gpt_disk,'ide-hd,bus=ide.3'),('nvme',nvme_disk,'nvme,serial=engine-mixed')]:argv+=['-drive',f'if=none,id={identifier},format=raw,file={path}','-device',device+',drive='+identifier]
    p=subprocess.run(argv,capture_output=True,timeout=90,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));log=serial.read_text(encoding='utf-8',errors='replace');(out/'mixed.log').write_bytes(log.encode())
    passed=p.returncode==33 and 'M45_MARKED_WRITE_REGRESSION_PASS' in log and 'Disk 3 NVMe' in log and all(hashlib.sha256(p.read_bytes()).hexdigest()==h for p,h in before.items())
    try:check(bytearray(marked.read_bytes()))
    except ValueError:passed=False
    passed=passed and struct.unpack_from('<Q',marked.read_bytes(),520)[0]==1;results.append({'name':'mixed-four-disks','passed':passed,'exit':p.returncode,'ordinary_backing_unchanged':all(hashlib.sha256(p.read_bytes()).hexdigest()==h for p,h in before.items()),'marked_ext2_write_regression':passed});print('mixed-four-disks','PASS' if passed else 'FAIL',flush=True)
    (out/'storage-results.json').write_bytes((json.dumps({'build_id':manifest['build_id'],'full_m45_complete':False,'checks':results},indent=2)+'\n').encode())
    if not all(r['passed'] for r in results):raise SystemExit(1)
if __name__=='__main__':main()
