"""Build the M4.5 hardware-discovery increment without claiming full acceptance."""
from m2 import main
from m1 import ROOT
from bios import dual_media
import hashlib,json
def build():
    manifest=main('M45');manifest['limits']+=['M4.5 remains in progress: ordinary AHCI/NVMe reads, GPT/MBR validation and read-only FAT32',
        'Existing marked 16 MiB M4 ext2 write path retained; ordinary disks are not formatted or mounted writable',
        'Secure Boot tests use owner-enrolled disposable TCG NVRAM and a separate signed/hash-pinned test loader, not factory trust',
        'BIOS EDD/E820/VBE and unsigned UEFI share the kernel; USB mass storage, broader input/filesystems and physical qualification remain open']
    layout=dual_media(ROOT/'build/m45');manifest['bios_media_layout']=layout;manifest['artifacts']={p.name:{'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'size':p.stat().st_size} for p in sorted((ROOT/'dist').glob('EngineOS-M45*')) if p.is_file()}
    (ROOT/'dist/m45-build-manifest.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode());(ROOT/'dist/M45-SHA256SUMS.txt').write_bytes(''.join(f'{r["sha256"]}  {n}\n' for n,r in manifest['artifacts'].items()).encode());return manifest
if __name__=='__main__':build()
