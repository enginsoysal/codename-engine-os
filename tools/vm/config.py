"""Pinned QEMU configuration shared by tests and the developer launcher."""
from pathlib import Path
import hashlib
import json
import os
import shutil
ROOT=Path(__file__).resolve().parents[2]
def qemu_tools(directory=None):
    base=Path(directory) if directory else Path(os.environ.get('ProgramFiles','C:/Program Files'))/'qemu'
    lock=json.loads((ROOT/'tools/toolchain-lock.json').read_text(encoding='utf-8'))
    files={'qemu-system-x86_64':base/'qemu-system-x86_64.exe',
           'ovmf-code':base/'share/edk2-x86_64-code.fd','ovmf-vars-template':base/'share/edk2-i386-vars.fd'}
    for name,path in files.items():
        if not path.exists() or hashlib.sha256(path.read_bytes()).hexdigest()!=lock['files'][name]['sha256']:
            raise RuntimeError('Pinned VM component missing or changed: '+name)
    actual={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(base.glob('*.dll'))}
    if actual!=lock['files']['qemu-dlls']:raise RuntimeError('QEMU DLL set differs from lock')
    return files
def arguments(files,media,image,work,ram=512,headless=True,debug=False,milestone='M1'):
    work.mkdir(parents=True,exist_ok=True)
    variables=work/'vars.fd';shutil.copyfile(files['ovmf-vars-template'],variables)
    serial=work/'serial.log'
    if serial.exists():serial.unlink()
    args=[str(files['qemu-system-x86_64']),'-machine','pc-q35-11.1','-accel','tcg','-m',str(ram),'-smp','2',
          '-monitor','none','-no-reboot','-net','none',
          '-drive',f'if=pflash,format=raw,unit=0,readonly=on,file={files["ovmf-code"]}',
          '-drive',f'if=pflash,format=raw,unit=1,file={variables}',
          '-serial',f'file:{serial}','-device','isa-debug-exit,iobase=0xf4,iosize=0x04',
          '-device','qemu-xhci,id=xhci','-device','usb-kbd,bus=xhci.0','-device','usb-mouse,bus=xhci.0']
    if headless:args+=['-display','none']
    else:args+=['-name','Codename: Engine OS - '+milestone]
    if media=='iso':args+=['-cdrom',str(image),'-boot','order=d']
    elif media=='usb':args+=['-drive',f'if=none,id=engineusb,format=raw,snapshot=on,file={image}',
                            '-device','usb-storage,bus=xhci.0,drive=engineusb,bootindex=1']
    else:raise ValueError('media')
    if debug:args+=['-gdb','tcp:127.0.0.1:1234','-S']
    return args,serial
