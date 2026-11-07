"""Bind the visual delivery to committed native bytes and exact artifact proof."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'evidence/M7.3'
spec=importlib.util.spec_from_file_location('verify7',ROOT/'tests/m7/verify.py');old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old)
sha,load=old.sha,old.load
def main(commit):
 commit=subprocess.check_output(['git','rev-parse',commit],cwd=ROOT,text=True).strip();meta=load(ROOT/'dist/m73-build-manifest.json');runtime=load(OUT/'runtime-results.json');starter=load(ROOT/'dist/m73-starter-manifest.json');errors=[]
 published=old.committed_hashes(commit,sorted(meta['source_sha256']))
 for name,digest in meta['source_sha256'].items():
  if sha(ROOT/name)!=digest or published[name]!=digest:errors.append('Native bytes differ: '+name)
 artifacts=dict(meta['artifacts']);artifacts[starter['archive']]={k:starter[k] for k in ['sha256','size']}
 for name,value in artifacts.items():
  p=ROOT/'dist'/name
  if not p.exists() or sha(p)!=value['sha256'] or p.stat().st_size!=value['size']:errors.append('Artifact differs: '+name)
 if not runtime['passed'] or len(runtime['gates'])!=14 or not runtime['native_inputs_unchanged']:errors.append('Runtime gates incomplete')
 if runtime['build_id']!=meta['build_id'] or runtime['iso_sha256']!=artifacts['EngineOS-M73.iso']['sha256']:errors.append('Runtime identity differs')
 for name,digest in runtime['test_sha256'].items():
  if sha(ROOT/name)!=digest:errors.append('Harness changed: '+name)
 gates=[];resources=[]
 for gate in runtime['gates']:
  proof=load(OUT/gate['report'])
  if not gate['passed'] or gate['returncode'] or not proof['passed'] or sha(OUT/gate['report'])!=gate['report_sha256'] or sha(OUT/gate['log'])!=gate['log_sha256']:errors.append('Gate differs/failed: '+gate['name'])
  if 'build_id' in proof and proof['build_id']!=meta['build_id']:errors.append('Gate build differs: '+gate['name'])
  if 'iso_sha256' in proof and proof['iso_sha256']!=artifacts['EngineOS-M73.iso']['sha256']:errors.append('Gate ISO differs: '+gate['name'])
  if gate['name'].startswith('vmware') and (not proof['existing_vm_inventory_unchanged'] or proof['boot_order_forced'] or not proof['same_vm_reused']):errors.append('VMware ownership/discovery failed')
  if gate['name']=='vmware-starter' and proof['starter_sha256']!=starter['sha256']:errors.append('Tested ZIP differs')
  if 'managed_current_bytes' in proof:
   if not proof['idle_all_cpus_halted'] or proof['idle_repaint_count'] or proof['navigation_retained_page_delta'] or proof['navigation_allocation_failures']:errors.append('Resource gate failed')
   resources.append({k:proof[k] for k in ['resolution','managed_current_bytes','managed_peak_bytes','navigation_retained_page_delta','navigation_allocation_failures','idle_all_cpus_halted','idle_repaint_count']})
  gates.append(dict(name=gate['name'],report=gate['report'],passed=proof['passed']))
 if len(resources)!=6:errors.append('Display/resource profiles incomplete')
 four=load(OUT/'uefi-3840x2160/results.json')
 if four['resolution']!=[3840,2160] or four['ui_scale_percent']!=200:errors.append('Actual 4K/200% evidence missing')
 repository=load(OUT/'repository-results.json');review=load(OUT/'visual-review.json')
 if not repository['passed'] or not review['passed'] or review['human_usability_study']:errors.append('Repository/design review missing or misrepresented')
 gates += [dict(name='repository',report='repository-results.json',passed=repository['passed']),dict(name='visual-review',report='visual-review.json',passed=review['passed'])]
 historical=0;retained={}
 for milestone in ['M4.5','M5','M5.1','M6','M7','M7.1','M7.2']:
  previous=load(ROOT/'evidence'/milestone/'acceptance-results.json')
  if not previous['passed']:errors.append('Prior acceptance failed: '+milestone)
  for name,digest in previous['evidence_sha256'].items():
   if sha(ROOT/'evidence'/milestone/name)!=digest:errors.append('Prior proof differs: '+milestone+'/'+name)
   historical+=1
 for prefix in ['m6','m6u1','m6minimal','m7','m7night','m71','m71night','m72']:
  previous=load(ROOT/'dist'/(prefix+'-build-manifest.json'))
  for name,value in previous['artifacts'].items():
   if sha(ROOT/'dist'/name)!=value['sha256']:errors.append('Prior artifact differs: '+name)
   retained[name]=value['sha256']
 previous=load(ROOT/'dist/m72-starter-manifest.json');retained[previous['archive']]=previous['sha256']
 if sha(ROOT/'dist'/previous['archive'])!=previous['sha256']:errors.append('Prior VMware ZIP differs')
 for name in ['m73-build-manifest.json','m73-starter-manifest.json']:shutil.copyfile(ROOT/'dist'/name,OUT/name)
 (OUT/'historical-artifacts.json').write_bytes((json.dumps(dict(passed=not errors,artifacts=retained),indent=2)+'\n').encode())
 font=meta['font'];lock=load(ROOT/'third_party/inter/raster-lock.json')
 if any(font[k]!=lock[k] for k in lock) or font['coverage_bytes']>1024*1024:errors.append('Font rasterizer lock/bound differs')
 kernel=(ROOT/'dist/EngineOS-M73-installed-kernel.elf').read_bytes();import struct
 offset=struct.unpack_from('<Q',kernel,32)[0];count=struct.unpack_from('<H',kernel,56)[0];loads=[struct.unpack_from('<IIQQQQQQ',kernel,offset+n*56) for n in range(count)];loads=[v for v in loads if v[0]==1]
 frozen={p.relative_to(OUT).as_posix():sha(p) for p in sorted(OUT.rglob('*')) if p.is_file() and p.name!='acceptance-results.json'}
 report=dict(passed=not errors,scope_id='M73-VM-v1',code_source_commit=commit,build_id=meta['build_id'],iso_sha256=artifacts['EngineOS-M73.iso']['sha256'],vmware_starter_sha256=starter['sha256'],gate_count=len(gates),gates=gates,artifacts=artifacts,prior_frozen_proofs_verified=historical,prior_binary_artifacts_verified=len(retained),resources=resources,font=font,kernel_physical_span_bytes=max(v[4]+v[6] for v in loads)-min(v[4] for v in loads),visible_4k_framebuffer_bytes=3840*2160*4,evidence_sha256=frozen,actual_4k_framebuffer=True,human_usability_study=False,physical_hardware_qualified=False,arm_raspberry_pi_implemented=False,windows_dos_runtime_implemented=False,limitations=meta['limits'],errors=errors)
 (OUT/'acceptance-results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(dict(passed=not errors,gates=len(gates),prior_proofs=historical,errors=errors),indent=2))
 if errors:raise SystemExit(1)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--commit',default='HEAD');main(p.parse_args().commit)
