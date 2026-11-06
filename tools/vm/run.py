"""Launch normal M5 or an explicit earlier milestone; expose only designated disposable images."""
import argparse
from pathlib import Path
import subprocess
from config import ROOT,arguments,qemu_tools
def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--usb',action='store_true')
    parser.add_argument('--headless',action='store_true')
    parser.add_argument('--debug',action='store_true',help='wait for debugger on loopback port 1234')
    parser.add_argument('--milestone',choices=['M1','M2','M3','M4','M45','M5'],default='M5')
    parser.add_argument('--cpus',type=int,choices=(1,2,4),help='M3 defaults to 2; M2 uses 1')
    parser.add_argument('--ram',type=int,default=128,help='guest RAM in MiB; defaults to 128')
    parser.add_argument('--data-disk',type=Path,help='disposable .img; M4 requires its development marker, M45 allows ordinary images')
    parser.add_argument('--create-data-disk',action='store_true',help='create/reuse a marked M4 disk in the build directory')
    parser.add_argument('--create-install-disk',action='store_true',help='create/reuse a disposable 256 MiB M5 target in build/m5/manual')
    parser.add_argument('--installed',action='store_true',help='M5: boot the designated target without ISO/USB')
    parser.add_argument('--qemu-dir',type=Path)
    args=parser.parse_args()
    if (args.create_install_disk or args.installed) and args.milestone!='M5':parser.error('Installation options require M5')
    if args.installed and args.usb:parser.error('Installed boot must detach USB media')
    media='usb' if args.usb else 'iso'
    milestone=args.milestone
    if milestone=="M2" and args.cpus not in (None,1):parser.error("M2 qualifies one vCPU; select M3 for active SMP")
    image=ROOT/'dist'/(f'EngineOS-{milestone}-usb.img' if args.usb else f'EngineOS-{milestone}.iso')
    if not image.exists():raise SystemExit('Build first: python tools/build/'+milestone.lower()+'.py')
    command,serial=arguments(qemu_tools(args.qemu_dir),media,image,ROOT/'build'/milestone.lower()/'manual',ram=args.ram,headless=args.headless,debug=args.debug,milestone=milestone)
    command[command.index('-smp')+1]=str(args.cpus or (1 if milestone=='M2' else 2))
    if args.installed:
        i=command.index('-cdrom');del command[i:i+2];command[command.index('-boot')+1]='order=c'
    if milestone in ('M4','M45','M5'):
        import struct,zlib,sys
        sys.path.insert(0,str(ROOT/'tools/build'))
        from data_image import create
        if milestone=='M5' and args.create_data_disk:parser.error('Use --create-install-disk for M5')
        disk=args.data_disk or (ROOT/'build/m5/manual/install-target.img' if args.create_install_disk else None) or (ROOT/'build'/milestone.lower()/'manual/fs-data.img' if args.create_data_disk else None)
        if args.create_install_disk and disk and not disk.exists():
            with disk.open('xb') as f:f.truncate(256*1024*1024)
        if args.installed and not disk:parser.error('Specify --data-disk or --create-install-disk')
        if args.create_data_disk and disk and not disk.exists():create(disk)
        if disk:
            disk=disk.resolve()
            if disk.suffix.lower()!='.img' or not disk.is_file() or disk.stat().st_size<512 or disk.stat().st_size%512:raise SystemExit('Use a regular sector-aligned disposable .img file')
            with disk.open('rb') as f:header=f.read(512)
            if milestone=='M4' and (disk.stat().st_size!=32768*512 or header[:16]!=b'ENGINE-M4-DATA02' or struct.unpack_from('<I',header,508)[0]!=zlib.crc32(header[:508])):raise SystemExit('Unmarked/invalid version-2 data image; not attaching')
            command+=['-drive',f'if=none,id=m4data,format=raw,file={disk}'+(',snapshot=on' if milestone=='M45' else ''),'-device','ide-hd,bus=ide.0,drive=m4data']
    elif args.data_disk or args.create_data_disk:raise SystemExit('Data-disk options require M4, M45 or M5')
    print(f'Starting Engine OS {milestone}. Serial log: build/{milestone.lower()}/manual/serial.log',flush=True)
    print('M5 text installer: plan N first; use --create-install-disk for an owned target. Live writes are volatile.' if milestone=='M5' else 'M4 offers an ext2 file console and USB input on the qualified QEMU profile.' if milestone=='M4' else 'M4.5 offers read-only device/partition/FAT32/ext2 inspection and USB input; no installer.' if milestone=='M45' else 'No installer or shell in this milestone. Close the VM window to stop.',flush=True)
    process=subprocess.run(command,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    raise SystemExit(process.returncode)
if __name__=='__main__':main()
