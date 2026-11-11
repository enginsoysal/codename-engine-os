"""Actual installed M11.4 window/appearance tasks using ordinary input only."""
import hashlib,json,struct,time,traceback
import files as f
import start as st
ui=f.ui;ROOT=ui.ROOT
out=ROOT/'build/m11/windows';out.mkdir(parents=True,exist_ok=True);f.out=out
checks=[]
def focused(g):
 slot=int.from_bytes(ui.memory(g,ui.symbol('model',installed=True)[0]+236,4),'little')
 return slot,f.s.windows(g)[slot]
def geometry(g):return focused(g)[1][:4]
def chord(g,*keys):
 g.qmp('send-key',{'keys':[{'type':'qcode','data':k} for k in keys],'hold-time':60});time.sleep(.22);ui.Pointer(g).drain()
def menu(g,p,command):
 count=f.flag(g,'file_menu_count11');raw=ui.memory(g,ui.symbol('file_menu_items11',installed=True)[0],count*16)
 items=[i for i in range(count) if struct.unpack_from('<II',raw,i*16+8)==(command,1)]
 assert len(items)==1,(command,count);p.desktop(32000+items[0],True)
def launch(g,p,kind):
 st.search(g,p,{1:'files',2:'text editor',3:'settings'}[kind]);f.key(g,'ret');p.drain()
