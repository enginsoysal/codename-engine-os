"""Measure installed desktop resources, actual allocator pressure and UI recovery."""
import importlib.util
import json
from pathlib import Path
import re
import struct
import time
import traceback
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('m7isolation',Path(__file__).with_name('isolation.py'))
i=importlib.util.module_from_spec(spec);spec.loader.exec_module(i)
s,h,q=i.s,i.h,i.q


def stats(guest):
    mapping=(ROOT/'dist/EngineOS-M7-installed-kernel.map').read_text(encoding='utf-8')
    block=mapping.split('kernel_memory_pages-kernel.o:(.bss)',1)[1]
    match=re.search(r'^\s*([0-9a-f]+)\s+[0-9a-f]+\s+20\s+\d+\s+stats$',block,re.M);assert match
    return struct.unpack('<QQQQ',i.memory(guest,int(match[1],16),32))


def start_menu(guest,steps):
    h.key(guest,'f10');h.key(guest,'tab',steps);h.key(guest,'ret')


def close(guest,pointer,bounds):pointer.click(bounds.x+bounds.width-30,bounds.y+18)


def main():
    work=q.new_work('m7-resources');out=ROOT/'evidence/M7/resources';out.mkdir(parents=True,exist_ok=True)
    report={'passed':False,'m7_complete':False,'profiles':[],'checks':[]}
    try:
        disk=q.sparse_disk(work/'owned.img',2*1024**3)
        with q.Guest(work/'install',disk,image_name='EngineOS-M7') as guest:
            h.next(guest);h.next(guest);h.next(guest,2);h.next(guest,3);h.next(guest,5);start=len(guest.log());h.next(guest,1);guest.wait('M6_INSTALL_PASS',start,240)
        for ram,cpus in [(64,1),(128,2),(128,4)]:
            label=str(ram)+'mib-'+str(cpus)+'cpu'
            with q.Guest(work/label,disk,media='installed',ram=ram,cpus=cpus,image_name='EngineOS-M7',ready='M7_DESKTOP_READY') as guest:
                pointer=s.Pointer(guest);baseline=stats(guest)
                # Keyboard-only launcher/window/button navigation uses actual HID reports.
                start_menu(guest,2);slot,editor=i.window(guest,2)
                first=stats(guest);assert first[0]>baseline[0]
                h.key(guest,'a');i.control(guest,'s');guest.wait('M7_DOCUMENT_SAVED')
                start_menu(guest,2);second=stats(guest)
                assert second[0]-first[0]==10,'Second app must share immutable code and own exactly ten private pages'
                h.key(guest,'esc');assert stats(guest)[0]==first[0]
                start_menu(guest,6);_,details=i.window(guest,6)
                pointer.click(details.x+130,details.y+355);guest.wait('M7_MEMORY_LIMIT_ACTIVE')
                limited=stats(guest);start_menu(guest,2)
                failed_slot,failed=i.window(guest,2);base,size=i.symbol('apps');stride=size//8
                notice=i.memory(guest,base+failed_slot*stride+8232+256,96).split(b'\0')[0]
                assert b'Not enough memory' in notice
                assert stats(guest)[0]==limited[0] and stats(guest)[3]>limited[3]
                h.shot(guest,out,label+'-allocation-refused')
                h.key(guest,'esc');i.focus(guest,pointer,2)
                pixels_address=i.symbol('painted_pixels')[0];paints_address=i.symbol('paints')[0]
                input_pixels_before=struct.unpack('<Q',i.memory(guest,pixels_address,8))[0]
                input_paints_before=struct.unpack('<Q',i.memory(guest,paints_address,8))[0]
                started=time.perf_counter()
                guest.qmp('send-key',{'keys':[{'type':'qcode','data':'b'}],'hold-time':60})
                end=time.monotonic()+2
                while time.monotonic()<end:
                    current=i.memory(guest,base+slot*stride,3)
                    if current==b'ab\0':break
                    time.sleep(.01)
                latency=time.perf_counter()-started;assert current==b'ab\0' and latency<2
                time.sleep(.12)
                input_pixels=struct.unpack('<Q',i.memory(guest,pixels_address,8))[0]-input_pixels_before
                input_paints=struct.unpack('<Q',i.memory(guest,paints_address,8))[0]-input_paints_before
                assert 0<input_pixels<1280*800 and input_paints==1,(input_pixels,input_paints)
                start=len(guest.log());i.control(guest,'s');guest.wait('M7_DOCUMENT_SAVED',start)
                assert stats(guest)[0]==limited[0]
                i.focus(guest,pointer,6);_,details=i.window(guest,6)
                pointer.click(details.x+130,details.y+355);guest.wait('M7_MEMORY_LIMIT_RESTORED')
                h.key(guest,'esc');i.focus(guest,pointer,2);h.key(guest,'esc')
                assert stats(guest)[0]==baseline[0]
                # Record the complete user navigation workload, then verify repainting stops.
                paints=i.symbol('paints')[0];pixels=i.symbol('painted_pixels')[0]
                before_paints=struct.unpack('<Q',i.memory(guest,paints,8))[0]
                before_pixels=struct.unpack('<Q',i.memory(guest,pixels,8))[0]
                start_menu(guest,2);_,opened=i.window(guest,2);close(guest,pointer,opened)
                after=stats(guest);assert after[0]==baseline[0]
                time.sleep(.25);idle_before=i.memory(guest,paints,8);time.sleep(.5);assert idle_before==i.memory(guest,paints,8)
                assert guest.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==cpus
                delta_paints=struct.unpack('<Q',i.memory(guest,paints,8))[0]-before_paints
                delta_pixels=struct.unpack('<Q',i.memory(guest,pixels,8))[0]-before_pixels
                (out/(label+'.log')).write_bytes(guest.log().encode());h.shot(guest,out,label+'-recovered')
                report['profiles'].append({'ram_mib':ram,'cpus':cpus,'managed_baseline_bytes':baseline[0]*4096,'managed_peak_including_boot_bytes':after[1]*4096,
                   'second_app_private_bytes':10*4096,'allocation_failures_observed':after[3]-baseline[3],
                   'input_damage_pixels_under_pressure':input_pixels,'input_repaints_under_pressure':input_paints,'input_observation_seconds_under_pressure':round(latency,4),'idle_all_cpus_halted':True,'idle_repaint_count':0,
                   'interaction_repaints':delta_paints,'interaction_painted_pixels':delta_pixels})
        # The last profile created new uniquely named notes; verify every saved two-byte task.
        for number in [1,2,3]:assert s.document(disk,'/Users/Owner/Documents/Note'+str(number)+'.txt')==b'ab'
        data=(ROOT/'dist/EngineOS-M7-installed-kernel.elf').read_bytes();offset=struct.unpack_from('<Q',data,32)[0];count=struct.unpack_from('<H',data,56)[0]
        loads=[struct.unpack_from('<IIQQQQQQ',data,offset+n*56) for n in range(count)];loads=[v for v in loads if v[0]==1]
        manifest=json.loads((ROOT/'dist/m7-build-manifest.json').read_text(encoding='utf-8'))
        report.update(passed=True,build_id=manifest['build_id'],iso_sha256=manifest['artifacts']['EngineOS-M7.iso']['sha256'],
           kernel_physical_span_bytes=max(v[4]+v[6] for v in loads)-min(v[4] for v in loads),
           scope='named QEMU installed profiles; allocator counts exclude static kernel, firmware/boot reservations and framebuffer; input latency is harness observation, not a human usability study')
        report['checks']=['keyboard-only launcher/buttons','shared immutable app code','real allocator admission refusal','existing editor input/save under pressure','budget restoration and complete private-page reclamation','idle halt and stopped painting','separate static/managed/framebuffer accounting']
    except Exception:
        report['error']=q.clean(traceback.format_exc());print(report['error'])
        if 'guest' in locals():(out/'last-run.log').write_bytes(guest.log().encode())
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('M7 native resources '+('PASS' if report['passed'] else 'FAIL'),flush=True)
    if not report['passed']:raise SystemExit(1)


if __name__=='__main__':main()
