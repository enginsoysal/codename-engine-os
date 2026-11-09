"""Reuse normal guest input with M8 development map identities."""
import os,importlib.util
from pathlib import Path
os.environ["ENGINE74_IMAGE"]="EngineOS-M8"
spec=importlib.util.spec_from_file_location("shared74",Path(__file__).resolve().parents[1]/"m74/ui.py")
shared=importlib.util.module_from_spec(spec);spec.loader.exec_module(shared)
globals().update({k:v for k,v in vars(shared).items() if not k.startswith("__")})

_original_guest=guest
def guest(*args,**kwargs):
    value=_original_guest(*args,**kwargs)
    value.args=[arg for arg in value.args if arg!="-no-reboot"]
    return value


def raw_shot(g,out,name,installed=False):
    """Decode actual stopped-guest VRAM, independent of timer-driven VGA cache."""
    import struct,hashlib,json
    from PIL import Image
    boot=memory(g,symbol('view73',160,installed)[0],160)
    address=struct.unpack_from('<Q',boot,80)[0];stride,fmt,w,h=struct.unpack_from('<4I',boot,136)
    scale=integer(g,'scale73',installed);w=w*scale//2;h=h*scale//2
    assert 320<=w<=4096 and 200<=h<=2160 and stride>=w*4 and stride*h<=64*1024**2
    path=g.folder/(name+'-vram.bin');g.qmp('human-monitor-command',{'command-line':f'pmemsave 0x{address:x} 0x{stride*h:x} "{path.as_posix()}"'})
    data=path.read_bytes();assert len(data)==stride*h
    image=Image.frombytes('RGB',(stride//4,h),data,'raw','BGRX' if fmt==2 else 'RGBX');image.crop((0,0,w,h)).save(out/(name+'.png'))
    (out/(name+'-capture.json')).write_bytes((json.dumps({'source':'actual native framebuffer physical bytes; no VGA cache or image resizing','width':w,'height':h,'stride':stride,'raw_sha256':hashlib.sha256(data).hexdigest()},indent=2)+'\n').encode())
