"""A rejected final USB port must not start an input ring on its freed context."""
from pathlib import Path
import argparse,json,subprocess,sys,time
sys.path[:0]=[str(Path(__file__).resolve().parents[2]/'tools'),str(Path(__file__).resolve().parents[1]/'m3')]
from vm import ROOT,arguments,qemu_tools
from visual import free_port,connect

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--hub',action='store_true');options=parser.parse_args();profile='hub' if options.hub else 'root'
    out=ROOT/'evidence/M4.5/usb-reject'/profile;out.mkdir(parents=True,exist_ok=True)
    manifest=json.loads((ROOT/'dist/m45-build-manifest.json').read_text(encoding='utf-8'))
    args,serial=arguments(qemu_tools(),'iso',ROOT/'dist/EngineOS-M45.iso',ROOT/'build/m45/usb-reject'/profile,ram=128)
    if options.hub:args+=['-device','usb-hub,bus=xhci.0,port=3']
    location='3.1' if options.hub else '3'
    marker='M45_USB_HUB_CHILD_UNSUPPORTED' if options.hub else 'M4_USB_PORT_UNSUPPORTED port='
    port=free_port();args+=['-audiodev','none,id=silent','-device',f'usb-audio,bus=xhci.0,port={location},audiodev=silent','-qmp',f'tcp:127.0.0.1:{port},server=on,wait=off']
    process=subprocess.Popen(args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));checks=[];error=''
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
            def wait(marker):
                end=time.monotonic()+45
                while time.monotonic()<end and process.poll() is None:
                    text=log()
                    if 'KERNEL_PANIC' in text:raise RuntimeError('Kernel fault after rejected USB interface')
                    if marker in text:return
                    time.sleep(.03)
                raise RuntimeError('Missing result: '+marker)
            qmp('qmp_capabilities');wait('M4_CONSOLE_READY');wait(marker)
            checks.append({'name':f'unsupported {profile} port refused; kernel reaches console','passed':True})
            for code in ('h','e','l','p','ret'):
                qmp('send-key',{'keys':[{'type':'qcode','data':code}],'hold-time':10});time.sleep(.04)
            wait('fatcat N P PATH');checks.append({'name':'healthy USB keyboard remains usable after failed enumeration','passed':True})
            time.sleep(1)
            if log().count('M45_USB_ATTACH_UNSUPPORTED')>1 or log().count('M45_USB_HUB_CHILD_UNSUPPORTED')>1:raise RuntimeError('Unsupported device entered an enumeration retry storm')
            registers=qmp('human-monitor-command',{'command-line':'info registers -a'})
            checks.append({'name':'both CPUs halt after command','passed':registers.count('HLT=1')==2});qmp('quit');process.wait(timeout=5)
    except Exception as exception:error=str(exception).replace(str(ROOT),'<project-root>')
    finally:
        if process.poll() is None:process.terminate();process.wait(timeout=5)
        (out/'console.log').write_bytes(('\n'.join(line.rstrip() for line in log().splitlines())+'\n').encode())
        result={'build_id':manifest['build_id'],'checks':checks,'error':error,'full_m45_complete':False};(out/'results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print(json.dumps(result,indent=2))
    raise SystemExit(0 if not error and len(checks)==3 and all(c['passed'] for c in checks) else 1)
if __name__=='__main__':main()
