"""Qualify M7 desktop data across distinct-build update, rollback and boot repair."""
import importlib.util
import json
from pathlib import Path
import struct
import traceback
import zlib
ROOT=Path(__file__).resolve().parents[2]
def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);value=importlib.util.module_from_spec(spec);spec.loader.exec_module(value);return value
r=module('m7resources',Path(__file__).with_name('resources.py'))
s,i,h,q=r.s,r.i,r.h,r.q


def activation(path):
    candidates=[]
    with path.open('rb') as file:
        for sector in [41,42]:
            file.seek(sector*512);raw=file.read(512)
            if raw[:8]==b'ENGINEI6' and zlib.crc32(raw[:508])==struct.unpack_from('<I',raw,508)[0] and struct.unpack_from('<I',raw,72)[0]==2:
                candidates.append((struct.unpack_from('<Q',raw,80)[0],raw))
    assert candidates;return max(candidates)[1]


def data_extent(path):
    with path.open('rb') as file:
        file.seek(512);header=file.read(512);file.seek(struct.unpack_from('<Q',header,72)[0]*512+4*128)
        entry=file.read(128);first,last=struct.unpack_from('<QQ',entry,32)
    return first*512,(last-first+1)*512


def flow(guest,choice):
    h.next(guest,choice);h.next(guest);h.next(guest,2);start=len(guest.log());h.next(guest,1);guest.wait('M6_MAINTENANCE_PASS',start,240)


def main():
    work=q.new_work('m7-maintenance');out=ROOT/'evidence/M7/maintenance';out.mkdir(parents=True,exist_ok=True)
    report={'passed':False,'m7_complete':False,'checks':[]}
    try:
        base=json.loads((ROOT/'dist/m7-build-manifest.json').read_text(encoding='utf-8'))
        custom=json.loads((ROOT/'dist/m7night-build-manifest.json').read_text(encoding='utf-8'))
        assert base['build_id']!=custom['build_id']
        common=set(base['source_sha256'])&set(custom['source_sha256'])
        assert all(base['source_sha256'][p]==custom['source_sha256'][p] for p in common)
        for app in ['editor','calculator']:assert base['artifacts']['EngineOS-M7-'+app+'.elf']['sha256']==custom['artifacts']['EngineOS-M7NIGHT-'+app+'.elf']['sha256']
        disk=q.sparse_disk(work/'owned.img',2*1024**3)
        with q.Guest(work/'install',disk,image_name='EngineOS-M7') as guest:
            h.next(guest);h.next(guest);h.next(guest,2);h.next(guest,3);h.next(guest,5);start=len(guest.log());h.next(guest,1);guest.wait('M6_INSTALL_PASS',start,240)
        with q.Guest(work/'baseline',disk,media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY') as guest:
            r.start_menu(guest,2)
            for character in 'kept through update':h.key(guest,'spc' if character==' ' else character)
            i.control(guest,'s');guest.wait('M7_DOCUMENT_SAVED')
            r.start_menu(guest,3);h.key(guest,'tab',3);h.key(guest,'ret');h.key(guest,'tab',4);h.key(guest,'ret');h.key(guest,'tab',7);h.key(guest,'ret');guest.wait('M7_PREFERENCES_SAVED')
            (out/'baseline.log').write_bytes(guest.log().encode())
        assert s.document(disk,'/Users/Owner/Documents/Note1.txt')==b'kept through update'
        start,length=data_extent(disk);saved=q.digest(disk,start,length)
        with q.Guest(work/'update',disk,image_name='EngineOS-M7NIGHT') as guest:
            flow(guest,3);(out/'update.log').write_bytes(guest.log().encode())
        assert q.digest(disk,start,length)==saved and activation(disk)[24:56].decode()==custom['build_id']
        with q.Guest(work/'updated-desktop',disk,media='installed',image_name='EngineOS-M7NIGHT',ready='M7_DESKTOP_READY') as guest:
            assert 'M7_DESKTOP_READY build='+custom['build_id'] in guest.log()
            h.shot(guest,out,'updated-desktop');(out/'updated.log').write_bytes(guest.log().encode())
        assert s.document(disk,'/Users/Owner/Documents/Note1.txt')==b'kept through update'
        assert q.digest(disk,start,length)==saved
        with q.Guest(work/'rollback',disk,image_name='EngineOS-M7') as guest:
            flow(guest,4);(out/'rollback.log').write_bytes(guest.log().encode())
        assert q.digest(disk,start,length)==saved and activation(disk)[24:56].decode()==base['build_id']
        with disk.open('r+b') as file:
            file.seek(2112*512+511);old=file.read(1);file.seek(-1,1);file.write(bytes([old[0]^255]))
        refused=False
        try:
            with q.Guest(work/'corrupt-bios',disk,firmware='bios',media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY'):pass
        except RuntimeError:refused='BIOS_REJECT: kernel raw CRC' in (work/'corrupt-bios/serial.log').read_text(encoding='utf-8',errors='replace')
        assert refused
        with q.Guest(work/'repair',disk,image_name='EngineOS-M7') as guest:
            flow(guest,2);(out/'repair.log').write_bytes(guest.log().encode())
        assert q.digest(disk,start,length)==saved
        with q.Guest(work/'repaired-desktop',disk,firmware='bios',media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY') as guest:
            assert 'M7_DESKTOP_READY build='+base['build_id'] in guest.log()
            h.shot(guest,out,'repaired-bios-desktop');(out/'repaired.log').write_bytes(guest.log().encode())
        assert q.digest(disk,start,length)==saved and s.document(disk,'/Users/Owner/Documents/Note1.txt')==b'kept through update'
        report.update(passed=True,build_id=base['build_id'],update_build_id=custom['build_id'],iso_sha256=base['artifacts']['EngineOS-M7.iso']['sha256'],data_partition_sha256=saved,
                      scope='native verified boot-slot update/rollback/repair between identical app ABIs; no arbitrary package/data migration claim')
        report['checks']=['desktop creates exact persistent document and personal preferences','distinct original kernel build updates inactive slot and enters its desktop','entire data partition and app bytes preserved through update and reboot','rollback restores original build','deliberately corrupted BIOS payload refuses boot','ISO repair restores detached BIOS desktop without changing any data-partition bytes']
    except Exception:
        report['error']=q.clean(traceback.format_exc());print(report['error'])
        if 'guest' in locals():(out/'last-run.log').write_bytes(guest.log().encode())
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('M7 native maintenance '+('PASS' if report['passed'] else 'FAIL'),flush=True)
    if not report['passed']:raise SystemExit(1)


if __name__=='__main__':main()
