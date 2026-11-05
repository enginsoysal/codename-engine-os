"""Read real GPT/FAT32 short and UTF-16 long-name files through the native kernel."""
from pathlib import Path
import hashlib,json,struct,subprocess,sys
sys.path[:0]=[str(Path(__file__).resolve().parents[2]/'tools'),str(Path(__file__).resolve().parents[2]/'tools/build')]
from vm import ROOT,arguments,qemu_tools
from media import package,fat32,gpt
def main():
    work=ROOT/'build/m45/fat-tests';work.mkdir(parents=True,exist_ok=True);out=ROOT/'evidence/M4.5';out.mkdir(parents=True,exist_ok=True);build=ROOT/'build/m45';manifest=json.loads((ROOT/'dist/m45-build-manifest.json').read_text(encoding='utf-8'))
    package(work,(build/'BOOTX64.EFI').read_bytes(),(build/'KERNEL.ELF').read_bytes(),struct.pack('<8sII32s',b'ENGINIT1',48,5,manifest['build_id'].encode()),'M45')
    volume=fat32([(2,b'HELLO   TXT',b'Engine OS FAT32 read test\n'),(2,b'RESUME~1TXT',b'Unicode name\n')]);offset=(32+2048)*512
    short=bytes(volume[offset+64:offset+96]);lfn=bytearray(32);lfn[0]=0x41;lfn[11]=15;checksum=0
    for b in short[:11]:checksum=((((checksum&1)<<7)|(checksum>>1))+b)&255
    lfn[13]=checksum;units='R\u00e9sum\u00e9.txt'.encode('utf-16le')+b'\0\0';units+=b'\xff'*(26-len(units))
    for i,p in enumerate((1,3,5,7,9,14,16,18,20,22,24,28,30)):lfn[p:p+2]=units[i*2:i*2+2]
    volume[offset+64:offset+96]=lfn;volume[offset+96:offset+128]=short;image=gpt(volume,'FAT32 native read test');results=[]
    for kind in ('ahci','nvme','bad-lfn'):
        disk=work/(kind+'.img');data=bytearray(image)
        if kind=='bad-lfn':data[2048*512+offset+64+13]^=1
        disk.write_bytes(data);before=hashlib.sha256(data).hexdigest();a,serial=arguments(qemu_tools(),'iso',work/'EngineOS-M45.iso',work/kind,ram=128)
        a+=['-drive',f'if=none,id=data,format=raw,file={disk}','-device','nvme,drive=data,serial=engine-fat' if kind=='nvme' else 'ide-hd,bus=ide.0,drive=data']
        p=subprocess.run(a,capture_output=True,timeout=90,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));log=serial.read_text(encoding='utf-8',errors='replace');(out/(kind+'-fat32.log')).write_bytes(log.encode());expected=101 if kind=='bad-lfn' else 33
        passed=p.returncode==expected and ('M45_FAT32_PAYLOAD_FAIL' if kind=='bad-lfn' else 'M45_FAT32_PAYLOAD_PASS') in log and before==hashlib.sha256(disk.read_bytes()).hexdigest();results.append({'name':kind,'passed':passed,'exit':p.returncode,'backing_unchanged':before==hashlib.sha256(disk.read_bytes()).hexdigest()});print(kind,'PASS' if passed else 'FAIL',flush=True)
    (out/'fat32-results.json').write_bytes((json.dumps({'build_id':manifest['build_id'],'full_m45_complete':False,'checks':results},indent=2)+'\n').encode())
    if not all(r['passed'] for r in results):raise SystemExit(1)
if __name__=='__main__':main()
