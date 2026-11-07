"""Upgrade an actual M7 desktop to M7.1, then roll back/repair without data loss."""
import importlib.util
import json
from pathlib import Path
import traceback
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('maintenance7',ROOT/'tests/m7/maintenance.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
s,h,q=m.s,m.h,m.q


def main():
    out=ROOT/'evidence/M7.1/maintenance';out.mkdir(parents=True,exist_ok=True);work=q.new_work('m71-maintenance');report={'passed':False,'checks':[]}
    try:
        disk=q.sparse_disk(work/'owned.img',2*1024**3)
        old=json.loads((ROOT/'dist/m7-build-manifest.json').read_text());new=json.loads((ROOT/'dist/m71-build-manifest.json').read_text())
        with q.Guest(work/'m7-install',disk,image_name='EngineOS-M7') as g:
            h.next(g);h.next(g);h.next(g,2);h.next(g,3);h.next(g,5);start=len(g.log());h.next(g,1);g.wait('M6_INSTALL_PASS',start,240)
        with q.Guest(work/'m7-desktop',disk,media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY') as g:
            p=s.Pointer(g);p.click(55,210)
            for char in 'keep my m7 data':h.key(g,'spc' if char==' ' else char)
            m.i.control(g,'s');g.wait('M7_DOCUMENT_SAVED');(out/'m7-desktop.log').write_bytes(g.log().encode())
        first,length=m.data_extent(disk);saved=q.digest(disk,first,length)
        with q.Guest(work/'update',disk,image_name='EngineOS-M71') as g:m.flow(g,3);h.shot(g,out,'update-finish');(out/'update.log').write_bytes(g.log().encode())
        assert q.digest(disk,first,length)==saved and m.activation(disk)[24:56].decode()==new['build_id']
        with q.Guest(work/'m71-desktop',disk,media='installed',image_name='EngineOS-M71',ready='M7_DESKTOP_READY') as g:
            assert 'M7_DESKTOP_READY build='+new['build_id'] in g.log();h.shot(g,out,'upgraded-desktop');(out/'m71-desktop.log').write_bytes(g.log().encode())
        assert s.document(disk,'/Users/Owner/Documents/Note1.txt')==b'keep my m7 data' and q.digest(disk,first,length)==saved
        with q.Guest(work/'rollback',disk,image_name='EngineOS-M71') as g:m.flow(g,4);(out/'rollback.log').write_bytes(g.log().encode())
        assert m.activation(disk)[24:56].decode()==old['build_id'] and q.digest(disk,first,length)==saved
        with q.Guest(work/'rolled-back',disk,media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY') as g:
            assert 'M7_DESKTOP_READY build='+old['build_id'] in g.log();h.shot(g,out,'rolled-back-desktop')
        with disk.open('r+b') as f:f.seek(2112*512+511);value=f.read(1);f.seek(-1,1);f.write(bytes([value[0]^255]))
        refused=False
        try:
            with q.Guest(work/'damaged-bios',disk,firmware='bios',media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY'):pass
        except RuntimeError:refused='BIOS_REJECT: kernel raw CRC' in (work/'damaged-bios/serial.log').read_text(encoding='utf-8',errors='replace')
        assert refused
        with q.Guest(work/'repair',disk,image_name='EngineOS-M71') as g:m.flow(g,2);(out/'repair.log').write_bytes(g.log().encode())
        with q.Guest(work/'repaired',disk,firmware='bios',media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY') as g:h.shot(g,out,'repaired-desktop');(out/'repaired.log').write_bytes(g.log().encode())
        assert q.digest(disk,first,length)==saved and s.document(disk,'/Users/Owner/Documents/Note1.txt')==b'keep my m7 data'
        report.update(passed=True,build_id=new['build_id'],iso_sha256=new['artifacts']['EngineOS-M71.iso']['sha256'],previous_build_id=old['build_id'],data_partition_sha256=saved)
        report['checks']=['real M7 desktop creates persistent document','M7.1 Update stages distinct kernel and boots detached desktop with unchanged complete data partition','M7.1 Rollback restores original M7 build and exact data','deliberately damaged owned BIOS payload refuses boot; M7.1 ISO repairs stored active slot without data changes']
    except Exception:
        report['error']=q.clean(traceback.format_exc());print(report['error'])
        if 'g' in locals():(out/'last-run.log').write_bytes(g.log().encode())
    (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M7.1 maintenance '+('PASS' if report['passed'] else 'FAIL'),flush=True)
    if not report['passed']:raise SystemExit(1)


if __name__=='__main__':main()
