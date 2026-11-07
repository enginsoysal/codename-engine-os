"""Preserve a real M7.1 workspace through the native M7.2 recovery/update flow."""
import importlib.util
import json
from pathlib import Path
import traceback
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('session72',Path(__file__).with_name('session.py'));s=importlib.util.module_from_spec(spec);spec.loader.exec_module(s)
q,h=s.q,s.h
def main():
    work=q.new_work('m72-update');out=ROOT/'evidence/M7.2/update';out.mkdir(parents=True,exist_ok=True);report={'passed':False}
    try:
        disk=q.sparse_disk(work/'owned.img',2*1024**3)
        with q.Guest(work/'m71',disk,image_name='EngineOS-M71') as g:
            h.next(g);h.next(g);h.next(g,4);h.next(g,3);h.next(g,5);h.next(g,1);g.wait('M6_INSTALL_PASS',timeout=240)
        original=s.s.document(disk,'/Users/Owner/Documents/Welcome.txt')
        with q.Guest(work/'update',disk,image_name='EngineOS-M72') as g:
            pointer=s.s.Pointer(g);pointer.click(630,539);pointer.click(450,350);pointer.click(310,225);pointer.click(1090,684);h.shot(g,out,'update-review');pointer.click(1090,684);g.wait('M6_MAINTENANCE_PASS',timeout=240);h.shot(g,out,'update-finish');(out/'update.log').write_bytes(g.log().encode())
        assert s.s.document(disk,'/Users/Owner/Documents/Welcome.txt')==original
        guest=q.Guest(work/'desktop',disk,media='installed',image_name='EngineOS-M72',ready='M7_DESKTOP_READY');s.normal_boot(guest)
        with guest as g:
            current=json.loads((ROOT/'dist/m72-build-manifest.json').read_text());assert 'M7_DESKTOP_READY build='+current['build_id'] in g.log();h.shot(g,out,'updated-desktop');(out/'desktop.log').write_bytes(g.log().encode())
        assert s.s.document(disk,'/Users/Owner/Documents/Welcome.txt')==original
        # Older slots retain the standard fallback path. Test the actual retained
        # boot registration, rather than a fresh firmware that might rediscover it.
        rollback=q.Guest(work/'rollback',disk,image_name='EngineOS-M72')
        with rollback as g:
            pointer=s.s.Pointer(g);pointer.click(630,539);pointer.click(600,350);pointer.click(310,225);pointer.click(1090,684);h.shot(g,out,'rollback-review');pointer.click(1090,684);g.wait('M6_MAINTENANCE_PASS',timeout=240);h.shot(g,out,'rollback-finish');(out/'rollback.log').write_bytes(g.log().encode())
        older=q.Guest(work/'rolled-back-desktop',disk,media='installed',image_name='EngineOS-M71',ready='M7_DESKTOP_READY');s.normal_boot(older,rollback.folder/'vars.fd')
        with older as g:
            previous=json.loads((ROOT/'dist/m71-build-manifest.json').read_text());assert 'M7_DESKTOP_READY build='+previous['build_id'] in g.log();h.shot(g,out,'rolled-back-m71-desktop');(out/'rolled-back-desktop.log').write_bytes(g.log().encode())
        assert s.s.document(disk,'/Users/Owner/Documents/Welcome.txt')==original
        meta=json.loads((ROOT/'dist/m72-build-manifest.json').read_text());report.update(passed=True,build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M72.iso']['sha256'],checks=['M7.1 real installation','native Recovery -> Update -> select disk -> review -> Confirm','exact independently CRC-checked original guide retained','unforced ISO-detached M7.2 installed desktop','actual older-slot rollback keeps firmware fallback valid: retained M7.2 NVRAM boots the exact M7.1 desktop and original user file'])
    except Exception:
        report['error']=q.clean(traceback.format_exc());print(report['error'])
        if 'g' in locals():(out/'last-run.log').write_bytes(g.log().encode())
    (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M7.2 update '+('PASS' if report['passed'] else 'FAIL'))
    if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
