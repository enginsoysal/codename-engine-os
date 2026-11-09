"""Native installer refusal/review/cancel paths on disposable files only."""
import argparse,json,struct,traceback
import ui
from controls import click,control,state
ROOT=ui.ROOT;q=ui.q

def main():
 a=argparse.ArgumentParser();a.add_argument('--controller',choices=['sata','ide','mptsas','nvme','virtio'],default='ide');a=a.parse_args()
 out=ROOT/'build/m92/safety'/a.controller;out.mkdir(parents=True,exist_ok=True);work=q.new_work('m92-safety');report={'passed':False,'checks':[]};meta=json.loads((ROOT/'dist/m92-build-manifest.json').read_text());report.update(build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M92.iso']['sha256'])
 try:
  disk=q.sparse_disk(work/'existing.img',2*1024**3)
  with disk.open('r+b') as f:f.write(b'PRIVATE-TEST-DATA'+bytes(4096-17))
  before=q.digest(disk,0,2*1024**2)
  with ui.guest(work/'existing',disk,controller=a.controller) as g:
   p=ui.Pointer(g);click(g,p,2);click(g,p,100,True);assert not control(g,2);assert state(g,'step')==1;ui.shot(g,out,'01-existing-disk-guidance');assert q.digest(disk,0,2*1024**2)==before
   click(g,p,90);ui.shot(g,out,'02-disk-tools');click(g,p,200);assert state(g,'u71_mode')==0
   click(g,p,91);click(g,p,2);click(g,p,2);click(g,p,2);assert state(g,'step')==4;assert not control(g,2);ui.shot(g,out,'03-erasure-permission-required');assert q.digest(disk,0,2*1024**2)==before
   click(g,p,92);assert control(g,2);ui.shot(g,out,'04-erasure-confirmed');click(g,p,1);assert state(g,'step')==3;assert q.digest(disk,0,2*1024**2)==before
   report['checks'].append('existing data rejected by default; full review and separate erase consent; Disk tools return; Back cancels without writes')
   (out/'existing.log').write_bytes(g.log().encode())
  small=q.sparse_disk(work/'small.img',512*1024**2)
  small_before=q.digest(small,0,1024**2)
  with ui.guest(work/'small',small,controller=a.controller) as g:
   p=ui.Pointer(g);click(g,p,2);click(g,p,100);assert not control(g,2);assert state(g,'step')==1;ui.shot(g,out,'05-too-small-actionable');assert q.digest(small,0,1024**2)==small_before
  absent=ui.guest(work/'absent',small,controller=a.controller);i=next(i for i,x in enumerate(absent.args) if 'drive=target' in x and not x.startswith('if=none'));del absent.args[i-1:i+1]
  with absent as g:
   p=ui.Pointer(g);click(g,p,2);assert not control(g,2);ui.shot(g,out,'06-no-disk-actionable');(out/'absent.log').write_bytes(g.log().encode())
  report['checks'].append('missing disk and unsupported capacity produce in-place guidance without a misleading success')
  fault=q.sparse_disk(work/'fault.img',2*1024**3)
  # Fail the first target write after normal read-only planning.
  with ui.guest(work/'fault',fault,fault={'event':'write_aio','errno':5,'once':True,'immediately':True},controller=a.controller) as g:
   p=ui.Pointer(g);click(g,p,2);click(g,p,100);click(g,p,2);click(g,p,2);click(g,p,2);start=click(g,p,2);g.wait('M6_INSTALL_FAILED',start,180);assert 'M6_INSTALL_PASS' not in g.log();ui.shot(g,out,'07-write-failure');click(g,p,63);assert state(g,'step')==1;assert ui.memory(g,ui.symbol('preferences',40)[0],6)==b'Owner\0';ui.shot(g,out,'08-recoverable-return');(out/'failure.log').write_bytes(g.log().encode())
  report['checks'].append('injected supported-controller write failure never reports success; Back returns to disk choice with personal choices retained')
  report['passed']=True
 except Exception:
  report['error']=q.clean(traceback.format_exc());print(report['error'])
  if 'g' in locals():(out/'last-run.log').write_bytes(g.log().encode())
 (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M9.2 safety '+('PASS' if report['passed'] else 'FAIL'),flush=True)
 if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
