"""Build final M4.5 VM-scope media; fresh runtime acceptance is a separate gate."""
from m2 import main
from m1 import ROOT
from bios import dual_media
import hashlib,json
def build():
    manifest=main('M45');manifest['qualification_scope']='M45-VM-v1';manifest['release_version']='0.4.5';manifest['scope_contract_sha256']=hashlib.sha256((ROOT/'docs/milestones/m4.5-scope.json').read_bytes()).hexdigest();manifest['limits']+=['M4.5 VM scope: ordinary AHCI/NVMe/ATA/VirtIO/USB reads, GPT/MBR validation and read-only FAT32/extended ext2',
        'Existing marked 16 MiB M4 ext2 write path retained; ordinary disks are not formatted or mounted writable',
        'Secure Boot tests use owner-enrolled disposable TCG NVRAM and a separate signed/hash-pinned test loader, not factory trust',
        'BIOS EDD/E820/VBE and unsigned UEFI share the kernel; unlisted controller/input profiles, factory trust, production key operations and physical qualification are outside M45-VM-v1']
    layout=dual_media(ROOT/'build/m45');manifest['bios_media_layout']=layout;manifest['artifacts']={p.name:{'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'size':p.stat().st_size} for p in sorted((ROOT/'dist').glob('EngineOS-M45*')) if p.is_file()}
    (ROOT/'dist/m45-build-manifest.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode());(ROOT/'dist/M45-SHA256SUMS.txt').write_bytes(''.join(f'{r["sha256"]}  {n}\n' for n,r in manifest['artifacts'].items()).encode());return manifest
if __name__=='__main__':build()
