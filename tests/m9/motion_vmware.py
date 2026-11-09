"""Measure visible Files-window motion in a newly owned headless VMware VM."""
import argparse,hashlib,importlib.util,json,re,statistics,struct,subprocess,time,traceback,uuid
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('shared_m8_vmware',ROOT/'tests/m8/vmware.py')
shared=importlib.util.module_from_spec(spec);spec.loader.exec_module(shared)
v=shared.v

def pointer(display,x,y,buttons=0):
    display.socket.sendall(struct.pack('!BBHH',5,buttons,x,y))

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--image',choices=['M8','M9'],required=True);parser.add_argument('--frames',type=int,default=24);parser.add_argument('--relative',action='store_true')
    a=parser.parse_args();assert 8<=a.frames<=120
    work=ROOT/'build/m9/motion'/a.image/uuid.uuid4().hex;work.mkdir(parents=True)
    out=ROOT/'build/m9/motion-results'/(a.image+('-relative' if a.relative else ''));out.mkdir(parents=True,exist_ok=True)
    disk=work/'owned.img';assert not disk.exists() and disk.resolve().is_relative_to((ROOT/'build/m9').resolve())
    with disk.open('xb'):pass
    subprocess.run(['fsutil','sparse','setflag',str(disk)],check=True,capture_output=True)
    with disk.open('r+b') as file:file.truncate(2*1024**3)
    descriptor=work/'owned.vmdk';descriptor.write_bytes(b'version=1\nencoding="UTF-8"\nCID=fffffffe\nparentCID=ffffffff\ncreateType="monolithicFlat"\nRW 4194304 FLAT "owned.img" 0\nddb.adapterType = "ide"\n')
    meta=json.loads((ROOT/('dist/'+a.image.lower()+'-build-manifest.json')).read_text(encoding='utf-8'))
    iso=ROOT/('dist/EngineOS-'+a.image+'.iso');assert hashlib.sha256(iso.read_bytes()).hexdigest()==meta['artifacts'][iso.name]['sha256']
    before=v.old.v.inventory();result={'passed':False,'image':a.image,'build_id':meta['build_id'],'iso_sha256':meta['artifacts'][iso.name]['sha256'],'method':'host pointer event to first exact visible Files title patch; raw loopback RFB, same fixture procedure for baseline and candidate','human_motion_study':False}
    try:
        g=v.Guest(work/'same-vm',descriptor,'efi')
        cfg=g.vmx.read_text(encoding='utf-8').replace('EngineOS-M71.iso','EngineOS-'+a.image+'.iso')
        cfg='\n'.join(line for line in cfg.splitlines() if not line.startswith('bios.bootOrder'))+'\n'
        cfg=cfg.replace('ethernet0.present = "TRUE"','ethernet0.present = "FALSE"');g.vmx.write_bytes(cfg.encode())
        if a.relative:g.vmx.write_bytes((cfg+'vmmouse.present = "FALSE"\n').encode())
        with g:
            v.enter(g);v.enter(g);v.tab(g,3);v.enter(g);v.enter(g);v.enter(g);v.enter(g)
            g.wait('M6_INSTALL_PASS',timeout=240);g.wait('M73_VIEW step=0x0000000000000006')
            start=len(g.read());v.enter(g);g.wait('M7_DESKTOP_READY',start,180)
            g.rfb.close();g.rfb=None;display=g.connect_display();time.sleep(.5)
            display.shot(out/'before-drag.png');width,height=display.width,display.height
            scale=2 if width>=3840 and height>=2000 else 1.5 if width>=2560 and height>=1400 else 1
            logical_w,logical_h=int(width/scale),int(height/scale)
            fw=1080 if logical_w>1200 else logical_w-112;fh=720 if logical_h>900 else logical_h-180
            left,top=(logical_w-fw)//2,(logical_h-60-fh)//2
            patch=(int((left+12)*scale),int((top+8)*scale),int((left+120)*scale),int((top+32)*scale))
            initial=Image.open(out/'before-drag.png').copy();reference=initial.crop(patch);assert len(reference.getcolors(10000))>4
            grab_x,grab_y=int((left+220)*scale),int((top+18)*scale)
            pointer(display,grab_x,grab_y);time.sleep(.3);pointer(display,grab_x,grab_y,1);time.sleep(.4)
            latencies=[];previous_dx=previous_dy=0
            for index in range(a.frames):
                dx,dy=((24,20) if index%2==0 else (0,0))
                began=time.perf_counter();pointer(display,int(grab_x+dx*scale),int(grab_y+dy*scale),1)
                deadline=time.monotonic()+5
                while True:
                    display.shot(out/'latest-motion.png')
                    frame=Image.open(out/'latest-motion.png');area=tuple(int(value+(dx if n%2==0 else dy)*scale) for n,value in enumerate(patch))
                    leading_x=left+dx+(fw-3 if dx>previous_dx else 2)
                    old_edge_x=left+previous_dx+(2 if dx>previous_dx else fw-3)
                    row=top+previous_dy+350
                    expected_edge=initial.getpixel((int((left+(fw-3 if dx>previous_dx else 2))*scale),int((top+350)*scale)))
                    complete=(frame.crop(area).tobytes()==reference.tobytes()
                              and frame.getpixel((int(leading_x*scale),int((top+dy+350)*scale)))==expected_edge
                              and frame.getpixel((int(old_edge_x*scale),int(row*scale)))==initial.getpixel((0,int(row*scale))))
                    if complete:break
                    if time.monotonic()>deadline:raise AssertionError('Visible Files title did not reach the requested position')
                    time.sleep(.005)
                latencies.append((time.perf_counter()-began)*1000)
                result['completed_frames']=len(latencies);previous_dx,previous_dy=dx,dy
            pointer(display,int(grab_x+previous_dx*scale),int(grab_y+previous_dy*scale),0);time.sleep(.3);display.canvas75.save(out/'after-drag.png')
            result.update(framebuffer=[width,height],visible_frames=len(latencies),event_to_visible_ms=latencies,
                          median_ms=statistics.median(latencies),p95_ms=sorted(latencies)[int(.95*(len(latencies)-1))],max_ms=max(latencies))
            log=g.read();(out/'serial.log').write_bytes(log.encode())
            if a.image=='M9':
                g.wait('M9_DRAG_PROFILE');log=g.read();(out/'serial.log').write_bytes(log.encode())
                result['driver_fifo_active']='M9_VMWARE_SVGA_FIFO_ACTIVE' in log
                result['native_drag_profiles']=re.findall(r'M9_DRAG_PROFILE[^\n]+',log)
                assert 'M9_SVGA_QUARANTINED' not in log
            result['passed']=True
    except Exception:
        result['error']=v.old.v.redact(traceback.format_exc())
        if 'g' in locals():(out/'serial.log').write_bytes(g.output.encode())
        raise
    finally:
        deadline=time.monotonic()+10
        while v.old.v.inventory()!=before and time.monotonic()<deadline:time.sleep(.25)
        result['existing_vm_inventory_unchanged']=v.old.v.inventory()==before
        result['passed']=result['passed'] and result['existing_vm_inventory_unchanged']
        (out/'results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode())
        print(a.image+' visible motion '+('PASS' if result['passed'] else 'FAIL'),flush=True)

if __name__=='__main__':main()
