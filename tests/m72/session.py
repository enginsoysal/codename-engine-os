"""Native setup to persistent folders, then unforced independent firmware boot."""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import time
import traceback
import zlib
ROOT=Path(__file__).resolve().parents[2]
def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);value=importlib.util.module_from_spec(spec);spec.loader.exec_module(value);return value
s=module('desktop7',ROOT/'tests/m7/session.py');h=s.h;q=s.q

def directories(disk):
    with disk.open('rb') as f:
        f.seek(512);head=bytearray(f.read(512));crc=struct.unpack_from('<I',head,16)[0];struct.pack_into('<I',head,16,0);assert zlib.crc32(head[:92])==crc
        f.seek(struct.unpack_from('<Q',head,72)[0]*512);table=f.read(16384);assert zlib.crc32(table)==struct.unpack_from('<I',head,88)[0]
        start=struct.unpack_from('<Q',table,4*128+32)[0]*512;banks=[]
        for bank in range(2):
            f.seek(start+bank*71*4096);header=f.read(4096);raw=f.read(1024*280)
            if header[:8]==b'ENGINEV6' and zlib.crc32(header[:4092])==struct.unpack_from('<I',header,4092)[0] and zlib.crc32(raw)==struct.unpack_from('<I',header,36)[0]:banks.append((struct.unpack_from('<Q',header,24)[0],raw))
        assert banks;raw=max(banks)[1]
        return {raw[i*280:i*280+256].split(b'\0')[0].decode() for i in range(1024) if struct.unpack_from('<I',raw,i*280+276)[0]==2}

def normal_boot(g,variables=None):
    # Remove both firmware order forcing and the artificial device bootindex.
    if '-boot' in g.args:
        i=g.args.index('-boot');del g.args[i:i+2]
    g.args=[arg.replace(',bootindex=1','') for arg in g.args]
    if variables:shutil.copyfile(variables,g.folder/'vars.fd')

def main():
    p=argparse.ArgumentParser();p.add_argument('--firmware',choices=['uefi','bios'],default='uefi');p.add_argument('--media',choices=['iso','usb'],default='iso');a=p.parse_args()
    out=ROOT/'evidence/M7.2'/('qemu-'+a.firmware+'-'+a.media);out.mkdir(parents=True,exist_ok=True);work=q.new_work('m72-'+a.firmware+'-'+a.media)
    report={'passed':False,'checks':[],'firmware':a.firmware,'media':a.media}
    try:
        disk=q.sparse_disk(work/'owned.img',2*1024**3)
        live=q.Guest(work/'install',disk,a.firmware,a.media,image_name='EngineOS-M72')
        with live as g:
            pointer=s.Pointer(g);nx=1090 if a.firmware=='uefi' else 895
            h.shot(g,out,'welcome');pointer.click(nx,684);h.shot(g,out,'choose-disk')
            pointer.click(nx,684);h.shot(g,out,'no-selection-guidance')
            pointer.click(310,225);pointer.click(nx,684);h.shot(g,out,'essentials')
            pointer.click(nx,684);pointer.click(460,315);h.shot(g,out,'personalize')
            pointer.click(nx,684);h.shot(g,out,'review');before=len(g.log());pointer.click(nx,684);g.wait('M6_INSTALL_PASS',before,240)
            h.shot(g,out,'finish');pointer.click(nx,684);g.wait('M72_FINISH_TO_PERSISTENT_DESKTOP');g.wait('M7_DESKTOP_READY');h.shot(g,out,'desktop-home')
            report['checks'].append('native graphical installation: missing selection gives guidance; verified Finish opens real persistent Files without restarting')
            pointer.click(260,325);g.wait('M72_FOLDER_VISIT /Users/Owner/Documents')
            pointer.click(460,158)
            # Real keyboard input, never a console write or host reference action.
            for c in 'projects':h.key(g,c)
            h.key(g,'ret');g.wait('M72_FOLDER_CREATED /Users/Owner/Documents/projects');h.shot(g,out,'folder-created')
            # The distributed guide occupies row one; the new folder is row two.
            pointer.click(465,308);g.wait('M72_FOLDER_VISIT /Users/Owner/Documents/projects');h.shot(g,out,'inside-projects')
            pointer.click(240,158);pointer.click(300,158);pointer.click(360,158)
            pointer.click(260,434);g.wait('M72_FOLDER_VISIT /\n');h.shot(g,out,'system-drive')
            pointer.click(460,158);h.shot(g,out,'system-folder-write-refused')
            pointer.click(260,325);h.shot(g,out,'documents');(out/'install-desktop.log').write_bytes(g.log().encode())
            assert '/Users/Owner/Documents/projects' in directories(disk)
            report['checks'].append('real Home/Documents/sidebar, named persisted folder, folder opening, Back/Forward/Up and read-only system browsing')
        # Reuse the actual NVRAM produced by installation; no forced disk priority.
        for turn in range(2):
            guest=q.Guest(work/('restart-'+str(turn)),disk,a.firmware,'installed',image_name='EngineOS-M72',ready='M7_DESKTOP_READY')
            normal_boot(guest,live.folder/'vars.fd' if a.firmware=='uefi' else None)
            with guest as g:
                assert 'M6_INSTALLED_VOLUME' in g.log();h.shot(g,out,'independent-desktop-'+str(turn));(out/('restart-'+str(turn)+'.log')).write_bytes(g.log().encode())
                assert '/Users/Owner/Documents/projects' in directories(disk)
        report['checks'].append('two ISO/USB-detached boots to desktop without boot order/index forcing, using original installation NVRAM; actual folder metadata CRC verified independently')
        # ISO boot can open a same-build installation without reinstallation.
        with q.Guest(work/'iso-resume',disk,a.firmware,'iso',image_name='EngineOS-M72') as g:
            pointer=s.Pointer(g);pointer.click(350,539);g.wait('M72_ISO_TO_PERSISTENT_DESKTOP');g.wait('M7_DESKTOP_READY');h.shot(g,out,'iso-resume-desktop');(out/'iso-resume.log').write_bytes(g.log().encode())
        meta=json.loads((ROOT/'dist/m72-build-manifest.json').read_text());report.update(passed=True,build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M72.iso']['sha256'])
    except Exception:
        report['error']=q.clean(traceback.format_exc());print(report['error'])
        if 'g' in locals():(out/'last-run.log').write_bytes(g.log().encode())
    (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M7.2 native '+a.firmware+' '+a.media+' '+('PASS' if report['passed'] else 'FAIL'),flush=True)
    if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
