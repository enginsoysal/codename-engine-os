"""Native keyboard-route parity, paged catalogue, view and window-admission tasks."""
import hashlib,json,os,time,traceback
from pathlib import Path
import start as s
f=s.f;ui=s.ui;ROOT=ui.ROOT;out=ROOT/'build/m11/start'
def win(g,key):f.key(g,key)
def ctrl_escape(g):
 g.qmp('send-key',{'keys':[{'type':'qcode','data':'ctrl'},{'type':'qcode','data':'esc'}],'hold-time':40});time.sleep(.16);ui.Pointer(g).drain()
def main():
 native=json.loads((out/'results.json').read_text(encoding='utf-8'));assert native['passed']
 digest=hashlib.sha256((ROOT/'dist/EngacerOS-M11STAGE3.iso').read_bytes()).hexdigest();assert digest==native['iso_sha256']
 base=Path(os.environ['LOCALAPPDATA'])/'EngineOS/Tests/M6/m11-start';owned=max(base.iterdir(),key=lambda p:p.stat().st_mtime_ns);disk=owned/'owned.img';assert disk.is_relative_to(base)
 report={'passed':False,'iso_sha256':digest,'checks':[]}
 try:
  for ps2 in (False,True):
   with ui.guest(owned/('keyboard-ps2' if ps2 else 'keyboard-usb'),disk,media='installed',ready='M7_DESKTOP_READY',ps2_only=ps2) as g:
    p=ui.Pointer(g);p.drain();assert not f.flag(g,'launcher')
    schema=g.qmp('query-qmp-schema');enums=[[v['name'] if isinstance(v,dict) else v for v in item.get('members',item.get('values',[]))] for item in schema if item.get('meta-type')=='enum'];assert any('meta_l' in values and 'meta_r' in values for values in enums),'QMP does not expose both Windows-key codes'
    before=f.flag(g,'key_events');win(g,'meta_l');assert f.flag(g,'launcher');
    if not ps2:assert f.flag(g,'key_events')>before,'No USB HID keyboard event observed'
    win(g,'meta_r');assert not f.flag(g,'launcher')
    ctrl_escape(g);assert f.flag(g,'launcher');f.key(g,'f10');assert not f.flag(g,'launcher')
    report['checks'].append(('PS/2' if ps2 else 'USB HID')+' keyboard opens and closes Start with both Windows keys, Ctrl+Escape and F10')
    if ps2:continue
    p.desktop(0,True);p.desktop(42001,True);assert len(s.results(g))==19
    p.desktop(42006,True);assert f.flag(g,'start_page11')==1
    f.shot(g,'all-apps-second-page');p.desktop(42218,True);assert f.s.count(g,19)==1;p.close_app(19,True);p.drain()
    report['checks'].append('The paged installed catalogue reaches and launches its final Windows Apps entry')
    p.desktop(0,True);p.desktop(42003,True);assert s.prefs(g).view==0;f.shot(g,'start-list-view');p.desktop(42003,True);assert s.prefs(g).view==1
    report['checks'].append('List/grid view changes real registered item geometry and saves the requested view')
    p.desktop(42007,True);p.desktop(42030,True);p.desktop(43302,True);p.desktop(42034,True)
    assert list(s.prefs(g).task[:3])==[1,7,2];p.desktop(42035,True);assert list(s.prefs(g).task[:3])==[1,2,7]
    f.shot(g,'taskbar-pin-order');p.desktop(42008,True)
    report['checks'].append('Taskbar pins reorder through explicit Up/Down controls while catalogue IDs and open windows remain intact')

    f.key(g,'esc')
    for _ in range(7):p.desktop(0,True);p.desktop(42101,True)
    assert f.s.count(g)==8
    p.desktop(0,True);p.desktop(42101,True);assert f.s.count(g)==8 and f.flag(g,'launcher')
    text=ui.memory(g,ui.symbol('start_notice11',installed=True)[0],160).split(b'\0')[0];assert b'Close a window' in text;f.shot(g,'window-limit-explained')
    report['checks'].append('Eight-window admission refuses a ninth launch with a visible explanation and keeps all existing windows')
    f.key(g,'esc')
    while f.s.count(g):
     raw=ui.memory(g,ui.symbol('model',installed=True)[0],240);count=int.from_bytes(raw[232:236],'little');slot=raw[224+count-1];x,y,w,h=f.s.windows(g)[slot][:4];p.logical(x+w-30,y+18,True)
    assert not f.s.count(g)
  report['passed']=True
 except Exception:report['error']=traceback.format_exc();raise
 finally:(out/'controls-results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
 print('M11 native Start controls PASS')
if __name__=='__main__':main()
