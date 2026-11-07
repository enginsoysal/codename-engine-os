"""Package an ordinary VMware VM with a new growable SATA disk and matching ISO.

Never reads an existing VM or disk. Runtime paths are relative and contain no
remote-control, serial-pipe or host integration credentials.
"""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import uuid
import zipfile
ROOT=Path(__file__).resolve().parents[2]

def configuration():
    fields={'.encoding':'UTF-8','config.version':'8','virtualHW.version':'20',
      'displayName':'Engine OS','guestOS':'freebsd-64','firmware':'efi',
      'uefi.secureBoot.enabled':'FALSE','memsize':'128','numvcpus':'2',
      'cpuid.coresPerSocket':'2','hpet0.present':'TRUE','floppy0.present':'FALSE',
      'sata0.present':'TRUE','sata0:0.present':'TRUE','sata0:0.fileName':'EngineOS.vmdk',
      'sata0:0.deviceType':'disk','sata0:0.startConnected':'TRUE',
      'ide1:0.present':'TRUE','ide1:0.deviceType':'cdrom-image',
      'ide1:0.fileName':'EngineOS-M74.iso','ide1:0.startConnected':'TRUE',
      'usb.present':'FALSE','mks.enable3d':'FALSE','ethernet0.present':'TRUE','ethernet0.connectionType':'nat','ethernet0.virtualDev':'e1000e','ethernet0.startConnected':'TRUE','sound.present':'TRUE','sound.virtualDev':'hdaudio','sound.autodetect':'TRUE',
      'uuid.action':'create','msg.autoAnswer':'TRUE'}
    return ''.join(f'{key} = "{value}"\n' for key,value in fields.items())

def main():
    folder=ROOT/'build/m74/starter'/uuid.uuid4().hex;folder.mkdir(parents=True)
    tool=Path('C:/Program Files/VMware/VMware Workstation/vmware-vdiskmanager.exe')
    result=subprocess.run([str(tool),'-c','-s','20GB','-a','ide','-t','0',str(folder/'EngineOS.vmdk')],capture_output=True,timeout=60,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    if result.returncode:raise RuntimeError('Could not create the new VMware starter disk')
    (folder/'EngineOS.vmx').write_bytes(configuration().encode())
    shutil.copyfile(ROOT/'dist/EngineOS-M74.iso',folder/'EngineOS-M74.iso')
    shutil.copyfile(ROOT/'LICENSE',folder/'LICENSE')
    shutil.copyfile(ROOT/'third_party/inter/OFL.txt',folder/'Inter-OFL.txt')
    notices=(ROOT/'dist/EngineOS-M71-THIRD-PARTY-NOTICES.txt').read_text(encoding='utf-8').replace('Engine OS M7.1 third-party notices','Engine OS M7.4 third-party notices',1)
    notices+='\nInter typeface: Copyright 2020 The Inter Project Authors. SIL Open Font License 1.1.\n\n'+(ROOT/'third_party/inter/OFL.txt').read_text(encoding='utf-8')
    notices+='\nminiz 3.1.2: MIT license.\n\n'+(ROOT/'third_party/miniz/LICENSE').read_text(encoding='utf-8')
    (folder/'THIRD-PARTY-NOTICES.txt').write_bytes(notices.encode())
    (ROOT/'dist/EngineOS-M74-THIRD-PARTY-NOTICES.txt').write_bytes(notices.encode())
    (folder/'START-HERE.txt').write_bytes(b'Engine OS 0.7.4 - VMware starter\n\nExtract the entire ZIP to a local folder. Open EngineOS.vmx in VMware Workstation.\nIf VMware asks, choose I Copied It. Start the VM and follow Next.\nSelect Disk 1. This package contains a NEW empty growable 20 GiB SATA disk.\nAfter Install, choose Open desktop. Browse and create folders in Files.\n\nTo test independent disk boot: power off; disconnect only the CD/DVD ISO.\nKeep Hard Disk (SATA) connected. Start the VM again.\nThe included disk is blank, not a preinstalled desktop or a simulation.\nNAT networking and HDA audio are configured. Neither is required for installation.\nBrowser is a basic HTML/text viewer; Media Player currently accepts PCM WAV only.\nSecure Boot must remain off for this unsigned development build.\n\nGuide: https://engineos.brixcanvas.com/blog/docs-handbook-apps-m74\nSource: https://github.com/enginsoysal/codename-engine-os\n')
    archive=ROOT/'dist/EngineOS-M74-VMware.zip'
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as output:
        for file in sorted(folder.iterdir()):
            if b'C:\\Users\\' in file.read_bytes() or b'C:/Users/' in file.read_bytes():raise ValueError('Host user path found in the public package')
            output.write(file,'EngineOS/'+file.name)
    with zipfile.ZipFile(archive) as check:
        assert check.testzip() is None
        assert check.read('EngineOS/EngineOS.vmx').decode()==configuration()
        assert hashlib.sha256(check.read('EngineOS/EngineOS-M74.iso')).hexdigest()==hashlib.sha256((ROOT/'dist/EngineOS-M74.iso').read_bytes()).hexdigest()
    report={'passed':True,'archive':archive.name,'sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'size':archive.stat().st_size,'disk':'new growable 20 GiB SATA disk; not preinstalled','firmware':'UEFI, Secure Boot off','boot_order_forced':False,'network_required':False,'members':[p.name for p in sorted(folder.iterdir())]}
    (ROOT/'dist/m74-starter-manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report,indent=2))
if __name__=='__main__':main()
