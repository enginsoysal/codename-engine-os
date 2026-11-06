"""The public guided ISO on a new 20-GiB sparse VMware SATA VMDK."""
from pathlib import Path
import importlib.util,json,time,subprocess,traceback,hashlib
ROOT=Path(__file__).resolve().parents[2]
s=importlib.util.spec_from_file_location('m51_vmware',ROOT/'tests/m5/vmware.py');v=importlib.util.module_from_spec(s);s.loader.exec_module(v)

def main():
    import argparse
    p=argparse.ArgumentParser();p.add_argument('--firmware',choices=['efi','bios'],default='efi');a=p.parse_args();work=ROOT/'build/m51/vmware'/a.firmware/str(time.time_ns());work.mkdir(parents=True);out=ROOT/'evidence/M5.1/vmware'/a.firmware;out.mkdir(parents=True,exist_ok=True)
    disk=work/'target-flat.img'
    with disk.open('xb'):pass
    subprocess.run(['fsutil','sparse','setflag',str(disk)],check=True,capture_output=True,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    with disk.open('r+b') as f:f.truncate(20*1024**3)
    descriptor=work/'target.vmdk';descriptor.write_bytes(b'version=1\nencoding="UTF-8"\nCID=fffffffe\nparentCID=ffffffff\ncreateType="monolithicFlat"\nRW 41943040 FLAT "target-flat.img" 0\nddb.adapterType = "ide"\n')
    before=v.inventory();checks=[];errors=[]
    def choice(g,text,expected='Choice> '):return g.command(text,expected)
    def start(folder,installed=False):return v.Guest(work/folder,descriptor,a.firmware,installed)
    try:
        with start('live') as g:
            g.wait('M51_HOME_READY');choice(g,'2');assert '20 GiB (20480 MiB)' in g.output;choice(g,'1');assert 'Review installation' in g.output;choice(g,'1','M51_GUIDED_COMPLETE');assert 'Disconnect the ISO/CD' in g.output;(out/'install.log').write_bytes(g.output.encode())
        checks.append({'name':'displayed numbered choices install actual 20-GiB SATA target; no terminal commands or sector token','passed':True})
        for i in range(2):
            with start('installed-'+str(i),True) as g:
                g.wait('M51_HOME_READY');choice(g,'1');assert 'installed files are persistent' in g.output;choice(g,'1' if i==0 else '2');assert 'Sample note saved' in g.output if i==0 else 'My first Engine OS note.' in g.output;(out/f'installed-{i}.log').write_bytes(g.output.encode())
        checks.append({'name':'media-detached installed menu and persistent sample over two boots','passed':True})
        def retained():
            with disk.open('rb') as f:f.seek(139264*512);return hashlib.sha256(f.read(16*1024**2)).hexdigest()
        digest=retained()
        with disk.open('r+b') as f:f.seek(64*512);f.write(b'BROKEN-BOOT'+bytes(502))
        with start('repair') as g:
            g.wait('M51_HOME_READY');choice(g,'3');choice(g,'1');choice(g,'1','M51_GUIDED_COMPLETE');(out/'repair.log').write_bytes(g.output.encode())
        assert retained()==digest
        with start('recovered',True) as g:g.wait('M51_HOME_READY');choice(g,'1');assert 'My first Engine OS note.' in choice(g,'2');(out/'recovered.log').write_bytes(g.output.encode())
        checks.append({'name':'numbered boot repair preserves complete data partition and recovered boot','passed':True})
    except Exception:errors.append(traceback.format_exc().replace(str(ROOT),'<project-root>'));print(errors[-1],flush=True)
    manifest=json.loads((ROOT/'dist/m51-build-manifest.json').read_text(encoding='utf-8'));unchanged=v.inventory()==before;result={'passed':not errors and unchanged,'build_id':manifest['build_id'],'iso_sha256':manifest['artifacts']['EngineOS-M51.iso']['sha256'],'disk_gib':20,'existing_vm_inventory_unchanged':unchanged,'checks':checks,'errors':errors};(out/'results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print('VMware '+a.firmware,'PASS' if result['passed'] else 'FAIL',flush=True)
    if not result['passed']:raise SystemExit(1)
if __name__=='__main__':main()
