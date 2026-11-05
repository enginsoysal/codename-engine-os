"""Launch normal M2 (or explicitly M1) media. No host disk/directory is exposed."""
import argparse
from pathlib import Path
import subprocess
from config import ROOT,arguments,qemu_tools
def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--usb',action='store_true')
    parser.add_argument('--headless',action='store_true')
    parser.add_argument('--debug',action='store_true',help='wait for debugger on loopback port 1234')
    parser.add_argument('--milestone',choices=['M1','M2','M3'],default='M2')
    parser.add_argument('--ram',type=int,default=128,help='guest RAM in MiB; M2 test profile defaults to 128')
    parser.add_argument('--qemu-dir',type=Path)
    args=parser.parse_args()
    media='usb' if args.usb else 'iso'
    milestone=args.milestone
    image=ROOT/'dist'/(f'EngineOS-{milestone}-usb.img' if args.usb else f'EngineOS-{milestone}.iso')
    if not image.exists():raise SystemExit('Build first: python tools/build/'+milestone.lower()+'.py')
    command,serial=arguments(qemu_tools(args.qemu_dir),media,image,ROOT/'build'/milestone.lower()/'manual',ram=args.ram,headless=args.headless,debug=args.debug,milestone=milestone)
    if milestone in ('M2','M3'):command[command.index('-smp')+1]='1'
    print(f'Starting Engine OS {milestone}. Serial log: build/{milestone.lower()}/manual/serial.log',flush=True)
    print('This kernel has no installer, shell or keyboard handling yet. Close the VM window to stop.',flush=True)
    process=subprocess.run(command,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    raise SystemExit(process.returncode)
if __name__=='__main__':main()
