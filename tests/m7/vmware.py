"""Exercise only owned headless VMware M7 guests; never operate the host desktop."""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import secrets
import socket
import time
import traceback
import uuid
ROOT=Path(__file__).resolve().parents[2]
def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);value=importlib.util.module_from_spec(spec);spec.loader.exec_module(value);return value
v=module('m6vmware',ROOT/'tests/m6/vmware.py')
s=module('m7session',Path(__file__).with_name('session.py'))
rfb=module('m7rfb',Path(__file__).with_name('rfb.py'))
VISUAL=False


class Guest(v.Guest):
    def __init__(self,folder,disk,firmware,installed=False):
        self.installed=installed;self.rfb=None
        if VISUAL:
            with socket.socket() as probe:probe.bind(('127.0.0.1',0));self.rfb_port=probe.getsockname()[1]
            self.rfb_password=secrets.token_hex(4)
        super().__init__(folder,disk,firmware,installed)
        cfg=self.vmx.read_text(encoding='utf-8').replace('EngineOS-M6.iso','EngineOS-M7.iso').replace('Engine OS M6 isolated qualification','Engine OS M7 isolated qualification').replace('ethernet0.present = "FALSE"','ethernet0.present = "TRUE"')
        cfg+='ethernet0.virtualDev = "e1000e"\nethernet0.connectionType = "nat"\nethernet0.startConnected = "TRUE"\nethernet0.addressType = "static"\nethernet0.address = "00:50:56:3f:12:34"\n'
        cfg+='pciBridge0.present = \"TRUE\"\npciBridge4.present = \"TRUE\"\npciBridge4.virtualDev = \"pcieRootPort\"\npciBridge4.functions = \"8\"\n'
        if VISUAL:cfg+='RemoteDisplay.vnc.enabled = \"TRUE\"\nRemoteDisplay.vnc.ip = \"127.0.0.1\"\nRemoteDisplay.vnc.port = \"'+str(self.rfb_port)+'\"\nRemoteDisplay.vnc.password = \"'+self.rfb_password+'\"\n'
        self.vmx.write_text(cfg,encoding='utf-8')
    def connect_display(self):
        if self.rfb is None:
            log=(self.folder/'vmware.log').read_text(encoding='utf-8',errors='replace')
            assert 'Started VNC server, listening at 127.0.0.1:'+str(self.rfb_port) in log,'Owned display must bind only loopback'
            self.rfb=rfb.RFB(self.rfb_port,self.rfb_password)
        return self.rfb
    def __exit__(self,*args):
        if self.rfb is not None:self.rfb.close()
        return super().__exit__(*args)
    def wait(self,text,offset=0,timeout=90):
        if text=='M6_WIZARD_READY' and self.installed:text='M7_DESKTOP_READY'
        return super().wait(text,offset,timeout)


def press(guest,sequence,delay=.12):
    if isinstance(sequence,str):sequence=sequence.encode('ascii')
    for value in sequence:guest.pipe.write(bytes([value]));time.sleep(delay)


def launch(guest,steps):press(guest,bytes([137])+b'\t'*steps+b'\r')


def capture(guest,out,name):
    if not VISUAL:return {'passed':False,'scope':'diagnostic COM1 run only; separate --visual run qualifies framebuffer and PS/2 typing'}
    guest.connect_display().shot(out/(name+'.png'))
    return {'passed':True,'scope':'authenticated loopback framebuffer capture of this owned headless VM only'}


