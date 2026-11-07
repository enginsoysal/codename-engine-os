"""Install and exercise the separately built actual Engine OS Night desktop ISO."""
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
IMAGE='EngineOS-M7NIGHT'


def symbol(name):
    mapping=(ROOT/('dist/'+IMAGE+'-installed-kernel.map')).read_text(encoding='utf-8')
    if name=='preferences':mapping=mapping.split('kernel_desktop_session-kernel.o:(.bss)',1)[1]
    match=re.search(r'^\s*([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)\s+\d+\s+'+name+r'$',mapping,re.M);assert match,name
    return int(match[1],16),int(match[2],16)
i.symbol=symbol


def preferences(guest):return struct.unpack_from('<III',i.memory(guest,symbol('preferences')[0],24),8)


def main():
    from PIL import Image
    work=q.new_work('m7-distribution');out=ROOT/'evidence/M7/distribution';out.mkdir(parents=True,exist_ok=True)
    report={'passed':False,'m7_complete':False,'checks':[]}
    try:
        manifest=json.loads((ROOT/'dist/m7night-build-manifest.json').read_text(encoding='utf-8'))
        disk=q.sparse_disk(work/'owned.img',2*1024**3)
        with q.Guest(work/'install',disk,image_name=IMAGE) as guest:
            h.next(guest);h.next(guest);h.next(guest,2);h.shot(guest,out,'native-custom-components')
            h.next(guest,3);h.next(guest,5);start=len(guest.log());h.next(guest,1);guest.wait('M6_INSTALL_PASS',start,240)
            (out/'install.log').write_bytes(guest.log().encode())
        try:s.document(disk,'/Applications/Calculator/program.elf')
        except AssertionError:pass
        else:raise AssertionError('Custom profile installed Calculator despite disabled default')
        assert s.document(disk,'/System/Packages/Calculator.elf')==(ROOT/'dist/EngineOS-M7NIGHT-calculator.elf').read_bytes()
        with q.Guest(work/'desktop',disk,media='installed',image_name=IMAGE,ready='M7_DESKTOP_READY') as guest:
            assert 'M7_DESKTOP_READY build='+manifest['build_id'] in guest.log()
            assert preferences(guest)==(0x9caeff,1,0),preferences(guest)
            pointer=s.Pointer(guest);h.shot(guest,out,'native-night-desktop')
            with Image.open(out/'native-night-desktop.png') as frame:assert frame.getpixel((10,10))==(0x24,0x1d,0x35)
            pointer.click(55,210)
            for character in 'night distro':h.key(guest,'spc' if character==' ' else character)
            i.control(guest,'s');guest.wait('M7_DOCUMENT_SAVED')
            pointer.click(55,295);_,settings=i.window(guest,3)
            pointer.click(settings.x+95,settings.y+275);guest.wait('M7_CALCULATOR_INSTALLED')
            pointer.click(settings.x+100,settings.y+330);guest.wait('M7_PREFERENCES_SAVED')
            assert preferences(guest)==(0x9caeff,1,1)
            h.shot(guest,out,'optional-app-installed');(out/'desktop.log').write_bytes(guest.log().encode())
        assert s.document(disk,'/Users/Owner/Documents/Note1.txt')==b'night distro'
        assert s.document(disk,'/Applications/Calculator/program.elf')==(ROOT/'dist/EngineOS-M7NIGHT-calculator.elf').read_bytes()
        with q.Guest(work/'restart',disk,media='installed',image_name=IMAGE,ready='M7_DESKTOP_READY') as guest:
            assert preferences(guest)==(0x9caeff,1,1)
            h.shot(guest,out,'native-night-restart');(out/'restart.log').write_bytes(guest.log().encode())
        assert s.document(disk,'/Users/Owner/Documents/Note1.txt')==b'night distro'
        report.update(passed=True,custom_build_id=manifest['build_id'],iso_sha256=manifest['artifacts'][IMAGE+'.iso']['sha256'],
                      base_build_id=json.loads((ROOT/'dist/m7-build-manifest.json').read_text(encoding='utf-8'))['build_id'],
                      scope='separate native custom distro installation/defaults/app omission/reinstall and restart persistence; complete M7 acceptance is separate')
        report['checks']=['actual schema-2 custom ISO installs and boots detached','native dark/lavender/plum desktop and independently inspected missing optional executable','editor saves real bytes without Calculator','offline optional executable install and persistent settings','detached restart retains selected appearance, app and exact document']
    except Exception:
        report['error']=q.clean(traceback.format_exc());print(report['error'])
        if 'guest' in locals():(out/'last-run.log').write_bytes(guest.log().encode())
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('M7 native custom distribution '+('PASS' if report['passed'] else 'FAIL'),flush=True)
    if not report['passed']:raise SystemExit(1)


if __name__=='__main__':main()
