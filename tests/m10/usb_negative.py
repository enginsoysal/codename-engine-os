"""Real controller failures/refusals on owned disposable USB backing files."""
import hashlib,json,time,traceback,struct
import ui
from journey import wizard,shot,native_child
from media import fat32
ROOT=ui.ROOT
def main():
    out=ROOT/'build/m10/usb-negative';out.mkdir(parents=True,exist_ok=True)
    work=ui.q.new_work('m10-usb-negative');disk=ui.q.sparse_disk(work/'owned.img',2*1024**3)
    payload=bytes(fat32([(2,b'HELLO   TXT',b'Original negative-test data\n')]))
    ro=work/'readonly.img';ro.write_bytes(payload)
    bad=work/'error.img';bad.write_bytes(payload)
    malformed=work/'malformed.img';damaged=bytearray(payload);damaged[510:512]=bytes(2);malformed.write_bytes(damaged)
    hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [ro,bad,malformed]}
    node={'driver':'raw','node-name':'readonly','read-only':True,'file':{'driver':'file','filename':str(ro)}}
    extra=['-blockdev',json.dumps(node),'-device','usb-storage,id=readonly,bus=xhci.0,drive=readonly']
    meta=json.loads((ROOT/'dist/m10-build-manifest.json').read_text(encoding='utf-8'))
    report={'passed':False,'build_id':meta['build_id'],'iso_sha256':meta['artifacts']['EngacerOS-M10.iso']['sha256'],'checks':[]}
    try:
        with ui.guest(work/'install',disk,extra=extra) as g:
            assert 'M10_USB_READ_ONLY' in g.log();p=ui.Pointer(g)
            for action in (2,100,2,2,2):wizard(g,p,action)
            start=wizard(g,p,2);g.wait('M6_INSTALL_PASS',start,240);ui.wait_view(g,6,start)
            start=wizard(g,p,84);g.wait('M7_DESKTOP_READY',start,180);p=ui.Pointer(g)
            p.desktop(95,True);p.desktop(18000+native_child(g,'/Users/Owner/Downloads','ArchiveSample.zip'),True)
            p.desktop(21101,True);p.desktop(21200,True);start=len(g.log());p.desktop(21005,True)
            assert 'M10_USB_FILE_EXPORTED' not in g.log()[start:];shot(g,out,'read-only-copy-refused')
            report['checks'].append('Actual USB MODE SENSE write protection: native export refused with unchanged backing bytes')
            p.desktop(21006,True);g.wait('M10_USB_SAFE_TO_REMOVE');g.qmp('device_del',{'id':'readonly'})
            fault={'driver':'raw','node-name':'fault','file':{'driver':'blkdebug','image':{'driver':'file','filename':str(bad)},'inject-error':[{'event':'read_aio','iotype':'read','errno':5,'sector':0,'once':False}]}}
            g.qmp('blockdev-add',fault);start=len(g.log());g.qmp('device_add',{'driver':'usb-storage','id':'fault','bus':'xhci.0','drive':'fault'})
            g.wait('M45_USB_MASS_STORAGE_READY status=0x0000000000000000',start)
            # Registration can succeed while its required first read fails.
            # Actual registry capability, rather than a registration log, wins.
            raw=ui.memory(g,ui.symbol('disks',640,True)[0]+40,40)
            assert struct.unpack_from('<I',raw,16)[0]==0 and struct.unpack_from('<I',raw,36)[0]==3
            shot(g,out,'sector-zero-error-contained')
            report['checks'].append('Actual USB sector-zero I/O error contained without exposing a readable drive or successful copy')
            start=len(g.log());g.qmp('device_del',{'id':'fault'});g.wait('M45_USB_DISCONNECTED',start)
            g.qmp('blockdev-add',{'driver':'raw','node-name':'malformed','file':{'driver':'file','filename':str(malformed)}})
            start=len(g.log());g.qmp('device_add',{'driver':'usb-storage','id':'malformed','bus':'xhci.0','drive':'malformed'});g.wait('M10_USB_WRITABLE',start)
            p.desktop(21101,True);shot(g,out,'malformed-volume-refused')
            assert 'M10_USB_FILE_EXPORTED' not in g.log()
            report['checks'].append('Reconnection after failure rebinds; malformed FAT boot metadata is refused by native Files')
            # The guest is still responsive through its ordinary launcher.
            ui.h.key(g,'f10');time.sleep(.2);shot(g,out,'responsive-after-errors')
            (out/'serial.log').write_text(g.log(),encoding='utf-8')
        for path in [ro,bad,malformed]:assert hashlib.sha256(path.read_bytes()).hexdigest()==hashes[path.name]
        report['unchanged_backing_sha256']=hashes;report['passed']=True
    except Exception:
        report['error']=ui.q.clean(traceback.format_exc());raise
    finally:
        if 'g' in locals():(out/'serial.log').write_text(g.log(),encoding='utf-8')
        (out/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
if __name__=='__main__':main()
