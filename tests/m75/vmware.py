"""Same-VM installation/desktop/restart with ordinary VMware firmware discovery."""
import argparse
import ctypes
import hashlib
import importlib.util
import json
import msvcrt
from pathlib import Path
import re
import shutil
import traceback
import uuid
import zipfile
ROOT=Path(__file__).resolve().parents[2]
def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
v=module('vmware71',ROOT/'tests/m71/vmware.py');s=module('session72',ROOT/'tests/m72/session.py')

def sparse(path,size):
    assert path.resolve().is_relative_to((ROOT/'build/m75/vmware').resolve()) and not path.exists() and path.suffix=='.img'
    with path.open('xb'):pass
    v.subprocess.run(['fsutil','sparse','setflag',str(path)],capture_output=True,check=True,creationflags=getattr(v.subprocess,'CREATE_NO_WINDOW',0))
    with path.open('r+b',buffering=0) as f:
        handle=ctypes.wintypes.HANDLE(msvcrt.get_osfhandle(f.fileno()))
        assert v.workspace.api.SetFilePointerEx(handle,size,None,0) and v.workspace.api.SetEndOfFile(handle)
    assert path.stat().st_size==size and v.workspace.allocated(path)<1024**2
    return path

def main():
    p=argparse.ArgumentParser();p.add_argument('--firmware',choices=['efi','bios'],default='efi');p.add_argument('--starter',action='store_true');a=p.parse_args()
    work=ROOT/'build/m75/vmware'/a.firmware/uuid.uuid4().hex;work.mkdir(parents=True)
    out=ROOT/'evidence/M7.5/vmware'/(a.firmware+('-starter' if a.starter else ''));out.mkdir(parents=True,exist_ok=True)
    before=v.old.v.inventory();report={'passed':False,'firmware':a.firmware,'checks':[]}
    try:
        disk=sparse(work/'owned-flat.img',20*1024**3)
        descriptor=work/'owned.vmdk';descriptor.write_bytes(b'version=1\nencoding="UTF-8"\nCID=fffffffe\nparentCID=ffffffff\ncreateType="monolithicFlat"\nRW 41943040 FLAT "owned-flat.img" 0\nddb.adapterType = "ide"\n')
        if a.starter:
            archive=ROOT/'dist/EngineOS-M75-VMware.zip'
            with zipfile.ZipFile(archive) as z:
                assert set(z.namelist())=={'EngineOS/'+n for n in ['EngineOS.vmx','EngineOS.vmdk','EngineOS-M75.iso','START-HERE.txt','LICENSE','THIRD-PARTY-NOTICES.txt','Inter-OFL.txt']};z.extractall(work)
            report['starter_sha256']=hashlib.sha256(archive.read_bytes()).hexdigest()
            original=(work/'EngineOS/EngineOS.vmx').read_text(encoding='utf-8')
            descriptor=work/'EngineOS/EngineOS.vmdk'
        g=v.Guest(work/'same-vm',descriptor,a.firmware)
        cfg=g.vmx.read_text(encoding='utf-8').replace('EngineOS-M71.iso','EngineOS-M75.iso')
        cfg='\n'.join(line for line in cfg.splitlines() if not line.startswith('bios.bootOrder'))+'\n'
        if a.starter:
            # Add only the test's owned loopback display and COM1 observation.
            observation='\n'.join(line for line in cfg.splitlines() if line.startswith(('serial0.','RemoteDisplay.')))+'\n'
            cfg=original+observation
            for file in (work/'EngineOS').iterdir():shutil.copyfile(file,g.folder/file.name)
            g.vmx=g.folder/'EngineOS.vmx'
        # Offline is deliberate: neither setup nor Files requires a network.
        cfg=cfg.replace('ethernet0.present = "TRUE"','ethernet0.present = "FALSE"');g.vmx.write_bytes(cfg.encode())
        with g:
            v.shot(g,out,'welcome');v.enter(g);v.tab(g);v.enter(g);v.enter(g);v.enter(g);v.enter(g);v.shot(g,out,'review');v.enter(g);g.wait('M6_INSTALL_PASS',timeout=240);g.wait('M73_VIEW step=0x0000000000000006');v.shot(g,out,'finish')
            # Restart is primary. Keep this same ISO, VMX and NVRAM connected.
            start=len(g.read());v.enter(g);g.wait('M75_USER_RESTART',offset=start);g.wait('M7_DESKTOP_READY',offset=start,timeout=180)
            assert 'M6_INSTALLED_VOLUME' in g.read()[start:] and 'M6_WIZARD_READY' not in g.read()[start:]
            if a.firmware=='efi':assert 'M75_CHAIN_DISK_LOADER' in g.read()[start:] and 'M75_INSTALLED_LOADER' in g.read()[start:]
            v.shot(g,out,'desktop-home')
            v.tab(g,8);v.enter(g);g.wait('M72_FOLDER_VISIT /Users/Owner/Documents')
            v.tab(g,4);v.enter(g)
            for char in 'vmware-folders':v.key(g,ord(char))
            v.enter(g);g.wait('M72_FOLDER_CREATED /Users/Owner/Documents/vmware-folders');v.shot(g,out,'real-folders');(out/'install.log').write_bytes(g.read().encode())
        if not a.starter:assert '/Users/Owner/Documents/vmware-folders' in s.directories(disk)
        nvram=list(g.folder.glob('*.nvram'))+list(g.folder.glob('nvram'));assert nvram or a.firmware=='bios'
        report['checks'].append('offline 20-GiB real installation to immediate persistent desktop, keyboard-only folder creation, independent metadata verification')
        # Reopen this exact VMX/NVRAM. Disconnect the CD, retaining the SAME hard disk.
        cfg=g.vmx.read_text(encoding='utf-8');cfg=re.sub(r'(?im)^ide1:0.startConnected\s*=.*$', 'ide1:0.startConnected = "FALSE"',cfg)
        assert 'bios.bootOrder' not in cfg;g.vmx.write_bytes(cfg.encode());g.installed=True
        for turn in range(2):
            g.output='';g.pipe=None;g.rfb=None
            with g:
                g.wait('M7_DESKTOP_READY');assert 'M6_INSTALLED_VOLUME' in g.read();v.shot(g,out,'detached-desktop-'+str(turn));(out/('restart-'+str(turn)+'.log')).write_bytes(g.read().encode())
            if not a.starter:assert '/Users/Owner/Documents/vmware-folders' in s.directories(disk)
        if a.starter:
            converted=work/'independent-starter.img';assert not converted.exists()
            tool=Path('C:/Program Files/qemu/qemu-img.exe')
            v.subprocess.run([str(tool),'convert','-f','vmdk','-O','raw','-S','4k',str(g.folder/'EngineOS.vmdk'),str(converted)],capture_output=True,check=True,timeout=120,creationflags=getattr(v.subprocess,'CREATE_NO_WINDOW',0))
            assert v.workspace.allocated(converted)<128*1024**2
            assert '/Users/Owner/Documents/vmware-folders' in s.directories(converted)
            report['independent_container_reader_sha256']=hashlib.sha256(tool.read_bytes()).hexdigest()
        report['checks'].append('same VMX and retained NVRAM, optical drive still present but disconnected; two unforced hard-disk boots to desktop; actual folder retained')
        meta=json.loads((ROOT/'dist/m75-build-manifest.json').read_text());report.update(passed=True,build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M75.iso']['sha256'],boot_order_forced=False,same_vm_reused=True,network_present=False)
    except Exception:
        report['error']=v.old.v.redact(traceback.format_exc());print(report['error'])
        if 'g' in locals():(out/'last-run.log').write_bytes(g.output.encode())
    report['existing_vm_inventory_unchanged']=before==v.old.v.inventory();report['passed']=report['passed'] and report['existing_vm_inventory_unchanged']
    (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M7.5 VMware '+a.firmware+' '+('PASS' if report['passed'] else 'FAIL'),flush=True)
    if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
