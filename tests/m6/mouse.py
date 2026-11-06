"""Actual USB mouse reaches the native Next button and changes wizard pages."""
import importlib.util,json,time,traceback
from pathlib import Path
from PIL import Image
spec=importlib.util.spec_from_file_location('m6guided',Path(__file__).with_name('guided.py'));h=importlib.util.module_from_spec(spec);spec.loader.exec_module(h)
q=h.q;ROOT=q.ROOT

def main():
    work=ROOT/'build/m6/mouse'/str(time.time_ns());work.mkdir(parents=True);out=ROOT/'evidence/M6/mouse';out.mkdir(parents=True,exist_ok=True);result={'passed':False}
    try:
        disk=work/'owned.img'
        with disk.open('xb') as f:f.truncate(2*1024**3)
        with q.Guest(work/'live',disk) as g:
            h.shot(g,out,'before')
            with Image.open(out/'before.png') as frame:assert frame.size==(1280,800)
            for dx,dy in [(120,100)]*4+[(0,100)]*2+[(0,4)]:
                g.qmp('input-send-event',{'events':[{'type':'rel','data':{'axis':'x','value':dx}},{'type':'rel','data':{'axis':'y','value':dy}}]});time.sleep(.08)
            g.qmp('input-send-event',{'events':[{'type':'btn','data':{'down':True,'button':'left'}}]});time.sleep(.1);g.qmp('input-send-event',{'events':[{'type':'btn','data':{'down':False,'button':'left'}}]});h.shot(g,out,'disk-selected-by-mouse')
            m=json.loads((ROOT/'dist/m6-build-manifest.json').read_text(encoding='utf-8'));accent=tuple(bytes.fromhex(m['distribution']['accent'][1:]))
            with Image.open(out/'disk-selected-by-mouse.png') as frame:assert frame.getpixel((44,209))==accent
            registers=g.qmp('human-monitor-command',{'command-line':'info registers -a'});assert registers.count('HLT=1')==2
            (out/'serial.log').write_bytes(g.log().encode())
            result.update(passed=True,build_id=m['build_id'],iso_sha256=m['artifacts']['EngineOS-M6.iso']['sha256'],checks=[{'name':'native USB mouse reaches footer, Next activates disk selection, press/release is consumed and both CPUs return to halt','passed':True}])
    except Exception:result['error']=traceback.format_exc().replace(str(ROOT),'<project-root>');print(result['error'])
    (out/'results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print('M6 USB mouse '+('PASS' if result['passed'] else 'FAIL'),flush=True)
    if not result['passed']:raise SystemExit(1)
if __name__=='__main__':main()
