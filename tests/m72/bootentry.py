"""Execute the actual UEFI boot-registration C against bounded mock variables."""
import ctypes as c
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import traceback
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools/build'))
from m1 import pinned_tools
ERROR=1<<63;NOT_FOUND=ERROR|14;SMALL=ERROR|5
GET=c.CFUNCTYPE(c.c_uint64,c.c_void_p,c.c_void_p,c.POINTER(c.c_uint32),c.POINTER(c.c_uint64),c.c_void_p)
SET=c.CFUNCTYPE(c.c_uint64,c.c_void_p,c.c_void_p,c.c_uint32,c.c_uint64,c.c_void_p)
class Runtime(c.Structure):_fields_=[('prefix',c.c_byte*72),('get',GET),('next',c.c_void_p),('set',SET),('tail',c.c_byte*16)]
class Variables:
    def __init__(self,values=None,fail_write=False):
        self.values=dict(values or {});self.writes=[];self.fail_write=fail_write;self.get=GET(self.read);self.set=SET(self.write);self.runtime=Runtime();self.runtime.get=self.get;self.runtime.set=self.set
    def read(self,name,guid,attrs,size,out):
        name=c.wstring_at(name)
        if name not in self.values:return NOT_FOUND
        attribute,value=self.values[name];attrs[0]=attribute;capacity=size[0];size[0]=len(value)
        if capacity<len(value):return SMALL
        c.memmove(out,value,len(value));return 0
    def write(self,name,guid,attrs,size,data):
        name=c.wstring_at(name)
        if self.fail_write:return ERROR|8
        assert attrs==7 and size<=514
        value=c.string_at(data,size);self.values[name]=(attrs,value);self.writes.append(name);return 0

def main():
    out=ROOT/'evidence/M7.2/bootentry';out.mkdir(parents=True,exist_ok=True);report={'passed':False,'checks':[]}
    try:
        work=ROOT/'build/m72/host';work.mkdir(parents=True,exist_ok=True);tools=pinned_tools()
        wrapper=work/'bootentry.c';wrapper.write_bytes(b'#include "boot/uefi/uefi.h"\n#include "boot/uefi/bootentry72.inc"\nuint64_t engine_boot_register(efi_runtime_services*r){return boot72_register(r);}\n')
        objects=[]
        for source,name in [(wrapper,'bootentry'),(ROOT/'runtime/memory.c','memory')]:
            obj=work/(name+'.obj');objects.append(obj)
            args=[tools['clang'],'--target=x86_64-pc-windows-msvc','-std=c17','-O2','-ffreestanding','-fno-builtin','-fno-stack-protector','-Wall','-Wextra','-Werror','-I',ROOT,'-I',ROOT/'include','-c',source,'-o',obj]
            p=subprocess.run(list(map(str,args)),capture_output=True,text=True);assert p.returncode==0,p.stderr.replace(str(ROOT),'<project-root>')
        p=subprocess.run(list(map(str,[tools['lld-link'],'/dll','/noentry','/nodefaultlib','/export:engine_boot_register',*objects,'/out:'+str(work/'bootentry.dll')])),capture_output=True,text=True);assert p.returncode==0,p.stderr
        lib=c.CDLL(str(work/'bootentry.dll'));fn=lib.engine_boot_register;fn.argtypes=[c.POINTER(Runtime)];fn.restype=c.c_uint64
        foreign={'Boot0001':(7,b'foreign OS option'),'BootE000':(7,b'foreign large '+b'x'*400),'BootOrder':(7,struct.pack('<HHH',1,0xe000,1))}
        state=Variables(foreign);assert fn(c.byref(state.runtime))==0
        for name,value in foreign.items():
            if name!='BootOrder':assert state.values[name]==value
        order=struct.unpack('<4H',state.values['BootOrder'][1]);assert order==(0xe001,1,0xe000,1)
        value=state.values['BootE001'][1];attrs,pathbytes=struct.unpack_from('<IH',value);assert attrs==1
        stop=6
        while value[stop:stop+2]!=b'\0\0':stop+=2
        assert value[6:stop].decode('utf-16-le')=='Engine OS';node=value[stop+2:]
        assert len(node)==pathbytes and node[:2]==b'\x04\x04' and node[-4:]==b'\x7f\xff\x04\0'
        length=struct.unpack_from('<H',node,2)[0];assert node[4:length].decode('utf-16-le').rstrip('\0')=='\\EFI\\BOOT\\BOOTX64.EFI'
        writes=list(state.writes);assert fn(c.byref(state.runtime))==0 and state.writes==writes
        report['checks'] += ['exact short-form FilePath load option independently decoded','foreign entries and relative order/duplicates preserved, occupied identifier skipped','readback-verified registration is idempotent: second boot performs zero writes']
        empty=Variables();assert fn(c.byref(empty.runtime))==0
        denied=Variables(foreign,True);assert fn(c.byref(denied.runtime))&ERROR and denied.values==foreign
        for attrs,raw in [(7,b'\x01'),(3,b'\x01\0'),(7,b'\0'*514)]:
            bad=Variables({'BootOrder':(attrs,raw)});before=dict(bad.values);assert fn(c.byref(bad.runtime))&ERROR and bad.values==before
        full=Variables({f'Boot{n:04X}':(7,b'foreign') for n in range(0xe000,0xe100)});before=dict(full.values);assert fn(c.byref(full.runtime))&ERROR and full.values==before
        report['checks'] += ['missing BootOrder safely created','read-only NVRAM refuses without overwriting variables','odd/oversized/wrong-attribute BootOrder rejected without mutation','exhausted reserved identifier range leaves foreign entries unchanged']
        report.update(passed=True,source_sha256=hashlib.sha256((ROOT/'boot/uefi/bootentry72.inc').read_bytes()).hexdigest())
    except Exception:report['error']=str(traceback.format_exc()).replace(str(ROOT),'<project-root>');print(report['error'])
    (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M7.2 boot entry '+('PASS' if report['passed'] else 'FAIL'))
    if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
