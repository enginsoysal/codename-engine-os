"""Read actual native UI controls, then inject normal guest input into owned VMs."""
import importlib.util
from pathlib import Path
import re
import struct
import time
import uuid
import os
IMAGE74=os.environ.get("ENGINE74_IMAGE","EngineOS-M74")
ROOT=Path(__file__).resolve().parents[2]
def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);value=importlib.util.module_from_spec(spec);spec.loader.exec_module(value);return value
s=module('session72',ROOT/'tests/m72/session.py');q,h=s.q,s.h
MAPS74={installed:(ROOT/path).read_text(encoding="utf-8") for installed,path in [(False,"dist/"+IMAGE74+"-kernel.map"),(True,"dist/"+IMAGE74+"-installed-kernel.map")]}

def symbol(name,size=None,installed=False):
    text=MAPS74[installed]
    values=re.findall(r'^\s*([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)\s+\d+\s+'+name+r'$',text,re.M)
    if size is not None:values=[v for v in values if int(v[1],16)==size]
    assert len(values)==1,(name,values);return int(values[0][0],16),int(values[0][1],16)
def memory(g,address,size):
    text=g.qmp('human-monitor-command',{'command-line':f'xp /{(size+7)//8}gx 0x{address:x}'})
    values=[]
    for line in text.splitlines():
        if ':' in line:values.extend(int(v,16) for v in re.findall('0x[0-9a-fA-F]+',line.split(':',1)[1]))
    return b''.join(struct.pack('<Q',v) for v in values)[:size]
def integer(g,name,installed=False):return struct.unpack('<I',memory(g,symbol(name,4,installed)[0],4))[0]
class Pointer(s.s.Pointer):
    def close_app(self,kind,installed=False):
        raw=memory(self.guest,symbol('model',installed=installed)[0],28*8)
        windows=[struct.unpack_from('<4iII2B',raw,i*28) for i in range(8)]
        found=[v for v in windows if v[5]==kind and v[6]];assert len(found)==1
        x,y,w,h,*_=found[0];at=symbol('paints',8,installed)[0];before=memory(self.guest,at,8);self.logical(x+w-30,y+18,installed)
        until=time.monotonic()+15
        while True:
            painted=memory(self.guest,at,8)!=before
            if painted and self.guest.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==2:break
            if time.monotonic()>until:raise TimeoutError('Window close did not finish')
            time.sleep(.025)
    def logical(self,x,y,installed=False):
        scale=integer(self.guest,'scale73',installed);self.click(x*scale//2,y*scale//2)
    def wizard(self,action):
        g=self.guest;count=integer(g,'controls');raw=memory(g,symbol('buttons',480)[0],count*20)
        matches=[struct.unpack_from('<IIIII',raw,i*20) for i in range(count) if struct.unpack_from('<IIIII',raw,i*20)[4]==action]
        assert len(matches)==1,(action,matches,count)
        x,y,w,h,_=matches[0];before=len(g.log());self.logical(x+w//2,y+h//2)
        if action not in (3,30):g.wait('M73_VIEW step=',before)
        return before
    def desktop(self,action,installed=False,settle=True):
        g=self.guest;until=time.monotonic()+15
        while True:
            count=integer(g,'control_count',installed);raw=memory(g,symbol('controls',16384,installed)[0],count*32)
            matches=[struct.unpack_from('<iiiiQI',raw,i*32) for i in range(count) if struct.unpack_from('<iiiiQI',raw,i*32)[5]==action]
            if matches:break
            if time.monotonic()>until:raise AssertionError((action,count))
            time.sleep(.05)
        x,y,w,h,_,_=matches[-1];at=symbol('paints',8,installed)[0];before=memory(g,at,8);self.logical(x+w//2,y+h//2,installed)
        until=time.monotonic()+15
        while True:
            painted=memory(g,at,8)!=before
            if painted and (not settle or g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==2):break
            if time.monotonic()>until:raise TimeoutError('Native desktop did not finish input and painting')
            time.sleep(.025)
class Guest74(q.Guest):
    def __exit__(self,*exc):
        # QMP quit can close its socket before the response. Let device/file
        # finalizers finish before falling back to termination on this owned VM.
        if self.process.poll() is None:
            try:self.qmp('quit')
            except Exception:pass
            try:self.process.wait(timeout=10)
            except Exception:self.process.terminate();self.process.wait(timeout=5)
        super().__exit__(*exc)

def guest(folder,disk,resolution=(1280,800),firmware='uefi',media='iso',**kwargs):
    g=Guest74(folder,disk,firmware,media,image_name=IMAGE74,**kwargs)
    if firmware=='uefi':g.args += ['-vga','none','-device',f'VGA,vgamem_mb=64,edid=on,xres={resolution[0]},yres={resolution[1]}']
    if firmware=='uefi':
        # Configure the owned firmware's ordinary preferred-resolution variable.
        # This old OVMF build does not publish an Active EDID protocol.
        path=folder/'vars.fd';data=bytearray(path.read_bytes());at=0x64
        assert data[0x48:0x58]==uuid.UUID('aaf32c78-947b-439a-a180-2e144ec37792').bytes_le
        assert data[at:at+256]==b'\xff'*256,'Expected a new, empty owned NVRAM store'
        name=('PlatformConfig\0').encode('utf-16le');payload=struct.pack('<II',*resolution)
        header=struct.pack('<HBBIQ16sIII16s',0x55aa,0x3f,0,7,0,bytes(16),0,len(name),len(payload),uuid.UUID('7235c51c-0c80-4cab-87ac-3b084a6304b1').bytes_le)
        # x86 variable name/data alignment is one; only record headers align to four.
        record=header+name+payload;record+=bytes((-len(record))%4)
        data[at:at+len(record)]=record;path.write_bytes(data)
    return g
def shot(g,out,name):
    until=time.monotonic()+20
    while g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')!=2:
        if time.monotonic()>until:raise TimeoutError('Native frame did not finish before capture')
        time.sleep(.025)
    h.shot(g,out,name)
def wait_view(g,step,start=0):g.wait('M73_VIEW step=0x'+format(step,'016x'),start)
