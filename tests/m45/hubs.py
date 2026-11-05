"""Native hub topology: downstream storage/HID reconnect, errors and bounded DMA lifetime."""
from pathlib import Path
import argparse,hashlib,json,re,subprocess,sys,time
sys.path[:0]=[str(Path(__file__).resolve().parents[2]/'tools'),str(Path(__file__).resolve().parents[2]/'tools/build'),str(Path(__file__).resolve().parents[1]/'m3')]
from vm import ROOT,arguments,qemu_tools
from media import fat32,gpt
from visual import free_port,connect

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--nested',action='store_true');options=parser.parse_args();profile='nested' if options.nested else 'direct';mass_port='1.3.1.1.1.1' if options.nested else '1.3'
    chain=['1.3','1.3.1','1.3.1.1','1.3.1.1.1'] if options.nested else []
    work=ROOT/'build/m45/hub-tests'/profile;work.mkdir(parents=True,exist_ok=True);out=ROOT/'evidence/M4.5/hubs'/profile;out.mkdir(parents=True,exist_ok=True);manifest=json.loads((ROOT/'dist/m45-build-manifest.json').read_text(encoding='utf-8'))
    disk=work/'ordinary.img';disk.write_bytes(gpt(fat32([(2,b'HELLO   TXT',b'Original USB BOT payload\n')]),'USB read test'));before=hashlib.sha256(disk.read_bytes()).hexdigest();args,serial=arguments(qemu_tools(),'iso',ROOT/'dist/EngineOS-M45.iso',work,ram=128);args[args.index('-smp')+1]='4'
    for value in ('usb-kbd,bus=xhci.0','usb-mouse,bus=xhci.0'):
        index=args.index(value);del args[index-1:index+1]
    args+=['-device','usb-hub,bus=xhci.0,port=1,id=hub','-device','usb-kbd,bus=xhci.0,port=1.1,id=keyboard','-device','usb-mouse,bus=xhci.0,port=1.2,id=mouse'];
    for n,path in enumerate(chain):args+=['-device',f'usb-hub,bus=xhci.0,port={path},id=hub{n}']
    port=free_port();args+=['-blockdev',json.dumps({'driver':'raw','node-name':'data','file':{'driver':'file','filename':str(disk)}}),'-device',f'usb-storage,bus=xhci.0,port={mass_port},drive=data,id=mass','-qmp',f'tcp:127.0.0.1:{port},server=on,wait=off'];process=subprocess.Popen(args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));checks=[];error=''
    def log():return serial.read_text(encoding='utf-8',errors='replace') if serial.exists() else ''
    try:
        with connect(port,process) as sock:
            stream=sock.makefile('rwb');stream.readline()
            def qmp(command,params=None):
                request={'execute':command}
                if params is not None:request['arguments']=params
                stream.write((json.dumps(request)+'\n').encode());stream.flush()
                while True:
                    response=json.loads(stream.readline())
                    if 'error' in response:raise RuntimeError(str(response['error']))
                    if 'return' in response:return response['return']
            def wait(marker,offset=0):
                deadline=time.monotonic()+30
                while time.monotonic()<deadline and process.poll() is None:
                    text=log()[offset:]
                    if marker in text:return text
                    if 'KERNEL_PANIC' in text or '_FAIL' in text:raise RuntimeError('Kernel fault while waiting for '+marker)
                    time.sleep(.03)
                raise RuntimeError('Missing USB result: '+marker)
            def keys(text):
                for c in text:
                    code={' ':'spc','\n':'ret','/':'slash','.':'dot','-':'minus'}.get(c,c.lower());qmp('send-key',{'keys':[{'type':'qcode','data':code}],'hold-time':10});time.sleep(.025)
            def command(text,marker):
                offset=len(log());keys(text+'\n');wait(marker,offset);return wait('engine> ',offset)
            def metrics():
                text=command('info','Managed current bytes=');m=re.search(r'Managed current bytes=(0x[0-9a-f]+) peak bytes=(0x[0-9a-f]+)',text)
                if not m:raise RuntimeError('Missing USB memory accounting')
                return int(m.group(1),16),int(m.group(2),16)
            qmp('qmp_capabilities');wait('M4_CONSOLE_READY');wait('M45_USB_HUB_ONLINE');wait('Disk 0 USB');command('fatcat 0 0 /hello.txt','Original USB BOT payload');baseline,peak=metrics();checks.append({'name':'boot-attached USB BOT plus FAT32 file read','passed':True})
            for cycle in range(1,4):
                offset=len(log());qmp('device_del',{'id':'mass'});wait('M45_USB_HUB_CHILD_DISCONNECTED',offset);released,_=metrics()
                if released!=baseline-7*4096:raise RuntimeError('USB disconnect did not reclaim exactly seven owned pages')
                stale=command('fatcat 0 0 /hello.txt','FAT status: 2')
                if 'Original USB BOT payload' in stale:raise RuntimeError('Stale disk reached payload')
                offset=len(log());qmp('device_add',{'driver':'usb-storage','bus':'xhci.0','port':mass_port,'drive':'data','id':'mass'});wait('M45_USB_MASS_STORAGE_READY',offset);command(f'fatcat {cycle} 0 /hello.txt','Original USB BOT payload');retained,peak=metrics()
                if retained!=baseline:raise RuntimeError('USB reconnect retained memory grew')
                checks.append({'name':f'USB mass disconnect/reconnect {cycle}: stale disk denied, new generation readable, DMA reclaimed','passed':True,'retained_bytes':retained,'disconnected_bytes':released})
            fault_disk=work/'fault.img';fault_disk.write_bytes(disk.read_bytes());fault_before=hashlib.sha256(fault_disk.read_bytes()).hexdigest();offset=len(log());qmp('device_del',{'id':'mass'});wait('M45_USB_HUB_CHILD_DISCONNECTED',offset)
            qmp('blockdev-add',{'driver':'raw','node-name':'fault','file':{'driver':'blkdebug','image':{'driver':'file','filename':str(fault_disk)},'inject-error':[{'event':'read_aio','iotype':'read','errno':5,'sector':0,'once':False}]}});offset=len(log());qmp('device_add',{'driver':'usb-storage','bus':'xhci.0','port':mass_port,'drive':'fault','id':'badmass'});wait('M45_USB_MASS_STORAGE_READY',offset);text=command('disks','Disk 4 USB')
            if 'read error 3' not in text or hashlib.sha256(fault_disk.read_bytes()).hexdigest()!=fault_before:raise RuntimeError('USB read EIO was not contained/read-only')
            offset=len(log());qmp('device_del',{'id':'badmass'});wait('M45_USB_HUB_CHILD_DISCONNECTED',offset);qmp('device_add',{'driver':'usb-storage','bus':'xhci.0','port':mass_port,'drive':'data','id':'mass'});wait('M45_USB_MASS_STORAGE_READY',offset);command('fatcat 5 0 /hello.txt','Original USB BOT payload');retained,peak=metrics()
            if retained!=baseline:raise RuntimeError('USB read fault recovery leaked DMA pages')
            checks.append({'name':'hot-attached USB sector-zero EIO contained; failed device removed and healthy device reads again','passed':True,'fault_backing_unchanged':True})
            offset=len(log());qmp('device_del',{'id':'keyboard'});wait('M45_USB_HUB_CHILD_DISCONNECTED',offset);qmp('device_add',{'driver':'usb-kbd','bus':'xhci.0','port':'1.1','id':'keyboard'});wait('M4_USB_KEYBOARD_ONLINE',offset);text=command('help','fatcat N P PATH');retained,peak=metrics()
            if retained!=baseline:raise RuntimeError('HID reconnect retained memory grew')
            checks.append({'name':'USB keyboard disconnect/reconnect then actual command input','passed':True})
            qmp('input-send-event',{'events':[{'type':'rel','data':{'axis':'x','value':25}},{'type':'rel','data':{'axis':'y','value':15}}]});text=command('info','mouse events=');match=re.search(r'mouse events=(0x[0-9a-f]+)',text)
            if not match or int(match.group(1),16)==0:raise RuntimeError('Mouse reports lost during reconnect')
            offset=len(log());qmp('device_del',{'id':'hub'});wait('M45_USB_DISCONNECTED port=',offset);released,_=metrics()
            if released!=baseline-(22+5*len(chain))*4096:raise RuntimeError('Whole hub tree did not reclaim all device pages')
            offset=len(log());qmp('device_add',{'driver':'usb-hub','bus':'xhci.0','port':'1','id':'hub'});qmp('device_add',{'driver':'usb-kbd','bus':'xhci.0','port':'1.1','id':'keyboard'});qmp('device_add',{'driver':'usb-mouse','bus':'xhci.0','port':'1.2','id':'mouse'});
            for n,path in enumerate(chain):qmp('device_add',{'driver':'usb-hub','bus':'xhci.0','port':path,'id':f'hub{n}'})
            qmp('device_add',{'driver':'usb-storage','bus':'xhci.0','port':mass_port,'drive':'data','id':'mass'});wait('M45_USB_MASS_STORAGE_READY',offset);command('fatcat 6 0 /hello.txt','Original USB BOT payload');retained,peak=metrics()
            if retained!=baseline:raise RuntimeError('Whole hub tree reconnect retained memory grew')
            checks.append({'name':'populated hub unplug retires children before parent; reconnect restores native reads and exact memory','passed':True,'after_tree_disable_bytes':released,'after_tree_reconnect_bytes':retained})
            time.sleep(.2);registers=qmp('human-monitor-command',{'command-line':'info registers -a'});halted=registers.count('HLT=1')==4;checks.append({'name':'mouse remains active; all four CPUs halt at console idle','passed':halted});(out/'idle-registers.log').write_bytes(('\n'.join(line.rstrip() for line in registers.splitlines())+'\n').encode());ppm=work/'screen.ppm';qmp('screendump',{'filename':str(ppm)});from PIL import Image
            with Image.open(ppm) as frame:frame.save(out/'console.png')
            import ctypes
            from ctypes import wintypes as w
            api=ctypes.WinDLL('kernel32',use_last_error=True);api.GetProcessTimes.argtypes=[w.HANDLE]+[ctypes.POINTER(w.FILETIME)]*4;api.GetProcessTimes.restype=w.BOOL
            def cpu_time():
                values=[w.FILETIME() for _ in range(4)]
                if not api.GetProcessTimes(int(process._handle),*[ctypes.byref(v) for v in values]):raise ctypes.WinError(ctypes.get_last_error())
                return sum((v.dwHighDateTime<<32)|v.dwLowDateTime for v in values[2:])/10000000
            initial=cpu_time();start=time.monotonic();time.sleep(2);elapsed=time.monotonic()-start;used=cpu_time()-initial
            checks.append({'name':'idle QEMU host process sample after reconnect','passed':used/elapsed<=.15,'wall_seconds':elapsed,'cpu_seconds':used,'one_cpu_percent':used/elapsed*100,'scope':'two-second emulator sample, not physical CPU measurement'})
            checks.append({'name':'bounded native managed memory after repeated reconnect','passed':baseline<=(400 if options.nested else 320)*1024 and peak<=512*1024,'retained_bytes':baseline,'peak_bytes':peak});qmp('quit');process.wait(timeout=5)
    except Exception as exception:error=str(exception).replace(str(ROOT),'<project-root>')
    finally:
        if process.poll() is None:process.terminate();process.wait(timeout=5)
        unchanged=hashlib.sha256(disk.read_bytes()).hexdigest()==before;checks.append({'name':'ordinary USB backing hash unchanged','passed':unchanged});(out/'console.log').write_bytes(('\n'.join(line.rstrip() for line in log().splitlines())+'\n').encode());(out/'results.json').write_bytes((json.dumps({'build_id':manifest['build_id'],'full_m45_complete':False,'checks':checks,'error':error},indent=2)+'\n').encode())
    print(json.dumps({'checks':checks,'error':error},indent=2));raise SystemExit(0 if not error and len(checks)==11 and all(c['passed'] for c in checks) else 1)
if __name__=='__main__':main()
