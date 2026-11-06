"""Native installation/installed boot via COM1 in owned headless VMware guests only."""
from pathlib import Path
import ctypes,json,os,re,subprocess,time,uuid,msvcrt,sys
from ctypes import wintypes as w
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(Path(__file__).resolve().parent))
from workspace import BASE,new_work,sparse_disk,redact
RUN=Path('C:/Program Files/VMware/VMware Workstation/vmrun.exe')
api=ctypes.WinDLL('kernel32',use_last_error=True)
api.PeekNamedPipe.argtypes=[w.HANDLE,ctypes.c_void_p,w.DWORD,ctypes.c_void_p,ctypes.POINTER(w.DWORD),ctypes.c_void_p];api.PeekNamedPipe.restype=w.BOOL

def run(*args):return subprocess.run([str(RUN),'-T','ws',*map(str,args)],capture_output=True,text=True,timeout=60,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
def inventory():
    r=run('list');assert not r.returncode,'Cannot read VMware inventory';return set(r.stdout.splitlines()[1:])

class Guest:
    def __init__(self,folder,disk,firmware,installed=False):
        self.folder=folder;folder.mkdir(parents=True);self.vmx=folder/'EngineOS-M6-owned.vmx';self.name='EngineOS-M6-'+uuid.uuid4().hex;self.path='\\\\.\\pipe\\'+self.name;self.output='';self.pipe=None;self.owned=False
        cfg={'.encoding':'UTF-8','config.version':'8','virtualHW.version':'20','displayName':'Engine OS M6 isolated qualification','guestOS':'freebsd-64','firmware':firmware,'uefi.secureBoot.enabled':'FALSE','memsize':'128','numvcpus':'2','cpuid.coresPerSocket':'2','hpet0.present':'TRUE','ethernet0.present':'FALSE','floppy0.present':'FALSE','usb.present':'FALSE','mks.enable3d':'FALSE','ide1:0.present':'FALSE' if installed else 'TRUE','ide1:0.deviceType':'cdrom-image','ide1:0.fileName':str(ROOT/'dist/EngineOS-M6.iso'),'ide1:0.startConnected':'FALSE' if installed else 'TRUE','sata0.present':'TRUE','sata0:0.present':'TRUE','sata0:0.fileName':str(disk),'serial0.present':'TRUE','serial0.fileType':'pipe','serial0.fileName':self.path,'serial0.pipe.endPoint':'server','serial0.tryNoRxLoss':'FALSE','serial0.startConnected':'TRUE','serial0.yieldOnMsrRead':'TRUE','uuid.action':'create','msg.autoAnswer':'TRUE','bios.bootOrder':'hdd' if installed else 'cdrom,hdd'}
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
            self.wait('M6_WIZARD_READY');return self
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
    parser=argparse.ArgumentParser();parser.add_argument('--firmware',choices=['efi','bios'],default='efi');a=parser.parse_args();work=new_work('vmware-'+a.firmware);out=ROOT/'evidence/M6/vmware'/a.firmware;out.mkdir(parents=True,exist_ok=True)
    # This host's VMware VMX process can access the project, not AppData fixtures.
    # Use genuinely sparse owned files here, verified before starting the guest.
    vmwork=ROOT/'build/m6/vmware'/a.firmware/uuid.uuid4().hex;vmwork.mkdir(parents=True)
    disk=vmwork/'owned-flat.img';sparse_disk(disk,20*1024**3)
    descriptor=vmwork/'owned.vmdk';descriptor.write_bytes(b'version=1\nencoding="UTF-8"\nCID=fffffffe\nparentCID=ffffffff\ncreateType="monolithicFlat"\nRW 41943040 FLAT "owned-flat.img" 0\nddb.adapterType = "ide"\n')
    before=inventory();checks=[];errors=[]
    def press(g,keys):
        for k in keys:g.pipe.write(k.encode());time.sleep(.2)
    try:
        with Guest(vmwork/'live',descriptor,a.firmware) as g:
            assert 'M6_PS2_MOUSE_ONLINE' in g.output
            press(g,'\r\r\t\t\r\t\t\r'+('\t'*5)+'\r\t\r');g.wait('M6_INSTALL_PASS',timeout=240);(out/'install.log').write_bytes(g.output.encode())
        checks.append({'name':'native graphical wizard COM1 keyboard flow installs actual 20 GiB SATA disk','passed':True})
        for i in range(2):
            with Guest(vmwork/('installed-'+str(i)),descriptor,a.firmware,True) as g:
                assert 'M6_INSTALLED_VOLUME' in g.output
                # Installed controls: accent, save, remove guide, details.
                press(g,'\t\t\t\r'+('\t' if i else '')+'\r');g.wait('M6_LARGE_FILE_PASS',timeout=120);(out/('installed-'+str(i)+'.log')).write_bytes(g.output.encode())
        checks.append({'name':'two media-detached VMware boots and real >2 MiB file persistence/checksum','passed':True})
    except Exception:errors.append(redact(traceback.format_exc()));print(errors[-1])
    logpath=vmwork/'live/vmware.log';log=logpath.read_text(encoding='utf-8',errors='replace') if logpath.exists() else '';version=re.search(r'VMware Workstation[^\r\n]*version=([\d.]+) build=(\d+)',log);vmware={'version':version.group(1),'build':version.group(2)} if version else None
    m=json.loads((ROOT/'dist/m6-build-manifest.json').read_text(encoding='utf-8'));result={'passed':not errors and inventory()==before,'build_id':m['build_id'],'iso_sha256':m['artifacts']['EngineOS-M6.iso']['sha256'],'firmware':a.firmware,'vmware':vmware,'disk_gib':20,'existing_vm_inventory_unchanged':inventory()==before,'checks':checks,'errors':errors,'m6_acceptance_complete':False}
    (out/'results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print('M6 VMware '+a.firmware+' '+('PASS' if result['passed'] else 'FAIL'),flush=True)
    if not result['passed']:raise SystemExit(1)
if __name__=='__main__':main()
