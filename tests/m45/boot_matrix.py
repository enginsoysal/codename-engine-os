"""Boot the same hybrid ISO/USB through actual UEFI and BIOS paths; observe real kernel idle."""
from pathlib import Path
import hashlib,json,re,shutil,subprocess,sys,time
sys.path[:0]=[str(Path(__file__).resolve().parents[2]/'tools'),str(Path(__file__).resolve().parents[1]/'m3')]
from vm import ROOT,arguments,qemu_tools
from visual import free_port,connect

def main():
    files=qemu_tools();bios=files['ovmf-code'].parent/'bios-256k.bin';lock=json.loads((ROOT/'tools/security-toolchain.json').read_text(encoding='utf-8'));assert hashlib.sha256(bios.read_bytes()).hexdigest()==lock['files']['seabios']['sha256']
    out=ROOT/'evidence/M4.5/boot';out.mkdir(parents=True,exist_ok=True);manifest=json.loads((ROOT/'dist/m45-build-manifest.json').read_text(encoding='utf-8'));checks=[]
    for name,firmware,media,cpus,ram in [('uefi-iso','uefi','iso',2,128),('bios-iso','bios','iso',2,128),('uefi-usb','uefi','usb',2,128),('bios-usb','bios','usb',2,128),('bios-lowram','bios','iso',1,64),('bios-smp4','bios','iso',4,128)]:
        work=ROOT/'build/m45/boot-tests'/name;image=ROOT/'dist'/('EngineOS-M45.iso' if media=='iso' else 'EngineOS-M45-usb.img');argv,serial=arguments(files,media,image,work,ram=ram);argv[argv.index('-smp')+1]=str(cpus)
        if firmware=='bios':
            for unit in (0,1):
                i=next(i for i,s in enumerate(argv) if s.startswith(f'if=pflash,format=raw,unit={unit},'));del argv[i-1:i+1]
            argv+=['-bios',str(bios)]
        port=free_port();argv+=['-qmp',f'tcp:127.0.0.1:{port},server=on,wait=off'];p=subprocess.Popen(argv,stdout=subprocess.PIPE,stderr=subprocess.PIPE,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));passed=False;error=''
        try:
            with connect(port,p) as sock:
                stream=sock.makefile('rwb');stream.readline()
                def qmp(command,params=None):
                    request={'execute':command}
                    if params is not None:request['arguments']=params
                    stream.write((json.dumps(request)+'\n').encode());stream.flush()
                    while True:
                        r=json.loads(stream.readline())
                        if 'return' in r:return r['return']
                        if 'error' in r:raise RuntimeError(str(r['error']))
                qmp('qmp_capabilities');end=time.monotonic()+60
                while time.monotonic()<end and p.poll() is None:
                    log=serial.read_text(encoding='utf-8',errors='replace') if serial.exists() else ''
                    if 'M4_CONSOLE_READY' in log or 'BIOS_REJECT:' in log or 'KERNEL_REJECT:' in log or 'KERNEL_PANIC' in log:break
                    time.sleep(.1)
                else:raise RuntimeError('Boot did not become ready')
                time.sleep(.2);regs=qmp('human-monitor-command',{'command-line':'info registers -a'});passed=all(s in log for s in ('M4_CONSOLE_READY','M3_COMPLETE_PASS','ENGINE_OS_KERNEL_ENTRY build='+manifest['build_id'])) and regs.count('HLT=1')==cpus
                passed=passed and (firmware=='bios' or 'UEFI_TEMP_KERNEL_BUFFER_RELEASED bytes=' in log)
                passed=passed and ('BIOS_KERNEL_HANDOFF' in log and 'ENGINE_OS_LOADER' not in log if firmware=='bios' else 'EXIT_BOOT_SERVICES_OK' in log and 'BIOS_KERNEL_HANDOFF' not in log)
                (out/(name+'-registers.log')).write_bytes(('\n'.join(s.rstrip() for s in regs.splitlines())+'\n').encode())
                ppm=work/'screen.ppm';qmp('screendump',{'filename':str(ppm)});from PIL import Image
                with Image.open(ppm) as frame:frame.save(out/(name+'.png'));width,height=frame.size
                qmp('quit');p.wait(timeout=5)
        except Exception as exception:error=str(exception).replace(str(ROOT),'<project-root>');width=height=0
        finally:
            if p.poll() is None:p.terminate();p.wait(timeout=5)
        log=serial.read_text(encoding='utf-8',errors='replace') if serial.exists() else '';log=re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]','',log);(out/(name+'.log')).write_bytes(('\n'.join(s.rstrip() for s in log.splitlines())+'\n').encode())
        memory=re.search(r'M4_MEMORY retained_owned_bytes=(0x[0-9a-f]+) peak_owned_bytes=(0x[0-9a-f]+)',log);reserved=re.search(r'M2_BOOT_RESERVED_BYTES=(0x[0-9a-f]+)',log)
        checks.append({'managed_retained_bytes':int(memory.group(1),16) if memory else None,'managed_peak_bytes':int(memory.group(2),16) if memory else None,'boot_reserved_bytes':int(reserved.group(1),16) if reserved else None,'name':name,'passed':passed,'firmware':firmware,'media':media,'cpus':cpus,'ram_mib':ram,'framebuffer_width':width,'framebuffer_height':height,'error':error});print(name,'PASS' if passed else 'FAIL',flush=True)
    (out/'results.json').write_bytes((json.dumps({'build_id':manifest['build_id'],'full_m45_complete':False,'checks':checks},indent=2)+'\n').encode())
    if not all(c['passed'] for c in checks):raise SystemExit(1)
if __name__=='__main__':main()
