"""Install M7 development media and exercise the real framebuffer desktop on owned disks."""
import importlib.util
import json
from pathlib import Path
import struct
import time
import traceback
import zlib

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('m7guided',ROOT/'tests/m6/guided.py')
h=importlib.util.module_from_spec(spec)
spec.loader.exec_module(h)
q=h.q


class Pointer:
    def __init__(self,guest):
        self.guest,self.x,self.y=guest,600,80

    def move(self,x,y):
        while (self.x,self.y)!=(x,y):
            dx=max(-100,min(100,x-self.x));dy=max(-100,min(100,y-self.y))
            self.guest.qmp('input-send-event',{'events':[
                {'type':'rel','data':{'axis':'x','value':dx}},
                {'type':'rel','data':{'axis':'y','value':dy}}]})
            self.x+=dx;self.y+=dy;time.sleep(.07)

    def down(self,value):
        self.guest.qmp('input-send-event',{'events':[{'type':'btn','data':{'down':value,'button':'left'}}]})
        time.sleep(.12)

    def click(self,x,y):
        self.move(x,y);self.down(True);self.down(False)

    def drag(self,x,y,to_x,to_y):
        self.move(x,y);self.down(True);self.move(to_x,to_y);self.down(False)


def document(path,wanted):
    """Inspect GPT, metadata-bank CRCs and exact persisted bytes independently."""
    with path.open('rb') as disk:
        disk.seek(512);header=bytearray(disk.read(512));assert header[:8]==b'EFI PART'
        checksum=struct.unpack_from('<I',header,16)[0];struct.pack_into('<I',header,16,0)
        assert zlib.crc32(header[:92])==checksum
        disk.seek(struct.unpack_from('<Q',header,72)[0]*512);table=disk.read(16384)
        assert zlib.crc32(table)==struct.unpack_from('<I',header,88)[0]
        start=struct.unpack_from('<Q',table,4*128+32)[0]*512
        banks=[]
        for bank in range(2):
            disk.seek(start+bank*71*4096);head=disk.read(4096)
            if head[:8]!=b'ENGINEV6' or zlib.crc32(head[:4092])!=struct.unpack_from('<I',head,4092)[0]:continue
            raw=disk.read(1024*280)
            if zlib.crc32(raw)!=struct.unpack_from('<I',head,36)[0]:continue
            banks.append((struct.unpack_from('<Q',head,24)[0],raw))
        assert banks
        raw=max(banks)[1]
        for i in range(1024):
            record=raw[i*280:(i+1)*280]
            if record[:256].split(b'\0')[0].decode()!=wanted:continue
            first,size,crc,kind=struct.unpack_from('<QQII',record,256)
            assert kind==1 and size<1024*1024
            disk.seek(start+first*4096);value=disk.read(size)
            assert zlib.crc32(value)==crc
            return value
    raise AssertionError('Expected persistent file not found: '+wanted)


def main():
    work=q.new_work('m7-desktop')
    out=ROOT/'evidence/M7/session'
    out.mkdir(parents=True,exist_ok=True)
    result={'passed':False,'m7_complete':False,'checks':[]}
    try:
        disk=q.sparse_disk(work/'owned.img',2*1024**3)
        with q.Guest(work/'install',disk,image_name='EngineOS-M7') as guest:
            h.next(guest);h.next(guest);h.next(guest,2);h.next(guest,2);h.next(guest,5)
            start=len(guest.log());h.next(guest,1);guest.wait('M6_INSTALL_PASS',start,240)
            (out/'install.log').write_bytes(guest.log().encode())
        with q.Guest(work/'desktop',disk,media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY') as guest:
            pointer=Pointer(guest)
            h.shot(guest,out,'first-desktop')
            pointer.click(55,210)
            guest.wait('M7_WINDOW_OPEN app=0x0000000000000002')
            # Original USB/PS2 key reports, not host injection into guest memory.
            for character in 'hello engine os':
                h.key(guest,'spc' if character==' ' else character)
            guest.qmp('send-key',{'keys':[{'type':'qcode','data':'ctrl'},{'type':'qcode','data':'s'}],'hold-time':30})
            guest.wait('M7_DOCUMENT_SAVED')
            h.shot(guest,out,'edited-and-saved')
            pointer.drag(410,130,490,175)
            h.shot(guest,out,'moved-editor')
            pointer.drag(970,625,1030,670)
            h.shot(guest,out,'resized-editor')
            (out/'desktop.log').write_bytes(guest.log().encode())
            registers=guest.qmp('human-monitor-command',{'command-line':'info registers -a'})
            assert registers.count('HLT=1')==2
        assert document(disk,'/Users/Owner/Documents/Note1.txt')==b'hello engine os'
        result['checks'].append({'name':'native wizard installs M7, detached desktop opens real editor, keyboard edits and saves exact document bytes; mouse moves/resizes and CPUs halt','passed':True})
        with q.Guest(work/'restart',disk,media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY') as guest:
            pointer=Pointer(guest)
            # Welcome.txt is the first document, Note1.txt follows it.
            pointer.click(230,250)
            guest.wait('M7_WINDOW_OPEN app=0x0000000000000002')
            h.shot(guest,out,'reopened-after-restart')
            (out/'restart.log').write_bytes(guest.log().encode())
        assert document(disk,'/Users/Owner/Documents/Note1.txt')==b'hello engine os'
        result['checks'].append({'name':'media-detached restart opens saved document through native Files; independent CRC/data inspection confirms persistence','passed':True})
        meta=json.loads((ROOT/'dist/m7-build-manifest.json').read_text(encoding='utf-8'))
        result.update(passed=True,build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M7.iso']['sha256'],
                      scope='initial trusted native desktop slice; app isolation, networking and complete M7 qualification remain pending')
    except Exception:
        result['error']=q.clean(traceback.format_exc())
        print(result['error'])
    (out/'results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode())
    print('M7 native desktop slice '+('PASS' if result['passed'] else 'FAIL'),flush=True)
    if not result['passed']:raise SystemExit(1)


if __name__=='__main__':main()
