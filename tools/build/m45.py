"""Build the M4.5 hardware-discovery increment without claiming full acceptance."""
from m2 import main
from m1 import ROOT
import json
def build():
    manifest=main('M45');manifest['limits']+=['M4.5 remains in progress: ordinary AHCI/NVMe reads, GPT/MBR validation and read-only FAT32',
        'Existing marked 16 MiB M4 ext2 write path retained; ordinary disks are not formatted or mounted writable',
        'Secure Boot tests use owner-enrolled disposable TCG NVRAM and a separate signed/hash-pinned test loader, not factory trust',
        'Legacy BIOS, USB/virtio storage, broader input/filesystems and physical qualification remain open']
    (ROOT/'dist/m45-build-manifest.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode());return manifest
if __name__=='__main__':build()
