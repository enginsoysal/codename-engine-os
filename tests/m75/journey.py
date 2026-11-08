"""Observe every real installer step and restart the owned guest with ISO retained."""
import argparse,json,struct,time,traceback
import ui
ROOT=ui.ROOT;q=ui.q;h=ui.h

def state(g,name,installed=False):return ui.integer(g,name,installed)
def control(g,action):
 count=state(g,'controls');raw=ui.memory(g,ui.symbol('buttons',480)[0],count*20)
 return [struct.unpack_from('<5I',raw,i*20) for i in range(count) if struct.unpack_from('<I',raw,i*20+16)[0]==action]
def click(g,p,action,edge=False):
 rows=control(g,action);assert len(rows)==1,(action,rows);x,y,w,height,_=rows[0];start=len(g.log());p.logical(x+w-(12 if edge else w//2),y+height//2)
 if action not in [30,84,3]:g.wait('M73_VIEW step=',start)
 return start

def main():
 parser=argparse.ArgumentParser();parser.add_argument('--firmware',choices=['uefi','bios'],default='uefi');parser.add_argument('--resolution',default='1280x800');parser.add_argument('--ram',type=int,default=128);a=parser.parse_args()
 resolution=tuple(map(int,a.resolution.split('x')));label=a.firmware+'-'+a.resolution+'-'+str(a.ram);out=ROOT/'evidence/M7.5/journey'/label;out.mkdir(parents=True,exist_ok=True)
 work=q.new_work('m75-journey');disk=q.sparse_disk(work/'owned.img',2*1024**3);report={'passed':False,'checks':[],'firmware':a.firmware,'resolution':resolution,'ram_mib':a.ram,'human_usability_study':False}
 meta=json.loads((ROOT/'dist/m75-build-manifest.json').read_text());report.update(build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M75.iso']['sha256'])
 try:
  live=ui.guest(work/'install',disk,resolution,a.firmware,ram=a.ram)
  with live as g:
   assert g.qmp('query-memory-size-summary')['base-memory']==a.ram*1024**2
   p=ui.Pointer(g);ui.shot(g,out,'01-welcome');click(g,p,2);assert state(g,'step')==1;assert not control(g,2);ui.shot(g,out,'02-select-a-disk-disabled-next')
   # The entire row, including its rightmost capacity column, selects the disk.
   click(g,p,100,True);assert state(g,'chosen')==0;assert control(g,2);ui.shot(g,out,'03-full-row-selected')
   click(g,p,2);ui.shot(g,out,'04-your-apps');click(g,p,20);click(g,p,2);ui.shot(g,out,'05-your-style');click(g,p,32)
   click(g,p,30)
   for unused in range(5):h.key(g,'backspace')
   for c in 'alex':h.key(g,c)
   click(g,p,2);assert state(g,'step')==4;ui.shot(g,out,'06-review')
   click(g,p,1);assert state(g,'accent')==0x9caeff;assert ui.memory(g,ui.symbol('preferences',40)[0],5)==b'alex\0';click(g,p,2)
   assert state(g,'name_frames75')>0,'Typing must paint only the input field'
   assert 'M73_TEXT_OVERFLOW' not in g.log()
   assert state(g,'full_frames75')==1,'Next must never redraw the whole screen'
   before=state(g,'content_frames75');h.key(g,'tab');g.wait('M75_FRAME');assert state(g,'content_frames75')==before;assert state(g,'focus_frames75')>0
   # Return focus to Install via a real pointer action. Snapshot progress while writes run.
   start=click(g,p,2);g.wait('M6_PROGRESS',start)
   for _ in range(160):
    g.qmp('stop')
    if state(g,'step')==5 and state(g,'painting73')==0:break
    g.qmp('cont');time.sleep(.005)
   else:raise AssertionError('Could not observe a completed progress frame')
   ui.raw_shot(g,out,'07-real-progress');g.qmp('cont');g.wait('M6_INSTALL_PASS',start,240);ui.wait_view(g,6,start);ui.shot(g,out,'08-ready-restart')
   assert state(g,'full_frames75')==1
   report['checks']+=['all seven native steps captured; full-width disk selection and disabled Next before selection','guide/account/accent choices retained through Back; readable review before writes','outer frame drawn once; keyboard focus repaints rings only; real installation progress']
   report['frames']={key:state(g,key) for key in ['full_frames75','content_frames75','focus_frames75','name_frames75']}
   start=click(g,p,84);g.wait('M75_USER_RESTART',start);g.wait('M7_DESKTOP_READY',start,150);assert 'M6_INSTALLED_VOLUME' in g.log()[start:];assert 'M6_WIZARD_READY' not in g.log()[start:]
   assert ui.integer(g,'m6_bundle_size',True)==0,'Restart must load disk kernel, not resume the live kernel'
   if a.firmware=='uefi':assert 'M75_CHAIN_DISK_LOADER' in g.log()[start:] and 'M75_INSTALLED_LOADER' in g.log()[start:]
   p=ui.Pointer(g);ui.shot(g,out,'09-restarted-installed-desktop-iso-connected')
   # Create a real folder via the ordinary Files controls after independent startup.
   p.desktop(94,True);g.wait('M72_FOLDER_VISIT /Users/alex/Documents');p.desktop(26,True)
   for c in 'projects':h.key(g,c)
   h.key(g,'ret');g.wait('M72_FOLDER_CREATED /Users/alex/Documents/projects');p.desktop(1000,True);g.wait('M72_FOLDER_VISIT /Users/alex/Documents/projects');ui.shot(g,out,'09b-real-folder-navigation')
   p.desktop(0,True);p.desktop(4,True)
   for action in [13010,13011,13016,13006,13008,13011,13017]:p.desktop(action,True)
   raw=ui.memory(g,ui.symbol('apps',installed=True)[0],ui.symbol('apps',installed=True)[1]);stride=len(raw)//8
   assert any(raw[i:i+8192].split(b'\0')[0]==b'25' for i in range(0,len(raw),stride));ui.shot(g,out,'09c-installed-calculator');p.close_app(4,True)
   paints=ui.memory(g,ui.symbol('paints',8,True)[0],8);time.sleep(.6);assert paints==ui.memory(g,ui.symbol('paints',8,True)[0],8)
   assert g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==2
   report['checks'].append('installed Files creates and enters a real folder; idle does not repaint and both CPUs halt')
   report['checks'].append('ordinary Restart with ISO still connected loads installed kernel (zero installer bundle), mounted disk and real desktop')
   (out/'install-and-restart.log').write_bytes(g.log().encode());report['managed_current_bytes']=struct.unpack('<Q',ui.memory(g,ui.symbol('stats',32,True)[0],8))[0]*4096
  # Separate unforced disk startup, preserving original NVRAM, with no media.
  detached=ui.guest(work/'detached',disk,resolution,a.firmware,media='installed',ready='M7_DESKTOP_READY',ram=a.ram)
  ui.s.normal_boot(detached,live.folder/'vars.fd' if a.firmware=='uefi' else None)
  with detached as g:
   assert 'M6_INSTALLED_VOLUME' in g.log();assert ui.integer(g,'m6_bundle_size',True)==0;ui.shot(g,out,'10-desktop-without-iso');(out/'detached.log').write_bytes(g.log().encode())
  assert '/Users/alex/Documents/projects' in ui.s.directories(disk)
  saved=ui.s.s.document(disk,'/System/Settings/setup.bin');assert saved[:32].split(b'\0')[0]==b'alex' and struct.unpack_from('<II',saved,32)==(0x9caeff,0)
  report['checks'].append('separate unforced boot with ISO absent and original NVRAM; independent metadata/CRC reader confirms account/accent/guide choices')
  from PIL import Image
  with Image.open(out/'01-welcome.png') as image:assert image.size==(resolution if a.firmware=='uefi' else (1024,768));report['actual_framebuffer']=image.size
  report['passed']=True
 except Exception:
  report['error']=q.clean(traceback.format_exc());print(report['error'])
  if 'g' in locals():(out/'last-run.log').write_bytes(g.log().encode())
 (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M7.5 journey '+label+(' PASS' if report['passed'] else ' FAIL'),flush=True)
 if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
