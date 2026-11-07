"""Run final M7.1 gate groups against immutable, already-built artifacts."""
import argparse
from concurrent.futures import ThreadPoolExecutor,as_completed
import hashlib
import json
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'evidence/M7.1'
GROUPS={
 'host':[('disk',[],'disk-results.json')],
 'native':[('native',['--firmware','uefi','--table','gpt'],'native-uefi-gpt/results.json'),('native',['--firmware','bios','--table','mbr'],'native-bios-mbr/results.json'),('advanced',[],'advanced/results.json'),('multidisk',[],'multidisk/results.json'),('maintenance',[],'maintenance/results.json')],
 'regression':[('regression',['isolation'],'regression/isolation/results.json'),('regression',['resources'],'regression/resources/results.json'),('regression',['https_negative'],'regression/https-negative/results.json'),('regression',['distribution'],'regression/distribution/results.json'),('regression',['firmware','--firmware','uefi','--media','usb'],'regression/firmware/uefi-usb/results.json'),('regression',['firmware','--firmware','bios','--media','usb'],'regression/firmware/bios-usb/results.json')],
 'vmware':[('vmware',['--firmware',f],'vmware/'+f+'/results.json') for f in ['efi','bios']]}


def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()


def main():
 p=argparse.ArgumentParser();p.add_argument('group',choices=GROUPS);a=p.parse_args();base=json.loads((ROOT/'dist/m71-build-manifest.json').read_text());manifests=[base]
 if a.group=='regression':manifests.append(json.loads((ROOT/'dist/m71night-build-manifest.json').read_text()))
 def verify():
  for m in manifests:
   for n,d in m['source_sha256'].items():assert sha(ROOT/n)==d,'Changed native input: '+n
   for n,v in m['artifacts'].items():assert sha(ROOT/'dist'/n)==v['sha256'],'Changed media: '+n
 verify()
 files=sorted((ROOT/'tests/m71').glob('*.py'))+sorted((ROOT/'tests/m7').glob('*.py'))+[ROOT/'tests/m6'/n for n in ['vm.py','volume.py','guided.py','workspace.py','vmware.py']]+[ROOT/'tools/vm/config.py',ROOT/'tools/toolchain-lock.json',ROOT/'tools/security-toolchain.json']
 harness={f.relative_to(ROOT).as_posix():sha(f) for f in files}
 logs=OUT/'gate-logs';logs.mkdir(parents=True,exist_ok=True)
 def execute(task):
  name,args,relative=task;label=name+('-'+'-'.join(v.strip('-') for v in args) if args else '')
  r=subprocess.run([sys.executable,str(ROOT/'tests/m71'/(name+'.py')),*args],cwd=ROOT,capture_output=True,text=True,timeout=1800,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
  text=(r.stdout+r.stderr).replace(str(ROOT),'<project-root>');log=logs/(label+'.log');log.write_bytes(('\n'.join(line.rstrip(' \t') for line in text.splitlines())+'\n').encode())
  report=OUT/relative
  if report.exists():report.write_bytes(report.read_bytes().replace(b'\r\n',b'\n'))
  data=json.loads(report.read_text(encoding='utf-8')) if report.exists() else {};passed=r.returncode==0 and data.get('passed') is True
  print(a.group+' '+label+' '+('PASS' if passed else 'FAIL'),flush=True)
  return {'test':name,'arguments':args,'report':relative,'passed':passed,'returncode':r.returncode,'report_sha256':sha(report) if report.exists() else None,'log':log.relative_to(OUT).as_posix(),'log_sha256':sha(log)}
 with ThreadPoolExecutor(max_workers=1 if a.group in ['host','vmware'] else 2) as pool:
  futures=[pool.submit(execute,t) for t in GROUPS[a.group]];results=[v.result() for v in as_completed(futures)]
 verify();assert all(sha(ROOT/n)==d for n,d in harness.items()),'Harness changed during qualification'
 report={'passed':all(v['passed'] for v in results),'group':a.group,'build_id':base['build_id'],'iso_sha256':base['artifacts']['EngineOS-M71.iso']['sha256'],'native_source_snapshot_sha256':hashlib.sha256(json.dumps(base['source_sha256'],sort_keys=True).encode()).hexdigest(),'test_harness_sha256':harness,'gates':results,'scope':'one group alone does not establish complete M7.1 acceptance'}
 (OUT/(a.group+'-gate-results.json')).write_bytes((json.dumps(report,indent=2)+'\n').encode())
 if not report['passed']:raise SystemExit(1)


if __name__=='__main__':main()
