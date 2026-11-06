"""Owned disposable QEMU guests; native keyboard commands and independent disk checks."""
from pathlib import Path
import hashlib,json,re,subprocess,sys,time
ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/build'),str(ROOT/'tests/m3')]
from vm.config import arguments,qemu_tools
from visual import free_port,connect
from ext2_image import check,Image,U32

def clean(log):return re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]','',log).replace(str(ROOT),'<project-root>')
def digest(path,start=0,length=None):
    with path.open('rb') as f:f.seek(start);return hashlib.sha256(f.read(length) if length else f.read()).hexdigest()

def inspect_disk(path,manifest):
    import struct,zlib
    from bios import flatten64
    with path.open('rb') as f:
        sectors=path.stat().st_size//512
        def read(lba,length=512):f.seek(lba*512);return f.read(length)
        mbr=read(0);assert mbr[450]==0xee and mbr[510:512]==b'\x55\xaa'
        tables=[];guids=[]
        for where,backup in [(1,sectors-1),(sectors-1,1)]:
            h=bytearray(read(where));assert h[:8]==b'EFI PART' and struct.unpack_from('<QQ',h,24)==(where,backup)
            crc=struct.unpack_from('<I',h,16)[0];struct.pack_into('<I',h,16,0);assert zlib.crc32(h[:92])==crc
            table=read(struct.unpack_from('<Q',h,72)[0],16384);assert zlib.crc32(table)==struct.unpack_from('<I',h,88)[0];tables.append(table);guids.append(h[56:72])
        assert tables[0]==tables[1] and guids[0]==guids[1]
        assert struct.unpack_from('<QQ',tables[0],32)==(8192,139263) and struct.unpack_from('<QQ',tables[0],288)==(139264,172031)
        boot=read(8192);reserved=struct.unpack_from('<H',boot,14)[0];fats=boot[16];fat_sectors=struct.unpack_from('<I',boot,36)[0];cluster_bytes=boot[13]*512;data_lba=8192+reserved+fats*fat_sectors
        root=read(data_lba,cluster_bytes);entry=next(root[o:o+32] for o in range(0,cluster_bytes,32) if root[o:o+11]==b'KERNEL  ELF')
        cluster=(struct.unpack_from('<H',entry,20)[0]<<16)|struct.unpack_from('<H',entry,26)[0];length=struct.unpack_from('<I',entry,28)[0];content=bytearray();seen=set()
        while len(content)<length:
            assert 2<=cluster<0x0ffffff0 and cluster not in seen;seen.add(cluster);content+=read(data_lba+(cluster-2)*boot[13],cluster_bytes);fat=read(8192+reserved+cluster*4//512);cluster=struct.unpack_from('<I',fat,cluster*4%512)[0]&0x0fffffff
        content=bytes(content[:length]);assert hashlib.sha256(content).hexdigest()==manifest['installed_kernel_sha256']
        raw,_=flatten64(content);assert read(128,len(raw))==raw
        data=bytearray(read(139264,16*1024*1024));check(data)
    return {'name':'independent primary/backup GPT CRC, FAT kernel SHA256, BIOS raw-kernel bytes and ext2 graph','passed':True}

class Guest:
    def __init__(self,folder,disk,firmware='uefi',media='iso',ram=128,cpus=2,extra=None,fault=None):
        self.folder=folder;self.firmware=firmware;files=qemu_tools();self.args,self.serial=arguments(files,media if media!='installed' else 'iso',ROOT/'dist/EngineOS-M5.iso' if media!='usb' else ROOT/'dist/EngineOS-M5-usb.img',folder,ram=ram,milestone='M5')
        self.args[self.args.index('-smp')+1]=str(cpus)
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
        try:self.wait('M4_CONSOLE_READY');return self
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

def main():
    import argparse
    p=argparse.ArgumentParser();p.add_argument('--firmware',choices=['uefi','bios'],default='uefi');p.add_argument('--media',choices=['iso','usb'],default='iso');p.add_argument('--ram',type=int,default=128);p.add_argument('--cpus',type=int,default=2);opt=p.parse_args()
    name=f'{opt.firmware}-{opt.media}-{opt.ram}m-{opt.cpus}cpu';work=ROOT/'build/m5/tests'/name/str(time.time_ns());work.mkdir(parents=True);out=ROOT/'evidence/M5'/name;out.mkdir(parents=True,exist_ok=True)
    disk=work/'target.img'
    with disk.open('xb') as f:f.truncate(256*1024*1024)
    before=digest(disk);checks=[];errors=[];manifest=json.loads((ROOT/'dist/m5-build-manifest.json').read_text(encoding='utf-8'))
    try:
        with Guest(work/'live',disk,opt.firmware,opt.media,opt.ram,opt.cpus) as g:
            assert 'M5_LIVE_VOLATILE_VOLUME' in g.log();assert 'M3_COMPLETE_PASS' in g.log()
            assert 'Write status: 0x0000000000000000' in g.command('write /Users/test/Documents/live.txt temporary')
            assert '\ntemporary' in g.command('cat /Users/test/Documents/live.txt');checks.append({'name':'live volatile filesystem writes and reads','passed':True})
            assert 'Installer status: 2' in g.command('install 0 ERASE-524288') # no plan
            assert 'No changes yet' in g.command('plan 0');assert 'cancelled' in g.command('cancel')
            assert before==digest(disk);checks.append({'name':'confirmation and cancellation leave whole disk unchanged','passed':True})
            g.command('plan 0');assert 'Installer status: 2' in g.command('install 0 ERASE-524289')
            text=g.command('install 0 ERASE-524288','M5_INSTALL_PASS',180);assert 'M5_INSTALL_PASS' in text
            g.command('parts 0');assert 'GPT: 3 partitions' in g.log()
            regs=g.qmp('human-monitor-command',{'command-line':'info registers -a'});assert regs.count('HLT=1')==opt.cpus
            checks.append({'name':'native GPT/EFI/ext2 install, byte readback and idle CPU halt','passed':True})
            g.command('info');stats=re.search(r'Managed current bytes=(0x[0-9a-f]+) peak bytes=(0x[0-9a-f]+)',g.log());reserved=re.search(r'M2_BOOT_RESERVED_BYTES=(0x[0-9a-f]+)',g.log());assert stats and reserved
            retained,peak=(int(v,16) for v in stats.groups());boot_reserved=int(reserved.group(1),16)
            assert retained<=512*1024 and peak<=1024*1024 and boot_reserved<=2*1024*1024
            import ctypes
            from ctypes import wintypes as w
            api=ctypes.WinDLL('kernel32',use_last_error=True);api.GetProcessTimes.argtypes=[w.HANDLE]+[ctypes.POINTER(w.FILETIME)]*4;api.GetProcessTimes.restype=w.BOOL
            def cpu():
                vals=[w.FILETIME() for _ in range(4)];assert api.GetProcessTimes(int(g.process._handle),*[ctypes.byref(v) for v in vals]);return sum((v.dwHighDateTime<<32)|v.dwLowDateTime for v in vals[2:])/1e7
            used=cpu();start=time.monotonic();time.sleep(2);elapsed=time.monotonic()-start;used=cpu()-used;assert used/elapsed<=.35
            checks.append({'name':'bounded managed/image memory and actual idle QEMU CPU interval','passed':True,'managed_retained_bytes':retained,'managed_peak_bytes':peak,'boot_reserved_bytes':boot_reserved,'idle_wall_seconds':elapsed,'idle_host_cpu_seconds':used,'idle_one_cpu_percent':used/elapsed*100})
            (out/'live.log').write_text(g.log(),encoding='utf-8')
        checks.append(inspect_disk(disk,manifest))
        for reboot in range(2):
            with Guest(work/f'installed-{reboot}',disk,opt.firmware,'installed',opt.ram,opt.cpus) as g:
                assert 'M5_INSTALLED_VOLUME' in g.log();assert 'M3_COMPLETE_PASS' in g.log()
                if reboot==0:
                    assert 'Write status: 0x0000000000000000' in g.command('write /Users/test/Documents/proof.txt durable')
                    assert 'Read status:' in g.command('cat /Users/test/Documents/live.txt')
                else:
                    assert '\ndurable' in g.command('cat /Users/test/Documents/proof.txt')
                    ppm=work/'installed.ppm';g.qmp('screendump',{'filename':str(ppm)})
                    from PIL import Image as Bitmap
                    with Bitmap.open(ppm) as frame:frame.save(out/'installed.png')
                (out/f'installed-{reboot}.log').write_text(g.log(),encoding='utf-8')
        checks.append({'name':'media detached, fresh firmware installed boot and durable file after second boot','passed':True})
        retained=digest(disk,139264*512,16*1024*1024)
        with disk.open('r+b') as f:f.seek(64*512);f.write(b'BROKEN-BOOT'+bytes(502))
        with Guest(work/'repair',disk,opt.firmware,opt.media,opt.ram,opt.cpus) as g:
            g.command('repairplan 0');g.command('repair 0 REPAIR-524288','M5_REPAIR_PASS',180);(out/'repair.log').write_text(g.log(),encoding='utf-8')
        assert retained==digest(disk,139264*512,16*1024*1024)
        with Guest(work/'recovered',disk,opt.firmware,'installed',opt.ram,opt.cpus) as g:
            assert 'M5_INSTALLED_VOLUME' in g.log();assert '\ndurable' in g.command('cat /Users/test/Documents/proof.txt');(out/'recovered.log').write_text(g.log(),encoding='utf-8')
        checks.append({'name':'same-build boot repair preserves entire native data partition and boots without media','passed':True})
    except Exception as e:
        import traceback
        errors.append(traceback.format_exc().replace(str(ROOT),'<project-root>'));print(errors[-1],flush=True)
    result={'passed':not errors,'build_id':manifest['build_id'],'iso_sha256':manifest['artifacts']['EngineOS-M5.iso']['sha256'],'firmware':opt.firmware,'media':opt.media,'ram_mib':opt.ram,'cpus':opt.cpus,'checks':checks,'errors':errors}
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');print(name, 'PASS' if not errors else 'FAIL',flush=True)
    if errors:raise SystemExit(1)
if __name__=='__main__':main()
