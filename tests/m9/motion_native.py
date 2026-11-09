"""Ordinary input latency and cached-versus-full frame checks on owned QEMU disks."""
import argparse,hashlib,importlib.util,json,os,statistics,struct,time,traceback
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[2]

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--image',choices=['M8','M9'],required=True);parser.add_argument('--resolution',default='1280x800');parser.add_argument('--frames',type=int,default=24);parser.add_argument('--disk',type=Path)
    args=parser.parse_args();assert 8<=args.frames<=120
    os.environ['ENGINE74_IMAGE']='EngineOS-'+args.image
    spec=importlib.util.spec_from_file_location('motion_ui',ROOT/('tests/m9/ui.py' if args.image=='M9' else 'tests/m74/ui.py'));ui=importlib.util.module_from_spec(spec);spec.loader.exec_module(ui)
    out=ROOT/'build/m9/native-motion'/(args.image+'-'+args.resolution);out.mkdir(parents=True,exist_ok=True)
    work=ui.q.new_work('m9-motion-'+args.image.lower());disk=args.disk.resolve() if args.disk else ui.q.sparse_disk(work/'owned.img',2*1024**3)
    disk=disk.resolve()
    assert disk.is_relative_to(ui.q.new_work.__globals__['BASE'].resolve()) and disk.name=='owned.img' and disk.is_file()
    meta=json.loads((ROOT/('dist/'+args.image.lower()+'-build-manifest.json')).read_text(encoding='utf-8'))
    result={'passed':False,'image':args.image,'build_id':meta['build_id'],'iso_sha256':meta['artifacts']['EngineOS-'+args.image+'.iso']['sha256'],
            'measurement':'host relative-pointer event to completed native composition; includes QMP observation overhead; not monitor refresh rate'}
    def wait(g,predicate,timeout=60):
        deadline=time.monotonic()+timeout
        while not predicate():
            if time.monotonic()>deadline:raise TimeoutError('Native composition did not complete')
            time.sleep(.001)
    def shot(g,name):
        wait(g,lambda:g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==2)
        g.qmp('stop')
        try:
            boot=ui.memory(g,ui.symbol('view73',160,True)[0],160)
            address=struct.unpack_from('<Q',boot,80)[0];stride,fmt,w,h=struct.unpack_from('<4I',boot,136);scale=ui.integer(g,'scale73',True);w=w*scale//2;h=h*scale//2
            path=g.folder/(name+'.bin');g.qmp('human-monitor-command',{'command-line':f'pmemsave 0x{address:x} 0x{stride*h:x} "{path.as_posix()}"'})
            raw=path.read_bytes();picture=Image.frombytes('RGB',(stride//4,h),raw,'raw','BGRX' if fmt==2 else 'RGBX').crop((0,0,w,h));picture.save(out/(name+'.png'))
            return picture
        finally:g.qmp('cont')
    try:
        guest=ui.guest(work/'install',disk,tuple(map(int,args.resolution.split('x'))),ram=128,media='installed' if args.disk else 'iso',ready='M7_DESKTOP_READY' if args.disk else 'M6_WIZARD_READY')
        guest.args=[value for value in guest.args if value!='-no-reboot']
        if args.disk:ui.s.normal_boot(guest)
        with guest as g:
            p=ui.Pointer(g)
            if not args.disk:
                for action in [2,100,2,2,2,2]:p.wizard(action)
                g.wait('M6_INSTALL_PASS',timeout=240);ui.wait_view(g,6)
                count=ui.integer(g,'controls');raw=ui.memory(g,ui.symbol('buttons',480)[0],count*20)
                button=next(struct.unpack_from('<5I',raw,index*20) for index in range(count) if struct.unpack_from('<I',raw,index*20+16)[0]==84)
                start=len(g.log());p.logical(button[0]+button[2]//2,button[1]+button[3]//2);g.wait('M7_DESKTOP_READY',start,180)
            assert 'ENGINE_OS_KERNEL_ENTRY build='+meta['build_id'] in g.log()
            p=ui.Pointer(g);model=ui.symbol('model',installed=True)[0]
            rect=struct.unpack('<4i',ui.memory(g,model,16));scale=ui.integer(g,'scale73',True)
            grab=(rect[0]+220,rect[1]+18);p.logical(*grab,True)
            wait(g,lambda:ui.integer(g,'previous_buttons',True)==0 and ui.integer(g,'pointer_x',True)==grab[0] and ui.integer(g,'pointer_y',True)==grab[1])
            p.down(True)
            wait(g,lambda:ui.memory(g,ui.symbol('dragging',installed=True)[0],1)[0]==1)
            pages_before=struct.unpack('<4Q',ui.memory(g,ui.symbol('stats',32,True)[0],32))
            timings=[];x,y=grab
            for index in range(args.frames):
                nx,ny=(grab[0]+24,grab[1]+20) if index%2==0 else grab
                started=time.perf_counter()
                g.qmp('input-send-event',{'events':[{'type':'rel','data':{'axis':'x','value':(nx-x)*scale//2}},{'type':'rel','data':{'axis':'y','value':(ny-y)*scale//2}}]})
                wait(g,lambda:ui.integer(g,'pointer_x',True)==nx and ui.integer(g,'pointer_y',True)==ny)
                timings.append((time.perf_counter()-started)*1000);x,y=nx,ny;p.x=x*scale//2;p.y=y*scale//2
                moved=struct.unpack('<4i',ui.memory(g,model,16))
                assert moved[0]==rect[0]+nx-grab[0] and moved[1]==rect[1]+ny-grab[1],(rect,moved,nx,ny)
            p.down(False);wait(g,lambda:ui.memory(g,ui.symbol('dragging',installed=True)[0],1)[0]==0)
            cached=shot(g,'moved-frame')
            painted=ui.memory(g,ui.symbol('paints',8,True)[0],8)
            ui.h.key(g,'f10');wait(g,lambda:ui.memory(g,ui.symbol('launcher',installed=True)[0],1)[0]==1 and ui.memory(g,ui.symbol('paints',8,True)[0],8)!=painted)
            painted=ui.memory(g,ui.symbol('paints',8,True)[0],8)
            ui.h.key(g,'esc');wait(g,lambda:ui.memory(g,ui.symbol('launcher',installed=True)[0],1)[0]==0 and ui.memory(g,ui.symbol('paints',8,True)[0],8)!=painted)
            exact=shot(g,'full-repaint-frame')
            rect=struct.unpack('<4i',ui.memory(g,model,16));left,top,w,h=rect
            crop=tuple(max(0,value*scale//2) for value in [left-12,top-12,left+w+12,top+h+12])
            assert cached.crop(crop).tobytes()==exact.crop(crop).tobytes(),'Cached window differs from a complete native repaint'
            result.update(passed=True,framebuffer=list(cached.size),frames=len(timings),event_to_composed_ms=timings,median_ms=statistics.median(timings),p95_ms=sorted(timings)[int(.95*(len(timings)-1))],max_ms=max(timings),cached_matches_full_repaint=True)
            if args.image=='M9':
                values=struct.unpack('<8Q4I',ui.memory(g,ui.symbol('graphics9',80,True)[0],80))
                result['graphics']={key:value for key,value in zip(['shadow_bytes','presented_pixels','presents','copy_pixels','copy_frames','fifo_updates','fifo_copies','fifo_failures','adapter_bdf','adapter_id','backend','capabilities'],values)}
                assert result['graphics']['copy_frames']>=args.frames-2
                before=ui.memory(g,ui.symbol('paints',8,True)[0],8);time.sleep(.3);assert ui.memory(g,ui.symbol('paints',8,True)[0],8)==before
                assert g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==2
            idle_before=ui.memory(g,ui.symbol('paints',8,True)[0],8);time.sleep(.3)
            assert ui.memory(g,ui.symbol('paints',8,True)[0],8)==idle_before
            assert g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==2
            pages_after=struct.unpack('<4Q',ui.memory(g,ui.symbol('stats',32,True)[0],32))
            assert pages_after[0]==pages_before[0],'Dragging retained managed pages'
            result['managed_bytes_before_drag']=pages_before[0]*4096
            result['managed_bytes_after_drag']=pages_after[0]*4096
            result['idle_processors_halted']=True
            result['idle_repaint_unchanged']=True
            if args.image=='M9':
                profile=struct.unpack('<19Q',ui.memory(g,ui.symbol('drag9',152,True)[0],152))
                result['native_drag_profile']={'frames':profile[0],'ticks_10ms':profile[1],'longest_ticks_10ms':profile[2],'bins_ticks_0_to_15_or_more':list(profile[3:])}
            (out/'serial.log').write_bytes(g.log().encode())
    except Exception:
        result['passed']=False
        result['error']=ui.q.clean(traceback.format_exc())
        if 'g' in locals():(out/'serial.log').write_bytes(g.log().encode())
        raise
    finally:
        (out/'results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print(args.image+' native motion '+('PASS' if result['passed'] else 'FAIL'),flush=True)

if __name__=='__main__':main()
