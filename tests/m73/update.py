"""Keep an accepted M7.2 workspace through update, detached startup and rollback."""
import json
import traceback
import ui
q,h=ui.q,ui.h
def main():
 work=q.new_work('m73-update');out=ui.ROOT/'evidence/M7.3/update';out.mkdir(parents=True,exist_ok=True);report={'passed':False}
 try:
  disk=q.sparse_disk(work/'owned.img',2*1024**3)
  with q.Guest(work/'older',disk,image_name='EngineOS-M72') as g:
   h.next(g);h.next(g);h.next(g,4);h.next(g,2);h.next(g,5);h.next(g,1);g.wait('M6_INSTALL_PASS',timeout=240)
  original=ui.s.s.document(disk,'/Users/Owner/Documents/Welcome.txt')
  for action,label,expected in [(81,'update','m73'),(82,'rollback','m72')]:
   recovery=ui.guest(work/label,disk)
   with recovery as g:
    point=ui.Pointer(g);point.wizard(50);point.wizard(action);point.wizard(100);point.wizard(2);ui.shot(g,out,label+'-review');start=point.wizard(2);g.wait('M6_MAINTENANCE_PASS',start,240);ui.wait_view(g,6,start);ui.shot(g,out,label+'-finish');(out/(label+'.log')).write_bytes(g.log().encode())
   assert ui.s.s.document(disk,'/Users/Owner/Documents/Welcome.txt')==original
   guest=ui.guest(work/(label+'-desktop'),disk,media='installed',ready='M7_DESKTOP_READY') if expected=='m73' else q.Guest(work/(label+'-desktop'),disk,media='installed',image_name='EngineOS-M72',ready='M7_DESKTOP_READY')
   ui.s.normal_boot(guest,recovery.folder/'vars.fd')
   with guest as g:
    meta=json.loads((ui.ROOT/'dist'/(expected+'-build-manifest.json')).read_text(encoding='utf-8'));assert 'M7_DESKTOP_READY build='+meta['build_id'] in g.log();ui.shot(g,out,label+'-desktop');(out/(label+'-desktop.log')).write_bytes(g.log().encode())
   assert ui.s.s.document(disk,'/Users/Owner/Documents/Welcome.txt')==original
  meta=json.loads((ui.ROOT/'dist/m73-build-manifest.json').read_text(encoding='utf-8'));report.update(passed=True,build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M73.iso']['sha256'],checks=['exact accepted M7.2 installation','native graphical recovery/update/review/confirm','detached M7.3 desktop with original personal file bytes preserved','native rollback and unforced retained-NVRAM startup of exact M7.2 build'])
 except Exception:
  report['error']=q.clean(traceback.format_exc());print(report['error'])
  if 'g' in locals():(out/'last-run.log').write_bytes(g.log().encode())
 (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M7.3 update '+str(report['passed']),flush=True)
 if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
