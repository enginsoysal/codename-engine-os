"""Owned VMware discovery/admission checks before full per-controller lifecycles."""
import argparse,json,time,traceback,uuid
from pathlib import Path
import vmware as shared
ROOT=shared.ROOT;v=shared.v
def main():
 p=argparse.ArgumentParser();p.add_argument('--controller',required=True);p.add_argument('--firmware',choices=['efi','bios'],default='efi');a=p.parse_args()
 work=ROOT/'build/m92/vmware/_probe'/a.controller/a.firmware/uuid.uuid4().hex;work.mkdir(parents=True)
 out=ROOT/'build/m92/probe-vmware'/(a.controller+'-'+a.firmware);out.mkdir(parents=True,exist_ok=True)
 disk=shared.sparse(work/'owned-flat.img',2*1024**3);descriptor=work/'owned.vmdk';adapter='buslogic' if a.controller=='buslogic' else 'lsilogic' if a.controller in ['lsilogic','lsisas1068','pvscsi'] else 'ide'
 descriptor.write_bytes(('version=1\nencoding="UTF-8"\nCID=fffffffe\nparentCID=ffffffff\ncreateType="monolithicFlat"\nRW 4194304 FLAT "owned-flat.img" 0\nddb.adapterType = "'+adapter+'"\n').encode())
 meta=json.loads((ROOT/'dist/m92-build-manifest.json').read_text());before=v.old.v.inventory();result={'passed':False,'controller':a.controller,'firmware':a.firmware,'build_id':meta['build_id'],'iso_sha256':meta['artifacts']['EngineOS-M92.iso']['sha256']}
 try:
  g=v.Guest(work/'boot',descriptor,a.firmware);g.vmx.write_bytes(shared.configure92(g,descriptor,a.controller,a.firmware).encode())
  with g:
   v.shot(g,out,'welcome');v.enter(g);v.enter(g);v.tab(g,3);v.enter(g);g.wait('M73_VIEW step=0x0000000000000002',timeout=30);v.shot(g,out,'admitted');(out/'serial.log').write_bytes(g.read().encode());result['passed']=True
 except Exception:
  result['error']=v.old.v.redact(traceback.format_exc())
  if 'g' in locals():(out/'serial.log').write_bytes(g.output.encode())
  raise
 finally:
  end=time.monotonic()+10
  while before!=v.old.v.inventory() and time.monotonic()<end:time.sleep(.2)
  result['existing_vm_inventory_unchanged']=before==v.old.v.inventory();result['passed']=result['passed'] and result['existing_vm_inventory_unchanged'];(out/'results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print('VMware '+a.controller+' '+a.firmware+' probe '+('PASS' if result['passed'] else 'FAIL'),flush=True)
if __name__=='__main__':main()