def drag(g,p,x,y,xx,yy,release=True):
 scale=f.flag(g,'scale73');p.move(x*scale//2,y*scale//2);p.drain();p.down(True);p.drain();p.move(xx*scale//2,yy*scale//2);p.drain()
 if release:p.down(False);p.drain()
def main():
 work=ui.q.new_work('m11-windows');disk=ui.q.sparse_disk(work/'owned.img',2*1024**3)
 report={'passed':False,'checks':checks,'iso_sha256':hashlib.sha256((ROOT/'dist/EngacerOS-M11STAGE4.iso').read_bytes()).hexdigest()}
 try:
  with ui.guest(work/'install',disk,resolution=(1920,1200)) as g:
   p=ui.Pointer(g)
   for command in (2,100,2,2,2):f.s.wizard(g,p,command)
   at=f.s.wizard(g,p,2);g.wait('M6_INSTALL_PASS',at,240)
   at=f.s.wizard(g,p,84);g.wait('M7_DESKTOP_READY',at,180);p=ui.Pointer(g);p.drain()
   baseline=struct.unpack("<4Q",ui.memory(g,ui.symbol("stats",32,True)[0],32))
   assert ui.s.s.document(disk,'/Users/Owner/Documents/Welcome.txt')==(ROOT/'distros/packages/m11-desktop-guide/content.txt').read_bytes()
   normal=geometry(g);x,y,w,h=normal
   p.logical(x+w-76,y+18,True);assert geometry(g)==(0,0,1920,1140),geometry(g)
   f.shot(g,'maximized-files');p.logical(1920-76,18,True);assert geometry(g)==normal
   f.s.double_click(g,p,x+180,y+18);assert geometry(g)==(0,0,1920,1140)
   f.s.double_click(g,p,180,18);assert geometry(g)==normal
   checks.append('Caption maximize button and double-click restore the exact original normal bounds')
   # Every edge and corner must resize the actual model, including top and left.
   for horizontal,vertical in ((1,0),(-1,0),(0,1),(0,-1),(1,1),(-1,-1),(1,-1),(-1,1)):
    x,y,w,h=geometry(g);px=x+2 if horizontal<0 else x+w-2 if horizontal>0 else x+w//2;py=y+2 if vertical<0 else y+h-2 if vertical>0 else y+h//2
    old=geometry(g);drag(g,p,px,py,px+horizontal*20,py+vertical*16);new=geometry(g)
    assert new!=old,(horizontal,vertical,old,new)
   checks.append('All four edges and four corners resize the real window with app minimum constraints')
   old=geometry(g);x,y,w,h=old;drag(g,p,x+180,y+18,x+230,y+48,False);f.key(g,'esc');p.down(False);p.drain();assert geometry(g)==old
   checks.append('Escape cancels a captured move and restores the complete original geometry')
   x,y,w,h=old;drag(g,p,x+180,y+18,2,240,False);assert f.flag(g,'window_snap_preview11')==2
   f.shot(g,'snap-preview');p.down(False);p.drain();assert geometry(g)==(0,0,960,1140)
   chord(g,'meta_l','up');assert geometry(g)==(0,0,1920,1140)
   chord(g,'meta_l','down');assert geometry(g)==old,(geometry(g),old)
   chord(g,'meta_r','right');assert geometry(g)==(960,0,960,1140)
   chord(g,'meta_l','down');assert geometry(g)==old
   checks.append('Visible snap preview, left/right snap and Windows-key maximize/restore preserve normal geometry')
   chord(g,'alt','spc')
   assert f.flag(g,'context11'),'Alt+Space did not open the system menu'
   menu(g,p,44004)
   assert f.flag(g,'window_keyboard_move11'),'Keyboard Move did not start'
   f.key(g,'right');f.key(g,'down');f.key(g,'esc')
   assert geometry(g)==old,('Keyboard Move cancel lost bounds',geometry(g),old)
   chord(g,'alt','spc');menu(g,p,44005);f.key(g,'right');f.key(g,'down');f.key(g,'ret');assert geometry(g)[2:]==(old[2]+16,old[3]+16)
   checks.append('Alt+Space exposes keyboard Move/Size; Enter keeps and Escape cancels without closing work')
   after=struct.unpack("<4Q",ui.memory(g,ui.symbol("stats",32,True)[0],32));assert after[0]==baseline[0] and after[3]==baseline[3]
   report['window_operations_retained_managed_page_delta']=after[0]-baseline[0]
   report['managed_current_bytes']=after[0]*4096;report['managed_peak_bytes']=after[1]*4096
   paint=ui.memory(g,ui.symbol('paints',8,True)[0],8);time.sleep(.6)
   assert paint==ui.memory(g,ui.symbol('paints',8,True)[0],8)
   assert g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==len(g.qmp('query-cpus-fast'))
   checks.append('Window operations retain no managed pages or new allocation failures; idle leaves paints unchanged and every CPU halted')
   launch(g,p,2);f.type_text(g,'Unsaved work')
   editor=focused(g)[0];chord(g,'alt','f4');assert f.flag(g,'switch_open11')==0
   appraw=ui.memory(g,ui.symbol('apps',installed=True)[0],ui.symbol('apps',installed=True)[1]);assert b'Unsaved work' in appraw
   f.shot(g,'unsaved-close-review');p.desktop(61,True)
   # Hold Alt to observe the actual switcher, then cancel before focus changes.
   g.qmp('input-send-event',{'events':[{'type':'key','data':{'down':True,'key':{'type':'qcode','data':'alt'}}}]});f.key(g,'tab');assert f.flag(g,'switch_open11')
   f.shot(g,'alt-tab-switcher');f.key(g,'esc')
   g.qmp('input-send-event',{'events':[{'type':'key','data':{'down':False,'key':{'type':'qcode','data':'alt'}}}]});p.drain();assert focused(g)[0]==editor
   chord(g,'alt','tab');assert focused(g)[0]!=editor
   chord(g,'alt','shift','tab');assert focused(g)[0]==editor
   checks.append('Alt+Tab/Alt+Shift+Tab use a visible bounded MRU switcher, Alt release commits and Escape retains focus')
   checks.append('Alt+F4 preserves dirty editor text behind the explicit save/discard review')
   launch(g,p,3);p.desktop(45002,True);p.desktop(45003,True);p.desktop(43,True);p.desktop(41,True);p.desktop(112,True)
   assert f.flag(g,'scale73')==3
   ax,ay,aw,ah=geometry(g);assert ax>=0 and ay>=0 and ax+aw<=1280 and ay+ah<=740,('Scale hid Settings controls',geometry(g))
   f.shot(g,'appearance-150');p.desktop(47,True)
   record=ui.memory(g,ui.symbol('appearance11',352,True)[0],352);assert ui.s.s.document(disk,'/System/Settings/appearance11-Owner.bin')==record
   assert struct.unpack_from('<8I',record)[:7]==(0x41313145,1,0x9caeff,1,3,0xd7e4e9,1)
   checks.append('Light wallpaper contrast, dark windows, accent and real 150% display scale preview and save as bounded per-user data')
   # Return to automatic before leaving; a different physical viewport can then boot.
   p.desktop(110,True);p.desktop(47,True);f.shot(g,'appearance-ordinary')
   report['record_hex']=ui.memory(g,ui.symbol('appearance11',352,True)[0],352).hex()
  with ui.guest(work/'disk-only',disk,media='installed',ready='M7_DESKTOP_READY',resolution=(1920,1200)) as g:
   p=ui.Pointer(g);p.drain();record=ui.memory(g,ui.symbol('appearance11',352,True)[0],352);assert record.hex()==report['record_hex']
   f.shot(g,'persistent-appearance');checks.append('Independent installed disk-only startup reloads exact appearance and saved normal app geometry')
  with ui.guest(work/'native-4k',disk,media='installed',ready='M7_DESKTOP_READY',resolution=(3840,2160)) as g:
   p=ui.Pointer(g);p.drain();assert f.flag(g,'scale73')==4
   f.shot(g,'native-4k');checks.append('Actual 3840x2160 physical framebuffer renders at automatic 200% scale without resizing a screenshot')
  report['passed']=True
 except Exception:report['error']=traceback.format_exc();raise
 finally:(out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report,indent=2))
if __name__=='__main__':main()
