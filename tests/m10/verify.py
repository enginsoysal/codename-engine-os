"""Close M10 only with matching source, artifacts and complete native tasks."""
import hashlib,json,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    meta=json.loads((ROOT/'dist/m10-build-manifest.json').read_text(encoding='utf-8'))
    iso=meta['artifacts']['EngacerOS-M10.iso']['sha256'];build=meta['build_id']
    assert sha(ROOT/'dist/EngacerOS-M10.iso')==iso
    batch=subprocess.Popen(['git','cat-file','--batch'],cwd=ROOT,stdin=subprocess.PIPE,stdout=subprocess.PIPE)
    try:
        for name,digest in meta['source_sha256'].items():
            assert sha(ROOT/name)==digest,'Build input changed: '+name
            batch.stdin.write(('HEAD:'+name+'\n').encode());batch.stdin.flush()
            header=batch.stdout.readline().split();assert len(header)==3 and header[1]==b'blob',name
            blob=batch.stdout.read(int(header[2]));assert batch.stdout.read(1)==b'\n'
            assert hashlib.sha256(blob).hexdigest()==digest,'Published build bytes differ: '+name
    finally:
        batch.stdin.close();batch.stdout.close();batch.wait(timeout=10)
    for name,info in meta['artifacts'].items():assert sha(ROOT/'dist'/name)==info['sha256'],name
    reports=[('filesystem-host',False),('input-host',False),('usb-negative',True),('usb-quota',True)]
    reports += [('journey/'+name,True) for name in ['uefi-1280x800-128','uefi-1280x800-64','uefi-3840x2160-128-hub','bios-1280x800-128','uefi-1280x800-128-large-usb']]
    reports += [('native-motion/M10-'+size,True) for size in ['1280x800','2560x1440','3840x2160']]
    reports += [('motion-results/M10',True)]
    reports += [('vmware-results/'+c+'-'+f,True) for c in ['sata','ide','lsilogic','lsisas1068','pvscsi','nvme'] for f in ['efi','bios']]
    reports += [('vmware-results/buslogic-bios',True),('vmware-results/sata-efi-starter',True)]
    out=ROOT/'evidence/M10';out.mkdir(parents=True,exist_ok=True);proof=[]
    for folder,native in reports:
        source=ROOT/'build/m10'/folder/'results.json';r=json.loads(source.read_text(encoding='utf-8'));assert r['passed'],folder
        if native:assert r['build_id']==build and r['iso_sha256']==iso,folder
        for name,digest in r.get('source_sha256',{}).items():assert sha(ROOT/name)==digest,folder+': '+name
        target=out/folder;target.mkdir(parents=True,exist_ok=True)
        # Owned-work pointers and raw VRAM are local fixtures, not public proof.
        for file in source.parent.iterdir():
            if file.name=='results.json' or file.suffix=='.png' or file.name.endswith('-capture.json') or file.name.endswith('.log'):
                raw=file.read_bytes();assert b'C:\\Users\\' not in raw and b'C:/Users/' not in raw,'Private path in '+str(file.relative_to(ROOT))
                shutil.copyfile(file,target/file.name)
        proof.append({'name':folder,'passed':True,'checks':r.get('checks',[])})
    starter=json.loads((ROOT/'dist/m10-starter-manifest.json').read_text(encoding='utf-8'))
    assert starter['passed'] and sha(ROOT/'dist'/starter['archive'])==starter['sha256']
    accepted=json.loads((out/'vmware-results/sata-efi-starter/results.json').read_text(encoding='utf-8'))
    assert accepted['starter_sha256']==starter['sha256'] and accepted['windows_lifecycle_exercised'] and accepted['devices_and_layout_frontend_verified']
    for relative in ['m10-build-manifest.json','m10-starter-manifest.json']:shutil.copyfile(ROOT/'dist'/relative,out/relative)
    hashes={p.relative_to(out).as_posix():sha(p) for p in sorted(out.rglob('*')) if p.is_file() and p.name!='acceptance-results.json'}
    known=ROOT/'build/m10/known-uefi64-boot-usb/results.json';failure=json.loads(known.read_text(encoding='utf-8'))
    assert failure['passed'] is False and failure['build_id']==build and failure['iso_sha256']==iso
    target=out/'known-unqualified';target.mkdir(exist_ok=True);shutil.copyfile(known,target/'uefi64-boot-usb.json')
    hashes['known-unqualified/uefi64-boot-usb.json']=sha(target/'uefi64-boot-usb.json')
    limits=meta['limits']+['64-MiB OVMF: USB storage connects after installed startup; boot-attached USB storage/init allocation remains a recorded unqualified failure. Standard VMware package uses 128 MiB.']
    harness={p.relative_to(ROOT).as_posix():sha(p) for p in sorted((ROOT/'tests/m10').iterdir()) if p.suffix in ('.py','.c')}
    harness['tools/release/vmware_starter10.py']=sha(ROOT/'tools/release/vmware_starter10.py')
    result={'passed':True,'scope_id':'M10-DEVICES-VM-v1','hardware_phases_1_and_2_complete_within_scope':True,'build_id':build,'iso_sha256':iso,'starter_sha256':starter['sha256'],'gates':proof,'known_unqualified':['64-MiB OVMF with boot-attached USB storage'], 'evidence_sha256':hashes,'qualification_harness_sha256':harness,'limits':limits}
    (out/'acceptance-results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('M10 acceptance PASS:',len(proof),'exact-source/artifact gates')
if __name__=='__main__':main()
