"""Freeze exact M7.1 acceptance after every required native/host gate passes."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'evidence/M7.1'
spec=importlib.util.spec_from_file_location('verify7',ROOT/'tests/m7/verify.py');old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old)
sha,load=old.sha,old.load


def main(commit):
 commit=subprocess.check_output(['git','rev-parse',commit],cwd=ROOT,text=True).strip();base=load(ROOT/'dist/m71-build-manifest.json');night=load(ROOT/'dist/m71night-build-manifest.json');errors=[];gates=[];frozen={};prior=0
 sources={**base['source_sha256'],**night['source_sha256']};published=old.committed_hashes(commit,sorted(sources))
 for n,d in sources.items():
  if sha(ROOT/n)!=d or published[n]!=d:errors.append('Native input is not exact committed bytes: '+n)
 for m in [base,night]:
  for n,v in m['artifacts'].items():
   p=ROOT/'dist'/n
   if not p.is_file() or p.stat().st_size!=v['size'] or sha(p)!=v['sha256']:errors.append('Artifact differs: '+n)
 for n in set(base['source_sha256'])&set(night['source_sha256']):
  if base['source_sha256'][n]!=night['source_sha256'][n]:errors.append('Common distro source differs: '+n)
 if base['build_id']==night['build_id']:errors.append('Distro build identity not distinct')
 for milestone in ['M4.5','M5','M5.1','M6','M7']:
  accepted=load(ROOT/'evidence'/milestone/'acceptance-results.json')
  if not accepted['passed']:errors.append('Prior acceptance failed: '+milestone)
  for n,d in accepted['evidence_sha256'].items():
   if sha(ROOT/'evidence'/milestone/n)!=d:errors.append('Prior frozen proof changed: '+milestone+'/'+n)
   prior+=1
 for group,count in [('host',1),('native',5),('regression',6),('vmware',2)]:
  p=OUT/(group+'-gate-results.json');r=load(p)
  if not r['passed'] or len(r['gates'])!=count:errors.append('Incomplete group: '+group)
  if r['build_id']!=base['build_id'] or r['iso_sha256']!=base['artifacts']['EngineOS-M71.iso']['sha256']:errors.append('Group media differs: '+group)
  snapshot=hashlib.sha256(json.dumps(base['source_sha256'],sort_keys=True).encode()).hexdigest()
  if r['native_source_snapshot_sha256']!=snapshot:errors.append('Group native source differs: '+group)
  for n,d in r['test_harness_sha256'].items():
   if sha(ROOT/n)!=d:errors.append('Harness changed after group: '+group+'/'+n)
  for gate in r['gates']:
   path=OUT/gate['report'];proof=load(path)
   if not gate['passed'] or gate['returncode'] or not proof.get('passed') or sha(path)!=gate['report_sha256']:errors.append('Gate failed/changed: '+gate['report'])
   if sha(OUT/gate['log'])!=gate['log_sha256']:errors.append('Gate transcript differs: '+gate['log'])
   if 'build_id' in proof and proof['build_id']!=base['build_id']:errors.append('Gate build differs: '+gate['report'])
   is_distro=gate['report']=='regression/distribution/results.json';expected=night['artifacts']['EngineOS-M71NIGHT.iso']['sha256'] if is_distro else base['artifacts']['EngineOS-M71.iso']['sha256']
   if 'iso_sha256' in proof and proof['iso_sha256']!=expected:errors.append('Gate media differs: '+gate['report'])
   if is_distro and (proof['custom_build_id']!=night['build_id'] or proof['base_build_id']!=base['build_id']):errors.append('Distro identity differs')
   if gate['test']=='vmware' and not proof.get('existing_vm_inventory_unchanged'):errors.append('VMware inventory changed')
   checks=proof.get('checks',proof.get('cases',[]))
   if any(isinstance(c,dict) and c.get('passed') is False for c in checks):errors.append('Subcheck failed: '+gate['report'])
   gates.append({'group':group,'report':gate['report'],'passed':proof.get('passed') is True,'checks':len(checks)});frozen[gate['report']]=sha(path)
  frozen[p.relative_to(OUT).as_posix()]=sha(p)
 repo=load(OUT/'repository-results.json')
 if not repo['passed']:errors.append('Repository documentation checks failed')
 gates.append({'group':'repository','report':'repository-results.json','passed':repo['passed'],'checks':len(repo['checks'])});frozen['repository-results.json']=sha(OUT/'repository-results.json')
 resources=load(OUT/'regression/resources/results.json')
 if len(resources['profiles'])!=3 or any(not p['idle_all_cpus_halted'] or p['idle_repaint_count'] or not p['allocation_failures_observed'] for p in resources['profiles']):errors.append('Pressure/idle recovery failed')
 disk=load(OUT/'disk-results.json')
 if disk['source_sha256']!=sha(ROOT/'kernel/install/disk71.c'):errors.append('Host disk test C differs')
 # Only this new milestone is normalized. Never touch earlier accepted evidence.
 for p in OUT.rglob('*'):
  if p.is_file() and 'history' not in p.relative_to(OUT).parts and p.suffix=='.log':
   text=p.read_text(encoding='utf-8').replace('\r\n','\n');p.write_bytes(('\n'.join(v.rstrip(' \t') for v in text.splitlines())+'\n').encode())
 for p in OUT.rglob('*'):
  if p.is_file() and 'history' not in p.relative_to(OUT).parts and p.suffix in ['.png','.log','.pcap']:frozen[p.relative_to(OUT).as_posix()]=sha(p)
 for name,m in [('m71-build-manifest.json',base),('m71night-build-manifest.json',night)]:
  p=OUT/name;p.write_bytes((json.dumps(m,indent=2)+'\n').encode());frozen[name]=sha(p)
 result={'passed':not errors,'scope_id':'M71-VM-v1','code_source_commit':commit,'build_id':base['build_id'],'iso_sha256':base['artifacts']['EngineOS-M71.iso']['sha256'],'distribution_build_id':night['build_id'],'gate_count':len(gates),'check_count':sum(g['checks'] for g in gates),'gates':gates,'evidence_sha256':frozen,'prior_frozen_proofs_verified':prior,'partition_editor_implemented':True,'desktop_implemented':True,'physical_hardware_qualified':False,'human_usability_study':False,'universal_hardware_support':False,'windows_dos_runtime_implemented':False,'limitations':['canonical 128x128 GPT and primary MBR; logical/extended mutation, resize/move and non-destructive conversion not supported','FAT32/Engine quick formats only; no secure erase or physical power-loss guarantee','whole-disk installation only; no install into a selected existing partition or dual boot','SATA/AHCI 512-byte sectors, named BIOS/UEFI virtual profiles and Secure Boot off'],'public_transcripts':'UTF-8/LF and trimmed prompt whitespace after runtime consumption; private roots redacted; earlier accepted proof untouched','errors':errors}
 (OUT/'acceptance-results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print('M7.1 acceptance '+('PASS' if not errors else 'FAIL')+': '+str(len(gates))+' gates, '+str(prior)+' prior proofs unchanged')
 for e in errors:print(e)
 if errors:raise SystemExit(1)


if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--source-commit',required=True);main(p.parse_args().source_commit)
