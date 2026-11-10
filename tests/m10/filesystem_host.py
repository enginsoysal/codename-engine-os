"""Actual C USB filesystem broker plus an independent FAT implementation."""
import ctypes as c,hashlib,importlib.util,json,os,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'tools/build'),str(ROOT/'tools/local/m10-python'),str(ROOT/'tests/m6')]
from m1 import pinned_tools,command
from media import fat32,gpt
from pyfatfs.PyFatFS import PyFatFS
spec=importlib.util.spec_from_file_location('volume_types',ROOT/'tests/m6/volume.py')
types=importlib.util.module_from_spec(spec);spec.loader.exec_module(types)
IO,FLUSH,SOURCE,Volume=types.IO,types.FLUSH,types.SOURCE,types.Volume
READER=c.CFUNCTYPE(c.c_uint,c.c_uint,c.c_uint64,c.c_void_p)
TRANSFER=c.CFUNCTYPE(c.c_uint,c.c_uint,c.c_uint64,c.c_void_p,c.c_uint,c.c_uint)
SYNC=c.CFUNCTYPE(c.c_uint,c.c_uint)
class Row(c.Structure):_fields_=[('name',c.c_char*256),('kind',c.c_uint),('size',c.c_uint32)]
class Native:
 def __init__(self,path):
  self.f=path.open('w+b');self.f.truncate(16*1024**2)
  self.r=IO(self.read);self.w=IO(self.write);self.s=FLUSH(self.sync)
  self.v=Volume(read=self.r,write=self.w,flush=self.s,blocks=16*1024**2//4096)
 def read(self,ctx,lba,p,n):
  self.f.seek(lba*4096);data=self.f.read(n)
  if len(data)!=n:return 1
  c.memmove(p,data,n);return 0
 def write(self,ctx,lba,p,n):self.f.seek(lba*4096);self.f.write(c.string_at(p,n));return 0
 def sync(self,ctx):self.f.flush();return 0
class USB:
 def __init__(self,path,data):
  self.path=path;path.write_bytes(data);self.f=path.open('r+b');self.calls=0;self.fail=0;self.writes=0
  self.r=READER(self.read);self.io=TRANSFER(self.transfer);self.s=SYNC(self.sync)
 def read(self,ctx,lba,p):return self.transfer(ctx,lba,p,512,0)
 def transfer(self,ctx,lba,p,n,writing):
  if lba*512+n>self.path.stat().st_size:return 2
  if writing:
   self.calls+=1
   if self.fail and self.calls==self.fail:return 3
   self.f.seek(lba*512);self.f.write(c.string_at(p,n));self.writes+=1
  else:
   self.f.seek(lba*512);data=self.f.read(n)
   if len(data)!=n:return 3
   c.memmove(p,data,n)
  return 0
 def sync(self,ctx):self.f.flush();return 0
 def close(self):self.f.flush();self.f.close()
def main():
 work=ROOT/'build/m10/filesystem-host';work.mkdir(parents=True,exist_ok=True)
 tools=pinned_tools();inputs=['tests/m10/filesystem_host.c','kernel/storage/partitions.c','kernel/storage/volume6.c','runtime/memory.c'];objects=[]
 for name in inputs:
  obj=work/(Path(name).stem+'.obj');command([tools['clang'],'--target=x86_64-pc-windows-msvc','-std=c17','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin','-fno-stack-protector','-mno-stack-arg-probe','-DENGINE_M10','-DENGINE_M92','-DENGINE_M7','-I',ROOT/'include','-c',ROOT/name,'-o',obj]);objects.append(obj)
 exports=['host_root10','host_attach10','storage_count','storage_get','storage_unavailable','rem10_volume','rem10_list','rem10_import','rem10_export','fat_read','v6_format','v6_mkdir','v6_put','v6_read','v6_verify']
 command([tools['lld-link'],'/dll','/noentry','/nodefaultlib','/timestamp:0',*['/export:'+n for n in exports],'/out:'+str(work/'filesystem.dll'),*objects])
 lib=c.CDLL(str(work/'filesystem.dll'))
 lib.host_root10.argtypes=[c.POINTER(Volume)];lib.host_attach10.argtypes=[c.c_uint,c.c_uint,c.c_uint64,READER,TRANSFER,SYNC]
 for n in ['format']:getattr(lib,'v6_'+n).argtypes=[c.POINTER(Volume)]
 for n in ['mkdir','verify']:getattr(lib,'v6_'+n).argtypes=[c.POINTER(Volume),c.c_char_p]
 lib.v6_put.argtypes=[c.POINTER(Volume),c.c_char_p,c.c_uint64,SOURCE,c.c_void_p]
 lib.v6_read.argtypes=[c.POINTER(Volume),c.c_char_p,c.c_uint64,c.c_void_p,c.c_uint,c.POINTER(c.c_uint)]
 lib.rem10_volume.argtypes=[c.c_uint,c.POINTER(c.c_uint),c.POINTER(c.c_uint)]
 lib.rem10_list.argtypes=[c.c_uint,c.c_uint,c.c_uint,c.c_char_p,c.c_uint,c.POINTER(Row),c.c_uint,c.POINTER(c.c_uint)]
 for n in ['import','export']:getattr(lib,'rem10_'+n).argtypes=[c.c_uint,c.c_uint,c.c_uint,c.c_char_p,c.c_char_p]
 checks=[];native=Native(work/'owned-native.img');assert lib.v6_format(c.byref(native.v))==0
 for p in [b'/Users',b'/Users/Owner',b'/Users/Owner/Documents',b'/Users/Owner/Downloads']:assert lib.v6_mkdir(c.byref(native.v),p)==0
 lib.host_root10(c.byref(native.v));payload=bytes((i*37+11)&255 for i in range(32768+113))
 @SOURCE
 def source(ctx,off,p,n):c.memmove(p,payload[off:off+n],n);return 0
 for name in [b'Welcome.txt',b'Another original file.txt']:assert lib.v6_put(c.byref(native.v),b'/Users/Owner/Documents/'+name,len(payload),source,None)==0
 original=b'Original USB data must remain unchanged.\n'
 fixture=bytes(fat32([(2,b'HELLO   TXT',original),(2,b'TAIL    TXT',b'Preserved tail\n')]))
 usb=USB(work/'owned-usb.img',fixture)
 def attach(token=0,writable=1):assert lib.host_attach10(token,writable,usb.path.stat().st_size//512,usb.r,usb.io,usb.s)==0
 def listing(token=0):
  rows=(Row*16)();total=c.c_uint();status=lib.rem10_list(0,token,0xffffffff,b'/',0,rows,16,c.byref(total));assert status==0,status
  return [bytes(rows[i].name).decode() for i in range(min(total.value,16))]
 attach();part,token=c.c_uint(),c.c_uint();assert lib.rem10_volume(0,c.byref(part),c.byref(token))==0 and part.value==0xffffffff
 assert 'HELLO.TXT' in listing();assert lib.rem10_export(0,0,part.value,b'/',b'/Users/Owner/Documents/Welcome.txt')==0
 assert lib.rem10_export(0,0,part.value,b'/',b'/Users/Owner/Documents/Another original file.txt')==0
 usb.f.flush()
 with PyFatFS(str(usb.path),read_only=True,preserve_case=True) as independent:
  assert independent.readbytes('/Welcome.txt')==payload
  assert independent.readbytes('/Another original file.txt')==payload
  assert independent.readbytes('/HELLO.TXT')==original
 checks.append('Actual C create-only FAT32 export; independent PyFatFS validates long names, unique short aliases and original payloads')
 before=usb.path.read_bytes();assert lib.rem10_export(0,0,part.value,b'/',b'/Users/Owner/Documents/Welcome.txt')!=0;usb.f.flush();assert usb.path.read_bytes()==before
 checks.append('Existing-name refusal performs no write')
 assert lib.rem10_import(0,0,part.value,b'/Welcome.txt',b'/Users/Owner/Downloads/Imported.txt')==0
 out=c.create_string_buffer(len(payload));got=c.c_uint();assert lib.v6_read(c.byref(native.v),b'/Users/Owner/Downloads/Imported.txt',0,out,len(payload),c.byref(got))==0 and out.raw==payload
 assert lib.v6_verify(c.byref(native.v),b'/Users/Owner/Downloads/Imported.txt')==0
 checks.append('Actual C FAT32-to-native import persists a CRC-verified file')
 attach(256,0);before=usb.path.read_bytes();assert lib.rem10_export(0,256,part.value,b'/',b'/Users/Owner/Documents/Welcome.txt')!=0;assert usb.path.read_bytes()==before
 for generation in range(2,42):attach(generation<<8);assert lib.storage_count()==1;assert lib.rem10_list(0,0,part.value,b'/',0,(Row*1)(),1,c.byref(got))!=0
 checks.append('Forty registry reconnect generations reuse one bounded slot and reject stale filesystem leases')
 usb.close()
 # Every negative case uses its own regular file, with no host block devices.
 fat_start=struct.unpack_from('<H',fixture,14)[0]*512
 fat_bytes=struct.unpack_from('<I',fixture,36)[0]*512
 for case in ['dirty','mirror','write-failure','deleted-hole']:
  data=bytearray(fixture)
  if case=='dirty':
   for at in [fat_start+4,fat_start+fat_bytes+4]:struct.pack_into('<I',data,at,struct.unpack_from('<I',data,at)[0]&~0x08000000)
  if case=='mirror':struct.pack_into('<I',data,fat_start+fat_bytes+400,0x0fffffff)
  if case=='deleted-hole':
   data=bytearray(fat32([(2,('F%07dTXT'%i).encode(),b'Original') for i in range(10)]+[(2,b'TAIL    TXT',b'Preserved tail\n')]))
   root=(struct.unpack_from('<H',data,14)[0]+2*struct.unpack_from('<I',data,36)[0])*512
   for i in range(1,8):data[root+i*32]=0xe5
  usb=USB(work/('owned-'+case+'.img'),data);attach(42<<8)
  before=usb.path.read_bytes()
  if case=='write-failure':usb.fail=4
  status=lib.rem10_export(0,42<<8,0xffffffff,b'/',b'/Users/Owner/Documents/Welcome.txt');usb.f.flush()
  if case=='deleted-hole':
   assert status==0
   with PyFatFS(str(usb.path),read_only=True,preserve_case=True) as independent:
    assert independent.readbytes('/TAIL.TXT')==b'Preserved tail\n' and independent.readbytes('/Welcome.txt')==payload
  else:
   assert status!=0
   if case!='write-failure':assert usb.path.read_bytes()==before and usb.writes==0
   else:
    usb.fail=0;before=usb.path.read_bytes();assert lib.rem10_export(0,42<<8,0xffffffff,b'/',b'/Users/Owner/Documents/Another original file.txt')!=0;usb.f.flush();assert usb.path.read_bytes()==before
  checks.append(case+' refusal/recovery or independent directory-preservation verification')
  usb.close()
 usb=USB(work/'owned-gpt.img',gpt(fixture,'Disposable M10 test'))
 attach(43<<8);part,token=c.c_uint(),c.c_uint();assert lib.rem10_volume(0,c.byref(part),c.byref(token))==0 and part.value==0
 assert lib.rem10_export(0,43<<8,0,b'/',b'/Users/Owner/Documents/Welcome.txt')==0;usb.f.flush()
 with PyFatFS(str(usb.path),read_only=True,preserve_case=True,offset=2048*512) as independent:assert independent.readbytes('/Welcome.txt')==payload
 checks.append('GPT-partitioned USB discovery, bounded partition-relative writes and independent FAT validation')
 usb.close();native.f.close()
 report={'passed':True,'scope':'Host execution of actual C code; native USB/controller tests remain separate','checks':checks,'independent_reader':'pyfatfs 1.1.0 (test-only, not shipped)','source_sha256':{n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in inputs+['kernel/storage/fat32.c','kernel/storage/fat32-write10.inc','kernel/storage/registry.c']}}
 (work/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');print('M10 filesystem host PASS:',len(checks),'checks')
if __name__=='__main__':main()
