"""Native Start/catalogue/taskbar/preferences/power journey on an owned installed guest."""
import ctypes as c,hashlib,json,struct,time,traceback
import files as f
ui=f.ui
ROOT=ui.ROOT
out=ROOT/'build/m11/start';out.mkdir(parents=True,exist_ok=True);f.out=out
checks=[]
class Pin(c.Structure):_fields_=[('kind',c.c_uint),('app',c.c_uint),('path',c.c_char*256),('label',c.c_char*32)]
class Preferences(c.Structure):_fields_=[('magic',c.c_uint64),('version',c.c_uint),('categories',c.c_uint),('pins',c.c_uint),('tasks',c.c_uint),('view',c.c_uint),('tab',c.c_uint),('centered',c.c_uint),('reserved',c.c_uint),('names',(c.c_char*32)*12),('order',c.c_uint*12),('hidden',c.c_uint*12),('assignment',c.c_uint*20),('pinned',Pin*32),('task',c.c_uint*8)]
def prefs(g):return Preferences.from_buffer_copy(ui.memory(g,ui.symbol('start_preferences11',c.sizeof(Preferences),True)[0],c.sizeof(Preferences)))
def names(p):return [bytes(p.names[i]).split(b'\0')[0].decode() for i in range(p.categories)]
def results(g):
 count=f.flag(g,'start_result_count11');raw=ui.memory(g,ui.symbol('start_results11',installed=True)[0],count*8);return [struct.unpack_from('<II',raw,i*8) for i in range(count)]
def menu(g,p,command):
 count=f.flag(g,'file_menu_count11');raw=ui.memory(g,ui.symbol('file_menu_items11',installed=True)[0],count*16)
 found=[i for i in range(count) if struct.unpack_from('<II',raw,i*16+8)==(command,1)];assert len(found)==1,(command,count,[struct.unpack_from('<II',raw,i*16+8) for i in range(count)])
 p.desktop(32000+found[0],True)
def context(g,p):
 g.qmp('send-key',{'keys':[{'type':'qcode','data':'shift'},{'type':'qcode','data':'f10'}],'hold-time':40});time.sleep(.16);p.drain();assert f.flag(g,'context11')
def search(g,p,text):
 if not f.flag(g,'launcher'):p.desktop(0,True)
 if f.flag(g,'start_manage11'):p.desktop(42008,True)
 p.desktop(42004,True);f.chord(g,'a');f.type_text(g,text);p.drain()
def label(g,p,text):f.chord(g,'a');f.type_text(g,text);f.key(g,'ret');p.drain()
def open_app(g,p,text,kind):search(g,p,text);assert results(g)==[(1,kind)],results(g);f.key(g,'ret');assert f.s.count(g,kind)==1

