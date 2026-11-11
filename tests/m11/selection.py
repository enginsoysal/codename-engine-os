"""Actual M11 pointer/keyboard tasks; uses ordinary QMP input, never writes guest memory."""
import hashlib,json,struct,time,traceback
import ui
ROOT=ui.ROOT
out=ROOT/'build/m11/selection';out.mkdir(parents=True,exist_ok=True)
checks=[]
def state(g,name,size=4):return ui.memory(g,ui.symbol(name,16384 if name=='controls' else None,True)[0],size)
def flag(g,name):
 address,size=ui.symbol(name,installed=True)
 return int.from_bytes(ui.memory(g,address,min(size,4)),'little')
def wait(g,name,value):
 end=time.monotonic()+30
 while flag(g,name)!=value:
  if time.monotonic()>end:raise TimeoutError(name+' did not become '+str(value))
  time.sleep(.025)
def modifier(g,name,down,expected):
 g.qmp('input-send-event',{'events':[{'type':'key','data':{'down':down,'key':{'type':'qcode','data':name}}}]})
 end=time.monotonic()+10
 while True:
  value=int.from_bytes(state(g,'legacy_modifiers11'),'little')|int.from_bytes(state(g,'usb_modifiers11'),'little')
  if value==expected:return
  if time.monotonic()>end:raise TimeoutError('Modifier was not consumed')
  time.sleep(.025)
def windows(g):
 raw=state(g,'model',224)
 return [struct.unpack_from('<4iII2B',raw,i*28) for i in range(8)]
def count(g,kind=None):return sum(bool(v[6]) and (kind is None or v[5]==kind) for v in windows(g))
def control(g,command):
 n=ui.integer(g,'control_count',True);raw=state(g,'controls',n*32)
 matches=[struct.unpack_from('<iiiiQI',raw,i*32) for i in range(n) if struct.unpack_from('<I',raw,i*32+24)[0]==command]
 assert matches,(command,n)
 x,y,w,h,_,_=matches[-1];return x+w//2,y+h//2
