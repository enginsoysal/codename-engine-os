"""Freeze exact native source, media, current task proof and retained milestones."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'evidence/M7.2'
spec=importlib.util.spec_from_file_location('verify7',ROOT/'tests/m7/verify.py');old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old)
sha,load=old.sha,old.load

def main(commit):
    commit=subprocess.check_output(['git','rev-parse',commit],cwd=ROOT,text=True).strip();meta=load(ROOT/'dist/m72-build-manifest.json');runtime=load(OUT/'runtime-results.json');starter=load(ROOT/'dist/m72-starter-manifest.json');errors=[]
    sources=meta['source_sha256'];published=old.committed_hashes(commit,sorted(sources))
    for name,digest in sources.items():
        if sha(ROOT/name)!=digest or published[name]!=digest:errors.append('Native input is not exact committed bytes: '+name)
    artifacts=dict(meta['artifacts']);artifacts[starter['archive']]={k:starter[k] for k in ['sha256','size']}
    for name,value in artifacts.items():
        p=ROOT/'dist'/name
        if not p.exists() or sha(p)!=value['sha256'] or p.stat().st_size!=value['size']:errors.append('Artifact changed: '+name)
    if not runtime['passed'] or len(runtime['gates'])!=11 or not runtime['native_inputs_unchanged']:errors.append('Runtime/host gates incomplete')
    if runtime['build_id']!=meta['build_id'] or runtime['iso_sha256']!=artifacts['EngineOS-M72.iso']['sha256']:errors.append('Runtime identity differs')
    for name,digest in runtime['test_sha256'].items():
        if sha(ROOT/name)!=digest:errors.append('Harness changed after tasks: '+name)
    gates=[]
    for gate in runtime['gates']:
        proof=load(OUT/gate['report'])
        if not gate['passed'] or gate['returncode'] or not proof['passed'] or sha(OUT/gate['report'])!=gate['report_sha256'] or sha(OUT/gate['log'])!=gate['log_sha256']:errors.append('Gate changed/failed: '+gate['name'])
        if 'build_id' in proof and proof['build_id']!=meta['build_id']:errors.append('Gate build differs: '+gate['name'])
        if 'iso_sha256' in proof and proof['iso_sha256']!=artifacts['EngineOS-M72.iso']['sha256']:errors.append('Gate ISO differs: '+gate['name'])
        if gate['name'].startswith('vmware'):
            if not proof['existing_vm_inventory_unchanged'] or proof['boot_order_forced'] or not proof['same_vm_reused']:errors.append('VMware lifecycle isolation failed')
        if gate['name']=='vmware-starter' and proof['starter_sha256']!=starter['sha256']:errors.append('Tested ZIP differs')
        if gate['name']=='bootentry' and proof['source_sha256']!=sha(ROOT/'boot/uefi/bootentry72.inc'):errors.append('Boot registration C differs')
        gates.append({'name':gate['name'],'report':gate['report'],'passed':proof['passed']})
    repository=load(OUT/'repository-results.json')
    if not repository['passed']:errors.append('Repository/documentation checks failed')
    gates.append({'name':'repository','report':'repository-results.json','passed':repository['passed']})
    resources=load(OUT/'resources/results.json')
    if len(resources['profiles'])!=3 or any(not p['idle_all_cpus_halted'] or p['idle_repaint_count'] or p['folder_navigation_retained_page_delta'] or p['folder_navigation_allocation_failures'] for p in resources['profiles']):errors.append('Resource/idle evidence failed')
    historical=0
    for milestone in ['M4.5','M5','M5.1','M6','M7','M7.1']:
        accepted=load(ROOT/'evidence'/milestone/'acceptance-results.json')
        if not accepted['passed']:errors.append('Prior acceptance failed: '+milestone)
        for name,digest in accepted['evidence_sha256'].items():
            if sha(ROOT/'evidence'/milestone/name)!=digest:errors.append('Prior frozen proof changed: '+milestone+'/'+name)
            historical+=1
    retained={}
    for name in ['m6','m6u1','m6minimal','m7','m7night','m71','m71night']:
        previous=load(ROOT/'dist'/(name+'-build-manifest.json'))
        for filename,value in previous['artifacts'].items():
            if sha(ROOT/'dist'/filename)!=value['sha256']:errors.append('Historical artifact changed: '+filename)
            retained[filename]=value['sha256']
    # Normalize only new proof before hashing it; do not touch frozen prior files.
    for p in OUT.rglob('*.log'):
        text=p.read_text(encoding='utf-8').replace('\r\n','\n');p.write_bytes(('\n'.join(v.rstrip(' \t') for v in text.splitlines())+'\n').encode())
    # Runtime runner log digests were taken before normalization. Require exact LF already.
    for gate in runtime['gates']:
        if sha(OUT/gate['log'])!=gate['log_sha256']:errors.append('Runner transcript was not normalized: '+gate['name'])
    for name in ['m72-build-manifest.json','m72-starter-manifest.json']:shutil.copyfile(ROOT/'dist'/name,OUT/name)
    (OUT/'historical-artifacts.json').write_bytes((json.dumps({'passed':True,'artifacts':retained},indent=2)+'\n').encode())
    frozen={p.relative_to(OUT).as_posix():sha(p) for p in sorted(OUT.rglob('*')) if p.is_file() and p.name!='acceptance-results.json'}
    report={'passed':not errors,'scope_id':'M72-VM-v1','code_source_commit':commit,'build_id':meta['build_id'],'iso_sha256':artifacts['EngineOS-M72.iso']['sha256'],'vmware_starter_sha256':starter['sha256'],'gate_count':len(gates),'gates':gates,'artifacts':artifacts,'prior_frozen_proofs_verified':historical,'prior_binary_artifacts_verified':len(retained),'resources':resources,'evidence_sha256':frozen,'installer_and_folder_desktop_implemented':True,'user_vm_original_failure_cause_established':False,'physical_hardware_qualified':False,'multi_os_esp_boot_qualified':False,'windows_dos_runtime_implemented':False,'human_usability_study':False,'limitations':meta['limits'],'errors':errors}
    (OUT/'acceptance-results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps({'passed':report['passed'],'gates':len(gates),'prior_proofs':historical,'errors':errors},indent=2))
    if errors:raise SystemExit(1)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--commit',default='HEAD');main(p.parse_args().commit)
