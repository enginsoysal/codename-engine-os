"""Run fresh M0 gates; fail rather than accepting stale evidence or skipped host checks."""
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]

def main():
    if os.name!='nt':
        raise SystemExit('Full M0 reference qualification currently requires a Windows host')
    evidence=ROOT/'evidence/M0'
    result={'schema':1,'time_utc':datetime.now(timezone.utc).isoformat(),'passed':False,'gates':[],
            'scope':'M0 architecture/toolchain readiness, not kernel/app/install acceptance'}
    output=evidence/'acceptance-results.json'
    output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    suites=[('probes/run_probes.py','probe-results.json'),('probes/contracts.py','contract-results.json'),
            ('probes/windows_reference.py','windows-reference-results.json'),('probes/environment.py','environment-results.json')]
    for script,filename in suites:
        path=evidence/filename
        if path.exists(): path.unlink()
        process=subprocess.run([sys.executable,str(ROOT/'m0'/script)],cwd=ROOT,capture_output=True,text=True,timeout=120)
        report=json.loads(path.read_text(encoding='utf-8')) if path.exists() else {}
        passed=process.returncode==0 and report.get('passed') is True
        result['gates'].append({'suite':script,'passed':passed,'checks':len(report.get('checks',[]))})
        output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
        print(f'{script}: {"PASS" if passed else "FAIL"} ({len(report.get("checks",[]))} checks)',flush=True)
        if not passed:
            print(process.stdout.replace(str(ROOT),'<project-root>'))
            print(process.stderr.replace(str(ROOT),'<project-root>'))
            raise SystemExit(1)
    for doc in ROOT.rglob('*.md'):
        if '.git' in doc.parts or 'local' in doc.parts or 'build' in doc.parts: continue
        for target in re.findall(r'\]\(([^)]+)\)',doc.read_text(encoding='utf-8')):
            if re.match(r'^[a-z]+://|^#',target): continue
            if not (doc.parent/target.split('#')[0]).exists():
                raise RuntimeError(f'Broken local link in {doc.relative_to(ROOT)}: {target}')
    targets=json.loads((ROOT/'m0/windows-targets.json').read_text(encoding='utf-8'))
    assert len(targets['generation_profiles'])==8 and targets['applications']==[]
    result['gates'].append({'suite':'document links and Windows architecture inventory','passed':True})
    sources=[p for p in (ROOT/'m0').rglob('*') if p.is_file() and '__pycache__' not in p.parts]
    result['source_sha256']={p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(sources)}
    result['technical_check_count']=sum(item.get('checks',0) for item in result['gates'])
    result['passed']=True
    output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(f'M0 PASS: {result["technical_check_count"]} technical checks plus document/inventory gate')

if __name__=='__main__':
    main()