def main():
    global VISUAL
    parser=argparse.ArgumentParser();parser.add_argument('--firmware',choices=['efi','bios'],default='efi');parser.add_argument('--visual',action='store_true');a=parser.parse_args();VISUAL=a.visual
    work=ROOT/'build/m7/vmware'/a.firmware/uuid.uuid4().hex;work.mkdir(parents=True)
    out=ROOT/'evidence/M7/vmware'/(a.firmware+('-visual' if VISUAL else ''));out.mkdir(parents=True,exist_ok=True)
    report={'passed':False,'m7_complete':False,'firmware':a.firmware,'checks':[]};before=v.inventory()
    try:
        disk=v.sparse_disk(work/'owned-flat.img',20*1024**3)
        descriptor=work/'owned.vmdk';descriptor.write_bytes(b'version=1\nencoding="UTF-8"\nCID=fffffffe\nparentCID=ffffffff\ncreateType="monolithicFlat"\nRW 41943040 FLAT "owned-flat.img" 0\nddb.adapterType = "ide"\n')
        with Guest(work/'install',descriptor,a.firmware) as guest:
            press(guest,b'\r\r\t\t\r\t\t\t\r'+b'\t'*5+b'\r\t\r');guest.wait('M6_INSTALL_PASS',timeout=240)
            (out/'install.log').write_bytes(guest.read().encode());report['installer_capture']=capture(guest,out,'installer-finish')
        with Guest(work/'desktop',descriptor,a.firmware,True) as guest:
            manifest=json.loads((ROOT/'dist/m7-build-manifest.json').read_text(encoding='utf-8'));assert 'M7_DESKTOP_READY build='+manifest['build_id'] in guest.output
            guest.wait('M7_DHCP_CONFIGURED',timeout=90)
            launch(guest,2);guest.wait('M7_WINDOW_OPEN app=0x0000000000000002')
            if VISUAL:
                display=guest.connect_display()
                for character in 'vmware engine os':display.press(ord(character))
                start=len(guest.read());display.control('s')
            else:press(guest,'vmware engine os');start=len(guest.read());press(guest,bytes([19]))
            guest.wait('M7_DOCUMENT_SAVED',start)
            report['desktop_capture']=capture(guest,out,'native-desktop-editor')
            launch(guest,4);press(guest,'6*7=')
            launch(guest,3);press(guest,b'\t'*2+b'\r'+b'\t'*4+b'\r'+b'\t'*7+b'\r');guest.wait('M7_PREFERENCES_SAVED')
            press(guest,b'\t'*6+b'\r');guest.wait('M7_CALCULATOR_REMOVED')
            press(guest,b'\t'*6+b'\r');guest.wait('M7_CALCULATOR_INSTALLED')
            launch(guest,5);press(guest,b'\r');guest.wait('M7_HTTPS_DOWNLOAD_SAVED',timeout=120)
            report['download_capture']=capture(guest,out,'native-https-download')
            (out/'desktop.log').write_bytes(guest.read().encode())
        assert s.document(disk,'/Users/Owner/Documents/Note1.txt')==b'vmware engine os'
        assert s.document(disk,'/Users/Owner/Downloads/Download1.bin')==(ROOT/'LICENSE').read_bytes()
        report['checks'].append('real 20-GiB installation and detached native desktop with editor save, settings, optional executable removal/reinstall and certificate-checked HTTPS over VMware e1000e NAT')
        with Guest(work/'restart',descriptor,a.firmware,True) as guest:
            report['restart_capture']=capture(guest,out,'desktop-after-restart');(out/'restart.log').write_bytes(guest.read().encode())
        assert s.document(disk,'/Users/Owner/Documents/Note1.txt')==b'vmware engine os'
        assert s.document(disk,'/Users/Owner/Downloads/Download1.bin')==(ROOT/'LICENSE').read_bytes()
        report['checks'].append('second detached VMware boot retains exact independently CRC-checked document and downloaded bytes')
        log=(work/'desktop/vmware.log').read_text(encoding='utf-8',errors='replace');match=re.search(r'VMware Workstation[^\r\n]*version=([\d.]+) build=(\d+)',log)
        report.update(passed=True,build_id=manifest['build_id'],iso_sha256=manifest['artifacts']['EngineOS-M7.iso']['sha256'],disk_gib=20,
                      vmware={'version':match[1],'build':match[2]} if match else None,
                      input_scope=('owned loopback RFB framebuffer/virtual PS2 editor typing and save; navigation via COM1' if VISUAL else 'native controls through diagnostic COM1; separate --visual gate required for framebuffer/PS2 typing')+'; no host desktop automation')
    except Exception:
        report['error']=v.redact(traceback.format_exc());print(report['error'])
        if 'guest' in locals():(out/'last-run.log').write_bytes(guest.output.encode())
    report['existing_vm_inventory_unchanged']=v.inventory()==before
    report['passed']=report['passed'] and report['existing_vm_inventory_unchanged']
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('M7 VMware '+a.firmware+' '+('PASS' if report['passed'] else 'FAIL'),flush=True)
    if not report['passed']:raise SystemExit(1)


if __name__=='__main__':main()
