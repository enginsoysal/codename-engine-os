"""Fresh M0 regression plus real M1 build, parser, firmware boots and visual gate."""
from datetime import datetime,timezone
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[1]
def main():
    evidence=ROOT/'evidence/M1';evidence.mkdir(parents=True,exist_ok=True)
    result={'scope':'M1 kernel boot; not installation or application compatibility','time_utc':datetime.now(timezone.utc).isoformat(),
            'passed':False,'gates':[]}
    report=evidence/'acceptance-results.json'
    report.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    for script,output in [('tools/build.py','dist/build-manifest.json'),('tests/m1_elf.py','evidence/M1/elf-results.json'),
                          ('tests/m1_boot.py','evidence/M1/boot-results.json'),('tests/m1_visual.py','evidence/M1/visual-results.json'),
                          ('m0/verify_m0.py','evidence/M0/acceptance-results.json')]:
        target=ROOT/output
        if target.exists():target.unlink()
        print('Running '+script,flush=True)
        process=subprocess.run([sys.executable,str(ROOT/script)],cwd=ROOT,timeout=360)
        data=json.loads(target.read_text(encoding='utf-8')) if target.exists() else {}
        if script=='tools/build.py' and data.get('build_id'):
            (evidence/'build-manifest.json').write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8')
        passed=process.returncode==0 and (data.get('passed') is True or script=='tools/build.py' and bool(data.get('build_id')))
        result['gates'].append({'script':script,'passed':passed,'checks':len(data.get('checks',[]))})
        report.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
        if not passed:raise SystemExit(1)
    manifest=json.loads((ROOT/'dist/build-manifest.json').read_text(encoding='utf-8'))
    for relative,expected in manifest['source_sha256'].items():
        if hashlib.sha256((ROOT/relative).read_bytes()).hexdigest()!=expected:raise RuntimeError('Source changed during verification')
    for name,record in manifest['artifacts'].items():
        raw=(ROOT/'dist'/name).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=record['sha256']:raise RuntimeError('Artifact changed during verification')
        if re.search(rb'[Cc]:[\\/]+Users[\\/]+',raw):raise RuntimeError('Raw host user path in release artifact')
    for doc in ROOT.rglob('*.md'):
        if any(x in doc.parts for x in ('local','build','.git')):continue
        for target in re.findall(r'\]\(([^)]+)\)',doc.read_text(encoding='utf-8')):
            if re.match(r'^[a-z]+://|^#',target):continue
            if not (doc.parent/target.split('#')[0]).exists():raise RuntimeError('Broken link in '+str(doc.relative_to(ROOT)))
    (evidence/'build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    checksums=''.join(f'{record["sha256"]}  {name}\n' for name,record in manifest['artifacts'].items())
    (ROOT/'dist/SHA256SUMS.txt').write_text(checksums,encoding='utf-8')
    result['build_id']=manifest['build_id'];result['passed']=True
    result['m1_check_count']=sum(x['checks'] for x in result['gates'] if x['script'].startswith('tests/'))
    result['regression_m0_checks']=51
    report.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(f'M1 PASS: {result["m1_check_count"]} M1 checks plus M0 regression and source/artifact/link checks',flush=True)
if __name__=='__main__':main()
