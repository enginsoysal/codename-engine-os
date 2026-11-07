"""Qualify real bounded COW download streaming and concurrent document writes."""
import ctypes as c
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import zlib
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/m6'))
spec=importlib.util.spec_from_file_location('volume6_test',ROOT/'tests/m6/volume.py')
v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)


def main():
    work=ROOT/'build/m7/stream-test';work.mkdir(parents=True,exist_ok=True);tools=v.pinned_tools()
    flags=['-std=c17','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin','-fno-stack-protector','-DENGINE_M7','-I',ROOT/'include']
    def run(args):
        result=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True)
        if result.returncode:raise RuntimeError(result.stderr.replace(str(ROOT),'<project-root>'))
    for target,suffix in [('x86_64-unknown-none-elf','o'),('x86_64-pc-windows-msvc','obj')]:
        run([tools['clang'],'--target='+target,*flags,'-c',ROOT/'kernel/storage/volume6.c','-o',work/('volume.'+suffix)])
    names=['format','mount','mkdir','put','read','verify','stream_begin','stream_append','stream_finish','stream_abort']
    run([tools['lld-link'],'/dll','/noentry','/nodefaultlib',*['/export:v6_'+n for n in names],work/'volume.obj','/out:'+str(work/'volume.dll')])
    lib=c.CDLL(str(work/'volume.dll'));VP=c.POINTER(v.Volume)
    for n in ['format','mount','stream_finish','stream_abort']:getattr(lib,'v6_'+n).argtypes=[VP]
    for n in ['mkdir','verify']:getattr(lib,'v6_'+n).argtypes=[VP,c.c_char_p]
    lib.v6_stream_begin.argtypes=[VP,c.c_char_p,c.c_uint64]
    lib.v6_stream_append.argtypes=[VP,c.c_void_p,c.c_uint]
    lib.v6_put.argtypes=[VP,c.c_char_p,c.c_uint64,v.SOURCE,c.c_void_p]
    lib.v6_read.argtypes=[VP,c.c_char_p,c.c_uint64,c.c_void_p,c.c_uint,c.POINTER(c.c_uint)]
    class Disk:
        def __init__(self):
            self.data=bytearray(8*1024**2);self.calls=0;self.fail=0
            self.read=v.IO(self._read);self.write=v.IO(self._write);self.flush=v.FLUSH(self._flush)
        def _read(self,ctx,block,p,n):
            if n!=4096 or block>=len(self.data)//4096:return 1
            c.memmove(p,bytes(self.data[block*4096:(block+1)*4096]),n);return 0
        def _write(self,ctx,block,p,n):
            self.calls+=1
            if self.calls==self.fail or n!=4096 or block>=len(self.data)//4096:return 1
            self.data[block*4096:(block+1)*4096]=c.string_at(p,n);return 0
        def _flush(self,ctx):self.calls+=1;return int(self.calls==self.fail)
        def volume(self):return v.Volume(read=self.read,write=self.write,flush=self.flush,blocks=len(self.data)//4096)
    def read(volume,path):
        out=c.create_string_buffer(50000);count=c.c_uint()
        assert not lib.v6_read(c.byref(volume),path,0,out,len(out),c.byref(count));return out.raw[:count.value]
    payload=bytes((i*73+5)&255 for i in range(20003));document=b'Independent editor save'
    @v.SOURCE
    def source(ctx,offset,p,n):c.memmove(p,document[offset:offset+n],n);return 0
    disk=Disk();volume=disk.volume();ptr=c.byref(volume);assert not lib.v6_format(ptr)
    assert not lib.v6_mkdir(ptr,b'/Downloads')
    assert not lib.v6_stream_begin(ptr,b'/Downloads/file.bin',25000)
    # Namespace remains absent and normal allocations avoid the reserved extent.
    assert not any(e.kind and e.path==b'/Downloads/file.bin' for e in volume.entries)
    assert not lib.v6_put(ptr,b'/document.txt',len(document),source,None)
    offset=0
    while offset<len(payload):
        n=min(4096-offset%4096,997,len(payload)-offset)
        assert not lib.v6_stream_append(ptr,payload[offset:offset+n],n);offset+=n
    assert not lib.v6_stream_finish(ptr)
    assert read(volume,b'/Downloads/file.bin')==payload and read(volume,b'/document.txt')==document
    entry=next(e for e in volume.entries if e.kind and e.path==b'/Downloads/file.bin')
    assert entry.crc==zlib.crc32(payload)
    remount=disk.volume();assert not lib.v6_mount(c.byref(remount));assert not lib.v6_verify(c.byref(remount),b'/Downloads/file.bin')
    assert lib.v6_stream_begin(ptr,b'/Downloads/file.bin',500)==4
    assert lib.v6_stream_begin(ptr,b'/Downloads/oversized',64*1024**2+1)==8
    assert not lib.v6_stream_begin(ptr,b'/Downloads/cancelled',4096)
    assert lib.v6_stream_append(ptr,b'x'*4097,4097)==2
    assert not lib.v6_stream_append(ptr,b'x'*4096,4096)
    assert lib.v6_stream_append(ptr,b'x',1)==2
    lib.v6_stream_abort(ptr);assert not any(e.kind and e.path==b'/Downloads/cancelled' for e in volume.entries)
    assert not lib.v6_stream_begin(ptr,b'/Downloads/conflict',4096)
    assert not lib.v6_put(ptr,b'/Downloads/conflict',len(document),source,None)
    assert lib.v6_stream_finish(ptr)==4 and read(volume,b'/Downloads/conflict')==document
    cases=0
    for fault in range(1,76):
        disk=Disk();volume=disk.volume();ptr=c.byref(volume);assert not lib.v6_format(ptr)
        assert not lib.v6_put(ptr,b'/document.txt',len(document),source,None)
        assert not lib.v6_stream_begin(ptr,b'/download.bin',5000)
        assert not lib.v6_stream_append(ptr,b'a'*4096,4096)
        assert not lib.v6_stream_append(ptr,b'b'*904,904)
        disk.calls=0;disk.fail=fault;status=lib.v6_stream_finish(ptr)
        assert status in (0,1)
        disk.fail=0;other=disk.volume();assert not lib.v6_mount(c.byref(other))
        assert read(other,b'/document.txt')==document
        exists=any(e.kind and e.path==b'/download.bin' for e in other.entries)
        if exists:assert read(other,b'/download.bin')==b'a'*4096+b'b'*904 and not lib.v6_verify(c.byref(other),b'/download.bin')
        cases+=1
    report={'passed':True,'m7_complete':False,'scope':'actual C storage host execution; not native network download qualification',
            'checks':['bounded 4 KiB staging','concurrent editor allocation avoids stream reservation','namespace invisible until commit','exact payload/CRC/remount','cancellation/limits/path conflict','write/flush interruption preserves complete namespace'],'fault_cases':cases}
    (ROOT/'evidence/M7/stream-results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('Download stream PASS: concurrent saves, cancellation and '+str(cases)+' I/O interruptions')


if __name__=='__main__':main()
