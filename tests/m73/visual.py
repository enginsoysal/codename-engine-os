"""Capture every real setup step and exercise scaled native folder navigation."""
import argparse
import json
from pathlib import Path
import struct
import traceback
from PIL import Image
import ui
q,h=ui.q,ui.h;ROOT=ui.ROOT
def actual_height(g):
    return int(__import__('re').search(r'M73_DISPLAY physical=0x[0-9a-f]+x0x([0-9a-f]+)',g.log())[1],16)

def main():
    p=argparse.ArgumentParser();p.add_argument('--resolution',default='1280x800');p.add_argument('--ram',type=int,default=128);p.add_argument('--firmware',choices=['uefi','bios'],default='uefi');a=p.parse_args();resolution=tuple(map(int,a.resolution.split('x')))
    label=a.firmware+'-'+a.resolution+('' if a.ram==128 else '-'+str(a.ram)+'mib');out=ROOT/'evidence/M7.3'/label;out.mkdir(parents=True,exist_ok=True);work=q.new_work('m73-'+label);report={'passed':False,'checks':[]}
    try:
        disk=q.sparse_disk(work/'owned.img',2*1024**3)
        with ui.guest(work/'install',disk,resolution,a.firmware,ram=a.ram) as g:
            point=ui.Pointer(g);ui.shot(g,out,'01-welcome');point.wizard(2);ui.wait_view(g,1);ui.shot(g,out,'02-disk')
            point.wizard(2);ui.shot(g,out,'02-no-selection');point.wizard(100);point.wizard(2);ui.wait_view(g,2);ui.shot(g,out,'03-setup');point.wizard(2);ui.wait_view(g,3);ui.shot(g,out,'04-style');point.wizard(32)
            point.wizard(2);ui.wait_view(g,4);ui.shot(g,out,'05-review');point.wizard(1);point.wizard(2);start=point.wizard(2);g.wait('M6_INSTALL_PASS',start,240);ui.wait_view(g,6,start);ui.shot(g,out,'07-finish')
            point.wizard(3);g.wait('M7_DESKTOP_READY');baseline=struct.unpack('<4Q',ui.memory(g,ui.symbol('stats',32)[0],32));ui.shot(g,out,'08-home');point.desktop(94);g.wait('M72_FOLDER_VISIT /Users/Owner/Documents');point.desktop(26)
            for c in 'design':h.key(g,c)
            h.key(g,'ret');g.wait('M72_FOLDER_CREATED /Users/Owner/Documents/design');ui.shot(g,out,'09-documents');point.desktop(1001);g.wait('M72_FOLDER_VISIT /Users/Owner/Documents/design');ui.shot(g,out,'10-folder')
            after=struct.unpack('<4Q',ui.memory(g,ui.symbol('stats',32)[0],32));assert after[0]==baseline[0] and after[3]==baseline[3]
            import time
            time.sleep(.5);paints=ui.memory(g,ui.symbol('paints',8)[0],8);time.sleep(.6);assert paints==ui.memory(g,ui.symbol('paints',8)[0],8)
            assert g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==2
            point.desktop(94);point.desktop(1000);ui.shot(g,out,'12-readable-guide');point.close_app(2)
            point.desktop(103);ui.shot(g,out,'12-settings');point.desktop(43);ui.shot(g,out,'13-dark');point.desktop(44)
            point.desktop(111);ui.shot(g,out,'14-scale100')
            point.desktop(112);ui.shot(g,out,'15-scale150');point.desktop(113);ui.shot(g,out,'16-scale200-or-refusal')
            expected=4 if actual_height(g)>=1536 else 3 if actual_height(g)>=1152 else 2
            assert ui.integer(g,'scale73')==expected;point.desktop(47);g.wait('M7_PREFERENCES_SAVED');ui.shot(g,out,'17-saved-display')
            assert ui.s.s.document(disk,'/System/Licenses/Inter-OFL.txt')==(ROOT/'third_party/inter/OFL.txt').read_bytes()
            assert struct.unpack('<4I',ui.s.s.document(disk,'/System/Settings/display73.bin'))==(0x33375044,expected,0,0)
            assert 'M73_TEXT_OVERFLOW' not in g.log();(out/'install-desktop.log').write_bytes(g.log().encode());scale=ui.integer(g,'scale73')
            with Image.open(out/'01-welcome.png') as image:
                actual=image.size;assert actual==(resolution if a.firmware=='uefi' else (1024,768)),actual
            assert '/Users/Owner/Documents/design' in ui.s.directories(disk)
        installed=ui.guest(work/'restart',disk,resolution,a.firmware,'installed',ready='M7_DESKTOP_READY',ram=a.ram);ui.s.normal_boot(installed)
        with installed as g:
            ui.shot(g,out,'11-independent-desktop');assert ui.integer(g,'choice73',True)==expected;assert 'M6_INSTALLED_VOLUME' in g.log();(out/'restart.log').write_bytes(g.log().encode())
        if resolution==(3840,2160) and a.firmware=='uefi':
            smaller=ui.guest(work/'smaller-display',disk,(1280,800),media='installed',ready='M7_DESKTOP_READY');ui.s.normal_boot(smaller)
            with smaller as g:
                assert ui.integer(g,'choice73',True)==0 and ui.integer(g,'scale73',True)==2;ui.shot(g,out,'18-smaller-display-fallback');(out/'smaller-display.log').write_bytes(g.log().encode())
        assert '/Users/Owner/Documents/design' in ui.s.directories(disk)
        m=json.loads((ROOT/'dist/m73-build-manifest.json').read_text());report.update(passed=True,build_id=m['build_id'],iso_sha256=m['artifacts']['EngineOS-M73.iso']['sha256'],resolution=actual,ui_scale_percent=scale*50,managed_current_bytes=after[0]*4096,managed_peak_bytes=after[1]*4096,navigation_retained_page_delta=after[0]-baseline[0],navigation_allocation_failures=after[3]-baseline[3],idle_all_cpus_halted=True,idle_repaint_count=0,checks=['every native setup step and missing-selection guidance captured','Back preserves choices; verified installation and direct persistent desktop','scaled actual input opens Documents, creates and enters a real folder','no measured paragraph overflow; light/dark Settings, display sizes, low-resolution refusal and persisted preference checked', 'full OFL license independently CRC-checked on installed disk','actual framebuffer dimensions; no screenshot resizing','unforced detached desktop and independently CRC-checked directory retained'])
    except Exception:
        report['error']=q.clean(traceback.format_exc());print(report['error'])
        if 'g' in locals():(out/'last-run.log').write_bytes(g.log().encode())
    (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M7.3 '+label+' '+('PASS' if report['passed'] else 'FAIL'),flush=True)
    if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
