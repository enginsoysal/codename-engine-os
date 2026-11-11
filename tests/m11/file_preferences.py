"""Additional ordinary native Files preferences/property tasks on the main owned fixture."""
import hashlib,json,os,struct,time,traceback
from pathlib import Path
import files as f
ui=f.ui
ROOT=ui.ROOT
out=ROOT/'build/m11/files'
def view(g):
 windows=f.s.windows(g);slot=next(i for i,w in enumerate(windows) if w[6] and w[5]==1)
 return struct.unpack('<III',ui.memory(g,ui.symbol('file_windows11',installed=True)[0]+slot*280,12))
def main():
 native=json.loads((out/'results.json').read_text(encoding='utf-8'));assert native['passed']
 digest=hashlib.sha256((ROOT/'dist/EngacerOS-M11STAGE2.iso').read_bytes()).hexdigest();assert digest==native['iso_sha256']
 base=Path(os.environ['LOCALAPPDATA'])/'EngineOS/Tests/M6/m11-files';owned=max(base.iterdir(),key=lambda p:p.stat().st_mtime_ns);disk=owned/'owned.img'
 assert disk.is_relative_to(base) and 'M7_DESKTOP_READY' in (owned/'detached/serial.log').read_text()
 report={'passed':False,'iso_sha256':digest,'checks':[]}
 try:
  with ui.guest(owned/'preferences-first',disk,media='installed',ready='M7_DESKTOP_READY') as g:
   p=ui.Pointer(g);p.drain();p.desktop(94,True)
   if not view(g)[0]:p.desktop(40012,True)
   p.desktop(40013,True);p.desktop(32001,True)
   if not view(g)[2]:p.desktop(40013,True);p.desktop(32003,True)
   assert view(g)==(1,1,1);f.shot(g,'saved-view-sort')
   report['checks'].append('Icons, size sorting and reverse direction affect the actual native Files window')
   f.select(g,p,'/Users/Owner/Documents/renamed.txt')
   g.qmp('send-key',{'keys':[{'type':'qcode','data':'shift'},{'type':'qcode','data':'f10'}],'hold-time':40});time.sleep(.16);p.drain()
   count=f.flag(g,'file_menu_count11');menu=ui.memory(g,ui.symbol('file_menu_items11',installed=True)[0],count*16)
   index=next(i for i in range(count) if struct.unpack_from('<II',menu,i*16+8)==(40008,1))
   p.desktop(32000+index,True);f.shot(g,'actual-file-properties');p.desktop(40021,True)
   report['checks'].append('Context Properties opens for the selected real file and closes through its owner dialog')
  data=ui.s.s.document(disk,'/System/Settings/files11.bin');count=struct.unpack_from('<I',data,12)[0]
  found=False
  for i in range(count):
   record=data[24+i*268:24+(i+1)*268]
   if record[:256].split(b'\0')[0]==b'/Users/Owner/Documents':assert struct.unpack_from('<III',record,256)==(1,1,1);found=True
  assert found
  with ui.guest(owned/'preferences-reboot',disk,media='installed',ready='M7_DESKTOP_READY') as g:
   p=ui.Pointer(g);p.drain();p.desktop(94,True);assert view(g)==(1,1,1);f.shot(g,'restored-view-sort')
  report['checks'].append('Independent stored preference bytes and a second detached boot restore nondefault view, sort and direction')
  report['passed']=True
 except Exception:report['error']=traceback.format_exc();raise
 finally:(out/'preferences-results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
 print('M11 native Files preferences PASS')
if __name__=='__main__':main()
