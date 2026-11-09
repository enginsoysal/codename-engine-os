"""M9 development journey using ordinary guest controls and disposable disks only."""
import argparse
import hashlib
import json
import io
import zipfile
import struct
import time
import traceback
import ui

ROOT=ui.ROOT

def stable_shot(g,out,name):
    until=time.monotonic()+30
    while g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')!=2:
        if time.monotonic()>until:raise TimeoutError('Native frame did not finish')
        time.sleep(.025)
    g.qmp('stop')
    try:ui.raw_shot(g,out,name,True)
    finally:g.qmp('cont')

ui.shot=stable_shot


def wizard(g,p,action):
    count=ui.integer(g,'controls')
    raw=ui.memory(g,ui.symbol('buttons',480)[0],count*20)
    matches=[struct.unpack_from('<5I',raw,i*20) for i in range(count)
             if struct.unpack_from('<I',raw,i*20+16)[0]==action]
    assert len(matches)==1,(action,matches)
    x,y,w,h,_=matches[0]
    start=len(g.log())
    p.logical(x+w//2,y+h//2)
    if action not in (3,30,84):
        g.wait('M73_VIEW step=',start)
    return start


def task(g,p,action,installed=False,timeout=180):
    start=len(g.log())
    p.desktop(action,installed,settle=False)
    g.wait('M8_CPL3_CREATED',start)
    until=time.monotonic()+timeout
    observed=None;next_probe=0
    while time.monotonic()<until:
        if time.monotonic()>next_probe:
            raw=ui.memory(g,ui.symbol("info8",installed=installed)[0],496)
            status=struct.unpack_from("<4I",raw)
            message=raw[304:496].split(b"\0")[0].decode(errors="replace")
            value=(status,message)
            if value!=observed:print("Windows task:",value,flush=True);observed=value
            next_probe=time.monotonic()+2
        log=g.log()[start:]
        if 'M8_JOB_DONE' in log or 'M8_JOB_FAILED' in log:
            info=ui.memory(g,ui.symbol('info8',installed=installed)[0],ui.symbol('info8',installed=installed)[1])
            state,command,status,percent=struct.unpack_from('<4I',info)
            message=info[304:496].split(b'\0')[0].decode(errors='replace')
            assert state==2 and status==0,(state,status,message,log[-3000:])
            idle_deadline=time.monotonic()+30
            while g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')!=2:
                if time.monotonic()>idle_deadline:raise TimeoutError('Job cleanup and native repaint did not finish')
                time.sleep(.025)
            return info
        time.sleep(.05)
    print(g.qmp('human-monitor-command',{'command-line':'info registers -a'}),flush=True)
    raise TimeoutError(g.log()[-4000:])


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--firmware',choices=['uefi','bios'],default='uefi')
    parser.add_argument('--ram',type=int,default=128)
    parser.add_argument('--resolution',default='1280x800')
    args=parser.parse_args()
    resolution=tuple(map(int,args.resolution.split('x')))
    out=ROOT/'build/m9/journey'/(args.firmware+'-'+args.resolution+'-'+str(args.ram))
    out.mkdir(parents=True,exist_ok=True)
    work=ui.q.new_work('m9-original-msi')
    disk=ui.q.sparse_disk(work/'owned.img',2*1024**3)
    manifest=json.loads((ROOT/'dist/m9-build-manifest.json').read_text(encoding='utf-8'))
    expected_iso=hashlib.sha256((ROOT/'dist/EngineOS-M9.iso').read_bytes()).hexdigest()
    assert expected_iso==manifest['artifacts']['EngineOS-M9.iso']['sha256']
    report={'passed':False,'scope':'development original-MSI guest journey','checks':[],'firmware':args.firmware,'resolution':resolution,'ram_mib':args.ram}
    try:
        with ui.guest(work/'install',disk,resolution,args.firmware,ram=args.ram) as g:
            memory=g.qmp('query-memory-size-summary')
            assert memory['base-memory']==args.ram*1024**2 and not memory.get('plugged-memory',0)
            report['observed_guest_memory_bytes']=memory['base-memory']
            report['observed_processor_count']=len(g.qmp('query-cpus-fast'))
            assert report['observed_processor_count']==2
            p=ui.Pointer(g)
            wizard(g,p,2)
            wizard(g,p,100)
            wizard(g,p,2)
            wizard(g,p,2)
            wizard(g,p,2)
            start=wizard(g,p,2)
            g.wait('M6_INSTALL_PASS',start,240)
            ui.wait_view(g,6,start)
            start=wizard(g,p,84)
            g.wait('M7_DESKTOP_READY',start,180)
            assert 'M6_INSTALLED_VOLUME' in g.log()[start:]
            assert ui.integer(g,'m6_bundle_size',True)==0
            p=ui.Pointer(g)
            p.desktop(0,True)
            p.desktop(19,True)
            ui.shot(g,out,'windows-apps')
            task(g,p,12000,True)
            ui.shot(g,out,'original-msi-preview')
            p.desktop(12001,True)
            ui.shot(g,out,'install-consent')
            task(g,p,12003,True)
            ui.shot(g,out,'installed-original-msi')
            report['checks'].append('Original MSI installed through native consent UI inside installed Engine OS')
            task(g,p,12005,True)
            ui.shot(g,out,'original-windows-7zip-running')
            task(g,p,12006,True)
            ui.shot(g,out,'original-7zip-created-archive')
            task(g,p,12007,True)
            ui.shot(g,out,'original-7zip-extracted-archive')
            report['checks'].append('Original Windows executable completed info, ZIP create and ZIP extract tasks')
            (out/'serial.log').write_bytes(g.log().encode())
        original=ui.s.s.document(disk,'/Users/Owner/Documents/EngineOS-Windows.txt')
        extracted=ui.s.s.document(disk,'/Users/Owner/Documents/EngineOS-Windows-Extracted/EngineOS-Windows.txt')
        assert original==extracted
        archive=ui.s.s.document(disk,'/Users/Owner/Documents/EngineOS-Windows.zip',8*1024**2)
        with zipfile.ZipFile(io.BytesIO(archive)) as check:
            assert check.testzip() is None and check.read('EngineOS-Windows.txt')==original
        provenance=json.loads((ROOT/'third_party/7zip/provenance.json').read_text(encoding='utf-8'))
        core={}
        for name in ('7z.exe','7z.dll'):
            digest=hashlib.sha256(ui.s.s.document(disk,'/Applications/Windows/{23170F69-40C1-2702-2606-000001000000}/'+name,8*1024**2)).hexdigest()
            assert digest==provenance['original_payload_sha256']['_'+name]
            core[name]=digest
        report['original_executable_and_dll_sha256']=core
        report['document_sha256']=hashlib.sha256(original).hexdigest()
        report['zip_sha256']=hashlib.sha256(archive).hexdigest()
        capture=json.loads((out/'original-windows-7zip-running-capture.json').read_text(encoding='utf-8'))
        report['actual_framebuffer']=[capture['width'],capture['height']]
        if args.firmware=='uefi':assert report['actual_framebuffer']==list(resolution)
        report['checks'].append('Independent persisted-volume inspection verifies identical original/extracted document bytes')
        report['passed']=True
    except Exception:
        report['error']=traceback.format_exc().replace(str(ROOT),'<project-root>')
        raise
    finally:
        report['build_id']=manifest['build_id']
        report['iso_sha256']=expected_iso
        assert hashlib.sha256((ROOT/'dist/EngineOS-M9.iso').read_bytes()).hexdigest()==expected_iso,'ISO changed during the guest journey'
        (out/'development-results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())


if __name__=='__main__':
    main()
