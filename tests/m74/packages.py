"""Native optional-package lifecycle and actual HiDPI surface checks."""
import json,struct,time,traceback
import ui
from apps import pages,open_app
ROOT=ui.ROOT

def controls(g):
    count=ui.integer(g,'control_count');raw=ui.memory(g,ui.symbol('controls',16384)[0],count*32)
    return [struct.unpack_from('<iiiiQI',raw,i*32)[5] for i in range(count)]

def main():
    out=ROOT/'evidence/M7.4/packages-4k';out.mkdir(parents=True,exist_ok=True)
    work=ui.q.new_work('m74-packages');disk=ui.q.sparse_disk(work/'owned.img',2*1024**3)
    report={'passed':False,'checks':[],'opened_window_extra_managed_bytes':{}}
    try:
        with ui.guest(work/'install',disk,(3840,2160),ram=128) as g:
            p=ui.Pointer(g);p.wizard(2);p.wizard(100);p.wizard(2);p.wizard(2);p.wizard(2)
            start=p.wizard(2);g.wait('M6_INSTALL_PASS',start,240);ui.wait_view(g,6,start);p.wizard(3);g.wait('M7_DESKTOP_READY');p.close_app(1)
            baseline=pages(g)
            for kind in [4,8,9,10,11,12,13,14,15,16,17,18]:
                open_app(p,kind)
                report['opened_window_extra_managed_bytes'][str(kind)]=(pages(g)[0]-baseline[0])*4096
                if kind==16:
                    p.desktop(11000);p.desktop(18001)
                    raw=ui.memory(g,ui.symbol('info74',64)[0],64)
                    assert struct.unpack_from('<I',raw)[0]==3,'Missing audio output was not reported'
                    assert pages(g)[0]==baseline[0]
                    report['checks'].append('missing HDA output gives a readable failure and retains no DMA pages')
                if kind==15:
                    p.desktop(10500)
                    from PIL import Image
                    import io
                    picture=ui.s.s.document(disk,'/Users/Owner/Pictures/Screenshots/Screenshot1.bmp',32*1024**2)
                    with Image.open(io.BytesIO(picture)) as actual:assert actual.size==(3840,2160)
                    report['checks'].append('actual 4K screenshot independently decoded; capture retains no full-screen buffer')
                ui.shot(g,out,'app-'+str(kind));p.close_app(kind)
                assert pages(g)[0]==baseline[0],('close leak',kind,pages(g),baseline)
            open_app(p,9)
            for c in 'keep my private note':ui.h.key(g,'spc' if c==' ' else c)
            ui.shot(g,out,'private-note-typed');p.desktop(10052);ui.shot(g,out,'private-note-saved')
            raw=ui.memory(g,ui.symbol('apps')[0],ui.symbol('apps')[1])
            note_paths=[v.decode() for v in raw.split(b'\0') if v.startswith(b'/Users/Owner/AppData/Calendar/') and v.endswith(b'.txt')]
            assert len(note_paths)==1;note_path=note_paths[0];p.close_app(9)
            saved={path:ui.s.s.document(disk,path) for path in [note_path,'/Users/Owner/Documents/Welcome.txt','/Users/Owner/Music/Chime.wav']}
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
                kind=package['kind'];select(kind);p.desktop(10300+kind);p.desktop(10004)
                path='/Applications/'+package['folder']+('/program.elf' if package['worker'] else '/manifest.json')
                try:ui.s.s.document(disk,path)
                except AssertionError:pass
                else:raise AssertionError(('package remained',kind))
                assert all(ui.s.s.document(disk,n)==data for n,data in saved.items()),('personal data changed',kind)
                p.desktop(10300+kind);assert ui.s.s.document(disk,path)
                report['checks'].append('removed and offline reinstalled '+package['title']);print(report['checks'][-1],flush=True)
            # An active Editor cannot be removed even through the independent Settings manager.
            p.close_app(3);open_app(p,2);open_app(p,3);p.desktop(10000);select(2);p.desktop(10302);p.desktop(10004)
            assert ui.s.s.document(disk,'/Applications/TextEditor/program.elf').startswith(b'\x7fELF')
            p.desktop(10003);p.close_app(3);p.close_app(2);open_app(p,3);p.desktop(10000)
            report['checks'].append('active worker removal refused; Calendar note and personal sample files retained after every removal')
            ui.shot(g,out,'settings-package-fallback');p.close_app(3)
            assert pages(g)[0]==baseline[0]
            ui.shot(g,out,'desktop-4k');assert 'M73_TEXT_OVERFLOW' not in g.log()
            from PIL import Image
            with Image.open(out/'desktop-4k.png') as im:assert im.size==(3840,2160)
            time.sleep(1.1);assert g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==2
            report.update(managed_current_bytes=pages(g)[0]*4096,managed_peak_bytes=pages(g)[1]*4096,framebuffer_bytes=3840*2160*4)
            (out/'native.log').write_bytes(g.log().encode('utf-8'))
        restart=ui.guest(work/'restart',disk,media='installed',ready='M7_DESKTOP_READY');ui.s.normal_boot(restart)
        with restart as g:
            p=ui.Pointer(g);p.close_app(1,True)
            for package in packages:
                open_app(p,package['kind'],True);p.close_app(package['kind'],True)
            (out/'restart.log').write_bytes(g.log().encode('utf-8'))
        manifest=json.loads((ROOT/'dist/m74-build-manifest.json').read_text(encoding='utf-8'))
        report.update(passed=True,build_id=manifest['build_id'],iso_sha256=manifest['artifacts']['EngineOS-M74.iso']['sha256'])
    except Exception:
        report['error']=ui.q.clean(traceback.format_exc());print(report['error'],flush=True)
        if 'g' in locals():(out/'failure.log').write_bytes(g.log().encode('utf-8'))
    (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode('utf-8'))
    print('M7.4 packages/4K '+('PASS' if report['passed'] else 'FAIL'),flush=True)
    if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
