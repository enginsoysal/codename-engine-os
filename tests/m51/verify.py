"""Collect exact guided-setup, distribution and real VM evidence without touching M5 reports."""
from pathlib import Path
import argparse,hashlib,json,subprocess,sys
ROOT=Path(__file__).resolve().parents[2]
CASES=[('tests/m51/guided.py',[],'uefi-iso/results.json'),('tests/m51/guided.py',['--firmware','bios'],'bios-iso/results.json'),('tests/m51/guided.py',['--usb'],'uefi-usb/results.json'),('tests/m51/guided.py',['--firmware','bios','--usb'],'bios-usb/results.json'),('tests/m51/guided.py',['--fault'],'uefi-iso-fault/results.json'),('tests/m51/refusals.py',[],'refusals/results.json'),('tests/m51/resources.py',[],'resources/results.json'),('tests/m51/profiles.py',[],'profile-results.json'),('tests/m51/vmware.py',[],'vmware/efi/results.json'),('tests/m51/vmware.py',['--firmware','bios'],'vmware/bios/results.json'),('tests/check_repository.py',['--milestone','M5.1'],'repository-results.json')]

def main():
    p=argparse.ArgumentParser();p.add_argument('--collect',action='store_true');p.add_argument('--runtime-only',action='store_true');a=p.parse_args();out=ROOT/'evidence/M5.1';out.mkdir(parents=True,exist_ok=True)
    m=json.loads((ROOT/'dist/m51-build-manifest.json').read_text(encoding='utf-8'));assert m['distribution']['id']=='engineos'
    assert hashlib.sha256((ROOT/'dist/EngineOS-M5.iso').read_bytes()).hexdigest()==m['artifacts']['EngineOS-M51.iso']['sha256'],'Internal test alias must match public ISO'
    (out/'build-manifest.json').write_bytes((json.dumps(m,indent=2)+'\n').encode());result={'passed':False,'scope_id':'M51-VM-v1','build_id':m['build_id'],'gates':[]};report=out/'acceptance-results.json';report.write_bytes((json.dumps(result,indent=2)+'\n').encode())
    for script,args,name in CASES:
        if not a.collect:
            if a.runtime_only and (script.endswith('vmware.py') or script.endswith('check_repository.py')):continue
            subprocess.run([sys.executable,script,*args],cwd=ROOT,check=True,timeout=600)
        r=json.loads((out/name).read_text(encoding='utf-8'));assert r['passed'] and all(c['passed'] for c in r['checks']),name
        if 'build_id' in r:assert r['build_id']==m['build_id'],name
        if 'iso_sha256' in r:assert r['iso_sha256']==m['artifacts']['EngineOS-M51.iso']['sha256'],name
        result['gates'].append({'report':name,'checks':len(r['checks']),'passed':True})
    if a.runtime_only:print('Guided runtime subset PASS');return
    forge=json.loads((ROOT/'dist/profiles/forge/build-manifest.json').read_text(encoding='utf-8'));r=json.loads((out/'forge/results.json').read_text(encoding='utf-8'));assert r['passed'] and r['build_id']==forge['build_id']!=m['build_id'];assert r['iso_sha256']==forge['artifacts']['EngineOS-forge-M51.iso']['sha256'];result['gates'].append({'report':'forge/results.json','checks':len(r['checks']),'passed':True})
    for manifest in (m,forge):
        for source,digest in manifest['source_sha256'].items():assert hashlib.sha256((ROOT/source).read_bytes()).hexdigest()==digest,source
    for name,r in m['artifacts'].items():assert hashlib.sha256((ROOT/'dist'/name).read_bytes()).hexdigest()==r['sha256'],name
    for gate in result['gates']:
        path=out/gate['report'];path.write_bytes(path.read_bytes().replace(b'\r\n',b'\n'))
    result.update(passed=True,full_m51_complete=True,iso_sha256=m['artifacts']['EngineOS-M51.iso']['sha256'],physical_qualified=False,human_usability_study=False,evidence_sha256={g['report']:hashlib.sha256((out/g['report']).read_bytes()).hexdigest() for g in result['gates']})
    report.write_bytes((json.dumps(result,indent=2)+'\n').encode());print('M5.1 COMPLETE within M51-VM-v1',flush=True)
if __name__=='__main__':main()
