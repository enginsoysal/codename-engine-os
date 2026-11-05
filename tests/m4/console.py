"""Drive the actual PS/2 console through QMP and inspect its disk and idle state."""
from pathlib import Path
import argparse,json,re,socket,struct,subprocess,sys,time,zlib
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools'))
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/build'))
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'m3'))
from vm import ROOT,arguments,qemu_tools
from data_image import create
from ext2_image import Image as Ext2Image,check,U32,BS
from visual import free_port,connect

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--usb',action='store_true');parser.add_argument('--milestone',choices=('M4','M45'),default='M4');options=parser.parse_args()
    work=ROOT/('build/m4/usb-console-test' if options.usb else 'build/m4/console-test');work.mkdir(parents=True,exist_ok=True)
    if options.milestone=='M45':work=ROOT/('build/m45/hid-console-test' if options.usb else 'build/m45/ps2-console-test');work.mkdir(parents=True,exist_ok=True)
    evidence=ROOT/('evidence/M4' if options.milestone=='M4' else 'evidence/M4.5/hid-console' if options.usb else 'evidence/M4.5/ps2-console');evidence.mkdir(parents=True,exist_ok=True)
    disk=work/'data.img'
    if disk.exists():disk.unlink()
    create(disk)
    args,serial=arguments(qemu_tools(),'iso',ROOT/'dist'/('EngineOS-'+options.milestone+'.iso'),work,ram=128)
    args[args.index('-smp')+1]='4'
    if not options.usb:
        for device in ('usb-kbd,bus=xhci.0','usb-mouse,bus=xhci.0'):
            i=args.index(device);del args[i-1:i+1]
    port=free_port();args+=['-drive',f'if=none,id=data,format=raw,file={disk}','-device','ide-hd,bus=ide.0,drive=data','-qmp',f'tcp:127.0.0.1:{port},server=on,wait=off']
    process=subprocess.Popen(args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    checks=[]
    try:
        with connect(port,process) as sock:
            stream=sock.makefile('rwb');json.loads(stream.readline())
            def qmp(cmd,params=None):
                p={'execute':cmd}
                if params is not None:p['arguments']=params
                stream.write((json.dumps(p)+'\n').encode());stream.flush()
                while True:
                    response=json.loads(stream.readline())
                    if 'error' in response:raise RuntimeError(str(response['error']))
                    if 'return' in response:return response['return']
            def log():return serial.read_text(encoding='utf-8',errors='replace') if serial.exists() else ''
            def wait(marker):
                end=time.monotonic()+60
                while time.monotonic()<end and process.poll() is None:
                    if marker in log():return
                    time.sleep(.05)
                raise RuntimeError('Missing console output: '+marker)
            def keys(text):
                for c in text:
                    code={' ':'spc','\n':'ret','\b':'backspace','/':'slash','.':'dot','-':'minus'}.get(c,c.lower())
                    codes=([{'type':'qcode','data':'shift'}] if c.isupper() else [])+[{'type':'qcode','data':code}]
                    qmp('send-key',{'keys':codes,'hold-time':15});time.sleep(.04)
            qmp('qmp_capabilities');wait('M4_CONSOLE_READY')
            if options.usb:wait('M4_USB_KEYBOARD_ONLINE');wait('M4_USB_MOUSE_ONLINE')
            keys('help\n');wait('ls PATH')
            keys('store HelloX\b\n');wait('Stored and flushed: 0x0000000000000000')
            keys('show\n');wait('Record generation 0x0000000000000001: Hello')
            checks.append({'name':('USB' if options.usb else 'PS/2')+' IRQ command input, Shift, backspace, store/flush/read','passed':True})
            keys('write /Users/test/Documents/console.txt persistent\n');wait('Write status: 0x0000000000000000')
            keys('cat /Users/test/Documents/console.txt\n');wait('\npersistent')
            if options.usb:
                qmp('input-send-event',{'events':[{'type':'rel','data':{'axis':'x','value':40}},{'type':'rel','data':{'axis':'y','value':20}},{'type':'btn','data':{'button':'left','down':True}}]});time.sleep(.1)
                qmp('input-send-event',{'events':[{'type':'btn','data':{'button':'left','down':False}}]});time.sleep(.1)
            keys('info\n');wait('Input drops=0x0000000000000000')
            if options.usb:
                wait('mouse events=');text=log();keys_count=re.search(r'USB keys=(0x[0-9a-f]+)',text);mouse_count=re.search(r'mouse events=(0x[0-9a-f]+)',text)
                if not keys_count or int(keys_count.group(1),16)<10 or not mouse_count or int(mouse_count.group(1),16)<1:raise RuntimeError('USB HID reports did not reach own driver')
                checks.append({'name':'own xHCI MSI keyboard and relative mouse/button reports','passed':True})
            time.sleep(.2);registers=qmp('human-monitor-command',{'command-line':'info registers -a'})
            if registers.count('HLT=1')!=4:raise RuntimeError('All CPUs must halt while waiting for console input')
            prefix='usb-' if options.usb else ''
            (evidence/(prefix+'console-idle-registers.log')).write_bytes(('\n'.join(s.rstrip() for s in registers.splitlines())+'\n').encode())
            checks.append({'name':'all four CPUs observed halted at input idle','passed':True})
            import ctypes
            from ctypes import wintypes as w
            api=ctypes.WinDLL('kernel32',use_last_error=True);api.GetProcessTimes.argtypes=[w.HANDLE]+[ctypes.POINTER(w.FILETIME)]*4;api.GetProcessTimes.restype=w.BOOL
            def cpu_time():
                values=[w.FILETIME() for _ in range(4)]
                if not api.GetProcessTimes(int(process._handle),*[ctypes.byref(v) for v in values]):raise ctypes.WinError(ctypes.get_last_error())
                return sum((v.dwHighDateTime<<32)|v.dwLowDateTime for v in values[2:])/10000000
            before=cpu_time();start=time.monotonic();time.sleep(2);elapsed=time.monotonic()-start;used=cpu_time()-before
            checks.append({'name':'idle QEMU host CPU interval','passed':used/elapsed<=.15,'wall_seconds':elapsed,'cpu_seconds':used,'one_cpu_percent':used/elapsed*100,'scope':'two-second QEMU process sample, not a physical-hardware benchmark'})
            metrics=re.search(r'Managed current bytes=(0x[0-9a-f]+) peak bytes=(0x[0-9a-f]+)',log());boot=re.search(r'M2_BOOT_RESERVED_BYTES=(0x[0-9a-f]+)',log())
            if not metrics or not boot:raise RuntimeError('Missing resource accounting')
            current,peak=map(lambda x:int(x,16),metrics.groups());reserved=int(boot.group(1),16)
            checks.append({'name':'separate managed and boot memory budgets','passed':current<=256*1024 and peak<=512*1024 and reserved<=1024*1024 and reserved+peak<=2*1024*1024,'managed_current_bytes':current,'managed_peak_bytes':peak,'boot_reserved_bytes':reserved})
            ppm=work/'console.ppm';qmp('screendump',{'filename':str(ppm)})
            from PIL import Image
            with Image.open(ppm) as image:image.save(evidence/(prefix+'console-screen.png'))
            qmp('quit');process.wait(timeout=5)
            (evidence/(prefix+'console.log')).write_bytes(('\n'.join(s.rstrip() for s in log().splitlines())+'\n').encode())
    finally:
        if process.poll() is None:process.terminate();process.wait(timeout=5)
    raw=bytearray(disk.read_bytes());check(raw);im=Ext2Image(raw);n=2
    for part in ('Users','test','Documents','console.txt'):n=next(number for name,number,typ in im.entries(n) if name==part)
    ino=im.inode(n)
    if U32(ino,4)!=10 or bytes(im.block(U32(ino,40))[:10])!=b'persistent':raise RuntimeError('Independent ext2 payload verification failed')
    checks.append({'name':'independent ext2 namespace, allocation and console file payload check','passed':True})
    record=raw[512:1024]
    if struct.unpack_from('<Q',record,8)[0]!=1 or struct.unpack_from('<I',record,16)[0]!=5 or record[24:29]!=b'Hello' or struct.unpack_from('<I',record,508)[0]!=zlib.crc32(record[:508]):raise RuntimeError('Independent host record verification failed')
    checks.append({'name':'independent host CRC, generation and exact payload verification','passed':True})
    manifest=json.loads((ROOT/'dist'/(options.milestone.lower()+'-build-manifest.json')).read_text(encoding='utf-8'))
    (evidence/(('usb-' if options.usb else '')+'console-report.json')).write_bytes((json.dumps({'build_id':manifest['build_id'],'checks':checks},indent=2)+'\n').encode())
    if not all(c['passed'] for c in checks):raise SystemExit(1)
    print('Console PASS: PS/2 editing, persistence, framebuffer and 4-CPU HLT')
if __name__=='__main__':main()
