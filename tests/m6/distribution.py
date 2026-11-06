"""Boot and install an actual Engine OS Minimal distribution, without guide data."""
import importlib.util,json,subprocess,time,traceback
from pathlib import Path
spec=importlib.util.spec_from_file_location('m6maintenance',Path(__file__).with_name('maintenance.py'));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
h=m.h;q=m.q;ROOT=q.ROOT

def main():
    work=q.new_work('distribution');out=ROOT/'evidence/M6/distribution';out.mkdir(parents=True,exist_ok=True);result={'passed':False}
    try:
        disk=work/'owned.img';q.sparse_disk(disk,2*1024**3)
        with q.Guest(work/'live',disk,image_name='EngineOS-M6MINIMAL') as g:
            h.shot(g,out,'custom-welcome');h.next(g);h.next(g);h.next(g,2);h.shot(g,out,'optional-guide-off');h.next(g,2);h.next(g,5);pos=len(g.log());h.next(g,1);g.wait('M6_INSTALL_PASS',pos,240);(out/'install.log').write_bytes(g.log().encode())
        start,length,entries,record=m.inspect(disk,'Owner',0)
        with q.Guest(work/'installed',disk,media='installed') as g:
            assert 'M6_INSTALLED_VOLUME' in g.log();h.shot(g,out,'custom-installed');(out/'installed.log').write_bytes(g.log().encode())
        meta=json.loads((ROOT/'dist/m6minimal-build-manifest.json').read_text(encoding='utf-8'));result.update(passed=True,distribution=meta['distribution']['id'],custom_build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M6MINIMAL.iso']['sha256'],checks=[{'name':'custom branding/accent/default component recipe boots, installs, independently validates its actual kernel and data choices, then boots without media','passed':True}])
    except Exception:result['error']=q.clean(traceback.format_exc());print(result['error'])
    (out/'results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print('M6 distribution '+('PASS' if result['passed'] else 'FAIL'),flush=True)
    if not result['passed']:raise SystemExit(1)
if __name__=='__main__':main()
