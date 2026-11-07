"""Real native progress and actionable missing/small-disk and SATA EIO screens."""
import json
import traceback
import ui
q=ui.q
def choose(g):
 point=ui.Pointer(g);point.wizard(2);point.wizard(100);point.wizard(2);point.wizard(2);point.wizard(2);return point
def main():
 work=q.new_work('m73-errors');out=ui.ROOT/'evidence/M7.3/errors';out.mkdir(parents=True,exist_ok=True);report={'passed':False}
 try:
  disk=q.sparse_disk(work/'progress.img',2*1024**3)
  with ui.guest(work/'progress',disk) as g:
   point=choose(g);start=point.wizard(2);g.wait('M6_PROGRESS',start)
   import time
   for attempt in range(100):
    g.qmp('stop')
    if ui.integer(g,'painting73')==0 and ui.integer(g,'step')==5:break
    g.qmp('cont');time.sleep(.003)
   else:raise AssertionError('No completed native progress frame before finishing')
   assert ui.integer(g,'step')==5,'Progress capture must show the actual installing state'
   ui.shot(g,out,'01-real-installation-progress');g.qmp('cont');g.wait('M6_INSTALL_PASS',start,240);ui.wait_view(g,6,start);(out/'progress.log').write_bytes(g.log().encode())
  disk=q.sparse_disk(work/'fault.img',2*1024**3);fault={'event':'write_aio','iotype':'write','errno':5,'sector':2048,'once':False}
  with ui.guest(work/'fault',disk,fault=fault) as g:
   point=choose(g);start=point.wizard(2);g.wait('M6_INSTALL_FAILED',start,240);failure=g.log().index('M6_INSTALL_FAILED',start);g.wait('M73_VIEW step=',failure);ui.shot(g,out,'02-real-write-failure')
   assert 'M6_INSTALL_PASS' not in g.log();point.wizard(50);ui.shot(g,out,'03-error-details');point.wizard(62);ui.shot(g,out,'04-return-to-error');point.wizard(63);assert ui.integer(g,'step')==0
   assert 'M73_TEXT_OVERFLOW' not in g.log();(out/'fault.log').write_bytes(g.log().encode())
  disk=q.sparse_disk(work/'small.img',512*1024**2);before=q.digest(disk)
  with ui.guest(work/'small',disk) as g:
   point=ui.Pointer(g);point.wizard(2);point.wizard(100);point.wizard(2);assert ui.integer(g,'step')==1;ui.shot(g,out,'05-disk-too-small');assert q.digest(disk)==before;(out/'small.log').write_bytes(g.log().encode())
  absent=ui.guest(work/'absent',disk);i=next(i for i,arg in enumerate(absent.args) if arg.startswith('ide-hd,bus=ide.0,drive=target'));assert absent.args[i-1]=='-device';del absent.args[i-1:i+1]
  with absent as g:
   point=ui.Pointer(g);point.wizard(2);ui.shot(g,out,'06-no-disk-guidance');assert 'M73_TEXT_OVERFLOW' not in g.log();(out/'absent.log').write_bytes(g.log().encode())
  meta=json.loads((ui.ROOT/'dist/m73-build-manifest.json').read_text(encoding='utf-8'));report.update(passed=True,build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M73.iso']['sha256'],checks=['actual native installing state and framebuffer, paused only for capture then resumed to verified success','real SATA write EIO refuses success and shows readable recovery instructions','Details from failure opens diagnostics; Return restores failure and Back returns safely','undersized disk receives concrete 2-GiB guidance without any byte changed','missing SATA disk displays graphical VMware connection instructions'])
 except Exception:
  report['error']=q.clean(traceback.format_exc());print(report['error'])
  if 'g' in locals():(out/'last-run.log').write_bytes(g.log().encode())
 (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M7.3 errors '+str(report['passed']),flush=True)
 if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
