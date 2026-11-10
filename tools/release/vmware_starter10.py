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

def configuration(controller="sata",firmware="efi"):
    fields={'.encoding':'UTF-8','config.version':'8','virtualHW.version':'20',
      'displayName':'Engacer OS','guestOS':'freebsd-64','firmware':'efi',
      'uefi.secureBoot.enabled':'FALSE','memsize':'128','numvcpus':'2',
      'cpuid.coresPerSocket':'2','pciBridge0.present':'TRUE','pciBridge4.present':'TRUE','pciBridge4.virtualDev':'pcieRootPort','pciBridge4.functions':'8','hpet0.present':'TRUE','floppy0.present':'FALSE',
      'sata0.present':'TRUE','sata0:0.present':'TRUE','sata0:0.fileName':'EngacerOS.vmdk',
      'sata0:0.deviceType':'disk','sata0:0.startConnected':'TRUE',
      'ide1:0.present':'TRUE','ide1:0.deviceType':'cdrom-image',
      'ide1:0.fileName':'EngacerOS-M10.iso','ide1:0.startConnected':'TRUE',
      'usb.present':'TRUE','usb_xhci.present':'TRUE','ehci.present':'FALSE','mks.enable3d':'FALSE','ethernet0.present':'TRUE','ethernet0.connectionType':'nat','ethernet0.virtualDev':'e1000e','ethernet0.startConnected':'TRUE','sound.present':'TRUE','sound.virtualDev':'hdaudio','sound.autodetect':'TRUE',
      'uuid.action':'create','msg.autoAnswer':'TRUE'}
    fields['firmware']=firmware
    if controller=='buslogic':fields['guestOS']='freebsd'
    if controller!='sata':
        fields={k:v for k,v in fields.items() if not k.startswith('sata0')}
        bus='ide0:0' if controller=='ide' else 'nvme0:0' if controller=='nvme' else 'scsi0:0'
        fields[bus+'.present']='TRUE';fields[bus+'.fileName']='EngacerOS.vmdk'
        if controller=='nvme':fields['nvme0.present']='TRUE'
        elif controller!='ide':fields['scsi0.present']='TRUE';fields['scsi0.virtualDev']=controller
    return ''.join(f'{key} = "{value}"\n' for key,value in fields.items())

