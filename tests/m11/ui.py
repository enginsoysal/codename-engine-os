"""Use ordinary guest input with isolated M11 media and map identities."""
import os,importlib.util
from pathlib import Path
os.environ["ENGINE74_IMAGE"]=os.environ.get("ENGACER11_IMAGE","EngacerOS-M11STAGE3")
spec=importlib.util.spec_from_file_location("shared74",Path(__file__).resolve().parents[1]/"m74/ui.py")
shared=importlib.util.module_from_spec(spec);spec.loader.exec_module(shared)
globals().update({k:v for k,v in vars(shared).items() if not k.startswith("__")})

_original_guest=guest
def guest(*args,controller='sata',**kwargs):
    value=_original_guest(*args,**kwargs)
    value.args=[arg for arg in value.args if arg!="-no-reboot"]
    if controller!='sata':
        at=next(i for i,x in enumerate(value.args) if x.startswith('ide-hd,bus=ide.0,drive=target'))
        old=value.args[at];boot=',bootindex=1' if ',bootindex=1' in old else ''
        if controller=='ide':extra=['-device','piix3-ide,id=pata','-device','ide-hd,bus=pata.0,drive=target'+boot]
        elif controller in ('mptsas','pvscsi'):
            extra=['-device',('mptsas1068' if controller=='mptsas' else 'pvscsi')+',id=scsi','-device','scsi-hd,bus=scsi.0,drive=target'+boot]
        elif controller=='nvme':extra=['-device','nvme,serial=ENGINEOS92,drive=target'+boot]
        elif controller=='virtio':extra=['-device','virtio-blk-pci,drive=target'+boot]
        else:raise ValueError('Unknown owned QEMU controller')
        value.args[at-1:at+1]=extra
    return value


def physical_boot(g,installed=False):
    """Use original boot dimensions; a logical 150% viewport loses rounding pixels."""
    import re,struct
    reference=memory(g,symbol('view73',160,installed)[0],160)
    values=re.findall(r'^\s*([0-9a-f]+)\s+[0-9a-f]+\s+8\s+\d+\s+screen$',MAPS74[installed],re.M)
    candidates=[]
    for address in values:
        pointer=struct.unpack('<Q',memory(g,int(address,16),8))[0]
        if not 0<pointer<2**32:continue
        data=memory(g,pointer,160)
        if struct.unpack_from('<Q',data)[0]!=0x304f464e49534f42:continue
        stride,fmt,w,h=struct.unpack_from('<4I',data,136)
        if data[80:96]!=reference[80:96] or not 320<=w<=4096 or not 200<=h<=2160 or stride<w*4:continue
        candidates.append((w*h,data))
    assert candidates,'No validated physical boot framebuffer found'
    return max(candidates,key=lambda item:item[0])[1]

def raw_shot(g,out,name,installed=False):
    """Decode actual stopped-guest VRAM, independent of timer-driven VGA cache."""
    import struct,hashlib,json
    from PIL import Image
    boot=physical_boot(g,installed)
    address=struct.unpack_from('<Q',boot,80)[0];stride,fmt,w,h=struct.unpack_from('<4I',boot,136)
    assert 320<=w<=4096 and 200<=h<=2160 and stride>=w*4 and stride*h<=64*1024**2
    path=g.folder/(name+'-vram.bin');g.qmp('human-monitor-command',{'command-line':f'pmemsave 0x{address:x} 0x{stride*h:x} "{path.as_posix()}"'})
    data=path.read_bytes();assert len(data)==stride*h
    image=Image.frombytes('RGB',(stride//4,h),data,'raw','BGRX' if fmt==2 else 'RGBX');image.crop((0,0,w,h)).save(out/(name+'.png'))
    (out/(name+'-capture.json')).write_bytes((json.dumps({'source':'actual native framebuffer physical bytes; no VGA cache or image resizing','width':w,'height':h,'stride':stride,'raw_sha256':hashlib.sha256(data).hexdigest()},indent=2)+'\n').encode())

# Wait for the owned guest to consume each edge before sending another click.
# A fast harness must not collapse successive human actions during a slow repaint.
_BasePointer=Pointer
class Pointer(_BasePointer):
    def drain(self):
        import time
        deadline=time.monotonic()+120
        while True:
            installed='M7_DESKTOP_READY' in self.guest.log()
            dirty=integer(self.guest,'mouse_dirty',installed)
            pending=integer(self.guest,'mouse_head11',installed)!=integer(self.guest,'mouse_tail11',installed)
            halted=self.guest.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==len(self.guest.qmp('query-cpus-fast'))
            queued=integer(self.guest,'head',installed)!=integer(self.guest,'tail',installed)
            if not dirty and not pending and not queued and halted:return
            if time.monotonic()>deadline:raise TimeoutError('Guest did not consume the previous pointer edge')
            time.sleep(.025)
    def click(self,x,y):
        self.move(x,y);self.drain()
        self.down(True);self.drain()
        self.down(False);self.drain()
    def desktop(self,action,installed=False,settle=True):
        g=self.guest;until=time.monotonic()+15
        while True:
            count=integer(g,'control_count',installed);raw=memory(g,symbol('controls',65536,installed)[0],count*32)
            matches=[struct.unpack_from('<iiiiQI',raw,i*32) for i in range(count) if struct.unpack_from('<iiiiQI',raw,i*32)[5]==action]
            if matches:break
            if time.monotonic()>until:raise AssertionError((action,count))
            time.sleep(.05)
        x,y,w,h,_,_=matches[-1];at=symbol('paints',8,installed)[0];before=memory(g,at,8);self.logical(x+w//2,y+h//2,installed)
        until=time.monotonic()+15
        while True:
            painted=memory(g,at,8)!=before
            if painted and (not settle or g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==len(g.qmp('query-cpus-fast'))):break
            if time.monotonic()>until:raise TimeoutError('Native desktop did not finish input and painting')
            time.sleep(.025)
