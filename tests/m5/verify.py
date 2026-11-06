"""Build and qualify the actual M5 VM installer, or collect fresh matching reports."""
from pathlib import Path
import argparse,hashlib,json,re,subprocess,sys
ROOT=Path(__file__).resolve().parents[2]
CASES=[('tests/m5/vm.py',[], 'uefi-iso-128m-2cpu/results.json'),
       ('tests/m5/vm.py',['--firmware','bios'],'bios-iso-128m-2cpu/results.json'),
       ('tests/m5/vm.py',['--media','usb','--ram','64','--cpus','1'],'uefi-usb-64m-1cpu/results.json'),
       ('tests/m5/vm.py',['--firmware','bios','--media','usb','--cpus','4'],'bios-usb-128m-4cpu/results.json'),
       ('tests/m5/overlay.py',[],'overlay/results.json'),('tests/m5/negative.py',[],'negative/results.json'),
       ('tests/m5/vmware.py',[],'vmware/efi/results.json'),('tests/m5/vmware.py',['--firmware','bios'],'vmware/bios/results.json'),
       ('tests/check_repository.py',['--milestone','M5'],'repository-results.json')]

def main():
    p=argparse.ArgumentParser();p.add_argument('--collect',action='store_true',help='verify existing reports against every exact source/artifact hash');p.add_argument('--no-build',action='store_true');opt=p.parse_args()
    out=ROOT/'evidence/M5';out.mkdir(parents=True,exist_ok=True);report=out/'acceptance-results.json';result={'passed':False,'full_m5_complete':False,'scope_id':'M5-VM-v1','physical_qualified':False,'factory_trust_qualified':False,'gates':[]};report.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    if not opt.collect and not opt.no_build:subprocess.run([sys.executable,'tools/build/m5.py'],cwd=ROOT,check=True)
    manifest=json.loads((ROOT/'dist/m5-build-manifest.json').read_text(encoding='utf-8'))
    (out/'build-manifest.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode('utf-8'))
    assert manifest.get('qualification_scope')=='M5-VM-v1' and 'installed_kernel_sha256' in manifest,'Wait for the complete M5 build before qualification'
    for source,expected in manifest['source_sha256'].items():assert hashlib.sha256((ROOT/source).read_bytes()).hexdigest()==expected,'Build before testing: '+source
    for script,args,name in CASES:
        if not opt.collect:
            print('Running '+script+' '+' '.join(args),flush=True);subprocess.run([sys.executable,script,*args],cwd=ROOT,check=True,timeout=600)
        data=json.loads((out/name).read_text(encoding='utf-8'));assert data.get('passed') and data['checks'] and all(c['passed'] for c in data['checks']),name
        if name!='repository-results.json':assert data['build_id']==manifest['build_id'],name
        if 'iso_sha256' in data:assert data['iso_sha256']==manifest['artifacts']['EngineOS-M5.iso']['sha256'],name
        if name.startswith('vmware/'):assert data['existing_vm_inventory_unchanged'],name
        result['gates'].append({'script':script,'arguments':args,'evidence':name,'passed':True,'checks':len(data['checks'])})
        report.write_bytes((json.dumps(result,indent=2)+'\n').encode('utf-8'))
    for source,expected in manifest['source_sha256'].items():assert hashlib.sha256((ROOT/source).read_bytes()).hexdigest()==expected,'Source changed: '+source
    for name,record in manifest['artifacts'].items():
        raw=(ROOT/'dist'/name).read_bytes();assert hashlib.sha256(raw).hexdigest()==record['sha256'],name;assert not re.search(rb'[Cc]:[\\/]+Users[\\/]+',raw),name
    (out/'build-manifest.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode('utf-8'))
    result.update(passed=True,full_m5_complete=True,build_id=manifest['build_id'],iso_sha256=manifest['artifacts']['EngineOS-M5.iso']['sha256'],gate_count=len(CASES),scope_contract_sha256=hashlib.sha256((ROOT/'docs/milestones/m5.md').read_bytes()).hexdigest(),limits=manifest['limits'])
    for _,_,name in CASES:
        path=out/name;path.write_bytes(path.read_bytes().replace(b'\r\n',b'\n'))
    result['evidence_sha256']={name:hashlib.sha256((out/name).read_bytes()).hexdigest() for _,_,name in CASES}
    report.write_bytes((json.dumps(result,indent=2)+'\n').encode('utf-8'));print('M5 COMPLETE within M5-VM-v1: actual native installation, detached boot and recovery',flush=True)
if __name__=='__main__':main()
