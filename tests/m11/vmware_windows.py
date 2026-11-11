"""Exact M11 stage starter: owned headless VMware install and independent boots."""
import argparse,hashlib,importlib.util,json,re,shutil,struct,time,traceback,uuid,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def module(name,path):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
m=module('vmware10',ROOT/'tests/m10/vmware.py');v=m.v
def click(g,x,y):m.guest_click10(g,x,y)
def main():
 p=argparse.ArgumentParser();p.add_argument('--revision',default='stage4');a=p.parse_args();assert a.revision in ('stage4','stage5','final')
 image='EngacerOS-M11'+a.revision.upper();archive=ROOT/'dist'/(image+'-VMware.zip');meta=json.loads((ROOT/('dist/m11'+a.revision+'-build-manifest.json')).read_text())
 work=ROOT/'build/m11/vmware'/a.revision/uuid.uuid4().hex;work.mkdir(parents=True);out=ROOT/'build/m11/windows/vmware';out.mkdir(parents=True,exist_ok=True)
 before=v.old.v.inventory();report={'passed':False,'build_id':meta['build_id'],'iso_sha256':meta['artifacts'][image+'.iso']['sha256'],'starter_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'checks':[]}
 try:
  with zipfile.ZipFile(archive) as z:assert z.testzip() is None;z.extractall(work)
  folder=work/'EngacerOS';original=(folder/'EngacerOS.vmx').read_text();g=v.Guest(work/'same-vm',folder/'EngacerOS.vmdk','efi')
  observation='\n'.join(s for s in g.vmx.read_text().splitlines() if s.startswith(('serial0.','RemoteDisplay.')))+'\n'
  for file in folder.iterdir():shutil.copyfile(file,g.folder/file.name)
  g.vmx=g.folder/'EngacerOS.vmx';g.vmx.write_bytes((original+observation).encode())
  assert 'ethernet0.present = "TRUE"' in original and 'pciBridge4.virtualDev = "pcieRootPort"' in original and 'sound.present = "TRUE"' in original and 'usb_xhci.present = "TRUE"' in original and 'bios.bootOrder' not in original
  with g:
   v.shot(g,out,'welcome');v.enter(g);v.enter(g);v.tab(g,3);v.enter(g);v.enter(g);v.enter(g);v.shot(g,out,'review');v.enter(g);g.wait('M6_INSTALL_PASS',timeout=240);v.shot(g,out,'finish')
   start=len(g.read());v.enter(g);g.wait('M75_USER_RESTART',offset=start);g.wait('M7_DESKTOP_READY',offset=start,timeout=180)
   assert 'M6_INSTALLED_VOLUME' in g.read()[start:] and 'M6_WIZARD_READY' not in g.read()[start:];g.wait('M7_DHCP_CONFIGURED',offset=start,timeout=90)
   # VMware closes its guest RFB connection across the real firmware reset.
   g.rfb.close();g.rfb=None
   v.shot(g,out,'installed-desktop');display=g.connect_display();width,height=display.width,display.height
   fw=1080 if width>1200 else width-112;fh=720 if height>900 else height-100;left=(width-fw)//2;top=(height-60-fh)//2
   click(g,left+fw-76,top+18);v.shot(g,out,'maximized-files');click(g,width-76,18);v.shot(g,out,'restored-files')
   v.key(g,0xffeb)
   for c in 'settings':v.key(g,ord(c))
   start=len(g.read());v.enter(g);g.wait('M7_WINDOW_OPEN app=0x0000000000000003',offset=start);v.shot(g,out,'appearance')
   sx=(width-700)//2+24;sy=(height-60-550)//2+56
   click(g,sx+52,sy+244);click(g,sx+52,sy+370);g.wait('M11_APPEARANCE_SAVED');v.shot(g,out,'saved-appearance')
   (out/'install.log').write_bytes(g.read().encode())
  report['checks'].append('Exact archived blank 20-GiB SATA starter installs in owned VMware with all shipped devices enabled; retained ISO restarts into the actual installed kernel and obtains DHCP')
  report['checks'].append('Ordinary guest mouse and Windows-key Start search exercise maximize/restore and native Appearance/save without controlling the host desktop')
  cfg=g.vmx.read_text();g.vmx.write_bytes(re.sub(r'(?im)^ide1:0.startConnected\s*=.*$','ide1:0.startConnected = "FALSE"',cfg).encode());g.installed=True
  for turn in range(2):
   g.output='';g.pipe=None;g.rfb=None
   with g:
    g.wait('M7_DESKTOP_READY build='+meta['build_id']);assert 'M6_INSTALLED_VOLUME' in g.read();v.shot(g,out,'disk-only-'+str(turn));(out/('disk-only-'+str(turn)+'.log')).write_bytes(g.read().encode())
  report['checks'].append('Two independent VMware boots retain the same installed disk, firmware variables and VM configuration with only the ISO disconnected; no forced boot order')
  report['passed']=True
 except Exception:report['error']=v.old.v.redact(traceback.format_exc());raise
 finally:
  report['existing_vm_inventory_unchanged']=before==v.old.v.inventory();report['passed']=report['passed'] and report['existing_vm_inventory_unchanged'];(out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report,indent=2))
if __name__=='__main__':main()
