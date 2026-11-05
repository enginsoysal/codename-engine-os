"""Qualify the first M4 increment without claiming full milestone acceptance."""
from pathlib import Path
import argparse,hashlib,json,re,subprocess,sys
ROOT=Path(__file__).resolve().parents[2]

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--collect',action='store_true',help='validate existing reports and source/artifact identity without rerunning suites')
    args=parser.parse_args()
    output=ROOT/'evidence/M4';output.mkdir(parents=True,exist_ok=True)
    result={'passed':False,'full_m4_complete':False,'scope':'AHCI sector-record and PS/2 console increment','gates':[]}
    report=output/'increment-results.json'
    report.write_bytes((json.dumps(result,indent=2)+'\n').encode())
    for script in ('tools/build/m4.py','tests/m4/boot.py','tests/m4/console.py','tests/check_repository.py'):
        if args.collect:continue
        print('Running '+script,flush=True)
        p=subprocess.run([sys.executable,script]+(['--milestone','M4'] if script=='tests/check_repository.py' else []),cwd=ROOT,timeout=360)
        result['gates'].append({'script':script,'passed':p.returncode==0})
        report.write_bytes((json.dumps(result,indent=2)+'\n').encode())
        if p.returncode:raise SystemExit(1)
    manifest=json.loads((ROOT/'dist/m4-build-manifest.json').read_text(encoding='utf-8'))
    for name in ('storage-report.json','console-report.json','repository-results.json'):
        data=json.loads((output/name).read_text(encoding='utf-8'))
        if not data.get('checks') or not all(c['passed'] for c in data['checks']):raise RuntimeError('Failed or incomplete report: '+name)
        if name!='repository-results.json' and data.get('build_id')!=manifest['build_id']:raise RuntimeError('Stale report: '+name)
        result['gates'].append({'report':name,'passed':True,'checks':len(data['checks'])})
    for name,expected in manifest['source_sha256'].items():
        if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=expected:raise RuntimeError('Source changed during tests')
    for name,record in manifest['artifacts'].items():
        raw=(ROOT/'dist'/name).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=record['sha256']:raise RuntimeError('Artifact changed')
        if re.search(rb'[Cc]:[\\/]+Users[\\/]+',raw):raise RuntimeError('Host path in artifact')
    result.update(passed=True,build_id=manifest['build_id'])
    (output/'build-manifest.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode())
    report.write_bytes((json.dumps(result,indent=2)+'\n').encode())
    print('M4 first increment PASS; full filesystem/USB acceptance remains pending')
if __name__=='__main__':main()
