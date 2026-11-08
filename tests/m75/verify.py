"""Bind M7.5 native journeys, publication files and immutable prior evidence."""
import argparse,hashlib,json,re,shutil,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'evidence/M7.5'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def load(p):return json.loads(Path(p).read_text(encoding='utf-8'))
def main():
 p=argparse.ArgumentParser();p.add_argument('--source-commit',required=True);a=p.parse_args();commit=subprocess.check_output(['git','rev-parse',a.source_commit],cwd=ROOT,text=True).strip()
 meta=load(ROOT/'dist/m75-build-manifest.json');starter=load(ROOT/'dist/m75-starter-manifest.json');gates=[]
 # Compare compiled source bytes with both current files and the named Git tree.
 names=list(meta['source_sha256']);request=''.join(commit+':'+n+'\n' for n in names).encode();data=subprocess.check_output(['git','cat-file','--batch'],cwd=ROOT,input=request);at=0
 for name in names:
  end=data.index(b'\n',at);header=data[at:end].split();assert len(header)==3 and header[1]==b'blob',name;size=int(header[2]);content=data[end+1:end+1+size];at=end+2+size
  assert hashlib.sha256(content).hexdigest()==meta['source_sha256'][name]==sha(ROOT/name),'Stale source: '+name
 native=['journey/uefi-1280x800-64/results.json','journey/uefi-3840x2160-128/results.json','journey/bios-1280x800-128/results.json','safety/results.json','replacement/results.json','vmware/efi-starter/results.json','vmware/bios/results.json']
 for name in native:
  value=load(OUT/name);assert value['passed'] and value['build_id']==meta['build_id'] and value['iso_sha256']==meta['artifacts']['EngineOS-M75.iso']['sha256'],name
  gates.append({'report':name,'passed':True})
 for name in ['repository-results.json','visual-review.json']:
  assert load(OUT/name)['passed'],name;gates.append({'report':name,'passed':True})
 for name in native[:3]:
  value=load(OUT/name);assert value['frames']['full_frames75']==1 and value['frames']['name_frames75']>0 and not value['human_usability_study']
  capture=load(OUT/Path(name).parent/'07-real-progress-capture.json');assert [capture['width'],capture['height']]==value['actual_framebuffer']
 for name in native[-2:]:
  value=load(OUT/name);assert value['existing_vm_inventory_unchanged'] and value['same_vm_reused'] and not value['boot_order_forced']
 assert load(OUT/native[-2])['starter_sha256']==starter['sha256']==sha(ROOT/'dist/EngineOS-M75-VMware.zip')
 history=0;artifacts={}
 for milestone in ['M4.5','M5','M5.1','M6','M7','M7.1','M7.2','M7.3','M7.4']:
  old=load(ROOT/'evidence'/milestone/'acceptance-results.json');assert old['passed']
  for name,digest in old['evidence_sha256'].items():assert sha(ROOT/'evidence'/milestone/name)==digest,milestone+'/'+name;history+=1
  artifacts.update(old.get('artifacts',{}))
 artifacts.update({k:{'sha256':v} for k,v in load(ROOT/'evidence/M7.3/historical-artifacts.json')['artifacts'].items()})
 for name,value in artifacts.items():assert sha(ROOT/'dist'/name)==value['sha256'],name
 current=dict(meta['artifacts']);current['EngineOS-M75-VMware.zip']={'sha256':starter['sha256'],'size':starter['size']}
 for name,value in current.items():
  path=ROOT/'dist'/name;raw=path.read_bytes();assert len(raw)==value['size'] and hashlib.sha256(raw).hexdigest()==value['sha256'];assert not re.search(rb'[Cc]:[\\/]+Users[\\/]+',raw),'Private host path in '+name
 for name in ['m75-build-manifest.json','m75-starter-manifest.json']:shutil.copyfile(ROOT/'dist'/name,OUT/name)
 for path in OUT.rglob('*'):
  if path.is_file() and path.suffix in ['.json','.log','.md','.txt']:assert not re.search(rb'[Cc]:[\\/]+Users[\\/]+',path.read_bytes()),'Private evidence path: '+path.name
 binary=(ROOT/'dist/EngineOS-M75-installed-kernel.elf').read_bytes();off=struct.unpack_from('<Q',binary,32)[0];count=struct.unpack_from('<H',binary,56)[0];segments=[struct.unpack_from('<IIQQQQQQ',binary,off+i*56) for i in range(count)];segments=[s for s in segments if s[0]==1]
 report={'passed':True,'scope_id':'M75-VM-v1','code_source_commit':commit,'build_id':meta['build_id'],'iso_sha256':meta['artifacts']['EngineOS-M75.iso']['sha256'],'vmware_starter_sha256':starter['sha256'],'gates':gates,'gate_count':len(gates),'artifacts':current,'prior_frozen_proofs_verified':history,'prior_binary_artifacts_verified':len(artifacts),'kernel_physical_span_bytes':max(s[4]+s[6] for s in segments)-min(s[4] for s in segments),'managed_idle_bytes':load(OUT/native[0])['managed_current_bytes'],'actual_4k_framebuffer':True,'actual_64_mib_guest':True,'human_usability_study':False,'physical_hardware_qualified':False,'original_user_vm_failure_diagnosed':False,'windows_dos_runtime_implemented':False,'limitations':meta['limits']+['Named x86-64 virtual profiles; original user VM configuration has not been independently reproduced','No human novice study or guarantee for every child/older adult','Matching-media redirection only; cross-release application migration not qualified'],'final_reproduction_harness_sha256':{x.relative_to(ROOT).as_posix():sha(x) for x in sorted((ROOT/'tests/m75').glob('*.py'))},'evidence_sha256':{x.relative_to(OUT).as_posix():sha(x) for x in sorted(OUT.rglob('*')) if x.is_file() and x.name!='acceptance-results.json'}}
 (OUT/'acceptance-results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M7.5 PASS: '+str(len(gates))+' exact-artifact gates; '+str(history)+' frozen proofs unchanged')
if __name__=='__main__':main()
