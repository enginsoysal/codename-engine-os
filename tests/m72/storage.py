"""Retain disk-manager safety and whole-disk consent in the redesigned wizard."""
import importlib.util
import json
from pathlib import Path
import traceback
ROOT=Path(__file__).resolve().parents[2]
def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
s=module('session72',Path(__file__).with_name('session.py'));n=module('native71',ROOT/'tests/m71/native.py');q,h=s.q,s.h
def main():
    out=ROOT/'evidence/M7.2/storage';out.mkdir(parents=True,exist_ok=True);work=q.new_work('m72-storage');report={'passed':False,'checks':[]}
    try:
        disk=q.sparse_disk(work/'owned.img',2*1024**3);parts=n.fixture(disk);before=q.digest(disk);keep=q.digest(disk,parts[0][0]*512,(parts[0][1]-parts[0][0]+1)*512)
        with q.Guest(work/'utility',disk,image_name='EngineOS-M72') as g:
            p=s.s.Pointer(g);p.click(1090,684);p.click(300,215);p.click(1090,684);assert q.digest(disk)==before;h.shot(g,out,'existing-disk-guidance')
            p.click(300,465);h.shot(g,out,'disk-manager');n.row(p,1);p.click(680,507);p.click(400,320);n.confirm(g,p,5);h.shot(g,out,'fat32-formatted')
            assert q.digest(disk,parts[0][0]*512,(parts[0][1]-parts[0][0]+1)*512)==keep
            with disk.open('rb') as f:f.seek(parts[1][0]*512);assert f.read(512)[82:90]==b'FAT32   '
            (out/'utility.log').write_bytes(g.log().encode())
        report['checks'].append('nonblank disk Next gives guidance without writes; actual FAT32 format through native partition manager preserves unrelated partition bytes')
        before=q.digest(disk)
        with q.Guest(work/'replace',disk,image_name='EngineOS-M72') as g:
            p=s.s.Pointer(g);p.click(1090,684);p.click(300,215);p.click(600,465);p.click(1090,684);p.click(1090,684);p.click(1090,684);h.shot(g,out,'erase-review');p.click(1090,684);assert q.digest(disk)==before;h.shot(g,out,'unchecked-erase-refused')
            p.click(400,455);p.click(1090,684);g.wait('M6_INSTALL_PASS',timeout=240);h.shot(g,out,'verified-install-finish');p.click(1090,684);g.wait('M72_FINISH_TO_PERSISTENT_DESKTOP');g.wait('M7_DESKTOP_READY');h.shot(g,out,'replacement-desktop');(out/'replace.log').write_bytes(g.log().encode())
        report['checks'].append('whole existing-disk installation refuses unchecked review with exact disk hash unchanged; separate consent permits verified installation and immediate real desktop')
        meta=json.loads((ROOT/'dist/m72-build-manifest.json').read_text());report.update(passed=True,build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M72.iso']['sha256'],unrelated_partition_sha256=keep)
    except Exception:
        report['error']=q.clean(traceback.format_exc());print(report['error'])
        if 'g' in locals():(out/'last-run.log').write_bytes(g.log().encode())
    (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M7.2 storage '+('PASS' if report['passed'] else 'FAIL'))
    if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
