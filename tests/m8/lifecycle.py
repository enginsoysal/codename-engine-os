"""Continue an owned development disk through detached boot, removal and reinstall."""
import argparse
import hashlib
import json
from pathlib import Path
import time
import ui
from journey import task
from database import original_tables

ROOT=ui.ROOT
PRODUCT='/Applications/Windows/{23170F69-40C1-2702-2606-000001000000}'


def shot(g,out,name):
    until=time.monotonic()+30
    while g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')!=2:
        if time.monotonic()>until:raise TimeoutError('Native rendering did not finish before capture')
        time.sleep(.025)
    g.qmp('stop')
    try:ui.raw_shot(g,out,name,True)
    finally:g.qmp('cont')


def payloads(disk):
    reference=original_tables()
    components={row[0]:row[2] for row in reference['Component']}
    provenance=json.loads((ROOT/'third_party/7zip/provenance.json').read_text(encoding='utf-8'))
    result={}
    for row in reference['File']:
        identifier,component,filename=row[:3]
        path=PRODUCT+('/Lang/' if components[component]=='MyLang' else '/')+filename.split('|')[-1]
        data=ui.s.s.document(disk,path,8*1024**2)
        digest=hashlib.sha256(data).hexdigest()
        assert digest==provenance['original_payload_sha256'][identifier],path
        result[path]=digest
    return result


def open_windows(g):
    p=ui.Pointer(g)
    p.desktop(0,True)
    p.desktop(19,True)
    return p


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--disk',type=Path,required=True)
    parser.add_argument('--resolution',default='1280x800')
    parser.add_argument('--ram',type=int,default=128)
    args=parser.parse_args()
    disk=args.disk.resolve()
    assert disk.is_relative_to(ui.q.new_work.__globals__['BASE'].resolve()) and disk.name=='owned.img' and disk.is_file()
    resolution=tuple(map(int,args.resolution.split('x')))
    out=ROOT/'build/m8/lifecycle'
    out.mkdir(parents=True,exist_ok=True)
    work=ui.q.new_work('m8-detached-lifecycle')
    report={'passed':False,'scope':'development detached boot/removal/reinstall','checks':[]}
    report['original_payload_sha256']=payloads(disk)
    source=ui.s.s.document(disk,'/Users/Owner/Documents/EngineOS-Windows.txt')
    before=hashlib.sha256(source).hexdigest()
    try:
        guest=ui.guest(work/'detached',disk,resolution,media='installed',ram=args.ram,ready='M7_DESKTOP_READY')
        ui.s.normal_boot(guest)
        with guest as g:
            assert 'M6_INSTALLED_VOLUME' in g.log()
            assert ui.integer(g,'m6_bundle_size',True)==0
            p=open_windows(g)
            task(g,p,12005,True)
            shot(g,out,'detached-original-windows-program')
            report['checks'].append('Original installed application runs after unforced media-detached startup')
            p.desktop(12002,True)
            shot(g,out,'remove-consent')
            task(g,p,12003,True)
            shot(g,out,'removed')
            (out/'remove.log').write_bytes(g.log().encode())
        try:ui.s.s.document(disk,PRODUCT+'/7z.exe')
        except AssertionError:pass
        else:raise AssertionError('Uninstall left the original executable installed')
        try:ui.s.s.document(disk,PRODUCT+'/product.bin')
        except AssertionError:pass
        else:raise AssertionError('Uninstall left the product registered')
        assert hashlib.sha256(ui.s.s.document(disk,'/Users/Owner/Documents/EngineOS-Windows.txt')).hexdigest()==before
        assert ui.s.s.document(disk,'/Users/Owner/Documents/EngineOS-Windows-Extracted/EngineOS-Windows.txt')==source
        report['checks'].append('Independent disk inspection verifies executable and product registration removal with personal data retained')
        guest=ui.guest(work/'reinstall',disk,resolution,media='installed',ram=args.ram,ready='M7_DESKTOP_READY')
        ui.s.normal_boot(guest)
        with guest as g:
            p=open_windows(g)
            p.desktop(12001,True)
            task(g,p,12003,True)
            task(g,p,12005,True)
            shot(g,out,'reinstalled-original-program')
            (out/'reinstall.log').write_bytes(g.log().encode())
        assert payloads(disk)==report['original_payload_sha256']
        assert ui.s.s.document(disk,'/Users/Owner/Documents/EngineOS-Windows.txt')==source
        report['checks'].append('Reinstallation restores all 106 original payload hashes and the real executable runs again')
        report['passed']=True
    finally:
        report['build_id']=json.loads((ROOT/'dist/m8-build-manifest.json').read_text(encoding='utf-8'))['build_id']
        report['iso_sha256']=hashlib.sha256((ROOT/'dist/EngineOS-M8.iso').read_bytes()).hexdigest()
        (out/'development-results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())


if __name__=='__main__':main()
