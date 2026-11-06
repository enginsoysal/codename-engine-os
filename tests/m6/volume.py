"""Host-execute actual native C storage on disposable sparse regular files."""
import ctypes as c,hashlib,json,struct,subprocess,sys,tempfile,zlib,traceback
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools/build'))
from m1 import pinned_tools
IO=c.CFUNCTYPE(c.c_uint,c.c_void_p,c.c_uint64,c.c_void_p,c.c_uint)
FLUSH=c.CFUNCTYPE(c.c_uint,c.c_void_p)
SOURCE=c.CFUNCTYPE(c.c_uint,c.c_void_p,c.c_uint64,c.c_void_p,c.c_uint)
class Entry(c.Structure):_fields_=[('path',c.c_char*256),('first',c.c_uint64),('size',c.c_uint64),('crc',c.c_uint32),('kind',c.c_uint32)]
class Volume(c.Structure):_fields_=[('context',c.c_void_p),('read',IO),('write',IO),('flush',FLUSH),('blocks',c.c_uint64),('generation',c.c_uint64),('active',c.c_uint),('mounted',c.c_uint),('readonly',c.c_uint),('previous_valid',c.c_uint),('entries',Entry*1024),('previous',Entry*1024),('pending',Entry*1024),('page',c.c_ubyte*4096)]
def main():
    work=ROOT/'build/m6/volume';work.mkdir(parents=True,exist_ok=True);out=ROOT/'evidence/M6';out.mkdir(exist_ok=True);checks=[];result={'passed':False,'scope':'actual C storage host execution, not guest install qualification','checks':checks}
    try:
        tools=pinned_tools();src=ROOT/'kernel/storage/volume6.c'
        flags=['-std=c17','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin','-fno-stack-protector','-I',ROOT/'include']
        def run(args):
            p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True)
            if p.returncode:raise RuntimeError(p.stderr.replace(str(ROOT),'<project-root>'))
        for target,suffix in [('x86_64-unknown-none-elf','o'),('x86_64-pc-windows-msvc','obj')]:run([tools['clang'],'--target='+target,*flags,'-c',src,'-o',work/('volume.'+suffix)])
        names=['format','mount','mkdir','put','read','remove','verify','capacity','used']
        run([tools['lld-link'],'/dll','/noentry','/nodefaultlib',*['/export:v6_'+n for n in names],work/'volume.obj','/out:'+str(work/'volume.dll')])
        lib=c.CDLL(str(work/'volume.dll'))
        for n in ['format','mount']:getattr(lib,'v6_'+n).argtypes=[c.POINTER(Volume)]
        for n in ['mkdir','remove','verify']:getattr(lib,'v6_'+n).argtypes=[c.POINTER(Volume),c.c_char_p]
        lib.v6_put.argtypes=[c.POINTER(Volume),c.c_char_p,c.c_uint64,SOURCE,c.c_void_p]
        lib.v6_read.argtypes=[c.POINTER(Volume),c.c_char_p,c.c_uint64,c.c_void_p,c.c_uint,c.POINTER(c.c_uint)]
        for n in ['capacity','used']:getattr(lib,'v6_'+n).argtypes=[c.POINTER(Volume)];getattr(lib,'v6_'+n).restype=c.c_uint64
        class Disk:
            def __init__(self,path,size):
                self.f=path.open('w+b');self.f.truncate(size);self.size=size;self.calls=0;self.fail=0;self.torn=False
                self.read=IO(self._read);self.write=IO(self._write);self.flush=FLUSH(self._flush)
            def _read(self,ctx,block,p,n):
                if n!=4096 or block>=self.size//4096:return 1
                self.f.seek(block*4096);raw=self.f.read(n)
                if len(raw)!=n:return 1
                c.memmove(p,raw,n);return 0
            def trip(self):self.calls+=1;return self.fail and self.calls==self.fail
            def _write(self,ctx,block,p,n):
                if n!=4096 or block>=self.size//4096:return 1
                bad=self.trip()
                if bad and not self.torn:return 1
                self.f.seek(block*4096);self.f.write(c.string_at(p,127 if bad else n));return 1 if bad else 0
            def _flush(self,ctx):
                self.f.flush();return 1 if self.trip() else 0
            def volume(self):return Volume(context=None,read=self.read,write=self.write,flush=self.flush,blocks=self.size//4096)
        def pattern(off,n,seed=19):return bytes(((off+i)*37+seed)&255 for i in range(n))
        @SOURCE
        def source(ctx,off,p,n):c.memmove(p,pattern(off,n),n);return 0
        @SOURCE
        def alternate(ctx,off,p,n):c.memmove(p,pattern(off,n,71),n);return 0
        def read(v,path,off,n):
            raw=c.create_string_buffer(n);actual=c.c_uint();s=lib.v6_read(c.byref(v),path,off,raw,n,c.byref(actual));assert s==0,s;return raw.raw[:actual.value]
        with tempfile.TemporaryDirectory(dir=work) as td:
            assert Path(td).resolve().is_relative_to(work.resolve())
            disk=Disk(Path(td)/'owned-large.img',20*1024**3);v=disk.volume();assert lib.v6_format(c.byref(v))==0
            assert lib.v6_mkdir(c.byref(v),b'/Users')==0;assert lib.v6_mkdir(c.byref(v),b'/Users/Owner')==0
            size=2*1024**2+113;path=b'/Users/Owner/large.bin';assert lib.v6_put(c.byref(v),path,size,source,None)==0
            assert lib.v6_verify(c.byref(v),path)==0
            assert lib.v6_capacity(c.byref(v))==disk.size-160*4096 and lib.v6_used(c.byref(v))==(size+4095)//4096*4096
            for i in range(3):
                v=disk.volume();assert lib.v6_mount(c.byref(v))==0
                for off,n in [(0,7001),(4093,17000),(size-5000,8192),(size,20)]:assert read(v,path,off,n)==pattern(off,min(n,size-off))
                assert lib.v6_verify(c.byref(v),path)==0
            checks.append({'name':'20 GiB sparse target, full data capacity accounting, >2 MiB file, streaming partial reads and three remounts','passed':True})
            for bad in [b'/../x',b'/Users//x',b'/missing/x',b'/Users/Owner/'+b'a'*64,b'/Users/Owner/a\\b']:
                assert lib.v6_put(c.byref(v),bad,1,source,None)==2
            assert lib.v6_remove(c.byref(v),b'/Users')==2
            assert lib.v6_put(c.byref(v),path,2**64-1,source,None)==5
            assert lib.v6_put(c.byref(v),path,12345,alternate,None)==0
            assert read(v,path,0,12345)==pattern(0,12345,71)
            assert lib.v6_remove(c.byref(v),path)==0
            assert lib.v6_remove(c.byref(v),path)==3
            assert lib.v6_mkdir(c.byref(v),b'/Users/Owner/reused')==0
            checks.append({'name':'invalid paths, oversized writes, nonempty-directory refusal, replacement and deletion','passed':True})
            # Each injected interruption uses an independently recreated owned image.
            cases=0
            for torn in [False,True]:
                for fault in range(1,95):
                    d=Disk(Path(td)/'fault.img',16*1024**2);w=d.volume();assert lib.v6_format(c.byref(w))==0
                    assert lib.v6_put(c.byref(w),b'/document',65537,source,None)==0
                    d.calls=0;d.fail=fault;d.torn=torn
                    status=lib.v6_put(c.byref(w),b'/document',65537,alternate,None)
                    assert status in (0,1)
                    if status==1:assert w.readonly==1
                    d.fail=0;w=d.volume();assert lib.v6_mount(c.byref(w))==0
                    data=read(w,b'/document',0,65537)
                    assert data in (pattern(0,65537),pattern(0,65537,71))
                    assert lib.v6_verify(c.byref(w),b'/document')==0
                    d.f.close();cases+=1
            checks.append({'name':'write/flush and torn-write interruptions: remount yields complete old or complete new file with valid checksum','passed':True,'fault_cases':cases})
            # Independent byte-level validation and corruption of both metadata generations.
            d=Disk(Path(td)/'corrupt.img',8*1024**2);w=d.volume();assert lib.v6_format(c.byref(w))==0
            assert lib.v6_put(c.byref(w),b'/document',9000,source,None)==0
            d.f.seek(w.active*71*4096);header=d.f.read(4096);assert header[:8]==b'ENGINEV6' and zlib.crc32(header[:4092])==struct.unpack_from('<I',header,4092)[0]
            d.f.seek((w.active*71+1)*4096);table=d.f.read(1024*280);assert zlib.crc32(table)==struct.unpack_from('<I',header,36)[0]
            e=next(e for e in w.entries if e.kind==1);d.f.seek(e.first*4096);d.f.write(b'corrupt');d.f.flush();assert lib.v6_verify(c.byref(w),b'/document')==2
            for bank in [0,1]:d.f.seek(bank*71*4096);d.f.write(b'bad-magic')
            w=d.volume();assert lib.v6_mount(c.byref(w))==2 and w.readonly and not w.mounted
            d.f.close();disk.f.close()
            checks.append({'name':'independent header/table CRC inspection, payload corruption detected, dual invalid metadata refuses mount','passed':True})
        result.update(passed=True,source_sha256=hashlib.sha256(src.read_bytes()).hexdigest(),header_sha256=hashlib.sha256((ROOT/'include/engine/storage/volume6.h').read_bytes()).hexdigest(),volume_state_bytes=c.sizeof(Volume),whole_file_buffer_required=False,guest_qualification=False)
    except Exception:
        result['error']=traceback.format_exc().replace(str(ROOT),'<project-root>');print(result['error'])
    (out/'volume-results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode())
    print('M6 volume '+('PASS' if result['passed'] else 'FAIL'),flush=True)
    if not result['passed']:raise SystemExit(1)
if __name__=='__main__':main()
