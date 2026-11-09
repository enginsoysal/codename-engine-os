"""Bind first-profile M8 guest tasks to exact native inputs and immutable history."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import zipfile

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'evidence/M8'
sys.path.insert(0,str(ROOT/'tools/build'))
from m8 import privacy_input


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def load(path):return json.loads(Path(path).read_text(encoding='utf-8'))


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--source-commit',required=True)
    args=parser.parse_args()
    commit=subprocess.check_output(['git','rev-parse',args.source_commit],cwd=ROOT,text=True).strip()
    meta=load(ROOT/'dist/m8-build-manifest.json')
    starter=load(ROOT/'dist/m8-starter-manifest.json')
    iso=meta['artifacts']['EngineOS-M8.iso']['sha256']
    names=list(meta['source_sha256'])
    request=''.join(commit+':'+name+'\n' for name in names).encode()
    data=subprocess.check_output(['git','cat-file','--batch'],cwd=ROOT,input=request)
    at=0
    for name in names:
        end=data.index(b'\n',at);header=data[at:end].split()
        assert len(header)==3 and header[1]==b'blob',name
        size=int(header[2]);content=data[end+1:end+1+size];at=end+2+size
        assert hashlib.sha256(content).hexdigest()==meta['source_sha256'][name]==sha(ROOT/name),'Stale native input: '+name
    native=[]
    for directory in ['journey/uefi-3840x2160-128','journey/bios-1280x800-64','lifecycle','safety','recovery']:
        source=ROOT/'build/m8'/directory
        report=source/('development-results.json' if directory.startswith('journey/') or directory=='lifecycle' else 'results.json')
        value=load(report)
        assert value['passed'] and value['build_id']==meta['build_id'] and value['iso_sha256']==iso,directory
        target=OUT/directory;target.mkdir(parents=True,exist_ok=True)
        for path in source.iterdir():
            if path.suffix in ['.png','.json','.log'] and path.name not in ['development-results.json','results.json']:
                shutil.copyfile(path,target/path.name)
        shutil.copyfile(report,target/'results.json')
        native.append(directory+'/results.json')
    native+=['vmware/efi-starter/results.json','vmware/bios/results.json']
    for name in native:
        value=load(OUT/name)
        assert value['passed'] and value['build_id']==meta['build_id'] and value['iso_sha256']==iso,name
    for name in native[-2:]:
        value=load(OUT/name)
        assert value['same_vm_reused'] and value['existing_vm_inventory_unchanged'] and not value['boot_order_forced']
        assert len(value['original_payload_sha256'])==106
    assert load(OUT/native[-2])['starter_sha256']==starter['sha256']==sha(ROOT/'dist/EngineOS-M8-VMware.zip')
    assert load(OUT/native[0])['actual_framebuffer']==[3840,2160]
    assert load(OUT/native[1])['ram_mib']==64 and load(OUT/native[1])['observed_guest_memory_bytes']==64*1024**2
    for directory in ['lifecycle','recovery']:
        assert len(load(OUT/directory/'results.json')['original_payload_sha256'])==106
    resources=load(OUT/'safety/results.json')
    assert resources['resource_measurements']['retained_worker_pages_after_warmup']==0
    assert resources['idle_frame_unchanged']
    fmt=load(ROOT/'build/m8/db-host/format-results.json')
    assert set(fmt['cases'])=={'positive','signature','geometry','extended-fat','directory-cycle','truncated'}
    assert fmt['cases']['positive']['original_payloads']==106 and len(fmt['cases']['positive']['tables'])==13
    assert fmt['cases']['positive']['payload_sha256']==load(ROOT/'third_party/7zip/provenance.json')['original_payload_sha256']
    for case,value in fmt['cases'].items():
        if case!='positive':assert value['rejected'],case
    for name,digest in fmt['inputs_sha256'].items():assert sha(ROOT/name)==digest,name
    rename=load(ROOT/'build/m8/rename/results.json');assert rename['passed'] and rename['fault_cases']==146
    for name,digest in rename['source_sha256'].items():assert sha(ROOT/name)==digest,name
    (OUT/'format').mkdir(exist_ok=True)
    shutil.copyfile(ROOT/'build/m8/db-host/format-results.json',OUT/'format/results.json')
    shutil.copyfile(ROOT/'build/m8/rename/results.json',OUT/'format/rename-results.json')
    gates=native+['format/results.json','format/rename-results.json','repository-results.json','visual-review.json']
    for name in gates[-2:]:assert load(OUT/name)['passed'],name
    visual=load(OUT/'visual-review.json')
    assert visual['build_id']==meta['build_id'] and visual['iso_sha256']==iso and not visual['human_usability_study']
    assert len(visual['questions'])==5 and all(question['passed'] for question in visual['questions'])
    for name in visual['reviewed_captures']:assert (OUT/name).is_file(),name
    history=0;old_artifacts={}
    for milestone in ['M4.5','M5','M5.1','M6','M7','M7.1','M7.2','M7.3','M7.4','M7.5']:
        old=load(ROOT/'evidence'/milestone/'acceptance-results.json');assert old['passed']
        for name,digest in old['evidence_sha256'].items():
            assert sha(ROOT/'evidence'/milestone/name)==digest,milestone+'/'+name
            history+=1
        old_artifacts.update(old.get('artifacts',{}))
    old_artifacts.update({k:{'sha256':v} for k,v in load(ROOT/'evidence/M7.3/historical-artifacts.json')['artifacts'].items()})
    for name,value in old_artifacts.items():assert sha(ROOT/'dist'/name)==value['sha256'],name
    current=dict(meta['artifacts'])
    current['EngineOS-M8-VMware.zip']={'sha256':starter['sha256'],'size':starter['size']}
    for name in ['EngineOS-M8-THIRD-PARTY-NOTICES.txt']:
        current[name]={'sha256':sha(ROOT/'dist'/name),'size':(ROOT/'dist'/name).stat().st_size}
    for name,value in current.items():
        path=ROOT/'dist'/name;raw=path.read_bytes()
        assert len(raw)==value['size'] and hashlib.sha256(raw).hexdigest()==value['sha256'],name
        assert not re.search(rb'[Cc]:[\\/]+Users[\\/]+',privacy_input(raw)),'Private host path: '+name
    with zipfile.ZipFile(ROOT/'dist/EngineOS-M8-VMware.zip') as package:
        assert package.testzip() is None
        assert hashlib.sha256(package.read('EngineOS/EngineOS-M8.iso')).hexdigest()==iso
        assert package.read('EngineOS/7z2606-src.tar.xz')==(ROOT/'third_party/7zip/7z2606-src.tar.xz').read_bytes()
        assert package.read('EngineOS/libmspack-LGPL-2.1.txt')==(ROOT/'third_party/libmspack/COPYING.LIB').read_bytes()
        for name in package.namelist():
            assert not re.search(rb'[Cc]:[\\/]+Users[\\/]+',privacy_input(package.read(name))),'Private archived path: '+name
    for name in ['m8-build-manifest.json','m8-starter-manifest.json']:shutil.copyfile(ROOT/'dist'/name,OUT/name)
    for path in OUT.rglob('*'):
        if path.is_file() and path.suffix in ['.json','.log','.md','.txt']:
            assert not re.search(rb'[Cc]:[\\/]+Users[\\/]+',path.read_bytes()),'Private evidence path: '+path.name
    provenance=load(ROOT/'third_party/7zip/provenance.json')
    binary=(ROOT/'dist/EngineOS-M8-installed-kernel.elf').read_bytes()
    offset=struct.unpack_from('<Q',binary,32)[0];count=struct.unpack_from('<H',binary,56)[0]
    segments=[struct.unpack_from('<IIQQQQQQ',binary,offset+i*56) for i in range(count)]
    segments=[segment for segment in segments if segment[0]==1]
    report={'passed':True,'scope_id':'M8-MSI1-VM-v1','code_source_commit':commit,
            'build_id':meta['build_id'],'iso_sha256':iso,'vmware_starter_sha256':starter['sha256'],
            'gates':[{'report':name,'passed':True} for name in gates],'gate_count':len(gates),
            'artifacts':current,'prior_frozen_proofs_verified':history,'prior_binary_artifacts_verified':len(old_artifacts),
            'original_payload_count':106,'original_package_sha256':sha(ROOT/'third_party/7zip/7z2606-x64.msi'),
            'original_executable_sha256':provenance['original_payload_sha256']['_7z.exe'],
            'original_dll_sha256':provenance['original_payload_sha256']['_7z.dll'],
            'resource_measurements':resources['resource_measurements'],
            'installed_kernel_physical_span_bytes':max(segment[4]+segment[6] for segment in segments)-min(segment[4] for segment in segments),
            'actual_4k_framebuffer':True,'tested_64_mib_guest_configuration':True,
            'human_usability_study':False,'physical_hardware_qualified':False,
            'general_windows_installer_service':False,'windows_gui_qualified':False,'dos_qualified':False,
            'limitations':meta['limits']+['Existing ZIP update is unsupported; the native example frontend chooses fresh numbered output paths','Disk-full, arbitrary API-fault injection, hostile executable fuzzing and modified-owned-file removal are not separately qualified by this release'],
            'final_reproduction_harness_sha256':{x.relative_to(ROOT).as_posix():sha(x) for x in sorted((ROOT/'tests/m8').glob('*.py'))},
            'evidence_sha256':{x.relative_to(OUT).as_posix():sha(x) for x in sorted(OUT.rglob('*')) if x.is_file() and x.name!='acceptance-results.json'}}
    (OUT/'acceptance-results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print('M8 PASS: '+str(len(gates))+' exact-artifact gates; '+str(history)+' frozen prior proofs unchanged')


if __name__=='__main__':main()
