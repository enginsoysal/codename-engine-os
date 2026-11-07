"""Native optional-package lifecycle and actual HiDPI surface checks."""
import json,struct,time,traceback
import ui
from apps import pages,open_app
ROOT=ui.ROOT

def controls(g):
    count=ui.integer(g,'control_count');raw=ui.memory(g,ui.symbol('controls',16384)[0],count*32)
    return [struct.unpack_from('<iiiiQI',raw,i*32)[5] for i in range(count)]

def main():
    out=ROOT/'evidence/M7.4/omitted-apps';out.mkdir(parents=True,exist_ok=True)
    work=ui.q.new_work('m74-omitted');disk=ui.q.sparse_disk(work/'owned.img',2*1024**3)
    report={'passed':False,'checks':[]}
    try:
        with ui.guest(work/'install',disk,(1280,800),ram=128) as g:
            p=ui.Pointer(g);p.wizard(2);p.wizard(100);p.wizard(2);p.wizard(2);p.wizard(2)
            start=p.wizard(2);g.wait('M6_INSTALL_PASS',start,240);ui.wait_view(g,6,start);p.wizard(3);g.wait('M7_DESKTOP_READY');p.close_app(1)
            baseline=pages(g)
            open_app(p,3);p.desktop(10000)
            import sys
            sys.path.insert(0,str(ROOT/'tools/build'));from m74 import catalogue
            packages=catalogue()
            def select(kind):
                for attempt in range(9):
                    if 10300+kind in controls(g):return
                    p.desktop(10002 if attempt<4 else 10001)
                raise AssertionError(('package not shown',kind))
            for package in packages:
                kind=package['kind'];select(kind)
                path='/Applications/'+package['folder']+('/program.elf' if package['worker'] else '/manifest.json')
                try:ui.s.s.document(disk,path)
                except AssertionError:pass
                else:raise AssertionError(('omitted package was installed',kind))
                p.desktop(10300+kind);assert ui.s.s.document(disk,path)
                report['checks'].append('omitted, then offline installed '+package['title']);print(report['checks'][-1],flush=True)
            ui.shot(g,out,'settings-package-fallback');p.close_app(3)
            assert pages(g)[0]==baseline[0]
            ui.shot(g,out,'desktop-4k');assert 'M73_TEXT_OVERFLOW' not in g.log()
            from PIL import Image
            with Image.open(out/'desktop-4k.png') as im:assert im.size==(1280,800)
            time.sleep(1.1);assert g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==2
            report.update(managed_current_bytes=pages(g)[0]*4096,managed_peak_bytes=pages(g)[1]*4096,framebuffer_bytes=1280*800*4)
            (out/'native.log').write_bytes(g.log().encode('utf-8'))
        restart=ui.guest(work/'restart',disk,media='installed',ready='M7_DESKTOP_READY');ui.s.normal_boot(restart)
        with restart as g:
            p=ui.Pointer(g);p.close_app(1,True)
            for package in packages:
                open_app(p,package['kind'],True);p.close_app(package['kind'],True)
            (out/'restart.log').write_bytes(g.log().encode('utf-8'))
        manifest=json.loads((ROOT/'dist/m74minimal-build-manifest.json').read_text(encoding='utf-8'))
        report.update(passed=True,build_id=manifest['build_id'],iso_sha256=manifest['artifacts']['EngineOS-M74MINIMAL.iso']['sha256'])
    except Exception:
        report['error']=ui.q.clean(traceback.format_exc());print(report['error'],flush=True)
        if 'g' in locals():(out/'failure.log').write_bytes(g.log().encode('utf-8'))
    (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode('utf-8'))
    print('M7.4 omitted applications '+('PASS' if report['passed'] else 'FAIL'),flush=True)
    if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
