"""Bind the first graphics delivery to exact native inputs and owned guest tasks."""
import argparse,hashlib,json,re,shutil,subprocess,sys,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'evidence/M9'
sys.path.insert(0,str(ROOT/'tools/build'))
from m8 import privacy_input

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def load(path):return json.loads(Path(path).read_text(encoding='utf-8'))
def copy_gate(source,target,report='results.json',native=True):
    value=load(source/report);assert value['passed'],str(source)
    if native:assert value['build_id']==META['build_id'] and value['iso_sha256']==ISO,str(source)
    target.mkdir(parents=True,exist_ok=True)
    for file in source.iterdir():
        if file.is_file() and file.suffix in ('.json','.log','.png'):
            shutil.copyfile(file,target/('results.json' if file.name==report else file.name))
    return value

def main():
    global META,ISO
    p=argparse.ArgumentParser();p.add_argument('--source-commit',required=True);a=p.parse_args()
    commit=subprocess.check_output(['git','rev-parse',a.source_commit],cwd=ROOT,text=True).strip()
    META=load(ROOT/'dist/m9-build-manifest.json');ISO=META['artifacts']['EngineOS-M9.iso']['sha256']
    names=list(META['source_sha256']);request=''.join(commit+':'+n+'\n' for n in names).encode()
    data=subprocess.check_output(['git','cat-file','--batch'],cwd=ROOT,input=request);at=0
    for name in names:
        end=data.index(b'\n',at);header=data[at:end].split();assert len(header)==3 and header[1]==b'blob',name
        size=int(header[2]);content=data[end+1:end+1+size];at=end+size+2
        assert hashlib.sha256(content).hexdigest()==META['source_sha256'][name]==sha(ROOT/name),name
    gates=[];reports={}
    for directory,filename in [('journey/uefi-3840x2160-128','development-results.json'),('journey/bios-1280x800-64','development-results.json'),
                                ('native-motion/M9-1280x800','results.json'),('native-motion/M9-3840x2160','results.json'),
                                ('motion-results/M9','results.json'),('fallback','results.json'),('allocator','results.json')]:
        reports[directory]=copy_gate(ROOT/'build/m9'/directory,OUT/directory,filename,directory!='allocator')
        gates.append(directory+'/results.json')
    assert reports['journey/uefi-3840x2160-128']['actual_framebuffer']==[3840,2160]
    assert reports['journey/bios-1280x800-64']['observed_guest_memory_bytes']==64*1024**2
    for resolution in ['1280x800','3840x2160']:
        motion=reports['native-motion/M9-'+resolution]
        assert motion['cached_matches_full_repaint'] and motion['frames']>=24 and motion['graphics']['copy_frames']>=motion['frames']-2
        assert motion['managed_bytes_before_drag']==motion['managed_bytes_after_drag']
        assert motion['idle_processors_halted'] and motion['idle_repaint_unchanged']
    vm_motion=reports['motion-results/M9']
    assert vm_motion['driver_fifo_active'] and vm_motion['visible_frames']>=24 and vm_motion['existing_vm_inventory_unchanged']
    log=(OUT/'motion-results/M9/serial.log').read_text()
    assert 'M9_VMWARE_ABSOLUTE_POINTER_ACTIVE' in log and 'M9_SVGA_QUARANTINED' not in log
    for name,digest in reports['allocator']['source_sha256'].items():assert sha(ROOT/name)==digest,name
    assert reports['fallback']['backend']==0 and reports['fallback']['shadow_bytes']==0
    performance=load(OUT/'performance/results.json')
    baseline=load(OUT/'performance/m8-baseline-results.json')
    assert performance['passed'] and baseline['passed']
    assert performance['candidate_build_id']==META['build_id'] and performance['baseline_build_id']==load(ROOT/'evidence/M8/acceptance-results.json')['build_id']
    assert performance['candidate_median_ms']==reports['native-motion/M9-1280x800']['median_ms']
    assert performance['median_reduction_percent']>0 and performance['p95_reduction_percent']>0
    gates.append('performance/results.json')
    starter=load(ROOT/'dist/m9-starter-manifest.json')
    for firmware in ['efi-starter','bios']:
        name='vmware/'+firmware+'/results.json';value=load(OUT/name)
        assert value['passed'] and value['build_id']==META['build_id'] and value['iso_sha256']==ISO
        assert value['same_vm_reused'] and not value['boot_order_forced'] and value['existing_vm_inventory_unchanged']
        assert len(value['original_payload_sha256'])==106
        if firmware=='efi-starter':assert value['starter_sha256']==starter['sha256']
        gates.append(name)
    for name in ['repository-results.json','visual-review.json']:
        assert load(OUT/name)['passed'];gates.append(name)
    visual=load(OUT/'visual-review.json');assert visual['build_id']==META['build_id'] and visual['iso_sha256']==ISO and not visual['human_usability_study']
    for name in visual['reviewed_captures']:assert (OUT/name).is_file(),name
    frozen=0;old_artifacts={}
    for milestone in ['M4.5','M5','M5.1','M6','M7','M7.1','M7.2','M7.3','M7.4','M7.5','M8']:
        old=load(ROOT/'evidence'/milestone/'acceptance-results.json');assert old['passed']
        for name,digest in old['evidence_sha256'].items():
            assert sha(ROOT/'evidence'/milestone/name)==digest,milestone+'/'+name;frozen+=1
        old_artifacts.update(old.get('artifacts',{}))
    old_artifacts.update({k:{'sha256':v} for k,v in load(ROOT/'evidence/M7.3/historical-artifacts.json')['artifacts'].items()})
    for name,value in old_artifacts.items():assert sha(ROOT/'dist'/name)==value['sha256'],name
    artifacts=dict(META['artifacts']);artifacts['EngineOS-M9-VMware.zip']={'sha256':starter['sha256'],'size':starter['size']}
    notice=ROOT/'dist/EngineOS-M9-THIRD-PARTY-NOTICES.txt';artifacts[notice.name]={'sha256':sha(notice),'size':notice.stat().st_size}
    for name,value in artifacts.items():
        path=ROOT/'dist'/name;raw=path.read_bytes()
        assert len(raw)==value['size'] and hashlib.sha256(raw).hexdigest()==value['sha256'],name
        assert not re.search(rb'[Cc]:[\\/]+Users[\\/]+',privacy_input(raw)),name
    with zipfile.ZipFile(ROOT/'dist/EngineOS-M9-VMware.zip') as z:
        assert z.testzip() is None and hashlib.sha256(z.read('EngineOS/EngineOS-M9.iso')).hexdigest()==ISO
        assert z.read('EngineOS/7z2606-src.tar.xz')==(ROOT/'third_party/7zip/7z2606-src.tar.xz').read_bytes()
        for name in z.namelist():assert not re.search(rb'[Cc]:[\\/]+Users[\\/]+',privacy_input(z.read(name))),name
    for name in ['m9-build-manifest.json','m9-starter-manifest.json']:shutil.copyfile(ROOT/'dist'/name,OUT/name)
    for file in OUT.rglob('*'):
        if file.is_file() and file.suffix in ['.json','.log','.md','.txt']:
            assert not re.search(rb'[Cc]:[\\/]+Users[\\/]+',file.read_bytes()),file.name
    report={'passed':True,'scope_id':'M9-GFX-VM-v1','code_source_commit':commit,'build_id':META['build_id'],'iso_sha256':ISO,
            'vmware_starter_sha256':starter['sha256'],'gates':[{'report':n,'passed':True} for n in gates],'gate_count':len(gates),
            'artifacts':artifacts,'prior_frozen_proofs_verified':frozen,'prior_binary_artifacts_verified':len(old_artifacts),
            'actual_4k_framebuffer':True,'tested_64_mib_application_journey':True,'tested_over_cap_framebuffer_wizard_fallback_only':True,
            'human_usability_study':False,'physical_hardware_qualified':False,'gpu_3d_acceleration':False,'complete_m9_hardware_program':False,
            'limits':META['limits']+['Firmware display mode retained; runtime mode changes and universal GPU binding remain follow-ups','Motion timing includes test-observation overhead and is not monitor FPS'],
            'reproduction_harness_sha256':{x.relative_to(ROOT).as_posix():sha(x) for x in sorted((ROOT/'tests/m9').glob('*')) if x.is_file()},
            'evidence_sha256':{x.relative_to(OUT).as_posix():sha(x) for x in sorted(OUT.rglob('*')) if x.is_file() and x.name!='acceptance-results.json'}}
    (OUT/'acceptance-results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(f'M9 graphics PASS: {len(gates)} exact-artifact gates; {frozen} frozen prior proofs unchanged')

if __name__=='__main__':main()