def main():
    folder=ROOT/'build/m10/starter'/uuid.uuid4().hex;folder.mkdir(parents=True)
    tool=Path('C:/Program Files/VMware/VMware Workstation/vmware-vdiskmanager.exe')
    result=subprocess.run([str(tool),'-c','-s','20GB','-a','ide','-t','0',str(folder/'EngacerOS.vmdk')],capture_output=True,timeout=60,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    if result.returncode:raise RuntimeError('Could not create the new VMware starter disk')
    (folder/'EngacerOS.vmx').write_bytes(configuration().encode())
    shutil.copyfile(ROOT/'dist/EngacerOS-M10.iso',folder/'EngacerOS-M10.iso')
    shutil.copyfile(ROOT/'LICENSE',folder/'LICENSE')
    shutil.copyfile(ROOT/'third_party/inter/OFL.txt',folder/'Inter-OFL.txt')
    notices=(ROOT/'dist/EngineOS-M71-THIRD-PARTY-NOTICES.txt').read_text(encoding='utf-8').replace('Engine OS M7.1 third-party notices','Engacer OS M10 third-party notices',1)
    notices+='\nInter typeface: Copyright 2020 The Inter Project Authors. SIL Open Font License 1.1.\n\n'+(ROOT/'third_party/inter/OFL.txt').read_text(encoding='utf-8')
    notices+='\nminiz 3.1.2: MIT license.\n\n'+(ROOT/'third_party/miniz/LICENSE').read_text(encoding='utf-8')
    notices+='\n7-Zip 26.06 x64 original MSI and source: Igor Pavlov; LGPL with the additional BSD/unRAR conditions below.\n\n'+(ROOT/'third_party/7zip/LICENSE.txt').read_text(encoding='utf-8')
    notices+='\nlibmspack LZX decoder; GNU LGPL 2.1.\n\n'+(ROOT/'third_party/libmspack/COPYING.LIB').read_text(encoding='utf-8')
    shutil.copyfile(ROOT/'third_party/7zip/7z2606-src.tar.xz',folder/'7z2606-src.tar.xz')
    shutil.copyfile(ROOT/'third_party/libmspack/COPYING.LIB',folder/'libmspack-LGPL-2.1.txt')
    (folder/'THIRD-PARTY-NOTICES.txt').write_bytes(notices.encode())
    (ROOT/'dist/EngacerOS-M10-THIRD-PARTY-NOTICES.txt').write_bytes(notices.encode())
    (folder/'START-HERE.txt').write_bytes(b'Engacer OS M10: automatic supported storage, VMware graphics and pointer setup. No guest driver installer is required.\nSettings > Devices shows built-in support and keyboard layout.\nFiles > USB drives browses FAT32 media. Select a file and Copy selected to Downloads.\nIn Downloads, To USB chooses a destination. Existing files are never replaced.\nUse Safe remove before unplugging; reconnect to use the drive again.\nOnly supported BOT 512-byte FAT32 USB media has write support.\nMove Files by its title bar. This is bounded 2D presentation, not 3D acceleration.\n\nWindows Apps: open Start > Windows Apps after installation. Choose Install 7-Zip, confirm, then Open 7-Zip.\nCreate and extract a test archive with the original Windows console program. Remove app keeps your documents.\nOnly the included 7-Zip 26.06 x64 console profile is supported; no general Windows GUI/MSI or DOS support is claimed.\n7-Zip corresponding source is included. Engacer OS and libmspack runtime source: https://github.com/enginsoysal/engacer-os\n\nEngacer OS 0.10.0 - automatic storage and desktop\n\nExtract the entire ZIP to a local folder. Open EngacerOS.vmx in VMware Workstation.\nIf VMware asks, choose I Copied It. Start the VM and follow Next.\nClick the Disk 1 row in the bordered list. This package contains a NEW empty growable 20 GiB SATA disk.\nReview your choices, choose Install Engacer OS, then Restart to Engacer OS.\nThe matching ISO can remain connected. Browse and create folders in Files.\n\nTo test independent disk boot: power off; disconnect only the CD/DVD ISO.\nKeep Hard Disk (SATA) connected. Start the VM again.\nThe included disk is blank, not a preinstalled desktop or a simulation.\nNAT networking and HDA audio are configured. Neither is required for installation.\nBrowser is a basic HTML/text viewer; Media Player currently accepts PCM WAV only.\nSecure Boot must remain off for this unsigned development build.\n\nGuide: https://engaceros.brixcanvas.com/blog/docs-handbook-install-m10\nSource: https://github.com/enginsoysal/engacer-os\n')
    archive=ROOT/'dist/EngacerOS-M10-VMware.zip'
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as output:
        for file in sorted(folder.iterdir()):
            from sys import path as search_path
            search_path.insert(0,str(ROOT/'tools/build'))
            from m8 import privacy_input
            payload=privacy_input(file.read_bytes())
            if b'C:\\Users\\' in payload or b'C:/Users/' in payload:raise ValueError('Host user path found in the public package')
            output.write(file,'EngacerOS/'+file.name)
    with zipfile.ZipFile(archive) as check:
        assert check.testzip() is None
        assert check.read('EngacerOS/EngacerOS.vmx').decode()==configuration()
        assert hashlib.sha256(check.read('EngacerOS/EngacerOS-M10.iso')).hexdigest()==hashlib.sha256((ROOT/'dist/EngacerOS-M10.iso').read_bytes()).hexdigest()
    report={'passed':True,'archive':archive.name,'sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'size':archive.stat().st_size,'disk':'new growable 20 GiB SATA disk; not preinstalled','firmware':'UEFI, Secure Boot off','boot_order_forced':False,'network_required':False,'members':[p.name for p in sorted(folder.iterdir())]}
    (ROOT/'dist/m10-starter-manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report,indent=2))
if __name__=='__main__':main()
