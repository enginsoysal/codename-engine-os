"""Compare actual captured cached movement to launcher-forced full native repaint."""
import hashlib,json,os,statistics,struct,time,traceback
from pathlib import Path
from PIL import Image
import windows as w
ui=w.ui;f=w.f;ROOT=ui.ROOT
def main():
 base=Path(os.environ['LOCALAPPDATA'])/'EngineOS/Tests/M6/m11-windows';owned=max(base.iterdir(),key=lambda p:p.stat().st_mtime_ns);disk=owned/'owned.img'
 assert disk.resolve().is_relative_to(base.resolve()) and disk.is_file()
 native=json.loads((ROOT/'build/m11/windows/results.json').read_text());assert native['passed']
 out=ROOT/'build/m11/windows/motion';out.mkdir(parents=True,exist_ok=True);f.out=out
 report={'passed':False,'iso_sha256':hashlib.sha256((ROOT/'dist/EngacerOS-M11STAGE4.iso').read_bytes()).hexdigest(),'profiles':[]}
 assert report['iso_sha256']==native['iso_sha256']
 work=ui.q.new_work('m11-window-motion')
 try:
  for resolution,scale in [((1280,800),2),((1920,1200),3),((3840,2160),4)]:
   label=str(resolution[0])+'x'+str(resolution[1]);folder=work/label
   with ui.guest(folder,disk,media='installed',ready='M7_DESKTOP_READY',resolution=resolution,ps2_only=resolution==(1280,800)) as g:
    p=ui.Pointer(g);p.drain()
    if scale==3:
     w.launch(g,p,3);p.desktop(112,True);p.close_app(3,True);p.drain()
    assert f.flag(g,'scale73')==scale
    model=ui.symbol('model',installed=True)[0];width,height=struct.unpack('<2I',ui.memory(g,model+240,8))
    before=struct.unpack('<4Q',ui.memory(g,ui.symbol('stats',32,True)[0],32));timings=[];comparisons=[]
    for i,(nx,ny) in enumerate([(20,160),(width-20,160),(width//2,height-90),(width//2,200)]):
     x,y,ww,hh=w.geometry(g);gx=x+180;gy=y+18
     p.move(gx*scale//2,gy*scale//2);p.drain();p.down(True);p.drain();started=time.perf_counter();p.move(nx*scale//2,ny*scale//2);p.drain();timings.append((time.perf_counter()-started)*1000)
     assert f.flag(g,'dragging') and not f.flag(g,'window_snap_preview11')
     f.shot(g,label+'-cached-'+str(i));bounds=w.geometry(g)
     if i==0:assert bounds[0]<0,bounds
     if i==1:assert bounds[0]+bounds[2]>width,bounds
     if i==2:assert bounds[1]+bounds[3]>height-60,bounds
     p.down(False);p.drain();f.key(g,'f10');assert f.flag(g,'launcher');f.key(g,'esc');assert not f.flag(g,'launcher');f.shot(g,label+'-full-'+str(i))
     with Image.open(out/(label+'-cached-'+str(i)+'.png')) as cached,Image.open(out/(label+'-full-'+str(i)+'.png')) as full:
      crop=(0,0,resolution[0],(height-60)*scale//2)
      assert cached.crop(crop).tobytes()==full.crop(crop).tobytes(),('Cached/full mismatch',label,i,bounds)
     comparisons.append({'bounds':list(bounds),'cached_equals_full_viewport':True})
    after=struct.unpack('<4Q',ui.memory(g,ui.symbol('stats',32,True)[0],32));assert after[0]==before[0] and after[3]==before[3]
    paints=ui.memory(g,ui.symbol('paints',8,True)[0],8);time.sleep(.6);assert paints==ui.memory(g,ui.symbol('paints',8,True)[0],8)
    assert g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==len(g.qmp('query-cpus-fast'))
    graphics=struct.unpack('<8Q4I',ui.memory(g,ui.symbol('graphics9',80,True)[0],80))
    assert graphics[4]>0,'No cached copies were actually used'
    report['profiles'].append({'physical':list(resolution),'scale_percent':scale*50,'comparisons':comparisons,'managed_bytes_before':before[0]*4096,'managed_bytes_after':after[0]*4096,'managed_peak_bytes':after[1]*4096,'cached_copy_frames':graphics[4],'event_to_drain_ms':timings,'median_ms':statistics.median(timings),'measurement':'Host event to completed native composition including QMP observation, not display refresh/FPS','all_cpus_halted_when_idle':True,'idle_paints_unchanged':True})
    (out/(label+'.log')).write_bytes(g.log().encode())
  report['passed']=True
 except Exception:report['error']=ui.q.clean(traceback.format_exc());raise
 finally:(out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report,indent=2))
if __name__=='__main__':main()
