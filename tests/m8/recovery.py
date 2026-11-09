"""Power-cut a real cancel cleanup, then recover through the ordinary native UI."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import time
import traceback
import ui
from journey import task
from lifecycle import PRODUCT,open_windows,shot,payloads
from safety import entries,info

ROOT=ui.ROOT


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--disk',type=Path,required=True)
    args=parser.parse_args()
    disk=args.disk.resolve()
    assert disk.is_relative_to(ui.q.new_work.__globals__['BASE'].resolve()) and disk.name=='owned.img' and disk.is_file()
    out=ROOT/'build/m8/recovery'
    out.mkdir(parents=True,exist_ok=True)
    work=ui.q.new_work('m8-cleanup-power-cut')
    meta=json.loads((ROOT/'dist/m8-build-manifest.json').read_text(encoding='utf-8'))
    report={'passed':False,'scope':'interrupted original-MSI cleanup and guided recovery','checks':[],
            'build_id':meta['build_id'],'iso_sha256':meta['artifacts']['EngineOS-M8.iso']['sha256']}
    document=ui.s.s.document(disk,'/Users/Owner/Documents/EngineOS-Windows.txt')
    try:
        guest=ui.guest(work/'interrupt',disk,media='installed',ready='M7_DESKTOP_READY',ram=128)
        ui.s.normal_boot(guest)
        with guest as g:
            assert 'ENGINE_OS_KERNEL_ENTRY build='+meta['build_id'] in g.log()
            p=open_windows(g)
            if PRODUCT in entries(g):
                p.desktop(12002,True)
                task(g,p,12003,True)
            p.desktop(12001,True)
            start=len(g.log())
            p.desktop(12003,True,settle=False)
            g.wait('M8_PROGRESS percent=0x0000000000000032',start,240)
            p.desktop(12008,True,settle=False)
            deadline=time.monotonic()+20
            while struct.unpack_from('<I',info(g))[0]==1:
                if time.monotonic()>deadline:raise TimeoutError('Cancel was not observed')
                time.sleep(.005)
            g.qmp('stop')
            paths=entries(g)
            children=[path for path in paths if path.startswith(PRODUCT+'/')]
            markers={PRODUCT+'/install.plan',PRODUCT+'/product.bin'}
            assert any(path in markers for path in children),'Could not catch cleanup before its ownership marker was retired'
            assert any(path not in markers for path in children),'Cleanup finished before the intended power-cut window'
            report['remaining_owned_entries_at_cut']=len(children)
            report['checks'].append('Real payload writes preceded Cancel; the guest was stopped mid-cleanup with remaining files and a durable ownership marker')
            (out/'before-power-cut.log').write_bytes(g.log().encode())
            # The vCPU stays stopped. Quitting this owned emulator cannot run further guest cleanup.
            try:g.qmp('quit')
            except Exception:pass
        guest=ui.guest(work/'recover',disk,media='installed',ready='M7_DESKTOP_READY',ram=128)
        ui.s.normal_boot(guest)
        with guest as g:
            assert 'ENGINE_OS_KERNEL_ENTRY build='+meta['build_id'] in g.log()
            p=open_windows(g)
            shot(g,out,'recovery-offered-after-interrupted-cleanup')
            p.desktop(12002,True)
            task(g,p,12003,True)
            assert not any(path==PRODUCT or path.startswith(PRODUCT+'/') for path in entries(g))
            shot(g,out,'interrupted-product-cleared')
            p.desktop(12001,True)
            task(g,p,12003,True)
            task(g,p,12005,True)
            shot(g,out,'original-program-after-recovery-and-reinstall')
            (out/'recovery-and-reinstall.log').write_bytes(g.log().encode())
        report['original_payload_sha256']=payloads(disk)
        assert ui.s.s.document(disk,'/Users/Owner/Documents/EngineOS-Windows.txt')==document
        report['document_sha256']=hashlib.sha256(document).hexdigest()
        report['checks'].append('Media-detached boot preserves the desktop; guided recovery clears the interrupted tree; reinstall restores all 106 original hashes and the real executable runs')
        report['checks'].append('Independent disk inspection confirms the personal document is unchanged')
        report['passed']=True
    except Exception:
        report['error']=ui.q.clean(traceback.format_exc())
        raise
    finally:
        (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())


if __name__=='__main__':main()
