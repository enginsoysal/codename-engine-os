"""Native read-only VirtIO admission and unchanged media, with no driver mocks."""
import json,traceback
import ui
from controls import click,control
ROOT=ui.ROOT

def main():
 out=ROOT/'build/m92/readonly-virtio';out.mkdir(parents=True,exist_ok=True);w=ui.q.new_work('m92-readonly');d=ui.q.sparse_disk(w/'owned.img',2*1024**3);before=ui.q.digest(d);meta=json.loads((ROOT/'dist/m92-build-manifest.json').read_text(encoding='utf-8'));r={'passed':False,'build_id':meta['build_id'],'iso_sha256':meta['artifacts']['EngineOS-M92.iso']['sha256']}
 try:
  guest=ui.guest(w/'boot',d,controller='virtio');guest.args=[s+',readonly=on' if s.startswith('if=none,id=target,') else s for s in guest.args]
  with guest as g:
   assert ui.integer(g,'count')==1;p=ui.Pointer(g);click(g,p,2);click(g,p,100);assert not control(g,2);ui.shot(g,out,'read-only-inspection');assert 'M6_INSTALL_PASS' not in g.log();(out/'serial.log').write_bytes(g.log().encode())
  assert ui.q.digest(d)==before;r.update(passed=True,unchanged_disk_sha256=before,checks=['Actual VirtIO read-only feature negotiation allows inspection and refuses installation','No writable admission, installer success or changed disk bytes'])
 except Exception:r['error']=ui.q.clean(traceback.format_exc());raise
 finally:(out/'results.json').write_bytes((json.dumps(r,indent=2)+'\n').encode());print('M92 native read-only refusal '+str(r['passed']),flush=True)
if __name__=='__main__':main()
