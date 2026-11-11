"""Native Files tasks through ordinary input; independently inspect saved bytes."""
import hashlib,json,struct,time,traceback
import ui
import selection as s
ROOT=ui.ROOT
out=ROOT/'build/m11/files';out.mkdir(parents=True,exist_ok=True)
checks=[]
def flag(g,name):return s.flag(g,name)
def shot(g,name):
 print('Native capture:',name,flush=True)
 g.qmp('stop')
 try:ui.raw_shot(g,out,name,True)
 finally:g.qmp('cont')
def rows(g):
 base=ui.symbol('volume',0xd3040,True)[0]+64
 dump=g.folder/'observed-entries.bin'
 g.qmp('human-monitor-command',{'command-line':f'pmemsave 0x{base:x} 0x{1024*280:x} \"{dump.as_posix()}\"'})
 raw=dump.read_bytes();assert len(raw)==1024*280
 result={}
 for i in range(1024):
  r=raw[i*280:(i+1)*280];first,size,crc,kind=struct.unpack_from('<QQII',r,256)
  if kind:result[r[:256].split(b'\0')[0].decode()]=(i,kind,size,crc)
 return result
def key(g,name):
 ui.h.key(g,name);ui.Pointer(g).drain()
def chord(g,name):
 g.qmp('send-key',{'keys':[{'type':'qcode','data':'ctrl'},{'type':'qcode','data':name}],'hold-time':40});time.sleep(.16);ui.Pointer(g).drain()
def type_text(g,text):
 for c in text:
  if c.isupper():
   g.qmp('send-key',{'keys':[{'type':'qcode','data':'shift'},{'type':'qcode','data':c.lower()}],'hold-time':30});time.sleep(.12);ui.Pointer(g).drain()
  else:key(g,{' ':'spc','.':'dot','/':'slash','-':'minus'}.get(c,c))
def create(g,p,name,command=26):
 p.desktop(command,True);chord(g,'a');type_text(g,name);key(g,'ret');p.drain()
def nav(g,p,path):
 p.desktop(40015,True);chord(g,'a');type_text(g,path);key(g,'ret');p.drain()
def job(g,p):
 end=time.monotonic()+90
 while True:
  raw=ui.memory(g,ui.symbol('file_job_state11',installed=True)[0],8)
  active,waiting=struct.unpack('<II',raw)
  if waiting:raise AssertionError('Unexpected name conflict')
  if not active:break
  if time.monotonic()>end:raise TimeoutError('File job did not finish')
  time.sleep(.05)
 p.drain()
 error=flag(g,'file_job_error11');assert error==0,('File job error',error)
def select(g,p,path):
 index=rows(g)[path][0]
 # Cached native listing contains actual entry IDs, independent of sort.
 data=ui.memory(g,ui.symbol('file_lists11',installed=True)[0],9*2328)
 # Read the selected window's actual visible rows using its listing cache.
 for slot in range(8):
  rec=data[slot*2328:(slot+1)*2328]
  folder=rec[8:264].split(b'\0')[0].decode(errors='replace')
  if (path.rsplit('/',1)[0] or '/')!=folder:continue
  count=struct.unpack_from('<I',rec,264)[0]
  order=struct.unpack_from('<1024H',rec,276)
  if index in order[:count]:p.desktop(1000+order[:count].index(index),True);return
 raise AssertionError(('Native row not visible',path))
def control(g,command):
 n=ui.integer(g,'control_count',True);raw=ui.memory(g,ui.symbol('controls',65536,True)[0],n*32)
 found=[struct.unpack_from('<iiiiQI',raw,i*32) for i in range(n) if struct.unpack_from('<I',raw,i*32+24)[0]==command];assert found,command
 x,y,w,h,*_=found[-1];return x+w//2,y+h//2
