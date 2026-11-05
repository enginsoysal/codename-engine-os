"""Observe persistent normal kernel boot; capture the real framebuffer over QMP."""
from pathlib import Path
import hashlib
import json
import re
import socket
import subprocess
import sys
import time
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools'))
from vm import ROOT,arguments,qemu_tools

def free_port():
    with socket.socket() as s:s.bind(('127.0.0.1',0));return s.getsockname()[1]
def connect(port,process):
    deadline=time.monotonic()+15
    while time.monotonic()<deadline and process.poll() is None:
        try:return socket.create_connection(('127.0.0.1',port),timeout=2)
        except OSError:time.sleep(.1)
    raise RuntimeError('QEMU endpoint did not become reachable')
def main():
    files=qemu_tools();work=ROOT/'build/m1/visual';evidence=ROOT/'evidence/M1'
    image=ROOT/'dist/EngineOS-M1.iso'
    args,serial=arguments(files,'iso',image,work)
    port=free_port();args+=['-qmp',f'tcp:127.0.0.1:{port},server=on,wait=off']
    checks=[]
    process=subprocess.Popen(args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    try:
        with connect(port,process) as sock:
            stream=sock.makefile('rwb');json.loads(stream.readline())
            def qmp(command,params=None):
                payload={'execute':command}
                if params is not None:payload['arguments']=params
                stream.write((json.dumps(payload)+'\n').encode());stream.flush()
                while True:
                    response=json.loads(stream.readline())
                    if 'error' in response:raise RuntimeError(str(response['error']))
                    if 'return' in response:return response['return']
            qmp('qmp_capabilities')
            deadline=time.monotonic()+45
            while time.monotonic()<deadline and process.poll() is None:
                log=serial.read_text(encoding='utf-8',errors='replace') if serial.exists() else ''
                if 'ENGINE_OS_KERNEL_ONLINE' in log:break
                time.sleep(.1)
            else:raise RuntimeError('Normal kernel never became online')
            time.sleep(.4)
            status=qmp('query-status')
            if status.get('status')!='running' or process.poll() is not None:raise RuntimeError('Normal kernel is not persistent')
            checks.append({'name':'normal ISO remains online without debug-exit','passed':True})
            ppm=work/'screen.ppm';qmp('screendump',{'filename':str(ppm)})
            from PIL import Image,__version__ as pillow_version
            with Image.open(ppm) as im:
                rgb=im.convert('RGB')
                if rgb.width<640 or rgb.height<400:raise RuntimeError('Unexpected framebuffer size')
                # Verify the boot background/accent are present; visual review checks text.
                if rgb.getpixel((10,2))!=(57,213,187) or rgb.getpixel((10,20))!=(16,29,43):raise RuntimeError('Own framebuffer colors absent')
                rgb.save(evidence/'boot-screen.png')
                checks.append({'name':'own kernel framebuffer captured','passed':True,'width':rgb.width,'height':rgb.height,
                               'conversion_tool':f'Pillow {pillow_version}'})
            qmp('quit');process.wait(timeout=5)
            log=re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]','',log)
            (evidence/'normal-iso.log').write_text(log,encoding='utf-8')
    finally:
        if process.poll() is None:process.terminate();process.wait(timeout=5)
    # A separate paused launch verifies the actual loopback GDB stub handshake.
    args,_=arguments(files,'iso',image,ROOT/'build/m1/gdb-check')
    port=free_port();args+=['-gdb',f'tcp:127.0.0.1:{port}','-S']
    process=subprocess.Popen(args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    try:
        with connect(port,process) as sock:
            payload=b'qSupported';sock.sendall(b'$'+payload+b'#'+f'{sum(payload)%256:02x}'.encode())
            response=b'';deadline=time.monotonic()+5
            while b'PacketSize=' not in response and time.monotonic()<deadline:response+=sock.recv(4096)
            if b'PacketSize=' not in response:raise RuntimeError('GDB RSP capability response missing')
            checks.append({'name':'loopback GDB RSP handshake','passed':True})
    finally:
        if process.poll() is None:process.terminate();process.wait(timeout=5)
    result={'passed':True,'checks':checks,'screenshot_sha256':hashlib.sha256((evidence/'boot-screen.png').read_bytes()).hexdigest()}
    (evidence/'visual-results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result,indent=2))
if __name__=='__main__':main()
