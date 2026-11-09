"""Normal native UI tests for MSI rejection, cancellation, resources and recovery."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import time
import traceback
import ui
from journey import task
from lifecycle import PRODUCT,open_windows,shot

ROOT=ui.ROOT


def info(g):
    return ui.memory(g,ui.symbol('info8',installed=True)[0],ui.symbol('info8',installed=True)[1])


def pages(g):
    return struct.unpack('<4Q',ui.memory(g,ui.symbol('stats',32,True)[0],32))


def entries(g):
    address=ui.symbol('volume',864320,True)[0]
    raw=ui.memory(g,address+64,1024*280)
    return [raw[i*280:i*280+256].split(b'\0')[0].decode() for i in range(1024)
            if struct.unpack_from('<I',raw,i*280+276)[0]]


def generation(g):
    return struct.unpack('<Q',ui.memory(g,ui.symbol('volume',864320,True)[0]+40,8))[0]


def focus(g,p,kind):
    raw=ui.memory(g,ui.symbol('model',installed=True)[0],28*8)
    slots=[i for i in range(8) if struct.unpack_from('<I',raw,i*28+20)[0]==kind and raw[i*28+24]]
    assert len(slots)==1
    slot=slots[0]
    count=ui.integer(g,'control_count',True)
    raw=ui.memory(g,ui.symbol('controls',16384,True)[0],count*32)
    buttons=[struct.unpack_from('<4iQI',raw,i*32) for i in range(count)]
    x,y,w,h,token,action=next(b for b in buttons if b[5]==70 and (b[4]&0xffffffff)==slot+1)
    p.logical(x+w//2,y+h//2,True)
    time.sleep(.1)


def wait_stopped(g,timeout=40):
    until=time.monotonic()+timeout
    while time.monotonic()<until:
        value=info(g)
        if struct.unpack_from('<I',value)[0]!=1 and g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==2:
            return value
        time.sleep(.025)
    raise TimeoutError('Windows task did not stop')


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--disk',type=Path,required=True)
    args=parser.parse_args()
    disk=args.disk.resolve()
    assert disk.is_relative_to(ui.q.new_work.__globals__['BASE'].resolve()) and disk.name=='owned.img' and disk.is_file()
    out=ROOT/'build/m8/safety'
    out.mkdir(parents=True,exist_ok=True)
    work=ui.q.new_work('m8-safety')
    report={'passed':False,'checks':[],'scope':'normal-UI first-profile failure and resource checks'}
    source=ui.s.s.document(disk,'/Users/Owner/Documents/EngineOS-Windows.txt')
    try:
        guest=ui.guest(work/'owned',disk,media='installed',ready='M7_DESKTOP_READY',ram=128)
        ui.s.normal_boot(guest)
        with guest as g:
            p=open_windows(g)
            if PRODUCT+'/product.bin' in entries(g):
                p.desktop(12002,True)
                task(g,p,12003,True)
            p.close_app(19,True)
            p.desktop(0,True)
            p.desktop(2,True)
            for key in 'unsupported package':ui.h.key(g,'spc' if key==' ' else key)
            p.desktop(80,True)
            bad_name='unsupported'+work.name[:8]+'.msi'
            for key in bad_name:ui.h.key(g,'dot' if key=='.' else key)
            start=len(g.log())
            p.desktop(81,True)
            g.wait('M7_DOCUMENT_SAVE_AS',start)
            p.close_app(2,True)
            p.desktop(94,True)
            paths=[path for path in entries(g) if path.startswith('/Users/Owner/Documents/') and '/' not in path[len('/Users/Owner/Documents/'):]]
            row=paths.index('/Users/Owner/Documents/'+bad_name)
            p.desktop(1000+row,True)
            before=generation(g)
            start=len(g.log())
            p.desktop(12000,True,settle=False)
            g.wait('M8_JOB_FAILED',start)
            value=wait_stopped(g)
            assert struct.unpack_from('<I',value)[0]==3 and generation(g)==before
            assert PRODUCT not in entries(g)
            shot(g,out,'unsupported-msi-refused-without-writes')
            report['checks'].append('Text Editor-created unsupported MSI is refused through Files/Windows Apps without a volume-generation change or product creation')
            p.close_app(19,True)
            p.desktop(0,True)
            p.desktop(19,True)
            p.desktop(12001,True)
            start=len(g.log())
            p.desktop(12003,True,settle=False)
            g.wait('Installing original Windows application files.',start)
            assert any(path.startswith(PRODUCT+'/') for path in entries(g))
            p.desktop(12008,True,settle=False)
            value=wait_stopped(g)
            assert struct.unpack_from('<I',value)[0]==3
            assert not any(path==PRODUCT or path.startswith(PRODUCT+'/') for path in entries(g))
            shot(g,out,'cancelled-installation-cleaned')
            report['checks'].append('Cancel after real payload writes reclaims the fresh product tree and the protected worker')
            p.desktop(12001,True)
            start=len(g.log())
            p.desktop(12003,True,settle=False)
            g.wait('Installing original Windows application files.',start)
            p.desktop(0,True,settle=False)
            p.desktop(6,True,settle=False)
            p.desktop(72,True,settle=False)
            g.wait('M8_JOB_FAILED',start)
            assert 'M8_INCOMPLETE_WRITE_DISCARDED' in g.log()[start:]
            assert not any(path==PRODUCT or path.startswith(PRODUCT+'/') for path in entries(g))
            shot(g,out,'incomplete-write-discarded-under-memory-pressure')
            focus(g,p,6)
            p.close_app(6,True)
            focus(g,p,19)
            report['checks'].append('Memory pressure during actual MSI writes discards the incomplete file, clears the unpublished product and keeps the desktop alive')
            p.desktop(12001,True)
            install_info=task(g,p,12003,True)
            warm=task(g,p,12005,True)
            before=pages(g)[0]
            peaks=[]
            for unused in range(3):
                value=task(g,p,12005,True)
                peaks.append(struct.unpack_from('<Q',value,24)[0]*4096)
                assert pages(g)[0]==before
            report['resource_measurements']={'managed_idle_bytes':before*4096,
                                             'install_peak_worker_and_writer_bytes':struct.unpack_from('<Q',install_info,24)[0]*4096,
                                             'run_peak_worker_and_writer_bytes':peaks,
                                             'retained_worker_pages_after_warmup':0,
                                             'guest_ram_mib':128,
                                             'excludes_static_kernel_boot_reservations_and_video_ram':True}
            report['checks'].append('Repeated original executable runs release all transient worker/writer pages after one-time transition-stack warmup')
            p.desktop(0,True)
            p.desktop(6,True)
            p.desktop(72,True)
            limited=pages(g)[0]
            focus(g,p,19)
            p.desktop(12005,True)
            value=wait_stopped(g)
            assert struct.unpack_from('<I',value)[0]==3 and pages(g)[0]==limited
            shot(g,out,'memory-admission-failure-keeps-desktop')
            focus(g,p,6)
            p.close_app(6,True)
            task(g,p,12005,True)
            report['checks'].append('System Monitor memory admission limit refuses a new Windows job without leaking pages; closing Monitor restores successful execution')
            before=ui.memory(g,ui.symbol('paints',8,True)[0],8)
            time.sleep(.3)
            report['idle_frame_unchanged']=ui.memory(g,ui.symbol('paints',8,True)[0],8)==before
            assert g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==2
            report['checks'].append('Both guest processors halt with no active Windows job')
            p.desktop(12011,True)
            shot(g,out,'complete-local-license-viewer')
            p.desktop(12010,True)
            shot(g,out,'local-license-next-page')
            p.desktop(12004,True)
            (out/'serial.log').write_bytes(g.log().encode())
        assert ui.s.s.document(disk,'/Users/Owner/Documents/EngineOS-Windows.txt')==source
        assert ui.s.s.document(disk,'/Users/Owner/Documents/EngineOS-Windows-Extracted/EngineOS-Windows.txt')==source
        report['checks'].append('Independent post-test disk inspection confirms original and extracted personal document bytes are retained')
        report['passed']=True
    except Exception:
        report['error']=ui.q.clean(traceback.format_exc())
        raise
    finally:
        meta=json.loads((ROOT/'dist/m8-build-manifest.json').read_text(encoding='utf-8'))
        report['build_id']=meta['build_id']
        report['iso_sha256']=hashlib.sha256((ROOT/'dist/EngineOS-M8.iso').read_bytes()).hexdigest()
        (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())


if __name__=='__main__':main()
