"""Reproduce, preserve and label actual unqualified QEMU SAS startup failures."""
import argparse,json,traceback
import ui
from controls import click
ROOT=ui.ROOT

def main():
 parser=argparse.ArgumentParser();parser.add_argument('--mixed-only',action='store_true');args=parser.parse_args()
 meta=json.loads((ROOT/'dist/m92-build-manifest.json').read_text(encoding='utf-8'));helper=ui.module('fixture71',ROOT/'tests/m71/native.py')
 for mixed in ([True] if args.mixed_only else [False,True]):
  name='scsi-target0-mixed-ovmf' if mixed else 'scsi-target1-ovmf';out=ROOT/'build/m92/known-issues'/name;out.mkdir(parents=True,exist_ok=True);w=ui.q.new_work('m92-known-mixed' if mixed else 'm92-known-target1');keep=ui.q.sparse_disk(w/'keeper.img',2*1024**3);target=ui.q.sparse_disk(w/'target.img',2*1024**3);helper.fixture(keep);helper.fixture(target);before=ui.q.digest(keep)
  extra=['-drive',f'if=none,id=second,format=raw,file={target}']+(['-device','mptsas1068,id=scsi'] if mixed else [])+['-device','scsi-hd,bus=scsi.0,scsi-id='+('0' if mixed else '1')+',drive=second']
  report={'passed':False,'qualified':False,'known_failure_reproduced':False,'build_id':meta['build_id'],'iso_sha256':meta['artifacts']['EngineOS-M92.iso']['sha256'],'test_topology':'SATA keeper and LSI SAS target 0' if mixed else 'LSI SAS targets 0 and 1','root_cause':'Not conclusively diagnosed; firmware visibility is a hypothesis, not an established cause.'}
  try:
   with ui.guest(w/'boot',keep,controller='sata' if mixed else 'mptsas',extra=extra) as g:
    p=ui.Pointer(g)
    for a in [2,101,91,2,2,2,92,2]:at=click(g,p,a)
    g.wait('M6_INSTALL_PASS',at,240);ui.wait_view(g,6,at);at=click(g,p,84);g.wait('M75_USER_RESTART',at);g.wait('M6_WIZARD_READY',at,180);assert 'M7_DESKTOP_READY' not in g.log()[at:];ui.shot(g,out,'returned-to-setup');(out/'last-run.log').write_bytes(g.log().encode())
   assert ui.q.digest(keep)==before;report.update(known_failure_reproduced=True,installation_verified=True,independent_startup_qualified=False,known_issue='Native installation completes but retained-ISO restart returns to live setup. VMware SAS startup has separate passing controller/firmware lifecycles.',unselected_disk_sha256=before)
  except Exception:report['test_error']=ui.q.clean(traceback.format_exc());raise
  finally:(out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('Unqualified startup case preserved:',name,report['known_failure_reproduced'],flush=True)
if __name__=='__main__':main()
