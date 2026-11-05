"""Qualify only the implemented M4.5 increment; full milestone explicitly remains open."""
from pathlib import Path
import hashlib,json,re,subprocess,sys
ROOT=Path(__file__).resolve().parents[2]
def main():
    out=ROOT/'evidence/M4.5';out.mkdir(parents=True,exist_ok=True);report=out/'increment-results.json';result={'passed':False,'full_m45_complete':False,'scope':'dual BIOS/UEFI media, ordinary AHCI/NVMe/ATA/VirtIO/USB reads, reconnect/EIO containment, partition/FAT32/extended ext2 validation, owner-enrolled VM trust chain; wider agreed tracks remain open','gates':[]}
    report.write_bytes((json.dumps(result,indent=2)+'\n').encode())
    suites=[('tools/build/m45.py',[]),('tests/m45/boot_matrix.py',[]),('tests/m45/bios_negative.py',[]),('tests/m45/storage.py',[]),('tests/m45/read_faults.py',[]),('tests/m45/usb.py',[]),('tests/m45/usb_timeout.py',[]),('tests/m4/console.py',['--milestone','M45']),('tests/m4/console.py',['--milestone','M45','--usb']),('tests/m45/ext2.py',[]),('tests/m45/fat32.py',[]),('tests/m45/sha256.py',[]),('tests/m45/secure_boot.py',[]),('tests/m45/vmware.py',[]),('tools/build/m4.py',[]),('tests/m4/boot.py',['--case','persist-first','--evidence','evidence/M4.5/m4-regression']),('tests/check_repository.py',['--milestone','M4.5']),('tests/m0/verify.py',[])]
    for script,args in suites:
        print('Running '+script+' '+' '.join(args),flush=True);p=subprocess.run([sys.executable,script,*args],cwd=ROOT,timeout=1200);result['gates'].append({'script':script,'arguments':args,'passed':p.returncode==0});report.write_bytes((json.dumps(result,indent=2)+'\n').encode())
        if p.returncode:raise SystemExit(1)
    manifest=json.loads((ROOT/'dist/m45-build-manifest.json').read_text(encoding='utf-8'))
    for name,expected in manifest['source_sha256'].items():
        if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=expected:raise RuntimeError('Source changed during qualification: '+name)
    for name,record in manifest['artifacts'].items():
        raw=(ROOT/'dist'/name).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=record['sha256'] or re.search(rb'[Cc]:[\\/]+Users[\\/]+',raw):raise RuntimeError('Invalid/private artifact: '+name)
    for name in ('storage-results.json','fat32-results.json','sha256-results.json','secure-boot-results.json','repository-results.json'):
        data=json.loads((out/name).read_text(encoding='utf-8'));assert data['checks'] and all(c['passed'] for c in data['checks']),name
    for name in ('results.json','negative-results.json'):
        data=json.loads((out/'boot'/name).read_text(encoding='utf-8'));assert data['build_id']==manifest['build_id'] and all(c['passed'] for c in data['checks']),name
    data=json.loads((out/'vmware/results.json').read_text(encoding='utf-8'));assert data['build_id']==manifest['build_id'] and data['existing_vm_inventory_unchanged'] and len(data['checks'])==5 and all(c['passed'] for c in data['checks'])
    for folder,count in (('usb',10),('usb-timeout',7),('ext2',8),('faults',3)):
        data=json.loads((out/folder/'results.json').read_text(encoding='utf-8'));assert data['build_id']==manifest['build_id'] and len(data['checks'])==count and not data.get('error') and all(c['passed'] for c in data['checks']),folder
    for folder,prefix in (('ps2-console',''),('hid-console','usb-')):
        data=json.loads((out/folder/(prefix+'console-report.json')).read_text(encoding='utf-8'));assert data['build_id']==manifest['build_id'] and all(c['passed'] for c in data['checks']),folder
    result.update(passed=True,build_id=manifest['build_id']);(out/'build-manifest.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode());report.write_bytes((json.dumps(result,indent=2)+'\n').encode());print('M4.5 increment PASS; full milestone remains IN PROGRESS',flush=True)
if __name__=='__main__':main()
