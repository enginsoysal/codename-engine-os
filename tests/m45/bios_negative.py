"""Exercise original BIOS loader CRC, memory and medium checks before/at kernel handoff."""
from pathlib import Path
import hashlib,json,struct,subprocess,sys
sys.path[:0]=[str(Path(__file__).resolve().parents[2]/'tools'),str(Path(__file__).resolve().parents[2]/'tools/build')]
from vm import ROOT,qemu_tools
from bios import stages
def main():
    files=qemu_tools();firmware=files['ovmf-code'].parent/'bios-256k.bin';lock=json.loads((ROOT/'tools/security-toolchain.json').read_text(encoding='utf-8'));assert hashlib.sha256(firmware.read_bytes()).hexdigest()==lock['files']['seabios']['sha256']
    work=ROOT/'build/m45/bios-negative';work.mkdir(parents=True,exist_ok=True);out=ROOT/'evidence/M4.5/boot';out.mkdir(parents=True,exist_ok=True);manifest=json.loads((ROOT/'dist/m45-build-manifest.json').read_text(encoding='utf-8'));kernel=(ROOT/'build/m45/KERNEL.ELF').read_bytes();init=struct.pack('<8sII32s',b'ENGINIT1',48,1,manifest['build_id'].encode());checks=[]
    for name,ram,video,bad_header,sector,expected,marker in [('raw-800x600',128,0x143,False,512,33,'M45_DISCOVERY_PASS'),('raw-640x480',128,0x142,False,512,33,'M45_DISCOVERY_PASS'),('serial-fallback',128,0x1ff,False,512,33,'BIOS_FRAMEBUFFER width=0x0000000000000000'),('kernel-crc',128,0x144,False,512,71,'BIOS_REJECT: kernel raw CRC'),('handoff-crc',128,0x144,True,512,71,'KERNEL_REJECT: boot header'),('kernel-region',2,0x144,False,512,71,'BIOS_REJECT: kernel region unavailable before copy'),('sector-mismatch',128,0x144,False,2048,71,'BE')]:
        mbr,stage,raw,config=stages(work/name,kernel,init,sector,64,128,video_mode=video,bad_boot=bad_header)
        if name=='kernel-crc':raw=bytes([raw[0]^1])+raw[1:]
        image=work/(name+'.img')
        with image.open('wb') as f:f.truncate(64*1024*1024);f.write(mbr);f.seek(64*512);f.write(stage);f.seek(128*512);f.write(raw)
        serial=work/(name+'.log');args=[str(files['qemu-system-x86_64']),'-machine','pc-q35-11.1','-accel','tcg','-m',str(ram),'-smp','2','-bios',str(firmware),'-display','none','-net','none','-no-reboot','-serial','file:'+str(serial),'-device','isa-debug-exit,iobase=0xf4,iosize=0x04','-drive','if=none,id=boot,format=raw,snapshot=on,file='+str(image),'-device','ide-hd,bus=ide.0,drive=boot','-boot','order=c']
        try:p=subprocess.run(args,capture_output=True,timeout=45,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));code=p.returncode;error=p.stderr.decode(errors='replace').replace(str(ROOT),'<project-root>')
        except subprocess.TimeoutExpired:code=None;error='45-second timeout'
        log=serial.read_text(encoding='utf-8',errors='replace') if serial.exists() else '';(out/(name+'.log')).write_bytes(log.encode());passed=code==expected and marker in log
        if name=='raw-800x600':passed=passed and 'BIOS_FRAMEBUFFER width=0x0000000000000320 height=0x0000000000000258' in log
        if name=='raw-640x480':passed=passed and 'BIOS_FRAMEBUFFER width=0x0000000000000280 height=0x00000000000001e0' in log
        if name in ('kernel-crc','kernel-region','sector-mismatch'):passed=passed and 'ENGINE_OS_KERNEL_ENTRY' not in log
        checks.append({'name':name,'passed':passed,'exit':code,'expected_exit':expected,'error':error});print(name,'PASS' if passed else 'FAIL',flush=True)
    (out/'negative-results.json').write_bytes((json.dumps({'build_id':manifest['build_id'],'full_m45_complete':False,'checks':checks},indent=2)+'\n').encode())
    if not all(c['passed'] for c in checks):raise SystemExit(1)
if __name__=='__main__':main()
