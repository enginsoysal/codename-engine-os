"""Package isolated M11 stage media; never reads or overwrites a user's VM."""
import argparse,hashlib,importlib.util,json,shutil,subprocess,sys,uuid,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/build'))
from m8 import privacy_input
spec=importlib.util.spec_from_file_location('configuration10',ROOT/'tools/release/vmware_starter10.py');base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base)
def configuration(image,controller='sata',firmware='efi'):
 return base.configuration(controller,firmware).replace('EngacerOS-M10.iso',image+'.iso')
def main():
 p=argparse.ArgumentParser();p.add_argument('--revision',default='stage4');a=p.parse_args()
 assert a.revision in ('stage4','stage5','final')
 image='EngacerOS-M11'+a.revision.upper();meta=json.loads((ROOT/('dist/m11'+a.revision+'-build-manifest.json')).read_text())
 iso=ROOT/'dist'/(image+'.iso');assert hashlib.sha256(iso.read_bytes()).hexdigest()==meta['artifacts'][iso.name]['sha256']
 folder=ROOT/'build/m11/starter'/a.revision/uuid.uuid4().hex;folder.mkdir(parents=True)
 tool=Path('C:/Program Files/VMware/VMware Workstation/vmware-vdiskmanager.exe')
 subprocess.run([str(tool),'-c','-s','20GB','-a','ide','-t','0',str(folder/'EngacerOS.vmdk')],capture_output=True,check=True,timeout=60,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
 (folder/'EngacerOS.vmx').write_bytes(configuration(image).encode())
 shutil.copyfile(iso,folder/iso.name)
 for source,dest in [('LICENSE','LICENSE'),('third_party/inter/OFL.txt','Inter-OFL.txt'),('third_party/7zip/7z2606-src.tar.xz','7z2606-src.tar.xz'),('third_party/libmspack/COPYING.LIB','libmspack-LGPL-2.1.txt')]:shutil.copyfile(ROOT/source,folder/dest)
 notices=(ROOT/'dist/EngineOS-M71-THIRD-PARTY-NOTICES.txt').read_text().replace('Engine OS M7.1 third-party notices','Engacer OS M11 third-party notices',1)
 for name in ['third_party/inter/OFL.txt','third_party/miniz/LICENSE','third_party/7zip/LICENSE.txt','third_party/libmspack/COPYING.LIB']:notices+='\n\n'+(ROOT/name).read_text()
 (folder/'THIRD-PARTY-NOTICES.txt').write_bytes(notices.encode())
 text=('ENGACER OS M11 / '+a.revision.upper()+'\n\nExtract the complete ZIP into an ordinary local folder. Open EngacerOS.vmx.\nChoose I Copied It if asked. Start the VM and follow the graphical installer.\nSelect the disk row, review erasure and install, then choose Restart to Engacer OS.\nThe matching ISO may stay connected. Keep the installed hard disk attached.\n\nThis is a new empty 20-GiB SATA disk, not a preinstalled desktop.\n128 MiB RAM, two CPUs, UEFI, Secure Boot off. No online account or driver installer.\nStage qualification and known limits: https://engaceros.brixcanvas.com/blog/documentation\nSource, releases, licenses and full checksums: https://github.com/enginsoysal/engacer-os\n')
 (folder/'START-HERE.txt').write_bytes(text.encode())
 archive=ROOT/'dist'/(image+'-VMware.zip')
 with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for file in sorted(folder.iterdir()):
   data=privacy_input(file.read_bytes());assert b'C:\\Users\\' not in data and b'C:/Users/' not in data
   z.write(file,'EngacerOS/'+file.name)
 with zipfile.ZipFile(archive) as z:
  assert z.testzip() is None and z.read('EngacerOS/EngacerOS.vmx').decode()==configuration(image)
  assert hashlib.sha256(z.read('EngacerOS/'+iso.name)).hexdigest()==meta['artifacts'][iso.name]['sha256']
 report={'passed':True,'archive':archive.name,'sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'size':archive.stat().st_size,'image':image,'build_id':meta['build_id'],'members':[f.name for f in sorted(folder.iterdir())],'disk':'new growable 20-GiB SATA; not preinstalled','firmware':'UEFI, Secure Boot off','boot_order_forced':False,'all_shipped_devices_enabled':True}
 (ROOT/'dist'/('m11'+a.revision+'-starter-manifest.json')).write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report,indent=2))
if __name__=='__main__':main()
