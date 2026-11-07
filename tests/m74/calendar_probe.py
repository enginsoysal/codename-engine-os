"""Reproduce native Calendar save after reopening on actual 4K."""
import ui
import json,time
from apps import open_app,app_text
out=ui.ROOT/'evidence/M7.4/calendar-probe';out.mkdir(parents=True,exist_ok=True)
work=ui.q.new_work('m74-calendar-probe');disk=ui.q.sparse_disk(work/'owned.img',2*1024**3)
with ui.guest(work/'install',disk,(3840,2160)) as g:
 p=ui.Pointer(g);p.wizard(2);p.wizard(100);p.wizard(2);p.wizard(2);p.wizard(2);start=p.wizard(2);g.wait('M6_INSTALL_PASS',start,240);ui.wait_view(g,6,start);p.wizard(3);g.wait('M7_DESKTOP_READY');p.close_app(1)
 open_app(p,9);p.close_app(9);open_app(p,9)
 for c in 'keep my private note':ui.h.key(g,'spc' if c==' ' else c)
 started=time.monotonic()
 while app_text(g,9)!=b'keep my private note' and time.monotonic()-started<1:time.sleep(.025)
 latency=time.monotonic()-started;assert app_text(g,9)==b'keep my private note','4K text input did not keep up'
 print(repr(app_text(g,9)),flush=True);ui.shot(g,out,'typed');p.desktop(10052);ui.shot(g,out,'saved');print(g.log()[-1000:],flush=True)
 (out/'native.log').write_bytes(g.log().encode('utf-8'))
 assert b'keep my private note'==app_text(g,9)
 assert 'M7_DOCUMENT_SAVED' in g.log()
 manifest=json.loads((ui.ROOT/'dist/m74-build-manifest.json').read_text(encoding='utf-8'))
 report={'passed':True,'build_id':manifest['build_id'],'iso_sha256':manifest['artifacts']['EngineOS-M74.iso']['sha256'],'queued_text_drain_seconds':latency,'resolution':[3840,2160],'checks':['Calendar close/reopen and 19 typed characters on actual 4K','native Save button persisted a real private note']}
 (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode('utf-8'))
