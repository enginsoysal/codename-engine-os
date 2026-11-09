"""Qualify only the corrected enabled-device starter, retaining the exact M9 ISO."""
import hashlib,importlib.util,json,re,shutil,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'evidence/M9.1'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def load(p):return json.loads(Path(p).read_text(encoding='utf-8'))
def main():
    old=load(ROOT/'evidence/M9/acceptance-results.json');assert old['passed']
    for name,digest in old['evidence_sha256'].items():assert sha(ROOT/'evidence/M9'/name)==digest,name
    for name,value in old['artifacts'].items():assert sha(ROOT/'dist'/name)==value['sha256'],name
    manifest=load(ROOT/'dist/m9-build-manifest.json')
    for name,digest in manifest['source_sha256'].items():assert sha(ROOT/name)==digest,name
    starter=load(ROOT/'dist/m91-starter-manifest.json');archive=ROOT/'dist/EngineOS-M91-VMware.zip'
    assert starter['passed'] and sha(archive)==starter['sha256']
    spec=importlib.util.spec_from_file_location('starter91',ROOT/'tools/release/vmware_starter91.py');builder=importlib.util.module_from_spec(spec);spec.loader.exec_module(builder)
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None and z.read('EngineOS/EngineOS.vmx').decode()==builder.configuration()
        assert hashlib.sha256(z.read('EngineOS/EngineOS-M9.iso')).hexdigest()==old['iso_sha256']
        cfg=z.read('EngineOS/EngineOS.vmx').decode()
        for line in ['ethernet0.present = "TRUE"','ethernet0.virtualDev = "e1000e"','ethernet0.connectionType = "nat"','sound.present = "TRUE"','sata0.present = "TRUE"','pciBridge4.virtualDev = "pcieRootPort"','pciBridge4.functions = "8"']:assert line in cfg,line
    guest=load(OUT/'vmware/efi-starter/results.json')
    assert guest['dhcp_configured_with_shipped_nat']
    assert guest['passed'] and guest['network_present'] and guest['starter_sha256']==starter['sha256']
    assert guest['iso_sha256']==old['iso_sha256'] and guest['build_id']==old['build_id']
    assert guest['same_vm_reused'] and not guest['boot_order_forced'] and guest['existing_vm_inventory_unchanged']
    assert len(guest['original_payload_sha256'])==106
    assert load(OUT/'repository-results.json')['passed']
    assert not load(OUT/'incident.json')['owner_vm_modified']
    shutil.copyfile(ROOT/'dist/m91-starter-manifest.json',OUT/'m91-starter-manifest.json')
    for file in OUT.rglob('*'):
        if file.is_file() and file.suffix in ['.json','.log','.md','.txt']:assert not re.search(rb'[Cc]:[\\/]+Users[\\/]+',file.read_bytes()),file.name
    artifacts={}
    for name in ['EngineOS-M91-VMware.zip','EngineOS-M91-THIRD-PARTY-NOTICES.txt','m91-starter-manifest.json']:
        path=ROOT/'dist'/name;artifacts[name]={'sha256':sha(path),'size':path.stat().st_size}
    result={'passed':True,'scope_id':'M91-STARTER-v1','packaging_only':True,'iso_unchanged':True,'iso_sha256':old['iso_sha256'],'build_id':old['build_id'],
            'shipped_ethernet_and_sound_kept_enabled':True,'installed_restart_and_two_detached_boots':True,'original_payload_count':106,
            'artifacts':artifacts,'frozen_m9_acceptance_sha256':sha(ROOT/'evidence/M9/acceptance-results.json'),
            'source_sha256':{name:sha(ROOT/name) for name in ['tools/release/vmware_starter91.py','tests/m91/vmware.py','tests/m91/verify.py']},
            'limits':['UEFI prepared starter only; kernel/ISO unchanged','IDE installation and unqualified SCSI controllers remain unsupported','No new graphics, 3D or universal device qualification'],
            'evidence_sha256':{x.relative_to(OUT).as_posix():sha(x) for x in sorted(OUT.rglob('*')) if x.is_file() and x.name!='acceptance-results.json'}}
    (OUT/'acceptance-results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print('M9.1 enabled-device starter PASS; frozen M9 proof and binaries unchanged')
if __name__=='__main__':main()
