"""Owned disposable QEMU guests; native keyboard commands and independent disk checks."""
from pathlib import Path
import hashlib,json,re,subprocess,sys,time
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(Path(__file__).resolve().parent))
from workspace import BASE,new_work,sparse_disk,redact
sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/build'),str(ROOT/'tests/m3')]
from vm.config import arguments,qemu_tools
from visual import free_port,connect
from ext2_image import check,Image,U32

def clean(log):return redact(re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]','',log))
def digest(path,start=0,length=None):
    h=hashlib.sha256()
    with path.open('rb') as f:
        f.seek(start);remaining=path.stat().st_size-start if length is None else length
        while remaining:
            data=f.read(min(1024*1024,remaining))
            if not data:raise RuntimeError('Short owned image read')
            h.update(data);remaining-=len(data)
    return h.hexdigest()

class Guest:
    def __init__(self,folder,disk,firmware='uefi',media='iso',ram=128,cpus=2,extra=None,fault=None,image_name="EngineOS-M6",ps2_only=False,ready="M6_WIZARD_READY"):
        self.ready=ready
        self.image_name=image_name
        self.expected_build_id=None
        if media!='installed':
            meta=json.loads((ROOT/'dist'/(image_name.removeprefix('EngineOS-').removeprefix('EngacerOS-').lower()+'-build-manifest.json')).read_text(encoding='utf-8'))
            filename=image_name+('.iso' if media!='usb' else '-usb.img')
            assert digest(ROOT/'dist'/filename)==meta['artifacts'][filename]['sha256'],'Media bytes differ from the declared build'
            self.expected_build_id=meta['build_id']
        self.folder=folder;self.firmware=firmware;files=qemu_tools();self.args,self.serial=arguments(files,media if media!='installed' else 'iso',ROOT/'dist'/(image_name+'.iso') if media!='usb' else ROOT/'dist'/(image_name+'-usb.img'),folder,ram=ram,milestone='M5')
        self.args[self.args.index('-smp')+1]=str(cpus)
        if ps2_only:
            for value in ['usb-kbd,bus=xhci.0','usb-mouse,bus=xhci.0']:
                i=self.args.index(value);assert self.args[i-1]=='-device';del self.args[i-1:i+1]
        if media=='installed':
            i=self.args.index('-cdrom');del self.args[i:i+2];self.args[self.args.index('-boot')+1]='order=c'
        if firmware=='bios':
            for unit in (0,1):
                i=next(i for i,s in enumerate(self.args) if s.startswith(f'if=pflash,format=raw,unit={unit},'));del self.args[i-1:i+1]
            self.args+=['-bios',str(files['ovmf-code'].parent/'bios-256k.bin')]
        if fault:
            node={'driver':'raw','node-name':'target','file':{'driver':'blkdebug','image':{'driver':'file','filename':str(disk)},'inject-error':[fault]}}
            self.args+=['-blockdev',json.dumps(node),'-device','ide-hd,bus=ide.0,drive=target,werror=report']
        else:
            self.args+=['-drive',f'if=none,id=target,format=raw,file={disk}','-device','ide-hd,bus=ide.0,drive=target'+(',bootindex=1' if media=='installed' else '')]
        if extra:self.args+=extra
        self.port=free_port();self.args+=['-qmp',f'tcp:127.0.0.1:{self.port},server=on,wait=off']
    def __enter__(self):
        self.process=subprocess.Popen(self.args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
        self.sock=connect(self.port,self.process);self.stream=self.sock.makefile('rwb');self.stream.readline();self.qmp('qmp_capabilities')
        try:
            self.wait(self.ready)
            if self.expected_build_id:assert 'ENGINE_OS_KERNEL_ENTRY build='+self.expected_build_id in self.log(),'Unexpected actual kernel build'
            return self
        except Exception:
            self.__exit__();raise
    def qmp(self,name,args=None):
        r={'execute':name}
        if args is not None:r['arguments']=args
        self.stream.write((json.dumps(r)+'\n').encode());self.stream.flush()
        while True:
            v=json.loads(self.stream.readline())
            if 'return' in v:return v['return']
            if 'error' in v:raise RuntimeError(str(v['error']))
    def log(self):return clean(self.serial.read_text(encoding='utf-8',errors='replace')) if self.serial.exists() else ''
    def wait(self,text,after=0,timeout=90):
        end=time.monotonic()+timeout
        while time.monotonic()<end and self.process.poll() is None:
            log=self.log()
            if text in log[after:]:return log
            if any(p in log for p in ['KERNEL_PANIC','KERNEL_REJECT:','BIOS_REJECT:']):raise RuntimeError(log[-1500:])
            time.sleep(.05)
        raise RuntimeError('Missing '+text+'; tail='+self.log()[-1500:])
    def command(self,text,expected='engine> ',timeout=90):
        start=len(self.log())
        for c in text+'\n':
            code={' ':'spc','\n':'ret','/':'slash','.':'dot','-':'minus',':':'semicolon'}.get(c,c.lower())
            keys=([{'type':'qcode','data':'shift'}] if c.isupper() else [])+[{'type':'qcode','data':code}]
            self.qmp('send-key',{'keys':keys,'hold-time':10});time.sleep(.025)
        return self.wait(expected,start,timeout)[start:]
    def __exit__(self,*exc):
        if self.process.poll() is None:
            try:self.qmp('quit');self.process.wait(timeout=5)
            except Exception:self.process.terminate();self.process.wait(timeout=5)
        self.stream.close();self.sock.close()
