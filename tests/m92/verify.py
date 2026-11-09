"""Bind the storage delivery to exact source, artifacts and complete owned tasks."""
import argparse,hashlib,json,re,shutil,subprocess,sys,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'evidence/M9.2'
sys.path.insert(0,str(ROOT/'tools/build'))
from m8 import privacy_input

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def load(p):return json.loads(Path(p).read_text(encoding='utf-8'))
def main():
 p=argparse.ArgumentParser();p.add_argument('--source-commit',required=True);a=p.parse_args();commit=subprocess.check_output(['git','rev-parse',a.source_commit],cwd=ROOT,text=True,encoding='utf-8').strip();meta=load(ROOT/'dist/m92-build-manifest.json');iso=meta['artifacts']['EngineOS-M92.iso']['sha256'];assert sha(ROOT/'dist/EngineOS-M92.iso')==iso
 names=list(meta['source_sha256']);raw=subprocess.check_output(['git','cat-file','--batch'],cwd=ROOT,input=''.join(commit+':'+n+'\n' for n in names).encode());at=0
 for name in names:
  end=raw.index(b'\n',at);header=raw[at:end].split();assert len(header)==3 and header[1]==b'blob',name;size=int(header[2]);content=raw[end+1:end+1+size];at=end+size+2;assert hashlib.sha256(content).hexdigest()==meta['source_sha256'][name]==sha(ROOT/name),name
 OUT.mkdir(parents=True,exist_ok=True);gates=[];reports={}
 def gate(directory,filename='results.json',native=True):
  source=ROOT/'build/m92'/directory;value=load(source/filename);assert value['passed'],directory
  if native:assert value['build_id']==meta['build_id'] and value['iso_sha256']==iso,directory+' is stale'
  else:
   for name,digest in value['source_sha256'].items():assert sha(ROOT/name)==digest,name
  target=OUT/directory;target.mkdir(parents=True,exist_ok=True)
  for file in source.iterdir():
   if file.is_file() and file.suffix in ['.json','.png','.log'] and file.name not in ['last-run.log','difference.png']:
    shutil.copyfile(file,target/('results.json' if file.name==filename else file.name))
  gates.append(directory+'/results.json');reports[directory]=value;return value
 pairs=[(c,f) for c in ['sata','ide','lsilogic','lsisas1068','pvscsi','nvme'] for f in ['efi','bios']]+[('buslogic','bios')]
 for controller,firmware in pairs+[('sata','efi-starter')]:
  r=gate('vmware-results/'+controller+'-'+firmware);assert r['same_vm_reused'] and not r['boot_order_forced'] and r['existing_vm_inventory_unchanged'] and r['network_present'] and r['dhcp_configured_with_shipped_nat'];assert r['persisted_distribution_sha256']
  if firmware=='efi-starter':assert r['hardware_input_during_protected_worker_verified'] and r['windows_lifecycle_exercised'] and len(r['original_payload_sha256'])==106 and r['starter_sha256']==sha(ROOT/'dist/EngineOS-M92-VMware.zip')
 for controller in ['ide','mptsas']:gate('replacement/'+controller);gate('safety/'+controller)
 gate('negative-pvscsi');gate('readonly-virtio');gate('window-host',native=False);gate('storage-host',native=False)
 for issue in ['scsi-target1-ovmf','scsi-target0-mixed-ovmf']:
  known=ROOT/'build/m92/known-issues'/issue;assert not load(known/'results.json')['passed'] and not load(known/'results.json')['qualified'];dest=OUT/'known-issues'/issue;dest.mkdir(parents=True,exist_ok=True)
  for name in ['results.json','last-run.log']:shutil.copyfile(known/name,dest/name)
 for resolution in ['1280x800','2560x1440','3840x2160']:
  r=gate('native-motion/M92-'+resolution);assert r['framebuffer']==list(map(int,resolution.split('x'))) and r['managed_bytes_before_drag']==r['managed_bytes_after_drag'] and r['idle_processors_halted'] and r['idle_repaint_unchanged'];assert len(r['comparisons'])==6 and all(v['whole_viewport_matches_full_repaint'] for v in r['comparisons'])
 for name in ['virtio-uefi-1280x800-128-2','ide-bios-1280x800-64-1','nvme-uefi-3840x2160-128-2']:
  r=gate('journey/'+name,'development-results.json');assert len(r['original_executable_and_dll_sha256'])==2
  if '-64-1' in name:assert r['observed_guest_memory_bytes']==64*1024**2 and r['observed_processor_count']==1
 assert load(OUT/'repository-results.json')['passed'];gates.append('repository-results.json');visual=load(OUT/'visual-review.json');assert visual['passed'] and visual['build_id']==meta['build_id'] and visual['iso_sha256']==iso;assert not visual['human_usability_study'];gates.append('visual-review.json')
 for name in visual['reviewed_captures']:assert (OUT/name).is_file() and sha(OUT/name)==visual['reviewed_capture_sha256'][name],name
 historical=0;oldartifacts={}
 for milestone in ['M4.5','M5','M5.1','M6','M7','M7.1','M7.2','M7.3','M7.4','M7.5','M8','M9','M9.1']:
  old=load(ROOT/'evidence'/milestone/'acceptance-results.json');assert old['passed']
  for name,digest in old['evidence_sha256'].items():assert sha(ROOT/'evidence'/milestone/name)==digest,milestone+'/'+name;historical+=1
  oldartifacts.update(old.get('artifacts',{}))
 oldartifacts.update({k:{'sha256':v} for k,v in load(ROOT/'evidence/M7.3/historical-artifacts.json')['artifacts'].items()})
 for name,value in oldartifacts.items():assert sha(ROOT/'dist'/name)==value['sha256'],name
 artifacts=dict(meta['artifacts'])
 for name in ['EngineOS-M92-VMware.zip','EngineOS-M92-THIRD-PARTY-NOTICES.txt']:
  path=ROOT/'dist'/name;artifacts[name]={'sha256':sha(path),'size':path.stat().st_size}
 for name,value in artifacts.items():
  path=ROOT/'dist'/name;assert sha(path)==value['sha256'] and path.stat().st_size==value['size'];assert not re.search(rb'[Cc]:[\\/]+Users[\\/]+',privacy_input(path.read_bytes())),name
 with zipfile.ZipFile(ROOT/'dist/EngineOS-M92-VMware.zip') as z:
  assert z.testzip() is None and hashlib.sha256(z.read('EngineOS/EngineOS-M92.iso')).hexdigest()==iso
  assert z.read('EngineOS/7z2606-src.tar.xz')==(ROOT/'third_party/7zip/7z2606-src.tar.xz').read_bytes()
  cfg=z.read('EngineOS/EngineOS.vmx').decode();assert 'pciBridge4.virtualDev = "pcieRootPort"' in cfg and 'ethernet0.present = "TRUE"' in cfg and 'sound.present = "TRUE"' in cfg
 for name in ['m92-build-manifest.json','m92-starter-manifest.json']:shutil.copyfile(ROOT/'dist'/name,OUT/name)
 for path in OUT.rglob('*'):
  if path.is_file() and path.suffix in ['.json','.log','.md','.txt']:assert not re.search(rb'[Cc]:[\\/]+Users[\\/]+',path.read_bytes()),path.name
 report={'passed':True,'scope_id':'M92-STORAGE-VM-v1','code_source_commit':commit,'build_id':meta['build_id'],'iso_sha256':iso,'vmware_starter_sha256':sha(ROOT/'dist/EngineOS-M92-VMware.zip'),'controller_firmware_matrix':[{'controller':c,'firmware':f,'passed':True} for c,f in pairs],'gates':[{'report':n,'passed':True} for n in gates],'gate_count':len(gates),'artifacts':artifacts,'prior_frozen_proofs_verified':historical,'prior_binary_artifacts_verified':len(oldartifacts),'actual_4k_framebuffer':True,'tested_64_mib_single_cpu_application_journey':True,'human_usability_study':False,'physical_hardware_qualified':False,'gpu_3d_acceleration':False,'complete_m9_hardware_program':False,'limits':meta['limits']+['BusLogic uses BIOS with the compatible freebsd device preset; the actual kernel remains x86-64','QEMU PVSCSI short completion is refused; VMware PVSCSI is separately tested','QEMU/OVMF retained-ISO startup from LSI SAS in the tested multi-disk topologies remains unqualified; see known-issues','No physical controllers, 4Kn installation, universal device support or multiple-monitor desktop qualification'],'reproduction_harness_sha256':{x.relative_to(ROOT).as_posix():sha(x) for x in sorted((ROOT/'tests/m92').glob('*')) if x.is_file()},'evidence_sha256':{x.relative_to(OUT).as_posix():sha(x) for x in sorted(OUT.rglob('*')) if x.is_file() and x.name!='acceptance-results.json'}}
 (OUT/'acceptance-results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M9.2 PASS:',len(gates),'exact-artifact gates;',historical,'frozen proofs unchanged')
if __name__=='__main__':main()
