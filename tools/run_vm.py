"""Launch the normal M1 ISO/USB. No host disk or directory is exposed."""
import argparse
from pathlib import Path
import subprocess
from vm import ROOT,arguments,qemu_tools
def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--usb',action='store_true')
    parser.add_argument('--headless',action='store_true')
    parser.add_argument('--debug',action='store_true',help='wait for debugger on loopback port 1234')
    parser.add_argument('--qemu-dir',type=Path)
    args=parser.parse_args()
    media='usb' if args.usb else 'iso'
    image=ROOT/'dist'/('EngineOS-M1-usb.img' if args.usb else 'EngineOS-M1.iso')
    if not image.exists():raise SystemExit('Build first: python tools/build.py')
    command,serial=arguments(qemu_tools(args.qemu_dir),media,image,ROOT/'build/m1/manual',headless=args.headless,debug=args.debug)
    print('Starting Engine OS M1. Serial log: build/m1/manual/serial.log',flush=True)
    print('This kernel has no installer, shell or keyboard handling yet. Close the VM window to stop.',flush=True)
    process=subprocess.run(command,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    raise SystemExit(process.returncode)
if __name__=='__main__':main()
