"""Exercise actual native C rename and every metadata-write/flush interruption."""
import ctypes as c
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'tools/build'),str(ROOT/'tests/m6')]
from m1 import pinned_tools,command
spec=importlib.util.spec_from_file_location('volume8_types',ROOT/'tests/m6/volume.py')
v=importlib.util.module_from_spec(spec)
spec.loader.exec_module(v)


def main():
    work=ROOT/'build/m8/rename'
    work.mkdir(parents=True,exist_ok=True)
    tools=pinned_tools()
    flags=['-std=c17','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin',
           '-fno-stack-protector','-DENGINE_M8','-I',ROOT/'include']
    for target,suffix in [('x86_64-unknown-none-elf','o'),('x86_64-pc-windows-msvc','obj')]:
        command([tools['clang'],'--target='+target,*flags,'-c',ROOT/'kernel/storage/volume6.c','-o',work/('volume.'+suffix)])
    exports=['format','mount','mkdir','put','read','verify','rename']
    command([tools['lld-link'],'/dll','/noentry','/nodefaultlib','/timestamp:0',
             *['/export:v6_'+name for name in exports],work/'volume.obj','/out:'+str(work/'volume.dll')])
    lib=c.CDLL(str(work/'volume.dll'))
    for name in ('format','mount'):
        getattr(lib,'v6_'+name).argtypes=[c.POINTER(v.Volume)]
    for name in ('mkdir','verify'):
        getattr(lib,'v6_'+name).argtypes=[c.POINTER(v.Volume),c.c_char_p]
    lib.v6_put.argtypes=[c.POINTER(v.Volume),c.c_char_p,c.c_uint64,v.SOURCE,c.c_void_p]
    lib.v6_rename.argtypes=[c.POINTER(v.Volume),c.c_char_p,c.c_char_p,c.c_uint]

    class Disk:
        def __init__(self,data=None,fail=0,torn=False):
            self.data=bytearray(data) if data is not None else bytearray(4*1024**2)
            self.calls=0
            self.fail=fail
            self.torn=torn
            self.read=v.IO(self._read)
            self.write=v.IO(self._write)
            self.flush=v.FLUSH(self._flush)

        def _read(self,ctx,block,p,n):
            if n!=4096 or block>=len(self.data)//4096:return 1
            c.memmove(p,bytes(self.data[block*4096:(block+1)*4096]),n)
            return 0

        def _write(self,ctx,block,p,n):
            self.calls+=1
            if n!=4096 or block>=len(self.data)//4096:return 1
            if self.calls==self.fail:
                if self.torn:self.data[block*4096:block*4096+n//2]=c.string_at(p,n//2)
                return 1
            self.data[block*4096:(block+1)*4096]=c.string_at(p,n)
            return 0

        def _flush(self,ctx):
            self.calls+=1
            return int(self.calls==self.fail)

        def volume(self):
            return v.Volume(read=self.read,write=self.write,flush=self.flush,blocks=len(self.data)//4096)

    a=b'Original archive payload'*600
    b=b'Previous target content'
    stable=b'Independent document'
    disk=Disk()
    volume=disk.volume()
    assert lib.v6_format(c.byref(volume))==0
    assert lib.v6_mkdir(c.byref(volume),b'/Documents')==0
    for name,payload in [(b'a.zip',a),(b'b.zip',b),(b'stable.txt',stable)]:
        @v.SOURCE
        def source(ctx,offset,p,n):
            c.memmove(p,payload[offset:offset+n],n)
            return 0
        assert lib.v6_put(c.byref(volume),b'/Documents/'+name,len(payload),source,None)==0

    baseline=bytes(disk.data)
    for source,target,replace in [(b'/Documents',b'/Moved',0),
                                  (b'/Documents/a.zip',b'/Missing/new.zip',0),
                                  (b'/Documents/a.zip',b'/Documents/b.zip',0),
                                  (b'/Documents/a.zip',b'/../escape',1)]:
        assert lib.v6_rename(c.byref(volume),source,target,replace)!=0
        assert bytes(disk.data)==baseline

    def inspect(d):
        mounted=d.volume()
        assert lib.v6_mount(c.byref(mounted))==0
        files={}
        for entry in mounted.entries:
            if entry.kind==1:
                path=entry.path
                assert lib.v6_verify(c.byref(mounted),path)==0
                files[path]=bytes(d.data[entry.first*4096:entry.first*4096+entry.size])
        return files

    disk.calls=0
    assert lib.v6_rename(c.byref(volume),b'/Documents/a.zip',b'/Documents/b.zip',1)==0
    writes=disk.calls
    before={b'/Documents/a.zip':a,b'/Documents/b.zip':b,b'/Documents/stable.txt':stable}
    after={b'/Documents/b.zip':a,b'/Documents/stable.txt':stable}
    assert inspect(disk)==after
    cases=0
    for torn in (False,True):
        for fail in range(1,writes+1):
            interrupted=Disk(baseline,fail=fail,torn=torn)
            mounted=interrupted.volume()
            assert lib.v6_mount(c.byref(mounted))==0
            assert lib.v6_rename(c.byref(mounted),b'/Documents/a.zip',b'/Documents/b.zip',1)!=0
            assert inspect(interrupted) in (before,after)
            cases+=1
    paths=[ROOT/'kernel/storage/volume6.c',ROOT/'include/engine/storage/volume6.h',Path(__file__)]
    report={'passed':True,'scope':'actual C native-volume rename; host execution, no guest app claim',
            'fault_cases':cases,'checks':['invalid destinations do not modify disk',
                                       'replace is atomic across every metadata write and flush, including torn writes',
                                       'remount verifies complete old or new namespace and unchanged independent document'],
            'source_sha256':{p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}
    (work/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print('PASS: native rename, invalid destinations and '+str(cases)+' interruption cases.')


if __name__=='__main__':main()
