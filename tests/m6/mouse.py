"""Actual USB mouse reaches the native Next button and changes wizard pages."""
import argparse,importlib.util,json,time,traceback
from pathlib import Path
from PIL import Image
spec=importlib.util.spec_from_file_location('m6guided',Path(__file__).with_name('guided.py'));h=importlib.util.module_from_spec(spec);spec.loader.exec_module(h)
q=h.q;ROOT=q.ROOT

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--device',choices=['usb','ps2'],default='usb');args=parser.parse_args()
    work=q.new_work('mouse');out=ROOT/'evidence/M6'/('mouse-ps2' if args.device=='ps2' else 'mouse');out.mkdir(parents=True,exist_ok=True);result={'passed':False}
    try:
        disk=work/'owned.img'
        q.sparse_disk(disk,2*1024**3)
        with q.Guest(work/'live',disk,ps2_only=args.device=='ps2') as g:
            if args.device=='ps2':assert 'M6_PS2_MOUSE_ONLINE' in g.log()
            h.shot(g,out,'before')
            with Image.open(out/'before.png') as frame:assert frame.size==(1280,800)
            for dx,dy in [(120,100)]*4+[(0,100)]*2+[(0,4)]:
                g.qmp('input-send-event',{'events':[{'type':'rel','data':{'axis':'x','value':dx}},{'type':'rel','data':{'axis':'y','value':dy}}]});time.sleep(.08)
            g.qmp('input-send-event',{'events':[{'type':'btn','data':{'down':True,'button':'left'}}]});time.sleep(.1);g.qmp('input-send-event',{'events':[{'type':'btn','data':{'down':False,'button':'left'}}]});h.shot(g,out,'disk-selected-by-mouse')
            m=json.loads((ROOT/'dist/m6-build-manifest.json').read_text(encoding='utf-8'));accent=tuple(bytes.fromhex(m['distribution']['accent'][1:]))
            with Image.open(out/'disk-selected-by-mouse.png') as frame:assert frame.getpixel((44,209))==accent
            if args.device=='ps2':
                h.next(g);h.next(g,2);h.shot(g,out,'components-by-ps2-keyboard')
                with Image.open(out/'components-by-ps2-keyboard.png') as frame:assert frame.getpixel((44,253))==accent
            registers=g.qmp('human-monitor-command',{'command-line':'info registers -a'});assert registers.count('HLT=1')==2
            (out/'serial.log').write_bytes(g.log().encode())
            result.update(passed=True,pointer_device=args.device,build_id=m['build_id'],iso_sha256=m['artifacts']['EngineOS-M6.iso']['sha256'],checks=[{'name':'native mouse reaches footer, Next activates disk selection, press/release is consumed and both CPUs return to halt','passed':True}])
    except Exception:result['error']=q.clean(traceback.format_exc());print(result['error'])
    (out/'results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print('M6 '+args.device+' mouse '+('PASS' if result['passed'] else 'FAIL'),flush=True)
    if not result['passed']:raise SystemExit(1)
if __name__=='__main__':main()
