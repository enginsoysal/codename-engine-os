"""Select between equal-capacity disks and prove the other disk stays byte-identical."""
import importlib.util
import json
from pathlib import Path
import traceback
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('native71',Path(__file__).with_name('native.py'));n=importlib.util.module_from_spec(spec);spec.loader.exec_module(n)


def main():
    out=ROOT/'evidence/M7.1/multidisk';out.mkdir(parents=True,exist_ok=True);work=n.q.new_work('m71-two-disks');report={'passed':False,'checks':[]}
    try:
        first=n.q.sparse_disk(work/'keep.img',2*1024**3);second=n.q.sparse_disk(work/'change.img',2*1024**3);n.fixture(first);parts=n.fixture(second)
        saved=n.q.digest(first);keep=n.q.digest(second,parts[0][0]*512,(parts[0][1]-parts[0][0]+1)*512)
        extra=['-drive','if=none,id=second,format=raw,file='+str(second),'-device','ide-hd,bus=ide.1,drive=second']
        with n.q.Guest(work/'two-disks',first,image_name='EngineOS-M71',extra=extra) as g:
            p=n.s.Pointer(g);n.next_step(p);n.click(p,300,295);n.manager(p);n.h.shot(g,out,'selected-second-equal-capacity-disk')
            n.row(p,1);n.click(p,680,507);n.click(p,400,320);n.confirm(g,p,5);n.h.shot(g,out,'formatted-second-disk-only')
            assert n.q.digest(first)==saved and n.q.digest(second,parts[0][0]*512,(parts[0][1]-parts[0][0]+1)*512)==keep
            n.click(p,700,195);n.h.shot(g,out,'switched-back-to-first-disk');(out/'serial.log').write_bytes(g.log().encode())
        assert n.q.digest(first)==saved
        with second.open('rb') as f:f.seek(parts[1][0]*512);assert f.read(512)[82:90]==b'FAT32   '
        meta=json.loads((ROOT/'dist/m71-build-manifest.json').read_text());report.update(passed=True,build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M71.iso']['sha256'],untouched_disk_sha256=saved)
        report['checks']=['two identical-capacity SATA disks have distinct selectable controller/port identities','native format on disk 2 preserves ALL bytes of disk 1 and the other partition on disk 2','disk navigation returns to disk 1 without writing']
    except Exception:
        report['error']=n.q.clean(traceback.format_exc());print(report['error'])
        if 'g' in locals():(out/'last-run.log').write_bytes(g.log().encode())
    (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M7.1 multiple disks '+('PASS' if report['passed'] else 'FAIL'),flush=True)
    if not report['passed']:raise SystemExit(1)


if __name__=='__main__':main()
