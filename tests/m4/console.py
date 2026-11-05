"""Drive the actual PS/2 console through QMP and inspect its disk and idle state."""
from pathlib import Path
import json,socket,struct,subprocess,sys,time,zlib
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools'))
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/build'))
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'m3'))
from vm import ROOT,arguments,qemu_tools
from data_image import create
from visual import free_port,connect

def main():
    work=ROOT/'build/m4/console-test';work.mkdir(parents=True,exist_ok=True)
    evidence=ROOT/'evidence/M4';evidence.mkdir(parents=True,exist_ok=True)
    disk=work/'data.img'
    if disk.exists():disk.unlink()
    create(disk)
    args,serial=arguments(qemu_tools(),'iso',ROOT/'dist/EngineOS-M4.iso',work,ram=128)
    args[args.index('-smp')+1]='4'
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
                    code={' ':'spc','\n':'ret','\b':'backspace'}.get(c,c.lower())
                    codes=([{'type':'qcode','data':'shift'}] if c.isupper() else [])+[{'type':'qcode','data':code}]
                    qmp('send-key',{'keys':codes,'hold-time':15});time.sleep(.04)
            qmp('qmp_capabilities');wait('M4_CONSOLE_READY')
            keys('help\n');wait('Writes affect only sector 1')
            keys('store HelloX\b\n');wait('Stored and flushed: 0x0000000000000000')
            keys('show\n');wait('Record generation 0x0000000000000001: Hello')
            checks.append({'name':'PS/2 IRQ command input, Shift, backspace, store/flush/read','passed':True})
            keys('info\n');wait('Input drops=0x0000000000000000')
            time.sleep(.2);registers=qmp('human-monitor-command',{'command-line':'info registers -a'})
            if registers.count('HLT=1')!=4:raise RuntimeError('All CPUs must halt while waiting for console input')
            (evidence/'console-idle-registers.log').write_bytes(('\n'.join(s.rstrip() for s in registers.splitlines())+'\n').encode())
            checks.append({'name':'all four CPUs observed halted at input idle','passed':True})
            ppm=work/'console.ppm';qmp('screendump',{'filename':str(ppm)})
            from PIL import Image
            with Image.open(ppm) as image:image.save(evidence/'console-screen.png')
            qmp('quit');process.wait(timeout=5)
            (evidence/'console.log').write_bytes(('\n'.join(s.rstrip() for s in log().splitlines())+'\n').encode())
    finally:
        if process.poll() is None:process.terminate();process.wait(timeout=5)
    record=disk.read_bytes()[512:1024]
    if struct.unpack_from('<Q',record,8)[0]!=1 or struct.unpack_from('<I',record,16)[0]!=5 or record[24:29]!=b'Hello' or struct.unpack_from('<I',record,508)[0]!=zlib.crc32(record[:508]):raise RuntimeError('Independent host record verification failed')
    checks.append({'name':'independent host CRC, generation and exact payload verification','passed':True})
    manifest=json.loads((ROOT/'dist/m4-build-manifest.json').read_text(encoding='utf-8'))
    (evidence/'console-report.json').write_bytes((json.dumps({'build_id':manifest['build_id'],'checks':checks},indent=2)+'\n').encode())
    print('Console PASS: PS/2 editing, persistence, framebuffer and 4-CPU HLT')
if __name__=='__main__':main()
