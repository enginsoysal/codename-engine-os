"""Qualify exact M7 BIOS/UEFI ISO/USB installation and detached desktop tasks."""
import importlib.util
import json
from pathlib import Path
import traceback
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('m7resources',Path(__file__).with_name('resources.py'))
r=importlib.util.module_from_spec(spec);spec.loader.exec_module(r)
s,i,h,q=r.s,r.i,r.h,r.q


def main():
    import argparse
    parser=argparse.ArgumentParser();parser.add_argument('--firmware',choices=['uefi','bios'],required=True);parser.add_argument('--media',choices=['iso','usb'],required=True);a=parser.parse_args()
    label=a.firmware+'-'+a.media;work=q.new_work('m7-'+label);out=ROOT/'evidence/M7/firmware'/label;out.mkdir(parents=True,exist_ok=True)
    report={'passed':False,'m7_complete':False,'firmware':a.firmware,'media':a.media,'checks':[]}
    try:
        disk=q.sparse_disk(work/'owned.img',2*1024**3);ps2=a.firmware=='bios'
        with q.Guest(work/'install',disk,firmware=a.firmware,media=a.media,image_name='EngineOS-M7',ps2_only=ps2) as guest:
            h.shot(guest,out,'native-welcome')
            h.next(guest);h.next(guest);h.next(guest,3 if a.media=='usb' else 2);h.next(guest,3);h.next(guest,5)
            start=len(guest.log());h.next(guest,1);guest.wait('M6_INSTALL_PASS',start,240)
            h.shot(guest,out,'native-install-complete');(out/'install.log').write_bytes(guest.log().encode())
        with q.Guest(work/'desktop',disk,firmware=a.firmware,media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY',ps2_only=ps2) as guest:
            r.start_menu(guest,2);guest.wait('M7_APP_RING3_CREATED')
            text=label+' desktop'
            for character in text:h.key(guest,{' ':'spc','-':'minus'}.get(character,character))
            i.control(guest,'s');guest.wait('M7_DOCUMENT_SAVED')
            h.shot(guest,out,'native-installed-desktop');(out/'desktop.log').write_bytes(guest.log().encode())
            assert guest.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==2
        assert s.document(disk,'/Users/Owner/Documents/Note1.txt')==text.encode()
        with q.Guest(work/'restart',disk,firmware=a.firmware,media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY',ps2_only=ps2) as guest:
            h.shot(guest,out,'desktop-after-restart');(out/'restart.log').write_bytes(guest.log().encode())
        assert s.document(disk,'/Users/Owner/Documents/Note1.txt')==text.encode()
        meta=json.loads((ROOT/'dist/m7-build-manifest.json').read_text(encoding='utf-8'))
        report.update(passed=True,build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M7.iso']['sha256'],
                      usb_sha256=meta['artifacts']['EngineOS-M7-usb.img']['sha256'] if a.media=='usb' else None,
                      input='PS/2' if ps2 else 'USB HID',scope='named native firmware/media/input and persisted desktop task; complete M7 acceptance is separate')
        report['checks']=['exact live medium boots own native wizard','actual guided blank-target installation','media-detached native desktop enters CPL3 editor through keyboard controls and saves exact bytes','second detached desktop boot retains independently CRC-checked document','idle CPUs halt']
    except Exception:
        report['error']=q.clean(traceback.format_exc());print(report['error'])
        if 'guest' in locals():(out/'last-run.log').write_bytes(guest.log().encode())
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('M7 '+label+' '+('PASS' if report['passed'] else 'FAIL'),flush=True)
    if not report['passed']:raise SystemExit(1)


if __name__=='__main__':main()
