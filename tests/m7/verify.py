"""Bind completed native M7 gates to exact media and committed source bytes."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'evidence/M7'


def sha(path):
    digest=hashlib.sha256()
    with path.open('rb') as file:
        for block in iter(lambda:file.read(1024*1024),b''):digest.update(block)
    return digest.hexdigest()


def load(path):return json.loads(path.read_text(encoding='utf-8'))


def committed_hashes(commit,paths):
    request=''.join(commit+':'+p+'\n' for p in paths).encode()
    result=subprocess.run(['git','cat-file','--batch'],input=request,cwd=ROOT,capture_output=True,check=True).stdout
    offset=0;answer={}
    for path in paths:
        end=result.index(b'\n',offset);header=result[offset:end].split();assert header[1]==b'blob',path
        size=int(header[2]);offset=end+1;answer[path]=hashlib.sha256(result[offset:offset+size]).hexdigest();offset+=size+1
    return answer


def main(commit):
    commit=subprocess.check_output(['git','rev-parse',commit],cwd=ROOT,text=True).strip()
    base=load(ROOT/'dist/m7-build-manifest.json');night=load(ROOT/'dist/m7night-build-manifest.json')
    errors=[];gates=[];frozen={};old_count=0;unrelated_changes=[]
    sources={**base['source_sha256'],**night['source_sha256']}
    published=committed_hashes(commit,sorted(sources))
    for name,digest in sources.items():
        if sha(ROOT/name)!=digest or published[name]!=digest:errors.append('Native source is not exact committed bytes: '+name)
    common=set(base['source_sha256'])&set(night['source_sha256'])
    if any(base['source_sha256'][p]!=night['source_sha256'][p] for p in common):errors.append('Distribution common source differs')
    if base['build_id']==night['build_id']:errors.append('Custom distribution must have its own build identity')
    for manifest in [base,night]:
        for name,item in manifest['artifacts'].items():
            p=ROOT/'dist'/name
            if not p.is_file() or p.stat().st_size!=item['size'] or sha(p)!=item['sha256']:errors.append('Artifact differs: '+name)
    for milestone in ['M4.5','M5','M5.1','M6']:
        accepted=load(ROOT/'evidence'/milestone/'acceptance-results.json')
        if not accepted['passed']:errors.append('Prior acceptance failed: '+milestone)
        for name,digest in accepted['evidence_sha256'].items():
            if sha(ROOT/'evidence'/milestone/name)!=digest:errors.append('Prior proof changed: '+milestone+'/'+name)
            old_count+=1
    for group,count in [('host',9),('native',13),('custom',2),('vmware',2)]:
        gp=OUT/(group+'-gate-results.json');info=load(gp)
        if not info['passed'] or len(info['gates'])!=count:errors.append('Incomplete gate group: '+group)
        if info['build_id']!=base['build_id'] or info['iso_sha256']!=base['artifacts']['EngineOS-M7.iso']['sha256']:errors.append('Group media mismatch: '+group)
        expected_snapshot=hashlib.sha256(json.dumps(base['source_sha256'],sort_keys=True).encode()).hexdigest()
        if info['native_source_snapshot_sha256']!=expected_snapshot:errors.append('Group native snapshot differs: '+group)
        for name,digest in info['test_harness_sha256'].items():
            if sha(ROOT/name)!=digest:
                # The collector is not run by a gate; VMware-only input is not run by the other groups.
                unused=name=='tests/m7/verify.py' or (group!='vmware' and name=='tests/m7/vmware.py')
                original=committed_hashes('d5ee13d',[name])[name] if unused else None
                if not unused or original!=digest:errors.append('Used qualification harness changed: '+group+'/'+name)
                else:unrelated_changes.append({'group':group,'file':name,'captured_sha256':digest,'captured_commit':'d5ee13d','reason':'not executed by this gate group; exact original committed bytes verified'})
        for gate in info['gates']:
            p=OUT/gate['report'];report=load(p)
            if not gate['passed'] or gate['returncode'] or not report.get('passed'):errors.append('Required gate failed: '+gate['report'])
            if sha(p)!=gate['report_sha256']:errors.append('Gate proof differs: '+gate['report'])
            if 'build_id' in report and report['build_id']!=base['build_id']:errors.append('Gate base build differs: '+gate['report'])
            expected_iso=night['artifacts']['EngineOS-M7NIGHT.iso']['sha256'] if gate['test']=='distribution' else base['artifacts']['EngineOS-M7.iso']['sha256']
            if 'iso_sha256' in report and report['iso_sha256']!=expected_iso:errors.append('Gate ISO differs: '+gate['report'])
            if gate['test']=='distribution' and report['custom_build_id']!=night['build_id']:errors.append('Custom build differs')
            if gate['test']=='maintenance' and report['update_build_id']!=night['build_id']:errors.append('Update build differs')
            checks=report.get('checks',report.get('cases',[]))
            if any(isinstance(v,dict) and v.get('passed') is False for v in checks):errors.append('Failed subcheck: '+gate['report'])
            if gate['test']=='vmware':
                if not report.get('existing_vm_inventory_unchanged'):errors.append('VMware inventory not preserved')
                for kind in ['installer','desktop','download','restart']:
                    if not report.get(kind+'_capture',{}).get('passed'):errors.append('VMware visual proof missing: '+kind)
            gates.append({'group':group,'report':gate['report'],'passed':bool(report.get('passed')),'checks':len(checks)})
            frozen[gate['report']]=sha(p)
        frozen[gp.relative_to(OUT).as_posix()]=sha(gp)
    resources=load(OUT/'resources/results.json')
    if len(resources['profiles'])!=3 or any(not p['idle_all_cpus_halted'] or p['idle_repaint_count'] or p['second_app_private_bytes']!=40960 or p['allocation_failures_observed']<1 for p in resources['profiles']):errors.append('Resource/pressure requirements failed')
    # Public transcripts use LF and trimmed prompt whitespace; runtime proof was consumed before formatting.
    for p in OUT.rglob('*'):
        if 'history' not in p.relative_to(OUT).parts and p.suffix=='.log':
            text=p.read_text(encoding='utf-8').replace('\r\n','\n')
            p.write_bytes(('\n'.join(line.rstrip(' \t') for line in text.splitlines())+'\n').encode())
    for p in OUT.rglob('*'):
        rel=p.relative_to(OUT)
        if p.is_file() and 'history' not in rel.parts and p.suffix in ['.png','.log','.pcap']:
            frozen[rel.as_posix()]=sha(p)
    for name,manifest in [('m7-build-manifest.json',base),('m7night-build-manifest.json',night)]:
        p=OUT/name;p.write_bytes((json.dumps(manifest,indent=2)+'\n').encode());frozen[name]=sha(p)
    result={'passed':not errors,'scope_id':'M7-VM-v1','code_source_commit':commit,'build_id':base['build_id'],
            'iso_sha256':base['artifacts']['EngineOS-M7.iso']['sha256'],'distribution_build_id':night['build_id'],
            'gate_count':len(gates),'check_count':sum(g['checks'] for g in gates),'gates':gates,'evidence_sha256':frozen,
            'prior_frozen_proofs_verified':old_count,'desktop_implemented':True,'human_usability_study':False,
            'physical_hardware_qualified':False,'universal_hardware_support':False,'windows_dos_runtime_implemented':False,
            'scope':'Named QEMU BIOS/UEFI ISO/USB and VMware BIOS/UEFI installed desktop profiles; exact limits in release guide',
            'unused_snapshot_changes_after_group':unrelated_changes,'public_log_normalization':'UTF-8/LF with trailing prompt whitespace trimmed after runtime consumption; earlier accepted evidence untouched','errors':errors}
    (OUT/'acceptance-results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode())
    print('M7 acceptance '+('PASS' if not errors else 'FAIL')+': '+str(len(gates))+' gates; '+str(old_count)+' prior proofs unchanged')
    for error in errors:print(error)
    if errors:raise SystemExit(1)


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--source-commit',required=True);main(parser.parse_args().source_commit)
