"""Operate only new headless VMware guests via authenticated loopback guest RFB."""
import argparse
import ctypes
import importlib.util
import json
import msvcrt
from pathlib import Path
import subprocess
import time
import traceback
import uuid
ROOT=Path(__file__).resolve().parents[2]
def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);value=importlib.util.module_from_spec(spec);spec.loader.exec_module(value);return value
old=module('m7vmware',ROOT/'tests/m7/vmware.py');old.VISUAL=True
n=module('m71native',Path(__file__).with_name('native.py'))
workspace=module('m71workspace',ROOT/'tests/m6/workspace.py')


class Guest(old.Guest):
    def __init__(self,*args,**kwargs):
        super().__init__(*args,**kwargs)
        cfg=self.vmx.read_text(encoding='utf-8').replace('EngineOS-M7.iso','EngineOS-M71.iso').replace('Engine OS M7 isolated qualification','Engine OS M7.1 isolated qualification')
        self.vmx.write_text(cfg,encoding='utf-8')


def sparse(path,size):
    assert path.resolve().is_relative_to((ROOT/'build/m71/vmware').resolve()) and path.suffix=='.img' and not path.exists()
    with path.open('xb'):pass
    subprocess.run(['fsutil','sparse','setflag',str(path)],capture_output=True,check=True,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    with path.open('r+b',buffering=0) as file:
        handle=ctypes.wintypes.HANDLE(msvcrt.get_osfhandle(file.fileno()))
        assert workspace.api.SetFilePointerEx(handle,size,None,0) and workspace.api.SetEndOfFile(handle)
    assert path.stat().st_size==size and workspace.allocated(path)<1024**2
    return path


def key(g,value,count=1):
    display=g.connect_display()
    for _ in range(count):display.press(value)
def enter(g):key(g,0xff0d)
def tab(g,count=1):key(g,0xff09,count)
def shot(g,out,name):g.connect_display().shot(out/(name+'.png'))


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--firmware',choices=['efi','bios'],default='efi');a=parser.parse_args()
    work=ROOT/'build/m71/vmware'/a.firmware/uuid.uuid4().hex;work.mkdir(parents=True)
    out=ROOT/'evidence/M7.1/vmware'/a.firmware;out.mkdir(parents=True,exist_ok=True)
    before=old.v.inventory();report={'passed':False,'checks':[],'firmware':a.firmware}
    try:
        disk=sparse(work/'owned-flat.img',20*1024**3);parts=n.fixture(disk)
        descriptor=work/'owned.vmdk';descriptor.write_bytes(b'version=1\nencoding="UTF-8"\nCID=fffffffe\nparentCID=ffffffff\ncreateType="monolithicFlat"\nRW 41943040 FLAT "owned-flat.img" 0\nddb.adapterType = "ide"\n')
        keep=n.q.digest(disk,parts[0][0]*512,(parts[0][1]-parts[0][0]+1)*512)
        with Guest(work/'utility',descriptor,a.firmware) as g:
            enter(g);enter(g);tab(g);enter(g);shot(g,out,'native-disk-manager')
            tab(g,3);enter(g);tab(g,7);enter(g);enter(g);g.wait('M71_DISK_REVIEW op=0x0000000000000005');shot(g,out,'format-review')
            # Safe default is Keep. Tab to Apply without consent must not start formatting.
            tab(g,2);enter(g);shot(g,out,'unchecked-format-refused')
            assert n.q.digest(disk,parts[0][0]*512,(parts[0][1]-parts[0][0]+1)*512)==keep
            tab(g);enter(g);tab(g,3);enter(g);tab(g,7);enter(g);enter(g);tab(g);enter(g);tab(g);start=len(g.read());enter(g);g.wait('M71_DISK_APPLIED op=0x0000000000000005',start,180)
            shot(g,out,'fat32-complete');(out/'utility.log').write_bytes(g.read().encode())
        assert n.q.digest(disk,parts[0][0]*512,(parts[0][1]-parts[0][0]+1)*512)==keep
        with disk.open('rb') as f:f.seek(parts[1][0]*512);assert f.read(512)[82:90]==b'FAT32   '
        report['checks'].append('real VMware native graphical disk manager and RFB PS/2 keyboard FAT32 format; unchecked consent refuses; unrelated 128-MiB partition bytes preserved')
        with Guest(work/'install',descriptor,a.firmware) as g:
            enter(g);enter(g);tab(g,2);enter(g);tab(g,2);enter(g);tab(g,3);enter(g);tab(g,5);enter(g)
            shot(g,out,'whole-disk-review');tab(g,2);enter(g);shot(g,out,'whole-disk-consent-required');tab(g);enter(g);tab(g,2);start=len(g.read());enter(g);g.wait('M6_INSTALL_PASS',start,240)
            shot(g,out,'verified-install-finish');(out/'install.log').write_bytes(g.read().encode())
        report['checks'].append('existing 20-GiB GPT disk replacement follows retained wizard choices and requires separate whole-disk consent before actual installation')
        with Guest(work/'desktop',descriptor,a.firmware,True) as g:
            meta=json.loads((ROOT/'dist/m71-build-manifest.json').read_text());g.wait('M7_DESKTOP_READY build='+meta['build_id']);g.wait('M7_DHCP_CONFIGURED',timeout=90)
            old.launch(g,2)
            for char in 'vmware m71':key(g,ord(char))
            start=len(g.read());g.connect_display().control('s');g.wait('M7_DOCUMENT_SAVED',start);shot(g,out,'real-installed-desktop')
            old.launch(g,5);old.press(g,b'\r');g.wait('M7_HTTPS_DOWNLOAD_SAVED',timeout=120);shot(g,out,'native-https-download');(out/'desktop.log').write_bytes(g.read().encode())
        assert old.s.document(disk,'/Users/Owner/Documents/Note1.txt')==b'vmware m71'
        assert old.s.document(disk,'/Users/Owner/Downloads/Download1.bin')==(ROOT/'LICENSE').read_bytes()
        with Guest(work/'restart',descriptor,a.firmware,True) as g:shot(g,out,'desktop-after-restart');(out/'restart.log').write_bytes(g.read().encode())
        assert old.s.document(disk,'/Users/Owner/Documents/Note1.txt')==b'vmware m71'
        report['checks'].append('detached desktop with real PS/2 Editor input/save, e1000e DHCP/checked HTTPS and exact persistent document/download bytes across restart')
        report.update(passed=True,build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M71.iso']['sha256'],disk_gib=20,preserved_partition_sha256=keep,input_scope='authenticated loopback RFB of owned guests; no host desktop automation')
    except Exception:
        report['error']=old.v.redact(traceback.format_exc());print(report['error'])
        if 'g' in locals():(out/'last-run.log').write_bytes(g.output.encode())
    report['existing_vm_inventory_unchanged']=before==old.v.inventory();report['passed']=report['passed'] and report['existing_vm_inventory_unchanged']
    (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M7.1 VMware '+a.firmware+' '+('PASS' if report['passed'] else 'FAIL'),flush=True)
    if not report['passed']:raise SystemExit(1)


if __name__=='__main__':main()
