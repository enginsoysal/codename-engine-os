"""Actual xHCI device quota, cleanup and keyboard recovery in the native wizard."""
import json,struct,time,traceback
import ui
from journey import shot,wizard
ROOT=ui.ROOT
def main():
    out=ROOT/'build/m10/usb-quota';out.mkdir(parents=True,exist_ok=True)
    work=ui.q.new_work('m10-usb-quota');disk=ui.q.sparse_disk(work/'owned.img',2*1024**3)
    meta=json.loads((ROOT/'dist/m10-build-manifest.json').read_text(encoding='utf-8'))
    r={'passed':False,'build_id':meta['build_id'],'iso_sha256':meta['artifacts']['EngacerOS-M10.iso']['sha256'],'checks':[]}
    try:
        with ui.guest(work/'boot',disk) as g:
            at=ui.symbol('dma_pages',8)[0];before=struct.unpack('<Q',ui.memory(g,at,8))[0]
            for n in range(6):
                start=len(g.log());g.qmp('device_add',{'driver':'usb-kbd','id':'extra'+str(n),'bus':'xhci.0'});g.wait('M4_USB_KEYBOARD_ONLINE',start)
            full=struct.unpack('<Q',ui.memory(g,at,8))[0]
            start=len(g.log());g.qmp('device_add',{'driver':'usb-kbd','id':'over-quota','bus':'xhci.0'});g.wait('M45_USB_DEVICE_QUOTA',start)
            assert struct.unpack('<Q',ui.memory(g,at,8))[0]==full
            r['checks'].append('Eight active USB devices; ninth refused without allocating DMA pages or crashing')
            g.qmp('device_del',{'id':'over-quota'})
            for n in range(6):
                start=len(g.log());g.qmp('device_del',{'id':'extra'+str(n)});g.wait('M45_USB_DISCONNECTED',start)
            assert struct.unpack('<Q',ui.memory(g,at,8))[0]==before
            start=len(g.log());g.qmp('device_add',{'driver':'usb-kbd','id':'recovered','bus':'xhci.0'});g.wait('M4_USB_KEYBOARD_ONLINE',start)
            wizard(g,ui.Pointer(g),2);shot(g,out,'responsive-after-quota',False)
            r['checks'].append('All six admitted device allocations reclaimed; replacement binds and the native wizard remains usable')
            r.update(passed=True,usb_dma_pages_before=before,usb_dma_pages_full=full)
            (out/'serial.log').write_text(g.log(),encoding='utf-8')
    except Exception:r['error']=ui.q.clean(traceback.format_exc());raise
    finally:
        if 'g' in locals():(out/'serial.log').write_text(g.log(),encoding='utf-8')
        (out/'results.json').write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8')
if __name__=='__main__':main()
