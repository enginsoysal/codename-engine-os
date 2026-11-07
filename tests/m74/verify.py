"""Bind the first bounded app delivery to committed code and native evidence."""
import argparse,hashlib,importlib.util,json,shutil,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'evidence/M7.4'
spec=importlib.util.spec_from_file_location('previous_verify',ROOT/'tests/m7/verify.py');old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old)
sha,load=old.sha,old.load

def main(commit):
 commit=subprocess.check_output(['git','rev-parse',commit],cwd=ROOT,text=True).strip()
 standard=load(ROOT/'dist/m74-build-manifest.json');minimal=load(ROOT/'dist/m74minimal-build-manifest.json');starter=load(ROOT/'dist/m74-starter-manifest.json')
 sources={**standard['source_sha256'],**minimal['source_sha256']};published=old.committed_hashes(commit,sorted(sources));errors=[];gates=[]
 for name,digest in sources.items():
  if sha(ROOT/name)!=digest or published[name]!=digest:errors.append('Native source differs: '+name)
 artifacts={**standard['artifacts'],**minimal['artifacts'],starter['archive']:{k:starter[k] for k in ['sha256','size']}}
 private_markers=[str(Path.home()).encode(),str(Path.home()).replace('\\','/').encode(),str(Path.home()).encode('utf-16le')]
 for name,v in artifacts.items():
  with (ROOT/'dist'/name).open('rb') as stream:
   tail=b''
   while True:
    chunk=stream.read(1024*1024)
    if not chunk:break
    block=tail+chunk
    if any(marker.lower() in block.lower() for marker in private_markers):errors.append('Private host path in artifact: '+name);break
    tail=block[-512:]
  if sha(ROOT/'dist'/name)!=v['sha256'] or (ROOT/'dist'/name).stat().st_size!=v['size']:errors.append('Artifact differs: '+name)
 native=['apps-development/results.json','audio-development/results.json','packages-4k/results.json','calendar-probe/results.json','uefi-1280x800-64mib/results.json','vmware/efi-starter/results.json','vmware/bios/results.json','omitted-apps/results.json']
 for path in native:
  value=load(OUT/path);meta=minimal if path.startswith('omitted') else standard
  passed=value['passed'] and value['build_id']==meta['build_id'] and value['iso_sha256']==meta['artifacts']['EngineOS-M74'+('MINIMAL' if meta is minimal else '')+'.iso']['sha256']
  if not passed:errors.append('Native gate failed or stale: '+path)
  gates.append({'name':path.split('/')[0],'report':path,'passed':passed})
 for path in ['calculator-host-results.json','inflate-host-results.json','browser-host-results.json','repository-results.json','visual-review.json']:
  value=load(OUT/path);gates.append({'name':path[:-5],'report':path,'passed':value['passed']})
  if not value['passed']:errors.append('Gate failed: '+path)
 package=load(OUT/'packages-4k/results.json');omitted=load(OUT/'omitted-apps/results.json');audio=load(OUT/'audio-development/results.json')
 if len([v for v in package['checks'] if v.startswith('removed and offline reinstalled')])!=16 or len(omitted['checks'])!=16:errors.append('Optional package matrix incomplete')
 if load(OUT/'calendar-probe/results.json')['resolution']!=[3840,2160]:errors.append('Actual 4K typing proof missing')
 if audio['audio_capture']['active_frames']<30000:errors.append('Actual repeated PCM capture missing')
 if load(OUT/'vmware/efi-starter/results.json')['starter_sha256']!=starter['sha256']:errors.append('Tested starter differs')
 for path in ['vmware/efi-starter/results.json','vmware/bios/results.json']:
  value=load(OUT/path)
  if not value['existing_vm_inventory_unchanged'] or value['boot_order_forced'] or not value['same_vm_reused']:errors.append('VM ownership/discovery failed: '+path)
 if load(OUT/'visual-review.json')['human_usability_study']:errors.append('Agent review misrepresented')
 history=0
 for milestone in ['M4.5','M5','M5.1','M6','M7','M7.1','M7.2','M7.3']:
  folder=ROOT/'evidence'/milestone;prior=load(folder/'acceptance-results.json')
  if not prior['passed']:errors.append('Prior acceptance failed: '+milestone)
  for name,digest in prior['evidence_sha256'].items():
   history+=1
   if sha(folder/name)!=digest:errors.append('Prior proof differs: '+milestone+'/'+name)
 retained=load(ROOT/'evidence/M7.3/historical-artifacts.json')['artifacts']
 previous=load(ROOT/'evidence/M7.3/acceptance-results.json')
 retained={**retained,**{k:v['sha256'] for k,v in previous['artifacts'].items()}}
 for name,digest in retained.items():
  if sha(ROOT/'dist'/name)!=digest:errors.append('Prior binary differs: '+name)
 for name in ['m74-build-manifest.json','m74minimal-build-manifest.json','m74-starter-manifest.json']:shutil.copyfile(ROOT/'dist'/name,OUT/name)
 kernel=(ROOT/'dist/EngineOS-M74-installed-kernel.elf').read_bytes();at=struct.unpack_from('<Q',kernel,32)[0];count=struct.unpack_from('<H',kernel,56)[0]
 loads=[struct.unpack_from('<IIQQQQQQ',kernel,at+i*56) for i in range(count)];loads=[v for v in loads if v[0]==1]
 report={'passed':not errors,'scope_id':'M74-VM-v1','delivery':'first bounded standard application suite; richer capabilities remain explicit follow-ups',
 'code_source_commit':commit,'build_id':standard['build_id'],'iso_sha256':standard['artifacts']['EngineOS-M74.iso']['sha256'],'vmware_starter_sha256':starter['sha256'],
 'minimal_build_id':minimal['build_id'],'gate_count':len(gates),'gates':gates,'artifacts':artifacts,'prior_frozen_proofs_verified':history,'prior_binary_artifacts_verified':len(retained),
 'kernel_physical_span_bytes':max(v[4]+v[6] for v in loads)-min(v[4] for v in loads),'font_coverage_bytes':standard['font']['coverage_bytes'],
 'resources':{k:package[k] for k in ['managed_current_bytes','managed_peak_bytes','framebuffer_bytes','opened_window_extra_managed_bytes']},
 'artifacts_private_path_scan_passed':not any('Private host path' in e for e in errors),'audio_dma_peak_bound_bytes':24*4096,'actual_4k_framebuffer':True,'human_usability_study':False,'physical_hardware_qualified':False,'arm_raspberry_pi_implemented':False,'windows_dos_runtime_implemented':False,
 'limitations':standard['limits']+['Basic static HTML/text HTTPS browser; no JavaScript/CSS layout/forms/cookies','PCM WAV/HDA only; BMP only; ZIP Stored/Deflate extraction only','Calendar stores local day notes; reminders/synchronization remain future','Fresh installs qualified; cross-version package-cache migration not qualified'],
 'test_sha256':{p.relative_to(ROOT).as_posix():sha(p) for p in sorted((ROOT/'tests/m74').glob('*.py'))},'errors':errors}
 report['evidence_sha256']={p.relative_to(OUT).as_posix():sha(p) for p in sorted(OUT.rglob('*')) if p.is_file() and p.name!='acceptance-results.json'}
 (OUT/'acceptance-results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode('utf-8'))
 print(json.dumps({'passed':not errors,'gates':len(gates),'prior_proofs':history,'errors':errors},indent=2))
 if errors:raise SystemExit(1)
if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('--commit',default='HEAD');main(parser.parse_args().commit)