def double_click(g,p,x,y):
 scale=ui.integer(g,'scale73',True);p.move(x*scale//2,y*scale//2);p.drain()
 began=time.monotonic()
 for down in (True,False,True,False):
  g.qmp('input-send-event',{'events':[{'type':'btn','data':{'down':down,'button':'left'}}]});time.sleep(.06)
 elapsed=time.monotonic()-began
 assert elapsed<.5,('Host could not deliver a 500-ms double click',elapsed)
 p.drain()
 return elapsed
def wizard(g,p,command):
 n=ui.integer(g,'controls');raw=ui.memory(g,ui.symbol('buttons',480)[0],n*20)
 found=[struct.unpack_from('<5I',raw,i*20) for i in range(n) if struct.unpack_from('<I',raw,i*20+16)[0]==command];assert len(found)==1
 x,y,w,h,_=found[0];start=len(g.log());p.logical(x+w//2,y+h//2);return start
def shot(g,name):
 g.qmp('stop')
 try:ui.raw_shot(g,out,name,True)
 finally:g.qmp('cont')
def main():
 work=ui.q.new_work('m11-selection');disk=ui.q.sparse_disk(work/'owned.img',2*1024**3)
 report={'passed':False,'checks':checks,'iso_sha256':hashlib.sha256((ROOT/'dist/EngacerOS-M11STAGE1.iso').read_bytes()).hexdigest()}
 try:
  with ui.guest(work/'install',disk) as g:
   p=ui.Pointer(g)
   for command in (2,100,2,2,2):wizard(g,p,command)
   start=wizard(g,p,2);g.wait('M6_INSTALL_PASS',start,240)
   start=wizard(g,p,84);g.wait('M7_DESKTOP_READY',start,180);p=ui.Pointer(g);p.drain()
   assert count(g,1)==1;p.close_app(1,True);p.drain();assert count(g)==0
   p.logical(65,145,True);assert count(g)==0
   assert int.from_bytes(state(g,'desktop_selection11'),'little')==1
   checks.append('Single desktop-icon click selects without launching')
   modifier(g,'ctrl',True,1);p.logical(65,240,True);modifier(g,'ctrl',False,0)
   assert int.from_bytes(state(g,'desktop_selection11'),'little')==3
   p.logical(65,145,True)
   modifier(g,'shift',True,2);p.logical(65,335,True);modifier(g,'shift',False,0)
   assert int.from_bytes(state(g,'desktop_selection11'),'little')==7
   checks.append('Ctrl toggles and Shift extends actual desktop icon selection')
   scale=ui.integer(g,'scale73',True)
   p.move(180*scale//2,100*scale//2);p.drain();p.down(True);p.drain()
   p.move(25*scale//2,295*scale//2);p.drain()
   assert flag(g,'marquee11')==1
   assert int.from_bytes(state(g,'desktop_selection11'),'little')==3
   p.down(False);p.drain()
   checks.append('Background marquee selects actual desktop icons without launching')
   p.logical(65,145,True)
   ui.h.key(g,'ret');p.drain();assert count(g,1)==1
   checks.append('Enter opens selected desktop icon')
   # Documents sidebar remains a one-release button.
   p.desktop(94,True);p.desktop(26,True)
   for key in ('m','1','1','ret'):ui.h.key(g,key)
   g.wait('M72_FOLDER_CREATED /Users/Owner/Documents/m11');p.drain()
   p.desktop(26,True)
   for key in ('m','1','2','ret'):ui.h.key(g,key)
   g.wait('M72_FOLDER_CREATED /Users/Owner/Documents/m12');p.drain()
   base=ui.symbol('volume',0xd3040,True)[0]+64;index=0;target=None
   for i in range(1024):
    row=ui.memory(g,base+i*280,280)
    if not struct.unpack_from('<I',row,276)[0]:continue
    path=row[:256].split(b'\0')[0].decode();prefix='/Users/Owner/Documents/'
    if not path.startswith(prefix) or '/' in path[len(prefix):]:continue
    if path==prefix+'m11':target=1000+index;break
    index+=1
   assert target is not None
   before=g.log().count('M72_FOLDER_VISIT ')
   x,y=control(g,target);p.logical(x,y,True);assert g.log().count('M72_FOLDER_VISIT ')==before
   checks.append('Single Files row click selects without navigating')
   # Secondary release produces only a menu; Escape cancels it.
   scale=ui.integer(g,'scale73',True);p.move(x*scale//2,y*scale//2);p.drain()
   for down in (True,False):g.qmp('input-send-event',{'events':[{'type':'btn','data':{'down':down,'button':'right'}}]});p.drain()
   wait(g,'context11',1);assert g.log().count('M72_FOLDER_VISIT ')==before
   shot(g,'selected-context');ui.h.key(g,'esc');wait(g,'context11',0)
   checks.append('Right release opens a native context menu; Escape consumes cancellation')
   g.qmp('send-key',{'keys':[{'type':'qcode','data':'shift'},{'type':'qcode','data':'f10'}],'hold-time':30})
   wait(g,'context11',1);ui.h.key(g,'down');ui.h.key(g,'ret');wait(g,'context11',0);p.drain()
   assert g.log().count('M72_FOLDER_VISIT ')==before
   checks.append('Shift+F10, arrows and Enter operate the menu without activating the underlying row')
   # A button pressed inside and released outside must not execute.
   x,y=control(g,95);p.move(x*scale//2,y*scale//2);p.drain();p.down(True);p.drain();p.move(4,4);p.drain();p.down(False);p.drain()
   assert g.log().count('M72_FOLDER_VISIT ')==before
   checks.append('Release outside a pressed control cancels activation')
   ui.h.key(g,'home');p.drain();ui.h.key(g,'down');p.drain()
   slot=next(i for i,v in enumerate(windows(g)) if v[6] and v[5]==1)
   assert struct.unpack_from('<I',state(g,'selection_focus11',32),slot*4)[0]==1
   assert state(g,'selected11',1024)[slot*128]&2
   shot(g,'keyboard-selection')
   checks.append('Arrow keyboard route changes the selected native row')
   x,y=control(g,1000);modifier(g,'ctrl',True,1);p.logical(x,y,True);modifier(g,'ctrl',False,0)
   assert state(g,'selected11',1024)[slot*128]&3==3
   checks.append('Ctrl adds a second actual Files item without opening it')
   w=next(v for v in windows(g) if v[6] and v[5]==1);wx,wy,ww,wh=w[:4]
   x,y=control(g,1000)
   p.move((wx+ww-200)*scale//2,(wy+wh-150)*scale//2);p.drain();p.down(True);p.drain()
   p.move((x-100)*scale//2,(y-5)*scale//2);p.drain()
   assert flag(g,'marquee11')==1
   assert state(g,'selected11',1024)[slot*128]&7==7
   shot(g,'marquee-selection');p.down(False);p.drain()
   assert flag(g,'marquee11')==0
   checks.append('Files background marquee selects three real entries and releases capture')
   ui.h.key(g,'esc');p.drain()
   # A caption movement within the threshold must keep the window still.
   w=next(v for v in windows(g) if v[6] and v[5]==1);wx,wy,ww,wh=w[:4]
   p.move((wx+150)*scale//2,(wy+17)*scale//2);p.drain();p.down(True);p.drain();p.move((wx+153)*scale//2,(wy+17)*scale//2);p.drain()
   assert next(v for v in windows(g) if v[6] and v[5]==1)[:4]==w[:4]
   p.move((wx+180)*scale//2,(wy+27)*scale//2);p.drain();p.down(False);p.drain()
   assert next(v for v in windows(g) if v[6] and v[5]==1)[:2]!=(wx,wy)
   checks.append('Caption capture respects the drag threshold, then moves the actual window')
   x,y=control(g,target);report["double_click_host_seconds"]=double_click(g,p,x,y)
   g.wait('M72_FOLDER_VISIT /Users/Owner/Documents/m11');p.drain()
   checks.append('Double-click opens the real selected folder once')
   shot(g,'double-opened-folder')
   p.desktop(0,True);p.desktop(3,True);p.desktop(32100,True)
   assert struct.unpack_from('<I',state(g,'interaction11',80),72)[0]==900
   shot(g,'saved-double-click-preference')
  assert ui.s.s.document(disk,'/System/Settings/input11.bin')==struct.pack('<II',0x31314955,900)
  with ui.guest(work/'disk-only',disk,media='installed',ready='M7_DESKTOP_READY') as g:
   assert struct.unpack_from('<I',state(g,'interaction11',80),72)[0]==900
   shot(g,'independent-boot-preference')
  checks.append('Click interval saved through Settings, independent disk-byte verification and detached disk startup')
  report['passed']=True
 except Exception:report['error']=traceback.format_exc();raise
 finally:(out/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
 print('M11 native selection PASS')
if __name__=='__main__':main()
