"""Exact-candidate native UI tasks, followed by isolated VMware lifecycle tests."""
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'evidence/M7.3'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def run(task):
 label,args,report=task;log=OUT/'runs'/(label+'.log');log.parent.mkdir(parents=True,exist_ok=True)
 with log.open('wb') as f:result=subprocess.run([sys.executable,*args],cwd=ROOT,stdout=f,stderr=subprocess.STDOUT,timeout=1200)
 text=log.read_text(encoding='utf-8');log.write_bytes(('\n'.join(v.rstrip(' \t') for v in text.splitlines())+'\n').encode())
 proof=OUT/report;passed=result.returncode==0 and proof.exists() and json.loads(proof.read_text(encoding='utf-8'))['passed']
 print(label+(' PASS' if passed else ' FAIL'),flush=True)
 return dict(name=label,passed=passed,returncode=result.returncode,report=report,report_sha256=sha(proof) if proof.exists() else None,log=log.relative_to(OUT).as_posix(),log_sha256=sha(log))
def main():
 meta=json.loads((ROOT/'dist/m73-build-manifest.json').read_text(encoding='utf-8'));sources=meta['source_sha256'];assert all(sha(ROOT/n)==d for n,d in sources.items())
 tasks=[('uefi-'+r,['tests/m73/visual.py','--resolution',r],'uefi-'+r+'/results.json') for r in ['1280x800','1920x1080','2560x1440','3840x2160']]
 tasks += [('bios-1024x768',['tests/m73/visual.py','--firmware','bios','--resolution','1024x768'],'bios-1024x768/results.json'),('low-memory',['tests/m73/visual.py','--ram','64'],'uefi-1280x800-64mib/results.json')]
 tasks += [('surfaces-'+r,['tests/m73/surfaces.py','--resolution',r],'surfaces-'+r+'/results.json') for r in ['1280x800','3840x2160']]
 tasks += [('bootentry',['tests/m73/bootentry.py'],'bootentry/results.json'),('update',['tests/m73/update.py'],'update/results.json')]
 with ThreadPoolExecutor(max_workers=2) as pool:gates=list(pool.map(run,tasks))
 for firmware in ['efi','bios']:gates.append(run(('vmware-'+firmware,['tests/m73/vmware.py','--firmware',firmware],'vmware/'+firmware+'/results.json')))
 gates.append(run(('vmware-starter',['tests/m73/vmware.py','--starter'],'vmware/efi-starter/results.json')))
 unchanged=all(sha(ROOT/n)==d for n,d in sources.items())
 report=dict(passed=all(g['passed'] for g in gates) and unchanged,build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M73.iso']['sha256'],native_inputs_unchanged=unchanged,gates=gates,test_sha256={p.relative_to(ROOT).as_posix():sha(p) for p in sorted((ROOT/'tests/m73').glob('*.py'))})
 (OUT/'runtime-results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
 if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
