"""USB/PS2 shortcut parity on independent installed boots, using ordinary input."""
import hashlib,json,os,time,traceback
from pathlib import Path
import windows as w
ui=w.ui;f=w.f;ROOT=ui.ROOT
def main():
 base=Path(os.environ['LOCALAPPDATA'])/'EngineOS/Tests/M6/m11-windows';owned=max(base.iterdir(),key=lambda p:p.stat().st_mtime_ns);disk=owned/'owned.img'
 native=json.loads((ROOT/'build/m11/windows/results.json').read_text());assert native['passed']
 report={'passed':False,'iso_sha256':hashlib.sha256((ROOT/'dist/EngacerOS-M11STAGE4.iso').read_bytes()).hexdigest(),'checks':[]};assert report['iso_sha256']==native['iso_sha256']
 out=ROOT/'build/m11/windows/keys';out.mkdir(parents=True,exist_ok=True);f.out=out;work=ui.q.new_work('m11-window-keys')
 try:
  for ps2 in (False,True):
   label='ps2' if ps2 else 'usb'
   with ui.guest(work/label,disk,media='installed',ready='M7_DESKTOP_READY',resolution=(1280,800),ps2_only=ps2) as g:
    p=ui.Pointer(g);p.drain();before=f.flag(g,'key_events')
    f.key(g,'meta_l');assert f.flag(g,'launcher');f.key(g,'meta_r');assert not f.flag(g,'launcher')
    assert not (f.flag(g,'usb_modifiers11')|f.flag(g,'legacy_modifiers11'))
    w.chord(g,'meta_l','e');assert f.s.count(g,1)==2 and not f.flag(g,'launcher')
    new=w.focused(g)[0];w.chord(g,'meta_l','left');assert w.geometry(g)==(0,0,640,740)
    w.chord(g,'meta_r','right');assert w.geometry(g)==(640,0,640,740)
    w.chord(g,'meta_l','up');assert w.geometry(g)==(0,0,1280,740)
    w.chord(g,'meta_l','down');assert w.geometry(g)!=(0,0,1280,740)
    w.chord(g,'alt','tab');assert w.focused(g)[0]!=new
    w.chord(g,'alt','shift','tab');assert w.focused(g)[0]==new
    w.chord(g,'alt','spc');assert f.flag(g,'context11');f.key(g,'esc');assert not f.flag(g,'context11')
    w.chord(g,'alt','f4');assert f.s.count(g,1)==1
    w.chord(g,'ctrl','esc');assert f.flag(g,'launcher');f.key(g,'esc')
    if not ps2:assert f.flag(g,'key_events')>before
    assert not (f.flag(g,'usb_modifiers11')|f.flag(g,'legacy_modifiers11'))
    w.launch(g,p,3);old=w.geometry(g);w.chord(g,'meta_l','left');assert w.geometry(g)==old
    text=ui.memory(g,ui.symbol('shell_notice',installed=True)[0],96).split(b'\0')[0];assert b'needs more room' in text
    f.shot(g,label+'-snap-limit');report['checks'].append(label.upper()+': both Windows keys, released modifiers, Win+E/arrows, forward/reverse Alt+Tab, Alt+Space, Alt+F4, Ctrl+Escape and truthful app-minimum snap refusal')
    (out/(label+'.log')).write_bytes(g.log().encode())
  report['passed']=True
 except Exception:report['error']=ui.q.clean(traceback.format_exc());raise
 finally:(out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report,indent=2))
if __name__=='__main__':main()
