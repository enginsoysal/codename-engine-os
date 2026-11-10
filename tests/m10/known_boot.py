"""Preserve the unqualified 64-MiB OVMF boot-attached USB topology honestly."""
import json
import ui
from journey import fat32
ROOT=ui.ROOT
def main():
    out=ROOT/'build/m10/known-uefi64-boot-usb';out.mkdir(parents=True,exist_ok=True)
    work=ui.q.new_work('m10-known-boot');disk=ui.q.sparse_disk(work/'owned.img',2*1024**3)
    usb=work/'owned-usb.img';usb.write_bytes(fat32([(2,b'HELLO   TXT',b'Owned firmware topology test\n')]))
    extra=['-drive',f'if=none,id=removable,format=raw,file={usb}','-device','usb-storage,bus=xhci.0,drive=removable']
    meta=json.loads((ROOT/'dist/m10-build-manifest.json').read_text(encoding='utf-8'))
    r={'passed':False,'qualified':False,'build_id':meta['build_id'],'iso_sha256':meta['artifacts']['EngacerOS-M10.iso']['sha256'],'condition':'64-MiB OVMF with USB storage attached before firmware boot','expected_refusal_recorded':False}
    try:
        with ui.guest(work/'boot',disk,ram=64,extra=extra):raise AssertionError('Topology behavior changed; investigate and qualify before updating this negative record')
    except RuntimeError as error:
        message=ui.q.clean(str(error));assert 'M10_KERNEL_STREAM_LOADED' in message and 'LOADER_REJECT: allocate low pages' in message,message
        r.update(error=message,expected_refusal_recorded=True)
    finally:
        serial=work/'boot/serial.log'
        if serial.exists():(out/'serial.log').write_text(ui.q.clean(serial.read_text(encoding='utf-8',errors='replace')),encoding='utf-8')
        (out/'results.json').write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8')
    print('Known 64-MiB OVMF boot-attached USB refusal retained as unqualified')
if __name__=='__main__':main()