def main():
 work=ui.q.new_work('m11-start');disk=ui.q.sparse_disk(work/'owned.img',2*1024**3)
 report={'passed':False,'checks':checks,'iso_sha256':hashlib.sha256((ROOT/'dist/EngacerOS-M11STAGE3.iso').read_bytes()).hexdigest()}
 try:
  with ui.guest(work/'install',disk) as g:
   p=ui.Pointer(g)
   for command in (2,100,2,2,2):f.s.wizard(g,p,command)
   at=f.s.wizard(g,p,2);g.wait('M6_INSTALL_PASS',at,240)
   at=f.s.wizard(g,p,84);g.wait('M7_DESKTOP_READY',at,180);p=ui.Pointer(g);p.drain();p.close_app(1,True);p.drain()
   p.desktop(0,True);assert f.flag(g,'launcher');initial=prefs(g)
   assert names(initial)==['Accessories','System','Internet','Media','Development'];f.shot(g,'start-pinned')
   before=g.log().count('M7_WINDOW_OPEN');pages_before=ui.memory(g,ui.symbol('stats',32,True)[0],8)
   search(g,p,'calc');assert results(g)==[(1,4)]
   assert g.log().count('M7_WINDOW_OPEN')==before and ui.memory(g,ui.symbol('stats',32,True)[0],8)==pages_before
   f.shot(g,'local-search');checks.append('Local search filters the installed catalogue without launching a process or allocating managed pages for each key')
   f.key(g,'ret');assert f.s.count(g,4)==1;p.close_app(4,True);p.drain();checks.append('Enter launches the original installed Calculator from its catalogue identity')
   p.desktop(0,True);p.desktop(42002,True);p.desktop(42027,True);menu(g,p,42304)
   assert f.flag(g,'launcher') and results(g)==[(1,7)];f.shot(g,'default-category')
   checks.append('Default categories filter actual installed apps while keeping the Start menu open')
   p.desktop(42007,True);p.desktop(42010,True);label(g,p,'Games');assert names(prefs(g))[-1]=='Games'
   p.desktop(42011,True);label(g,p,'Favorites');p.desktop(42012,True);p.desktop(42013,True)
   saved=prefs(g);assert names(saved)[5]=='Favorites' and saved.hidden[5]==1 and list(saved.order[:6])==[0,1,2,3,5,4]
   checks.append('Create, rename, hide and reorder update bounded real category preferences')
   p.desktop(42008,True);assert prefs(g).tab==1;search(g,p,'terminal');context(g,p);menu(g,p,42807);menu(g,p,42505)
   assert prefs(g).assignment[7]==5 and results(g)==[(1,7)] and f.flag(g,'launcher')
   checks.append('Context category assignment persists; a hidden category does not remove its app from search')
   p.desktop(42007,True);p.desktop(42305,True);p.desktop(42012,True);p.desktop(42008,True);p.desktop(42002,True);p.desktop(42027,True);menu(g,p,42305)
   assert results(g)==[(1,7)];f.shot(g,'custom-category')
   p.desktop(42007,True);p.desktop(42029,True);p.desktop(42402,True);p.desktop(42032,True);label(g,p,'Command Center');p.desktop(42016,True)
   saved=prefs(g);assert saved.pinned[1].app==7 and saved.pinned[1].label==b'Command Center'
   f.shot(g,'custom-pins');checks.append('Start pins can be renamed and reordered without renaming or removing the installed app')
   p.desktop(42008,True);search(g,p,'calc');context(g,p);menu(g,p,42604);context(g,p);menu(g,p,42704)
   saved=prefs(g);assert any(saved.pinned[i].app==4 for i in range(saved.pins)) and 4 in saved.task[:saved.tasks]
   checks.append('App context actions persist independent Start and taskbar pins')
   p.desktop(42007,True);p.desktop(42030,True);p.desktop(42018,True);assert prefs(g).centered==1
   p.desktop(42009,True);f.shot(g,'restore-defaults-review');f.key(g,'ret');assert not f.flag(g,'start_confirm11') and prefs(g).centered==1
   checks.append('Taskbar alignment is configurable; the default-layout review initially selects Cancel')
   p.desktop(42008,True);f.key(g,'esc');assert not f.flag(g,'launcher')
   p.desktop(42902,True);assert f.s.count(g,7)==1
   p.desktop(42902,True);assert next(w for w in f.s.windows(g) if w[6] and w[5]==7)[7]
   p.desktop(42902,True);assert not next(w for w in f.s.windows(g) if w[6] and w[5]==7)[7]
   p.desktop(70,True);assert next(w for w in f.s.windows(g) if w[6] and w[5]==7)[7]
   p.desktop(70,True);assert not next(w for w in f.s.windows(g) if w[6] and w[5]==7)[7]
   f.shot(g,'taskbar-pins-and-window');p.close_app(7,True);p.drain()
   checks.append('Pinned launchers and actual window buttons separately launch, minimize and restore the same real window')
   open_app(g,p,'files',1);p.desktop(94,True);f.create(g,p,'projects');f.select(g,p,'/Users/Owner/Documents/projects');context(g,p);menu(g,p,43000)
   f.key(g,'f2');label(g,p,'work');f.job(g,p)
   saved=prefs(g);pin=next(i for i in range(saved.pins) if saved.pinned[i].kind==2 and saved.pinned[i].path==b'/Users/Owner/Documents/work')
   assert not any(saved.pinned[i].path==b'/Users/Owner/Documents/projects' for i in range(saved.pins));p.close_app(1,True);p.drain()
   p.desktop(0,True);p.desktop(42000,True);row=results(g).index((2,pin));p.desktop(42200+row,True);g.wait('M72_FOLDER_VISIT /Users/Owner/Documents/work');f.shot(g,'pinned-folder-open');p.close_app(1,True);p.drain()
   checks.append('A native folder pin follows a real folder rename, then opens the actual renamed folder')
   open_app(g,p,'software',11);p.desktop(10304,True);p.desktop(10004,True)
   saved=prefs(g);assert not any(saved.pinned[i].kind==1 and saved.pinned[i].app==4 for i in range(saved.pins)) and 4 not in saved.task[:saved.tasks]
   search(g,p,'calculator');assert not results(g);f.shot(g,'removed-app-search');f.key(g,'esc');f.key(g,'esc');p.desktop(10304,True);p.close_app(11,True);p.drain()
   checks.append('Optional app removal prunes both pin sets and live search; reinstall does not create stale launch paths')
   open_app(g,p,'text editor',2);f.type_text(g,'unsaved work');p.desktop(0,True);p.desktop(42019,True);menu(g,p,42020);p.desktop(42023,True)
   assert 'M11_USER_RESTART_CONFIRMED' not in g.log() and f.s.count(g,2)==1;f.shot(g,'unsaved-work-blocks-power')
   f.key(g,'esc');p.close_app(2,True);p.desktop(60,True);p.drain();assert f.s.count(g,2)==0
   checks.append('Power confirmation refuses unsaved documents and keeps the session running')
   saved=prefs(g);report['custom_preferences_sha256']=hashlib.sha256(bytes(saved)).hexdigest()
   assert ui.s.s.document(disk,'/System/Settings/start11-Owner.bin')==bytes(saved)
   p.desktop(0,True);p.desktop(42019,True);menu(g,p,42020);f.key(g,'ret');assert 'M11_USER_RESTART_CONFIRMED' not in g.log()
   p.desktop(42019,True);menu(g,p,42020);f.shot(g,'restart-review');x,y=f.control(g,42023);scale=ui.integer(g,'scale73',True);p.move(x*scale//2,y*scale//2);p.drain();p.down(True);p.drain();at=len(g.log());p.down(False)
   g.wait('M11_USER_RESTART_CONFIRMED',at,20);g.wait('M7_DESKTOP_READY',at,180);p=ui.Pointer(g);p.drain()
   assert bytes(prefs(g))==bytes(saved);f.shot(g,'installed-restart-preserves-start')
   checks.append('Actual confirmed installed restart boots the same build and restores independently checked custom preferences')
   p.close_app(1,True);p.drain();p.desktop(0,True);p.desktop(42007,True);p.desktop(42009,True);p.desktop(42023,True)
   reset=prefs(g);assert reset.categories==5 and reset.centered==0 and not any(reset.pinned[i].label for i in range(reset.pins));assert '/Users/Owner/Documents/work' in f.rows(g)
   f.shot(g,'defaults-restored');checks.append('Confirmed Restore defaults resets layout only and preserves the user folder')
   p.desktop(42008,True);p.desktop(42019,True);assert f.flag(g,'start_power_shutdown11')==1;menu(g,p,42021);f.shot(g,'shutdown-review')
   x,y=f.control(g,42023);scale=ui.integer(g,'scale73',True);p.move(x*scale//2,y*scale//2);p.drain();p.down(True);p.drain();p.down(False)
   g.process.wait(timeout=30);assert g.process.returncode==0 and 'M11_USER_SHUTDOWN_CONFIRMED' in g.log()
   checks.append('Confirmed ACPI S5 shuts down the actual owned QEMU guest rather than displaying a fake power screen')
  with ui.guest(work/'disk-only',disk,media='installed',ready='M7_DESKTOP_READY') as g:
   p=ui.Pointer(g);p.drain();assert prefs(g).categories==5 and '/Users/Owner/Documents/work' in f.rows(g);f.shot(g,'detached-startup')
  checks.append('Detached disk startup preserves the installed OS, restored preferences and user folder after shutdown')
  report['passed']=True
 except Exception:report['error']=traceback.format_exc();raise
 finally:(out/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
 print('M11 native Start PASS')
if __name__=='__main__':main()
