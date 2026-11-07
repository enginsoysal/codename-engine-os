"""Native empty GPT initialization, typed creation, metadata clearing and GPT repair."""
import importlib.util
import json
from pathlib import Path
import struct
import traceback
import zlib
ROOT=Path(__file__).resolve().parents[2]
def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);value=importlib.util.module_from_spec(spec);spec.loader.exec_module(value);return value
n=module('m71native',Path(__file__).with_name('native.py'));d=module('m71disk',Path(__file__).with_name('disk.py'))
h,q,s=n.h,n.q,n.s
class FileDisk:
    def __init__(self,path):self.path=path;self.sectors=path.stat().st_size//512
    def raw(self,lba,length=512):
        with self.path.open('rb') as f:f.seek(lba*512);return f.read(length)


def main():
    out=ROOT/'evidence/M7.1/advanced';out.mkdir(parents=True,exist_ok=True);work=q.new_work('m71-advanced');report={'passed':False,'checks':[]}
    try:
        disk=q.sparse_disk(work/'blank.img',2*1024**3);before=q.digest(disk)
        with q.Guest(work/'initialize',disk,image_name='EngineOS-M71') as g:
            p=s.Pointer(g);n.next_step(p);n.click(p,300,240);n.manager(p);n.click(p,300,558);n.click(p,300,545);assert q.digest(disk)==before
            n.click(p,300,558);n.confirm(g,p,2);assert d.gpt(FileDisk(disk))==[];h.shot(g,out,'new-empty-gpt')
            n.row(p,0);n.click(p,300,507);n.click(p,300,300);h.key(g,'backspace',2)
            for char in '128':h.key(g,char)
            n.click(p,400,395);h.key(g,'backspace',4)
            for char in 'work':h.key(g,char)
            h.shot(g,out,'typed-size-and-name');n.click(p,320,510);n.confirm(g,p,3)
            assert d.gpt(FileDisk(disk))==[(2048,264191)]
            with disk.open('rb') as f:f.seek(1024+56);assert f.read(8).decode('utf-16-le')=='work'
            h.shot(g,out,'created-named-partition');before=q.digest(disk);n.click(p,520,558);n.click(p,300,545);assert q.digest(disk)==before
            n.click(p,520,558);n.confirm(g,p,1);h.shot(g,out,'cleared-metadata');raw=FileDisk(disk)
            assert raw.raw(0,64*512)==bytes(64*512) and raw.raw(raw.sectors-33,33*512)==bytes(33*512)
            (out/'initialize.log').write_bytes(g.log().encode())
        report['checks'].append('cancelled GPT initialization/clearing preserve full image; confirmed new GPT, typed 128-MiB named partition and quick metadata clear operate through real GUI')
        disk=q.sparse_disk(work/'damaged.img',2*1024**3);parts=n.fixture(disk);keep=q.digest(disk,parts[0][0]*512,(parts[0][1]-parts[0][0]+1)*512)
        with disk.open('r+b') as f:f.seek(512+16);value=f.read(1);f.seek(-1,1);f.write(bytes([value[0]^1]))
        with q.Guest(work/'repair',disk,image_name='EngineOS-M71') as g:
            p=s.Pointer(g);n.next_step(p);n.click(p,300,240);n.manager(p);h.shot(g,out,'damaged-primary-read-only');n.click(p,680,558);n.click(p,350,420);n.confirm(g,p,8);h.shot(g,out,'repaired-gpt');(out/'repair.log').write_bytes(g.log().encode())
        assert d.gpt(FileDisk(disk))==parts and q.digest(disk,parts[0][0]*512,(parts[0][1]-parts[0][0]+1)*512)==keep
        report['checks'].append('actual corrupted primary GPT refuses ordinary editing; native explicit backup-copy repair restores independently CRC-checked tables and retains foreign payload')
        meta=json.loads((ROOT/'dist/m71-build-manifest.json').read_text());report.update(passed=True,build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M71.iso']['sha256'])
    except Exception:
        report['error']=q.clean(traceback.format_exc());print(report['error'])
        if 'g' in locals():(out/'last-run.log').write_bytes(g.log().encode())
    (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M7.1 advanced disk tasks '+('PASS' if report['passed'] else 'FAIL'),flush=True)
    if not report['passed']:raise SystemExit(1)


if __name__=='__main__':main()
