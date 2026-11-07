"""Actual installer disk management and existing-disk replacement, on disposable guests."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import time
import traceback
import uuid
import zlib
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('session7',ROOT/'tests/m7/session.py');s=importlib.util.module_from_spec(spec);spec.loader.exec_module(s)
h,q=s.h,s.q


def fixture(path,mbr=False):
    sectors=path.stat().st_size//512
    parts=[(2048,264191),(264192,526335)]
    with path.open('r+b') as f:
        boot=bytearray(512);boot[510:]=b'\x55\xaa'
        if mbr:
            boot[:440]=b'M'*440
            for i,(first,last) in enumerate(parts):struct.pack_into('<B3sB3sII',boot,446+i*16,0,bytes(3),7,bytes(3),first,last-first+1)
        else:
            boot[450]=0xee;struct.pack_into('<II',boot,454,1,min(sectors-1,0xffffffff));table=bytearray(16384)
            for i,(first,last) in enumerate(parts):
                off=i*128;table[off:off+16]=uuid.UUID('ebd0a0a2-b9e5-4433-87c0-68b6b72699c7').bytes_le;table[off+16:off+32]=uuid.uuid4().bytes_le;struct.pack_into('<QQ',table,off+32,first,last);name=('Keep' if not i else 'Manage').encode('utf-16-le');table[off+56:off+56+len(name)]=name
            guid=uuid.uuid4().bytes_le
            for copy in [0,1]:
                header=bytearray(512);header[:8]=b'EFI PART';struct.pack_into('<II',header,8,0x10000,92);struct.pack_into('<QQQQ',header,24,sectors-1 if copy else 1,1 if copy else sectors-1,34,sectors-34);header[56:72]=guid;struct.pack_into('<QIII',header,72,sectors-33 if copy else 2,128,128,zlib.crc32(table));struct.pack_into('<I',header,16,zlib.crc32(header[:92]));f.seek((sectors-1 if copy else 1)*512);f.write(header);f.seek((sectors-33 if copy else 2)*512);f.write(table)
        f.seek(0);f.write(boot)
        for index,(first,last) in enumerate(parts):
            f.seek(first*512);f.write(('FOREIGN-KEEP' if not index else 'REPLACE-TARGET').encode().ljust(4096,b'!'));f.seek((last-7)*512);f.write(bytes([index+31])*4096)
    return parts


def click(p,x,y):p.click(x,y);time.sleep(.12)
def next_step(p):click(p,900 if p.guest.firmware=='bios' else 1090,684)
def manager(p):click(p,300,490)
def row(p,index):click(p,300,277+43*index)
def confirm(g,p,operation):
    click(p,400,490);start=len(g.log());click(p,510,545);g.wait('M71_DISK_APPLIED op=0x'+format(operation,'016x'),start,180)
def original_entry(path,index):
    with path.open('rb') as f:f.seek(1024+index*128);return f.read(128)


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--firmware',choices=['uefi','bios'],default='uefi');parser.add_argument('--table',choices=['gpt','mbr'],default='gpt');a=parser.parse_args()
    work=q.new_work('m71-native');out=ROOT/'evidence/M7.1'/('native-'+a.firmware+'-'+a.table);out.mkdir(parents=True,exist_ok=True)
    report={'passed':False,'checks':[],'firmware':a.firmware,'table':a.table}
    try:
        disk=q.sparse_disk(work/'owned.img',2*1024**3);parts=fixture(disk,a.table=='mbr');keep=q.digest(disk,parts[0][0]*512,(parts[0][1]-parts[0][0]+1)*512);before=q.digest(disk)
        entry=original_entry(disk,0) if a.table=='gpt' else None
        with q.Guest(work/'utility',disk,a.firmware,image_name='EngineOS-M71') as g:
            p=s.Pointer(g);next_step(p);next_step(p);h.shot(g,out,'no-selection-readable');assert q.digest(disk)==before
            click(p,300,240);next_step(p);h.shot(g,out,'existing-disk-guidance');assert q.digest(disk)==before
            manager(p);h.shot(g,out,'partitions-and-free-space')
            row(p,1);click(p,680,507);click(p,400,320);g.wait('M71_DISK_REVIEW op=0x0000000000000005')
            click(p,510,545);h.shot(g,out,'unchecked-format-refused');assert q.digest(disk)==before
            click(p,300,545);assert q.digest(disk)==before
            row(p,1);click(p,680,507);click(p,400,320);confirm(g,p,5);h.shot(g,out,'fat32-formatted')
            assert q.digest(disk,parts[0][0]*512,(parts[0][1]-parts[0][0]+1)*512)==keep
            with disk.open('rb') as f:f.seek(parts[1][0]*512);boot=f.read(512)
            assert boot[82:90]==b'FAT32   ' and boot[510:]==b'\x55\xaa'
            if entry is not None:assert original_entry(disk,0)==entry
            row(p,1);click(p,680,507);click(p,400,380);confirm(g,p,6);h.shot(g,out,'native-formatted')
            with disk.open('rb') as f:f.seek(parts[1][0]*512);assert f.read(8)==b'ENGINEV6'
            row(p,1);click(p,500,507);confirm(g,p,4);h.shot(g,out,'partition-deleted')
            # The removed second partition becomes free space; create a real new data partition there.
            row(p,1);click(p,300,507);click(p,320,510);confirm(g,p,3);h.shot(g,out,'partition-created')
            assert q.digest(disk,parts[0][0]*512,(parts[0][1]-parts[0][0]+1)*512)==keep
            if entry is not None:assert original_entry(disk,0)==entry
            (out/'utility.log').write_bytes(g.log().encode())
        report['checks'].append('no selection and nonblank disk stay in guided setup; unchecked confirmation and cancel preserve entire disk; FAT32/native format, delete and create preserve unrelated partition bytes/entry')
        before=q.digest(disk)
        with q.Guest(work/'replace',disk,a.firmware,image_name='EngineOS-M71') as g:
            p=s.Pointer(g);next_step(p);click(p,300,240);click(p,650,490);next_step(p);next_step(p);next_step(p)
            h.shot(g,out,'whole-disk-review');next_step(p);assert q.digest(disk)==before
            h.shot(g,out,'whole-disk-consent-required');click(p,430,480);start=len(g.log());next_step(p);g.wait('M6_INSTALL_PASS',start,240);h.shot(g,out,'installation-finish');(out/'install.log').write_bytes(g.log().encode())
        report['checks'].append('same previously partitioned disk installs only after separate whole-disk review/consent; actual verified system and app copy completes')
        with q.Guest(work/'desktop',disk,a.firmware,media='installed',image_name='EngineOS-M71',ready='M7_DESKTOP_READY') as g:
            p=s.Pointer(g);click(p,55,210)
            for char in 'm71 installed':h.key(g,'spc' if char==' ' else char)
            g.qmp('send-key',{'keys':[{'type':'qcode','data':'ctrl'},{'type':'qcode','data':'s'}],'hold-time':50});g.wait('M7_DOCUMENT_SAVED');h.shot(g,out,'real-installed-desktop');(out/'desktop.log').write_bytes(g.log().encode())
        assert s.document(disk,'/Users/Owner/Documents/Note1.txt')==b'm71 installed'
        with q.Guest(work/'restart',disk,a.firmware,media='installed',image_name='EngineOS-M71',ready='M7_DESKTOP_READY') as g:h.shot(g,out,'desktop-after-restart');(out/'restart.log').write_bytes(g.log().encode())
        assert s.document(disk,'/Users/Owner/Documents/Note1.txt')==b'm71 installed'
        report['checks'].append('media-detached desktop, actual original Editor save and independent exact-byte check across restart')
        # Existing Engine OS partitions are protected; clearing is a distinct whole-disk choice.
        before=q.digest(disk)
        with q.Guest(work/'protected',disk,a.firmware,image_name='EngineOS-M71') as g:
            p=s.Pointer(g);next_step(p);click(p,300,240);manager(p);row(p,0);click(p,500,507);h.shot(g,out,'system-partition-protected');assert q.digest(disk)==before;(out/'protected.log').write_bytes(g.log().encode())
        report['checks'].append('existing Engine OS system partition deletion is refused without changing any disk bytes')
        meta=json.loads((ROOT/'dist/m71-build-manifest.json').read_text());report.update(passed=True,build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M71.iso']['sha256'],preserved_partition_sha256=keep)
    except Exception:
        report['error']=q.clean(traceback.format_exc());print(report['error'])
        if 'g' in locals():(out/'last-run.log').write_bytes(g.log().encode())
    (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M7.1 native '+a.firmware+' '+a.table+' '+('PASS' if report['passed'] else 'FAIL'),flush=True)
    if not report['passed']:raise SystemExit(1)


if __name__=='__main__':main()
