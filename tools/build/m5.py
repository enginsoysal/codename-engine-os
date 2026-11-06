"""Build an actual native live installer, installed kernel and dual-firmware media."""
import hashlib,json,shutil,struct
from m1 import ROOT
from m2 import main
from media import fat32
from bios import stages,dual_media
from m5_layout import pack,trim,compact_elf,ESP,DATA,END,MIN

def build():
    work=ROOT/'build/m5';work.mkdir(parents=True,exist_ok=True)
    base=main('M5');installed=work/'installed';installed.mkdir(exist_ok=True)
    kernel=compact_elf((work/'KERNEL.ELF').read_bytes());efi=(work/'BOOTX64.EFI').read_bytes();init=(work/'INIT.BIN').read_bytes()
    for name,data in [('KERNEL.ELF',kernel),('BOOTX64.EFI',efi),('INIT.BIN',init)]: (installed/name).write_bytes(data)
    mbr,stage,raw,config=stages(installed/'bios',kernel,init)
    # Initial GPT is constructed by the kernel for the actual target capacity.
    low=bytearray(32768);low[:446]=mbr[:446];low[510:512]=b'\x55\xaa'
    fat=fat32([(4,b'BOOTX64 EFI',efi),(2,b'KERNEL  ELF',kernel),(2,b'INIT    BIN',init)])
    payload=pack([(0,bytes(low[:4096])),(64,stage),(128,raw),(ESP,trim(fat))])
    (work/'install.pack').write_bytes(payload)
    live=main('M5',m5_live=True);layout=dual_media(work,'M5')
    live.update(qualification_scope='M5-VM-v1',release_version='0.5.0',installed_kernel_sha256=hashlib.sha256(kernel).hexdigest(),installer_payload_bytes=len(payload),bios_media_layout=layout,
        installation_layout={'esp_first_sector':ESP,'esp_sectors':131072,'data_first_sector':DATA,'data_sectors':32768,'minimum_disk_sectors':MIN,'sector_bytes':512},
        limits=['x86-64 VM profile; unsigned BIOS/UEFI ISO and USB, SATA/AHCI 512-byte install targets only','16 MiB bounded persistent ext2 native volume, 48 KiB file limit; remaining target space unallocated','Live copy-on-write overlay capped at 128 pages (512 KiB); no host swap','Same-build boot repair preserves the complete data partition; no dual boot, in-place upgrade or atomic power-loss guarantee','Windows 95–11 compatibility remains planned; no Win32 loader, desktop or network stack'])
    live['artifacts']={p.name:{'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'size':p.stat().st_size} for p in sorted((ROOT/'dist').glob('EngineOS-M5*')) if p.is_file()}
    (ROOT/'dist/m5-build-manifest.json').write_text(json.dumps(live,indent=2)+'\n',encoding='utf-8')
    (ROOT/'dist/M5-SHA256SUMS.txt').write_text(''.join(f'{v["sha256"]}  {k}\n' for k,v in live['artifacts'].items()),encoding='utf-8')
    return live
if __name__=='__main__':build()
