"""Native controller discovery and installer admission on disposable guest disks."""
import argparse,json,struct,time,traceback
from pathlib import Path
import ui
ROOT=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--controller',choices=['sata','ide','mptsas','pvscsi','nvme','virtio'],required=True);p.add_argument('--firmware',choices=['uefi','bios'],default='uefi');a=p.parse_args()
 out=ROOT/'build/m92/probe'/(a.controller+'-'+a.firmware);out.mkdir(parents=True,exist_ok=True);work=ui.q.new_work('m92-probe-'+a.controller);disk=ui.q.sparse_disk(work/'owned.img',2*1024**3)
 meta=json.loads((ROOT/'dist/m92-build-manifest.json').read_text());report={'passed':False,'controller':a.controller,'firmware':a.firmware,'build_id':meta['build_id'],'iso_sha256':meta['artifacts']['EngineOS-M92.iso']['sha256']}
 try:
  with ui.guest(work/'boot',disk,firmware=a.firmware,controller=a.controller) as g:
   raw=ui.memory(g,ui.symbol('disks',640)[0],40);report['first_disk']={'transport':struct.unpack_from('<I',raw)[0],'readable':struct.unpack_from('<I',raw,16)[0],'sectors':struct.unpack_from('<Q',raw,24)[0]}
   assert report['first_disk']['readable'] and report['first_disk']['sectors']==4194304,report
   p=ui.Pointer(g)
   for action in [2,100,2]:p.wizard(action)
   assert ui.integer(g,'step')==2
   report['passed']=True
   (out/'serial.log').write_bytes(g.log().encode())
 except Exception:
  report['error']=ui.q.clean(traceback.format_exc())
  if 'g' in locals():(out/'serial.log').write_bytes(g.log().encode())
  raise
 finally:
  (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(a.controller+' '+a.firmware+' probe '+('PASS' if report['passed'] else 'FAIL'),flush=True)
if __name__=='__main__':main()
