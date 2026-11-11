"""Freeze M11.4 only from matching native, VMware, font and resource evidence."""
import hashlib,json,os,re,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'evidence/M11/4'
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def load(p):return json.loads(p.read_text(encoding='utf-8'))
def write(p,value):p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes((json.dumps(value,indent=2)+'\n').encode())
def main():
 meta=load(ROOT/'dist/m11stage4-build-manifest.json');starter=load(ROOT/'dist/m11stage4-starter-manifest.json')
 root=ROOT/'build/m11/windows';native=load(root/'results.json');keys=load(root/'keys/results.json');motion=load(root/'motion/results.json');vmware=load(root/'vmware/results.json');host=load(ROOT/'build/m11/input-host/results.json')
 assert all(r['passed'] for r in (starter,native,keys,motion,vmware,host))
 assert meta['build_revision']=='stage4' and meta['release_version']=='0.11.4'
 iso=meta['artifacts']['EngacerOS-M11STAGE4.iso']['sha256']
 assert all(r['iso_sha256']==iso for r in (native,keys,motion,vmware))
 assert starter['build_id']==vmware['build_id']==meta['build_id'] and starter['sha256']==vmware['starter_sha256']==digest(ROOT/'dist'/starter['archive'])
 for path,expected in meta['source_sha256'].items():assert digest(ROOT/path)==expected,('Changed source',path)
 for path,record in meta['artifacts'].items():assert digest(ROOT/'dist'/path)==record['sha256'],('Changed artifact',path)
 for path,expected in host['source_sha256'].items():assert digest(ROOT/path)==expected
 assert native['window_operations_retained_managed_page_delta']==0 and len(native['checks'])>=11 and len(keys['checks'])==2
 assert vmware['existing_vm_inventory_unchanged'] and len(vmware['checks'])==3
 assert [p['physical'] for p in motion['profiles']]==[[1280,800],[1920,1200],[3840,2160]]
 for p in motion['profiles']:
  assert p['managed_bytes_before']==p['managed_bytes_after'] and p['cached_copy_frames']>0 and p['all_cpus_halted_when_idle'] and p['idle_paints_unchanged']
  assert len(p['comparisons'])==4 and all(c['cached_equals_full_viewport'] for c in p['comparisons'])
 memory={}
 for stage in ('STAGE1','STAGE3','STAGE4'):
  path=ROOT/('dist/EngacerOS-M11'+stage+'-installed-kernel.map');text=path.read_text();parts={}
  for section in ('bss','data'):
   values=re.findall(r'^\s*[0-9a-f]+\s+[0-9a-f]+\s+([0-9a-f]+)\s+\d+\s+.*kernel_desktop_session-kernel.o:\(\.'+section+r'\)$',text,re.M);assert len(values)==1;parts[section]=int(values[0],16)
  memory[stage]={'bytes':sum(parts.values()),'sections':parts,'map_sha256':digest(path)}
 delta=memory['STAGE4']['bytes']-memory['STAGE3']['bytes'];total=memory['STAGE4']['bytes']-memory['STAGE1']['bytes']
 assert 0<delta<64*1024 and total<512*1024
 font=load(ROOT/'build/m11/font73-manifest.json');assert font['secondary_sizes']==[14,21,28] and font['coverage_bytes']<1024**2
 assert font['header_sha256']==load(ROOT/'third_party/inter/raster-lock11.json')['header_sha256']
 frozen=[]
 for path in [ROOT/'evidence/M10/m10-build-manifest.json']+[ROOT/('evidence/M11/'+str(n)+'/candidate-manifest.json') for n in (1,2,3)]:
  old=load(path)
  for name,record in old['artifacts'].items():assert digest(ROOT/'dist'/name)==record['sha256'],('Accepted artifact changed',name)
  frozen.append({'record':path.relative_to(ROOT).as_posix(),'artifact_count':len(old['artifacts']),'unchanged':True})
 assert digest(ROOT/'dist/EngacerOS-M10-VMware.zip')==load(ROOT/'evidence/M10/m10-starter-manifest.json')['sha256']
 resources={'passed':True,'scope':'Fixed native desktop BSS/data is separate from total kernel, font, firmware and shared framebuffer memory; configured RAM is not consumption','stages':memory,'added_from_stage3_bytes':delta,'added_from_stage1_bytes':total,'total_budget_bytes':512*1024,'font':font,'native':motion['profiles']}
 for folder in ('native','keys','motion','vmware'):(OUT/folder).mkdir(parents=True,exist_ok=True)
 for source,dest in [(root,OUT/'native'),(root/'keys',OUT/'keys'),(root/'motion',OUT/'motion'),(root/'vmware',OUT/'vmware')]:
  for file in source.iterdir():
   if file.is_file() and file.suffix in ('.png','.json','.log'):
    raw=file.read_bytes()
    if file.suffix!='.png':raw=raw.replace(str(ROOT).encode(),b'<project-root>')
    shutil.copyfile(file,dest/file.name) if file.suffix=='.png' else (dest/file.name).write_bytes(raw)
 base=Path(os.environ['LOCALAPPDATA'])/'EngineOS/Tests/M6/m11-windows';owned=max(base.iterdir(),key=lambda p:p.stat().st_mtime_ns)
 for name in ('install','disk-only','native-4k'):
  text=(owned/name/'serial.log').read_text(errors='replace');assert 'M7_DESKTOP_READY build='+meta['build_id'] in text and 'KERNEL_PANIC' not in text
  (OUT/'native'/(name+'.log')).write_bytes(text.encode())
 tests=['tests/m11/windows.py','tests/m11/window_keys.py','tests/m11/window_motion.py','tests/m11/vmware_windows.py','tests/m11/verify_windows.py','tests/m11/ui.py','tools/release/vmware_starter11.py']
 for name,value in [('candidate-manifest.json',meta),('starter-manifest.json',starter),('resources.json',resources),('pointer-host-results.json',host),('preserved-releases.json',frozen)]:write(OUT/name,value)
 report={'passed':True,'stage':'M11.4','version':'0.11.4','scope':'Native QEMU ordinary/150%/4K and exact enabled-device VMware SATA/UEFI starter installation, retained-ISO restart and two disk-only starts; full controller/original-app regression remains M11.6','build_id':meta['build_id'],'iso_sha256':iso,'vmware_zip_sha256':starter['sha256'],'source_inputs_verified':len(meta['source_sha256']),'artifacts_verified':len(meta['artifacts'])+1,'native_tasks':native['checks']+keys['checks']+vmware['checks'],'motion_profiles':len(motion['profiles']),'cached_full_frame_comparisons':12,'host_checks':host['checks'],'added_fixed_desktop_bytes':delta,'test_source_sha256':{p:digest(ROOT/p) for p in tests},'publication':'A separate visible GitHub release, complete English manual/site/download update and fresh public proof must close before M11.5'}
 write(OUT/'acceptance-results.json',report);print('M11.4 matching-source/native/VMware/resource acceptance PASS')
if __name__=='__main__':main()
