"""Native installation/installed boot via COM1 in owned headless VMware guests only."""
from pathlib import Path
import ctypes,json,os,re,subprocess,time,uuid,msvcrt
from ctypes import wintypes as w
ROOT=Path(__file__).resolve().parents[2]
RUN=Path('C:/Program Files/VMware/VMware Workstation/vmrun.exe')
api=ctypes.WinDLL('kernel32',use_last_error=True)
api.PeekNamedPipe.argtypes=[w.HANDLE,ctypes.c_void_p,w.DWORD,ctypes.c_void_p,ctypes.POINTER(w.DWORD),ctypes.c_void_p];api.PeekNamedPipe.restype=w.BOOL

def run(*args):return subprocess.run([str(RUN),'-T','ws',*map(str,args)],capture_output=True,text=True,timeout=60,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
def inventory():
    r=run('list');assert not r.returncode,'Cannot read VMware inventory';return set(r.stdout.splitlines()[1:])

class Guest:
    def __init__(self,folder,disk,firmware,installed=False):
        self.folder=folder;folder.mkdir(parents=True);self.vmx=folder/'EngineOS-M5-owned.vmx';self.name='EngineOS-M5-'+uuid.uuid4().hex;self.path='\\\\.\\pipe\\'+self.name;self.output='';self.pipe=None;self.owned=False
        cfg={'.encoding':'UTF-8','config.version':'8','virtualHW.version':'20','displayName':'Engine OS M5 isolated qualification','guestOS':'freebsd-64','firmware':firmware,'uefi.secureBoot.enabled':'FALSE','memsize':'128','numvcpus':'2','cpuid.coresPerSocket':'2','hpet0.present':'TRUE','ethernet0.present':'FALSE','floppy0.present':'FALSE','usb.present':'FALSE','mks.enable3d':'FALSE','ide1:0.present':'FALSE' if installed else 'TRUE','ide1:0.deviceType':'cdrom-image','ide1:0.fileName':str(ROOT/'dist/EngineOS-M5.iso'),'ide1:0.startConnected':'FALSE' if installed else 'TRUE','sata0.present':'TRUE','sata0:0.present':'TRUE','sata0:0.fileName':str(disk),'serial0.present':'TRUE','serial0.fileType':'pipe','serial0.fileName':self.path,'serial0.pipe.endPoint':'server','serial0.tryNoRxLoss':'FALSE','serial0.startConnected':'TRUE','serial0.yieldOnMsrRead':'TRUE','uuid.action':'create','msg.autoAnswer':'TRUE','bios.bootOrder':'hdd' if installed else 'cdrom,hdd'}
        self.vmx.write_text(''.join(f'{k} = "{v}"\n' for k,v in cfg.items()),encoding='utf-8')
    def __enter__(self):
        assert str(self.vmx) not in inventory();r=run('start',self.vmx,'nogui')
        if r.returncode:raise RuntimeError('Owned VMware start failed: '+(r.stdout+r.stderr).replace(str(ROOT),'<project-root>'))
        self.owned=True
        try:
            deadline=time.monotonic()+30
            while time.monotonic()<deadline:
                try:self.pipe=open(self.path,'r+b',buffering=0);break
                except OSError:time.sleep(.1)
            if not self.pipe:raise RuntimeError('Owned COM1 pipe unavailable')
            self.wait('M4_CONSOLE_READY');return self
        except Exception:self.__exit__();raise
    def read(self):
        count=w.DWORD();handle=msvcrt.get_osfhandle(self.pipe.fileno())
        if not api.PeekNamedPipe(handle,None,0,None,ctypes.byref(count),None):raise ctypes.WinError(ctypes.get_last_error())
        if count.value:self.output+=self.pipe.read(min(count.value,65536)).decode('utf-8',errors='replace')
        return self.output
    def wait(self,text,offset=0,timeout=90):
        deadline=time.monotonic()+timeout
        while time.monotonic()<deadline:
            log=self.read()
            if text in log[offset:]:return log[offset:]
            if any(s in log for s in ['KERNEL_PANIC','BIOS_REJECT:','LOADER_REJECT:','KERNEL_REJECT:']):break
            time.sleep(.03)
        raise RuntimeError('VMware missing '+text+': '+self.output[-1400:])
    def command(self,text,expected='engine> '):
        self.read();start=len(self.output)
        for c in (text+'\r').encode():self.pipe.write(bytes([c]));time.sleep(.015)
        return self.wait(expected,start,180)
    def __exit__(self,*args):
        if self.owned:
            r=run('stop',self.vmx,'hard');self.owned=False
            if r.returncode:raise RuntimeError('Owned VMware stop failed')
        if self.pipe:self.pipe.close()

def main():
    import argparse,hashlib,traceback
    p=argparse.ArgumentParser();p.add_argument('--firmware',choices=['efi','bios'],default='efi');opt=p.parse_args()
    work=ROOT/'build/m5/vmware'/opt.firmware/str(time.time_ns());work.mkdir(parents=True);out=ROOT/'evidence/M5/vmware'/opt.firmware;out.mkdir(parents=True,exist_ok=True)
    disk=work/'target-flat.img'
    with disk.open('xb') as f:f.truncate(256*1024*1024)
    descriptor=work/'target.vmdk';descriptor.write_text('version=1\nencoding="UTF-8"\nCID=fffffffe\nparentCID=ffffffff\ncreateType="monolithicFlat"\nRW 524288 FLAT "target-flat.img" 0\nddb.adapterType = "ide"\nddb.geometry.cylinders = "520"\nddb.geometry.heads = "16"\nddb.geometry.sectors = "63"\n',encoding='utf-8')
    before=inventory();checks=[];errors=[];version=None
    try:
        with Guest(work/'live',descriptor,opt.firmware) as g:
            assert 'M5_LIVE_VOLATILE_VOLUME' in g.output;assert 'No changes yet' in g.command('plan 0');g.command('install 0 ERASE-524288','M5_INSTALL_PASS');(out/'install.log').write_text(g.output,encoding='utf-8')
        checks.append({'name':'final public ISO native COM1 plan and SATA install','passed':True})
        for i in range(2):
            with Guest(work/f'installed-{i}',descriptor,opt.firmware,True) as g:
                assert 'M5_INSTALLED_VOLUME' in g.output and 'M3_COMPLETE_PASS' in g.output
                if i==0:assert 'Write status: 0x0000000000000000' in g.command('write /Users/test/Documents/vmware.txt durable')
                else:assert '\ndurable' in g.command('cat /Users/test/Documents/vmware.txt')
                (out/f'installed-{i}.log').write_text(g.output,encoding='utf-8')
        checks.append({'name':'fresh VMware firmware with ISO absent, two installed boots and durable file','passed':True})
        def data_hash():
            with disk.open('rb') as f:f.seek(139264*512);return hashlib.sha256(f.read(16*1024*1024)).hexdigest()
        retained=data_hash()
        with disk.open('r+b') as f:f.seek(64*512);f.write(b'BROKEN-BOOT'+bytes(502))
        with Guest(work/'repair',descriptor,opt.firmware) as g:
            g.command('repairplan 0');g.command('repair 0 REPAIR-524288','M5_REPAIR_PASS');(out/'repair.log').write_text(g.output,encoding='utf-8')
        assert retained==data_hash()
        with Guest(work/'recovered',descriptor,opt.firmware,True) as g:
            assert '\ndurable' in g.command('cat /Users/test/Documents/vmware.txt');(out/'recovered.log').write_text(g.output,encoding='utf-8')
        checks.append({'name':'same-build repair retains entire data partition; media-detached recovered boot','passed':True})
        log=(work/'live/vmware.log').read_text(encoding='utf-8',errors='replace');v=re.search(r'VMware Workstation[^\r\n]*version=([\d.]+) build=(\d+)',log);version={'version':v.group(1),'build':v.group(2)} if v else None
    except Exception:errors.append(traceback.format_exc().replace(str(ROOT),'<project-root>'));print(errors[-1],flush=True)
    unchanged=inventory()==before;manifest=json.loads((ROOT/'dist/m5-build-manifest.json').read_text(encoding='utf-8'));result={'passed':not errors and unchanged,'firmware':opt.firmware,'build_id':manifest['build_id'],'iso_sha256':manifest['artifacts']['EngineOS-M5.iso']['sha256'],'vmware':version,'existing_vm_inventory_unchanged':unchanged,'checks':checks,'errors':errors};(out/'results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');print('VMware '+opt.firmware,'PASS' if result['passed'] else 'FAIL',flush=True)
    if not result['passed']:raise SystemExit(1)
if __name__=='__main__':main()