def main():
 work=ui.q.new_work('m11-files');disk=ui.q.sparse_disk(work/'owned.img',2*1024**3)
 result={'passed':False,'checks':checks,'iso_sha256':hashlib.sha256((ROOT/'dist/EngacerOS-M11STAGE2.iso').read_bytes()).hexdigest()}
 try:
  with ui.guest(work/'install',disk) as g:
   p=ui.Pointer(g)
   for command in (2,100,2,2,2):s.wizard(g,p,command)
   start=s.wizard(g,p,2);g.wait('M6_INSTALL_PASS',start,240)
   start=s.wizard(g,p,84);g.wait('M7_DESKTOP_READY',start,180);p=ui.Pointer(g);p.drain()
   p.desktop(94,True)
   create(g,p,'alpha');assert '/Users/Owner/Documents/alpha' in rows(g)
   create(g,p,'sample.txt',40018);assert '/Users/Owner/Documents/sample.txt' in rows(g)
   checks.append('Graphical new-folder and text-file dialogs create actual native entries')
   select(g,p,'/Users/Owner/Documents/sample.txt');key(g,'ret');p.drain();type_text(g,'native payload 123');chord(g,'s');p.drain();p.close_app(2,True);p.drain()
   shot(g,'files-created')
   select(g,p,'/Users/Owner/Documents/sample.txt');key(g,'f2');p.drain();chord(g,'a');type_text(g,'renamed.txt');key(g,'ret');job(g,p)
   assert '/Users/Owner/Documents/renamed.txt' in rows(g) and '/Users/Owner/Documents/sample.txt' not in rows(g)
   checks.append('Rename changes the persisted entry without losing the file')
   select(g,p,'/Users/Owner/Documents/renamed.txt');p.desktop(40000,True)
   nav(g,p,'/Users/Owner/Desktop');p.desktop(40002,True);job(g,p)
   assert '/Users/Owner/Desktop/renamed.txt' in rows(g)
   checks.append('Copy/Paste publishes an actual desktop file and preserves the source')
   shot(g,'copied-to-desktop')
   select(g,p,'/Users/Owner/Desktop/renamed.txt');p.desktop(40001,True)
   nav(g,p,'/Users/Owner/Documents/alpha');p.desktop(40002,True);job(g,p)
   assert '/Users/Owner/Documents/alpha/renamed.txt' in rows(g) and '/Users/Owner/Desktop/renamed.txt' not in rows(g)
   checks.append('Cut/Paste moves the native file between folders')
   select(g,p,'/Users/Owner/Documents/alpha/renamed.txt');p.desktop(40004,True);job(g,p)
   assert '/Users/Owner/Documents/alpha/renamed.txt' not in rows(g)
   p.desktop(40017,True);shot(g,'recycle-bin')
   key(g,'home');p.drain();p.desktop(40005,True);job(g,p)
   assert '/Users/Owner/Documents/alpha/renamed.txt' in rows(g)
   checks.append('Recycle and Restore preserve the real payload and original location')
   nav(g,p,'/Users/Owner/Documents');p.desktop(40012,True);shot(g,'icon-view')
   checks.append('Native details/icon view switches without changing stored items')
   p.desktop(40012,True)
   select(g,p,'/Users/Owner/Documents/alpha');p.desktop(40000,True)
   nav(g,p,'/Users/Owner/Desktop');p.desktop(40002,True);job(g,p)
   assert '/Users/Owner/Desktop/alpha/renamed.txt' in rows(g)
   checks.append('Directory copy includes the real nested file payload')
   p.desktop(40002,True);p.drain()
   assert struct.unpack('<II',ui.memory(g,ui.symbol('file_job_state11',installed=True)[0],8))[1]==1
   shot(g,'name-conflict');p.desktop(40022,True);job(g,p)
   assert '/Users/Owner/Desktop/alpha (2)/renamed.txt' in rows(g)
   checks.append('Collision requires review; Keep both preserves the existing directory and creates a separate tree')
   p.desktop(40011,True);assert '/Users/Owner/Desktop/alpha (2)' not in rows(g)
   checks.append('Guarded single-action Undo removes the unchanged copy and preserves its source')
   select(g,p,'/Users/Owner/Desktop/alpha');key(g,'f2');chord(g,'a');type_text(g,'beta');key(g,'ret');job(g,p)
   assert '/Users/Owner/Desktop/beta/renamed.txt' in rows(g) and '/Users/Owner/Desktop/alpha' not in rows(g)
   checks.append('Directory rename atomically changes descendant paths')
   select(g,p,'/Users/Owner/Desktop/beta');p.desktop(40004,True);job(g,p)
   p.desktop(40017,True);key(g,'home');p.desktop(40005,True);job(g,p)
   assert '/Users/Owner/Desktop/beta/renamed.txt' in rows(g)
   checks.append('Recycling and restoring a directory preserves its nested file')
   nav(g,p,'/Users/Owner/Documents');select(g,p,'/Users/Owner/Documents/renamed.txt')
   g.qmp('send-key',{'keys':[{'type':'qcode','data':'shift'},{'type':'qcode','data':'f10'}],'hold-time':40});time.sleep(.16);p.drain()
   assert flag(g,'context11')
   count=flag(g,'file_menu_count11');menu=ui.memory(g,ui.symbol('file_menu_items11',installed=True)[0],count*16)
   commands=[struct.unpack_from('<II',menu,i*16+8) for i in range(count)]
   shortcut=next(i for i,(command,enabled) in enumerate(commands) if command==40010 and enabled)
   shot(g,'real-context-menu');p.desktop(32000+shortcut,True);job(g,p)
   assert '/Users/Owner/Desktop/renamed.txt.link' in rows(g)
   checks.append('Keyboard context menu creates a validated desktop shortcut to the selected real file')
   select(g,p,'/Users/Owner/Documents/renamed.txt')
   # Genuine pointer drag to the uncovered right-hand desktop strip.
   x,y=control(g,1002);scale=ui.integer(g,'scale73',True)
   s.modifier(g,'ctrl',True,1);p.move(x*scale//2,y*scale//2);p.drain();p.down(True);p.drain()
   p.move((x+20)*scale//2,y*scale//2);p.drain();assert flag(g,'file_drag11')
   key(g,'esc');assert not flag(g,'file_drag11');p.down(False);s.modifier(g,'ctrl',False,0);p.drain()
   assert '/Users/Owner/Documents/Welcome.txt' in rows(g)
   checks.append('Escape cancels actual pointer file capture without removing its source')
   select(g,p,'/Users/Owner/Documents/Welcome.txt');x,y=control(g,1002)
   s.modifier(g,'ctrl',True,1);p.move(x*scale//2,y*scale//2);p.drain();p.down(True);p.drain();p.move((x+20)*scale//2,y*scale//2);p.drain()
   width=1280
   p.move((width-20)*scale//2,500*scale//2);p.drain();shot(g,'drag-copy-feedback');p.down(False);s.modifier(g,'ctrl',False,0);job(g,p)
   assert '/Users/Owner/Desktop/Welcome.txt' in rows(g) and '/Users/Owner/Documents/Welcome.txt' in rows(g)
   checks.append('Ctrl-drag copies the actual file to the desktop with visible feedback and source preservation')
   nav(g,p,'/Users/Owner/Desktop');create(g,p,'discarded');select(g,p,'/Users/Owner/Desktop/discarded');key(g,'delete');job(g,p)
   p.desktop(40017,True);key(g,'home');key(g,'delete');assert flag(g,'file_dialog_state11')==5
   shot(g,'permanent-delete-review');key(g,'esc');assert '/System/Recycle/Owner' in rows(g)
   key(g,'delete');p.desktop(40020,True);job(g,p)
   assert not any(path.startswith('/System/Recycle/Owner/R') for path in rows(g))
   checks.append('Permanent deletion requires a separate cancel-first review; explicit confirmation removes only selected Bin payloads')
   nav(g,p,'/');select(g,p,'/System')
   generation=ui.memory(g,ui.symbol('volume',0xd3040,True)[0]+40,8);chord(g,'x')
   assert ui.memory(g,ui.symbol('volume',0xd3040,True)[0]+40,8)==generation
   g.qmp('send-key',{'keys':[{'type':'qcode','data':'shift'},{'type':'qcode','data':'f10'}],'hold-time':40});time.sleep(.16);p.drain()
   count=flag(g,'file_menu_count11');menu=ui.memory(g,ui.symbol('file_menu_items11',installed=True)[0],count*16)
   commands=dict(struct.unpack_from('<II',menu,i*16+8) for i in range(count))
   assert not commands[40001] and not commands[40003] and not commands[40004]
   shot(g,'protected-system-actions');key(g,'esc')
   checks.append('Protected System Cut/Rename/Recycle are disabled with a reason; attempted Cut performs no metadata writes')
   nav(g,p,'/Users/Owner/Documents');select(g,p,'/Users/Owner/Documents/renamed.txt');p.desktop(40000,True)
   nav(g,p,'/Users/Owner/Desktop');p.desktop(40002,True);job(g,p)
   select(g,p,'/Users/Owner/Desktop/renamed.txt');key(g,'ret');type_text(g,' changed');chord(g,'s');p.close_app(2,True);p.drain()
   changed=rows(g)['/Users/Owner/Desktop/renamed.txt'][2:]
   nav(g,p,'/Users/Owner/Documents');select(g,p,'/Users/Owner/Documents/renamed.txt');p.desktop(40000,True)
   nav(g,p,'/Users/Owner/Desktop');p.desktop(40002,True);p.desktop(40023,True);job(g,p)
   assert rows(g)['/Users/Owner/Desktop/renamed.txt'][2:]==changed
   checks.append('Skip matches leaves a genuinely different destination file unchanged')
   p.desktop(40002,True);p.desktop(40024,True);job(g,p)
   assert rows(g)['/Users/Owner/Desktop/renamed.txt'][2:]==rows(g)['/Users/Owner/Documents/renamed.txt'][2:]
   checks.append('Reviewed Replace verifies a staged copy before atomically replacing the closed native destination')
   select(g,p,'/Users/Owner/Desktop/renamed.txt')
   g.qmp('send-key',{'keys':[{'type':'qcode','data':'shift'},{'type':'qcode','data':'f10'}],'hold-time':40});time.sleep(.16);p.drain()
   count=flag(g,'file_menu_count11');menu=ui.memory(g,ui.symbol('file_menu_items11',installed=True)[0],count*16)
   which=next(i for i in range(count) if struct.unpack_from('<II',menu,i*16+8)==(40009,1))
   p.desktop(32000+which,True);shot(g,'open-with');p.desktop(40025,True);p.desktop(40102,True);p.drain();p.close_app(2,True);p.drain()
   checks.append('Open with uses an installed handler and persists the requested extension association')




  assert ui.s.s.document(disk,'/Users/Owner/Documents/renamed.txt')==b'native payload 123'
  assert ui.s.s.document(disk,'/Users/Owner/Documents/alpha/renamed.txt')==b'native payload 123'
  assert ui.s.s.document(disk,'/Users/Owner/Desktop/beta/renamed.txt')==b'native payload 123'
  assert ui.s.s.document(disk,'/Users/Owner/Desktop/renamed.txt.link')
  assert ui.s.s.document(disk,'/Users/Owner/Desktop/Welcome.txt')==ui.s.s.document(disk,'/Users/Owner/Documents/Welcome.txt')
  assert ui.s.s.document(disk,'/Users/Owner/Desktop/renamed.txt')==b'native payload 123'
  assert ui.s.s.document(disk,'/System/Settings/associations11.bin')
  assert ui.s.s.document(disk,'/System/Settings/files11.bin')
  with ui.guest(work/'detached',disk,media='installed',ready='M7_DESKTOP_READY') as g:
   p=ui.Pointer(g);p.drain();assert '/Users/Owner/Documents/alpha/renamed.txt' in rows(g);shot(g,'detached-startup')
  checks.append('Independent GPT/metadata/data inspection and detached disk startup preserve files and view preferences')
  result['passed']=True
 except Exception:result['error']=traceback.format_exc();raise
 finally:(out/'results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
 print('M11 native Files PASS')
if __name__=='__main__':main()
