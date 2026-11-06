"""Qualify the user-approved M45-VM-v1 milestone; external hardware/trust remain separate."""
from pathlib import Path
import hashlib,json,re,subprocess,sys
ROOT=Path(__file__).resolve().parents[2]
def main():
    out=ROOT/'evidence/M4.5';out.mkdir(parents=True,exist_ok=True);report=out/'acceptance-results.json';result={'passed':False,'full_m45_complete':False,'scope_id':'M45-VM-v1','completion_scope':'named virtual profiles only','physical_qualified':False,'factory_trust_qualified':False,'gates':[]}
    contract=json.loads((ROOT/'docs/milestones/m4.5-scope.json').read_text(encoding='utf-8'));assert contract['scope_id']=='M45-VM-v1' and contract['required_gate_count']==29 and contract['required_secure_boot_checks']==23
    result['scope_contract_sha256']=hashlib.sha256((ROOT/'docs/milestones/m4.5-scope.json').read_bytes()).hexdigest()
    result['excluded_from_completion']=contract['excluded_from_completion']
    report.write_bytes((json.dumps(result,indent=2)+'\n').encode())
    suites=[('tools/build/m45.py',[]),('tests/m45/smp_delay.py',['--label','fixed-short-delay','--delay-ms','150','--expected-exit','33']),('tests/m45/smp_delay.py',['--label','fixed-deadline-negative','--delay-ms','2000','--expected-exit','99']),('tests/m45/smp_pressure.py',['--label','fixed','--runs','8']),('tests/m45/boot_matrix.py',[]),('tests/m45/bios_negative.py',[]),('tests/m45/storage.py',[]),('tests/m45/read_faults.py',[]),('tests/m45/usb.py',[]),('tests/m45/usb_timeout.py',[]),('tests/m45/usb_reject.py',[]),('tests/m45/usb_reject.py',['--hub']),('tests/m45/hubs.py',[]),('tests/m45/hubs.py',['--nested']),('tests/m45/uas.py',[]),('tests/m45/uas.py',['--high-speed']),('tests/m45/uas.py',['--timeout']),('tests/m45/uas.py',['--high-speed','--timeout']),('tests/m4/console.py',['--milestone','M45']),('tests/m4/console.py',['--milestone','M45','--usb']),('tests/m45/ext2.py',[]),('tests/m45/fat32.py',[]),('tests/m45/sha256.py',[]),('tests/m45/secure_boot.py',[]),('tests/m45/vmware.py',[]),('tools/build/m4.py',[]),('tests/m4/boot.py',['--case','persist-first','--evidence','evidence/M4.5/m4-regression']),('tests/check_repository.py',['--milestone','M4.5']),('tests/m3/verify.py',[])]
    for script,args in suites:
        if script!='tools/build/m45.py':
            current=json.loads((ROOT/'dist/m45-build-manifest.json').read_text(encoding='utf-8'))
            for name,expected in current['source_sha256'].items():
                if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=expected:raise RuntimeError('Source changed before gate: '+name)
        print('Running '+script+' '+' '.join(args),flush=True);p=subprocess.run([sys.executable,script,*args],cwd=ROOT,timeout=1200);result['gates'].append({'script':script,'arguments':args,'passed':p.returncode==0});report.write_bytes((json.dumps(result,indent=2)+'\n').encode())
        if p.returncode:raise SystemExit(1)
    manifest=json.loads((ROOT/'dist/m45-build-manifest.json').read_text(encoding='utf-8'))
    for name,expected in manifest['source_sha256'].items():
        if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=expected:raise RuntimeError('Source changed during qualification: '+name)
    for name,record in manifest['artifacts'].items():
        raw=(ROOT/'dist'/name).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=record['sha256'] or re.search(rb'[Cc]:[\\/]+Users[\\/]+',raw):raise RuntimeError('Invalid/private artifact: '+name)
    for name in ('storage-results.json','fat32-results.json','sha256-results.json','secure-boot-results.json','repository-results.json'):
        data=json.loads((out/name).read_text(encoding='utf-8'));assert data['checks'] and all(c['passed'] for c in data['checks']),name
    secure=json.loads((out/'secure-boot-results.json').read_text(encoding='utf-8'));assert len(secure['checks'])==23 and secure['provenance']['build_id']==manifest['build_id']
    for name in ('results.json','negative-results.json'):
        data=json.loads((out/'boot'/name).read_text(encoding='utf-8'));assert data['build_id']==manifest['build_id'] and all(c['passed'] for c in data['checks']),name
    data=json.loads((out/'vmware/results.json').read_text(encoding='utf-8'));assert data['build_id']==manifest['build_id'] and data['existing_vm_inventory_unchanged'] and len(data['checks'])==5 and all(c['passed'] for c in data['checks'])
    assert data['iso_sha256']==manifest['artifacts']['EngineOS-M45.iso']['sha256']
    for folder,count in (('usb',10),('usb-timeout',7),('ext2',8),('faults',3),('usb-reject/root',4),('usb-reject/hub',4),('hubs/direct',11),('hubs/nested',11),('uas/ss-eio',10),('uas/hs-eio',10),('uas/ss-timeout',10),('uas/hs-timeout',10)):
        data=json.loads((out/folder/'results.json').read_text(encoding='utf-8'));assert data['build_id']==manifest['build_id'] and len(data['checks'])==count and not data.get('error') and all(c['passed'] for c in data['checks']),folder
    for folder,prefix in (('ps2-console',''),('hid-console','usb-')):
        data=json.loads((out/folder/(prefix+'console-report.json')).read_text(encoding='utf-8'));assert data['build_id']==manifest['build_id'] and all(c['passed'] for c in data['checks']),folder
    for folder,count in (('smp-delay/fixed-short-delay',1),('smp-delay/fixed-deadline-negative',1),('smp-pressure/fixed',8)):
        data=json.loads((out/folder/'results.json').read_text(encoding='utf-8'));assert data['build_id']==manifest['build_id'] and len(data['checks'])==count and not data.get('error') and all(c['passed'] for c in data['checks']),folder
    data=json.loads((ROOT/'evidence/M3/acceptance-results.json').read_text(encoding='utf-8'));assert data['passed'] and data['m0_regression_checks']==51
    assert hashlib.sha256((ROOT/'docs/milestones/m4.5-scope.json').read_bytes()).hexdigest()==result['scope_contract_sha256']
    result['evidence_sha256']={}
    for name in ('boot/results.json','boot/negative-results.json','storage-results.json','fat32-results.json','secure-boot-results.json','vmware/results.json','usb/results.json','usb-timeout/results.json','usb-reject/root/results.json','usb-reject/hub/results.json','hubs/direct/results.json','hubs/nested/results.json','uas/ss-eio/results.json','uas/hs-eio/results.json','uas/ss-timeout/results.json','uas/hs-timeout/results.json','ext2/results.json','faults/results.json','repository-results.json'):
        path=out/name;raw=('\n'.join(line.rstrip() for line in path.read_text(encoding='utf-8').splitlines())+'\n').encode();path.write_bytes(raw);result['evidence_sha256'][name]=hashlib.sha256(raw).hexdigest()
    assert len(result['gates'])==contract['required_gate_count'] and all(g['passed'] for g in result['gates'])
    assert manifest['qualification_scope']==contract['scope_id'] and manifest['scope_contract_sha256']==result['scope_contract_sha256']
    result.update(passed=True,full_m45_complete=True,build_id=manifest['build_id']);(out/'build-manifest.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode());report.write_bytes((json.dumps(result,indent=2)+'\n').encode());print('M4.5 COMPLETE within M45-VM-v1; physical hardware and factory trust remain outside scope',flush=True)
if __name__=='__main__':main()
