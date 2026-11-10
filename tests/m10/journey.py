"""M10 native installation and removable-media journey on disposable images."""
import argparse,hashlib,json,struct,time,traceback,sys
import ui
ROOT=ui.ROOT
sys.path.insert(0,str(ROOT/'tools/build'))
from media import fat32

def shot(g,out,name,installed=True):
    deadline=time.monotonic()+60
    while g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')!=len(g.qmp('query-cpus-fast')):
        if time.monotonic()>deadline:raise TimeoutError('Native frame did not settle')
        time.sleep(.025)
    g.qmp('stop')
    try:ui.raw_shot(g,out,name,installed)
    finally:g.qmp('cont')

def wizard(g,p,action):
    count=ui.integer(g,'controls');raw=ui.memory(g,ui.symbol('buttons',480)[0],count*20)
    matches=[struct.unpack_from('<5I',raw,i*20) for i in range(count) if struct.unpack_from('<I',raw,i*20+16)[0]==action]
    assert len(matches)==1,(action,matches)
    x,y,w,h,_=matches[0];start=len(g.log());p.logical(x+w//2,y+h//2)
    if action not in (3,30,84):g.wait('M73_VIEW step=',start)
    return start

def native_child(g,folder,wanted):
    base=ui.symbol('volume',0xd3040,True)[0]+64;index=0
    for number in range(1024):
        row=ui.memory(g,base+number*280,280)
        if not struct.unpack_from('<I',row,276)[0]:continue
        path=row[:256].split(b'\0')[0].decode();prefix=folder+'/'
        if not path.startswith(prefix) or '/' in path[len(prefix):]:continue
        if path==prefix+wanted:return index
        index+=1
    raise AssertionError('Native child is missing: '+wanted)

def wait_launcher(g,value):
    deadline=time.monotonic()+60
    while ui.memory(g,ui.symbol('launcher',installed=True)[0],1)!=bytes([value]):
        if time.monotonic()>deadline:raise TimeoutError('Keyboard launcher state did not change')
        time.sleep(.025)

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--firmware',choices=['uefi','bios'],default='uefi');parser.add_argument('--resolution',default='1280x800');parser.add_argument('--ram',type=int,default=128);parser.add_argument('--hub',action='store_true');a=parser.parse_args()
    resolution=tuple(map(int,a.resolution.split('x')))
    out=ROOT/'build/m10/journey'/(a.firmware+'-'+a.resolution+'-'+str(a.ram)+('-hub' if a.hub else ''));out.mkdir(parents=True,exist_ok=True)
    work=ui.q.new_work('m10-devices');disk=ui.q.sparse_disk(work/'owned.img',2*1024**3)
    usb=work/'owned-usb.img';payload=b'Engacer OS removable media round trip.\n';usb.write_bytes(fat32([(2,b'HELLO   TXT',payload)]))
    extra=['-drive',f'if=none,id=removable,format=raw,file={usb}','-device','usb-storage,id=removable-device,bus=xhci.0,drive=removable']
    if a.hub:extra=['-device','usb-hub,id=hub,bus=xhci.0,port=3']+[v.replace('id=removable-device,bus=xhci.0,','id=removable-device,bus=xhci.0,port=3.1,') for v in extra]
    meta=json.loads((ROOT/'dist/m10-build-manifest.json').read_text(encoding='utf-8'));iso=ROOT/'dist/EngacerOS-M10.iso';digest=hashlib.sha256(iso.read_bytes()).hexdigest()
    report={'passed':False,'build_id':meta['build_id'],'iso_sha256':digest,'checks':[],'firmware':a.firmware,'resolution':resolution,'ram_mib':a.ram}
    try:
        candidate=ui.guest(work/'install',disk,resolution,a.firmware,ram=a.ram,extra=extra)
        candidate.args=[v.replace('usb-kbd,bus=xhci.0','usb-kbd,id=keyboard,bus=xhci.0') for v in candidate.args]
        with candidate as g:
            p=ui.Pointer(g);shot(g,out,'welcome',False)
            if a.firmware=='uefi':assert 'M10_KERNEL_STREAM_LOADED whole_file_copy_bytes=0' in g.log()
            for action in (2,100,2,2,2):wizard(g,p,action)
            start=wizard(g,p,2);g.wait('M6_INSTALL_PASS',start,240);ui.wait_view(g,6,start)
            start=wizard(g,p,84);g.wait('M7_DESKTOP_READY',start,180)
            assert 'M6_INSTALLED_VOLUME' in g.log()[start:]
            p=ui.Pointer(g);shot(g,out,'installed-desktop')
            report['checks'].append('Graphical installation and installed desktop startup with retained ISO')
            start=len(g.log());g.qmp('device_del',{'id':'keyboard'});g.wait('M45_USB_DISCONNECTED',start)
            start=len(g.log());g.qmp('device_add',{'driver':'usb-kbd','id':'keyboard-new','bus':'xhci.0'});g.wait('M4_USB_KEYBOARD_ONLINE',start)
            ui.h.key(g,'f10');wait_launcher(g,1)
            shot(g,out,'keyboard-reconnected-launcher');ui.h.key(g,'esc');wait_launcher(g,0)
            report['checks'].append('USB keyboard reconnect followed by real keyboard launcher open/close')
            p.desktop(0,True);p.desktop(3,True);p.desktop(20000,True);shot(g,out,'devices')
            assert ui.integer(g,'layout10',True)==0
            p.desktop(20006,True);assert ui.integer(g,'layout10',True)==1
            p.close_app(3,True);p.desktop(94,True);p.desktop(26,True)
            for key in ('a','b','c','ret'):ui.h.key(g,key)
            g.wait('M72_FOLDER_CREATED /Users/Owner/Documents/axj')
            p.desktop(0,True);p.desktop(3,True);p.desktop(20000,True)
            p.desktop(20005,True);assert ui.integer(g,'layout10',True)==0
            report['checks'].append('Native Settings Devices, saved layouts and real Dvorak typing into a persisted folder name')
            p.close_app(3,True)
            p.desktop(21000,True);shot(g,out,'usb-drives')
            # The SATA installation target occupies registry slot zero.
            p.desktop(21101,True);shot(g,out,'usb-files')
            offset_at=ui.symbol('apps',installed=True)[0]+8588
            for name,expected in [('wheel-down',1),('wheel-up',0)]:
                g.qmp('input-send-event',{'events':[{'type':'btn','data':{'down':True,'button':name}},{'type':'btn','data':{'down':False,'button':name}}]})
                deadline=time.monotonic()+60
                while struct.unpack('<I',ui.memory(g,offset_at,4))[0]!=expected:
                    if time.monotonic()>deadline:raise TimeoutError('Native wheel did not scroll Files')
                    time.sleep(.025)
            report['checks'].append('Real mouse wheel scrolls the focused native USB folder down and back up')
            p.desktop(21201,True);p.desktop(21004,True);g.wait('M10_USB_FILE_IMPORTED')
            shot(g,out,'usb-imported')
            report['checks'].append('Native USB Files selection and copy to Downloads')
            p.desktop(21001,True);p.desktop(95,True)
            p.desktop(18000+native_child(g,'/Users/Owner/Downloads','HELLO.TXT'),True);p.desktop(21101,True);p.desktop(21200,True);p.desktop(21005,True)
            g.wait('M10_USB_FILE_EXPORTED');shot(g,out,'usb-exported')
            report['checks'].append('Native Downloads To USB and flushed create-only export')
            pages_before=struct.unpack('<Q',ui.memory(g,ui.symbol('stats',32,True)[0],8))[0]
            p.desktop(21006,True);g.wait('M10_USB_SAFE_TO_REMOVE');shot(g,out,'usb-safe-removal')
            report['checks'].append('Controller flush and native safe-removal action')
            g.qmp('device_del',{'id':'removable-device'})
            g.qmp('blockdev-add',{'node-name':'removable-again','driver':'raw','file':{'driver':'file','filename':str(usb)}})
            start=len(g.log());g.qmp('device_add',{'driver':'usb-storage','id':'removable-reconnected','bus':'xhci.0','drive':'removable-again'})
            g.wait('M10_USB_WRITABLE',start);p.desktop(21101,True);shot(g,out,'usb-reconnected')
            pages_after=struct.unpack('<Q',ui.memory(g,ui.symbol('stats',32,True)[0],8))[0]
            assert pages_after==pages_before,'USB reconnect retained managed pages'
            report['managed_bytes_before_reconnect']=pages_before*4096;report['managed_bytes_after_reconnect']=pages_after*4096
            report['checks'].append('Actual USB device detach/reconnect rebinds automatically and reuses the registry slot')
            (out/'serial.log').write_text(g.log(),encoding='utf-8')
        assert ui.s.s.document(disk,'/Users/Owner/Downloads/HELLO.TXT')==payload
        report['checks'].append('Independent installed-volume inspection validates copied USB bytes')
        sys.path.insert(0,str(ROOT/'tools/local/m10-python'))
        from pyfatfs.PyFatFS import PyFatFS
        with PyFatFS(str(usb),read_only=True,preserve_case=True) as independent:assert independent.readbytes('/EFI/HELLO.TXT')==payload and independent.readbytes('/HELLO.TXT')==payload
        report['checks'].append('Independent FAT implementation verifies the graphical round trip and unchanged original')
        for number in (1,2):
            live=ui.guest(work/('independent-'+str(number)),disk,resolution,a.firmware,media='installed',ram=a.ram,ready='M7_DESKTOP_READY')
            ui.s.normal_boot(live)
            with live as g:
                assert 'M6_INSTALLED_VOLUME' in g.log() and ui.integer(g,'m6_bundle_size',True)==0
                assert ui.integer(g,'layout10',True)==0
                shot(g,out,'independent-disk-boot-'+str(number))
        report['checks'].append('Two ordinary independent disk boots without ISO, persistent layout and installed desktop')
        report['passed']=True
    except Exception:
        report['error']=traceback.format_exc().replace(str(ROOT),'<project-root>');raise
    finally:
        assert hashlib.sha256(iso.read_bytes()).hexdigest()==digest
        if 'g' in locals():(out/'serial.log').write_text(g.log(),encoding='utf-8')
        (out/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')

if __name__=='__main__':main()
