"""Build M4 bounded ext2/input media and an immutable disposable filesystem template."""
from pathlib import Path
import hashlib,json
from m2 import main
from m1 import ROOT,sha
from data_image import create

def build():
    manifest=main('M4');dist=ROOT/'dist';temporary=ROOT/'build/m4/template-check.img'
    if temporary.exists():temporary.unlink()
    create(temporary);target=dist/'EngineOS-M4-fs-template.img'
    if target.exists() and sha(target)!=sha(temporary):raise RuntimeError('Data template was modified; preserve it and choose a new output directory')
    if not target.exists():target.write_bytes(temporary.read_bytes())
    descriptor='# Disk DescriptorFile\nversion=1\nencoding="UTF-8"\nCID=454e4705\nparentCID=ffffffff\ncreateType="monolithicFlat"\nRW 32768 FLAT "EngineOS-M4-fs-template.img" 0\nddb.adapterType = "ide"\n'
    desc=dist/'EngineOS-M4-fs-template.vmdk'
    if desc.exists() and desc.read_text(encoding='utf-8')!=descriptor:raise RuntimeError('Modified disk descriptor; not overwriting')
    desc.write_bytes(descriptor.encode())
    manifest['limits']+=['one marked v2 AHCI 16 MiB/512-byte-sector volume; 4 KiB DMA batches and one outstanding request',
                         'bounded ext2: 4 KiB blocks, 512 inodes, 12 direct blocks/48 KiB per file, 63-byte UTF-8 names, 255-byte absolute paths',
                         '8 trusted-kernel file handles; exact case/UTF-8, no hard/symbolic links or guest ACL adapters',
                         'dirty-volume write refusal and offline recovery to a new copy; no journal or universal power-loss recovery',
                         'QEMU xHCI direct-root boot keyboard/mouse, MSI/MSI-X, two devices/four enumeration attempts; no hubs/hotplug',
                         'PS/2 US ASCII and COM1; VMware qualification covers SATA persistence, not native USB input']
    manifest['artifacts']={p.name:{'sha256':sha(p),'size':p.stat().st_size} for p in sorted(dist.glob('EngineOS-M4*')) if p.is_file() and '-data-template' not in p.name}
    (dist/'m4-build-manifest.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode())
    (dist/'M4-SHA256SUMS.txt').write_bytes(''.join(f'{r["sha256"]}  {n}\n' for n,r in manifest['artifacts'].items()).encode())
    return manifest
if __name__=='__main__':build()
