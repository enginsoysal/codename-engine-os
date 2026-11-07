"""Run current tasks in isolated owned guests, never changing historical proof."""
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'evidence/M7.2'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def run(item):
    label,args,report=item;log=OUT/'runs'/(label+'.log');log.parent.mkdir(parents=True,exist_ok=True)
    with log.open('wb') as f:r=subprocess.run([sys.executable,*args],cwd=ROOT,stdout=f,stderr=subprocess.STDOUT,timeout=900)
    text=log.read_text(encoding='utf-8');log.write_bytes(('\n'.join(line.rstrip(' \t') for line in text.splitlines())+'\n').encode())
    proof=OUT/report;passed=r.returncode==0 and proof.is_file() and json.loads(proof.read_text(encoding='utf-8')).get('passed') is True
    print(label+(' PASS' if passed else ' FAIL'),flush=True)
    return {'name':label,'passed':passed,'returncode':r.returncode,'report':report,'report_sha256':sha(proof) if proof.exists() else None,'log':log.relative_to(OUT).as_posix(),'log_sha256':sha(log)}
def main():
    meta=json.loads((ROOT/'dist/m72-build-manifest.json').read_text());snapshot={n:sha(ROOT/n) for n in meta['source_sha256']};assert snapshot==meta['source_sha256']
    tasks=[(fw+'-'+media,['tests/m72/session.py','--firmware',fw,'--media',media],'qemu-'+fw+'-'+media+'/results.json') for fw in ['uefi','bios'] for media in ['iso','usb']]
    tasks += [(name,['tests/m72/'+name+'.py'],name+'/results.json') for name in ['storage','resources','update','bootentry']]
    with ThreadPoolExecutor(max_workers=2) as pool:gates=list(pool.map(run,tasks))
    # Serial VMware tests ensure exact before/after inventory comparisons.
    for fw in ['efi','bios']:gates.append(run(('vmware-'+fw,['tests/m72/vmware.py','--firmware',fw],'vmware/'+fw+'/results.json')))
    gates.append(run(('vmware-starter',['tests/m72/vmware.py','--starter'],'vmware/efi-starter/results.json')))
    unchanged=all(sha(ROOT/n)==d for n,d in snapshot.items())
    report={'passed':all(g['passed'] for g in gates) and unchanged,'build_id':meta['build_id'],'iso_sha256':meta['artifacts']['EngineOS-M72.iso']['sha256'],'native_inputs_unchanged':unchanged,'gates':gates,'test_sha256':{p.relative_to(ROOT).as_posix():sha(p) for p in sorted((ROOT/'tests/m72').glob('*.py'))}}
    (OUT/'runtime-results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
