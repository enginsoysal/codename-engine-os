"""Build the M4 storage/console increment and an immutable disposable-disk template."""
from pathlib import Path
import hashlib,json
from m2 import main
from m1 import ROOT,sha
from data_image import create

def build():
    manifest=main('M4');dist=ROOT/'dist';temporary=ROOT/'build/m4/template-check.img'
    if temporary.exists():temporary.unlink()
    create(temporary);target=dist/'EngineOS-M4-data-template.img'
    if target.exists() and sha(target)!=sha(temporary):raise RuntimeError('Data template was modified; preserve it and choose a new output directory')
    if not target.exists():target.write_bytes(temporary.read_bytes())
    descriptor='# Disk DescriptorFile\nversion=1\nencoding="UTF-8"\nCID=454e4704\nparentCID=ffffffff\ncreateType="monolithicFlat"\nRW 32768 FLAT "EngineOS-M4-data-template.img" 0\nddb.adapterType = "ide"\n'
    desc=dist/'EngineOS-M4-data-template.vmdk'
    if desc.exists() and desc.read_text(encoding='utf-8')!=descriptor:raise RuntimeError('Modified disk descriptor; not overwriting')
    desc.write_bytes(descriptor.encode())
    manifest['limits']+=['M4 first increment: one AHCI 512-byte-sector marked 16 MiB volume, writes restricted to sector 1',
                         'PS/2 US ASCII and COM1 console; ext2/VFS, USB input and mouse remain pending']
    manifest['artifacts']={p.name:{'sha256':sha(p),'size':p.stat().st_size} for p in sorted(dist.glob('EngineOS-M4*')) if p.is_file()}
    (dist/'m4-build-manifest.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode())
    (dist/'M4-SHA256SUMS.txt').write_bytes(''.join(f'{r["sha256"]}  {n}\n' for n,r in manifest['artifacts'].items()).encode())
    return manifest
if __name__=='__main__':build()
