"""Exercise actual native certificate, UTC, cancellation and cable-loss failures."""
import importlib.util
import json
from pathlib import Path
import struct
import time
import traceback
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('m7https',Path(__file__).with_name('https.py'))
t=importlib.util.module_from_spec(spec);spec.loader.exec_module(t)
s,i,h,q=t.s,t.i,t.h,t.q


def read_info(guest):return i.memory(guest,t.download_address(),288)


def state(guest):return struct.unpack_from('<I',read_info(guest))[0]


def type_url(guest,url):
    i.control(guest,'l')
    slot,_=i.window(guest,5);base,size=i.symbol('apps');address=base+slot*(size//8)
    mapping={':':'semicolon','/':'slash','.':'dot','-':'minus','_':'minus','?':'slash','=':'equal','&':'7'}
    prefix=''
    for char in url:
        keys=([{'type':'qcode','data':'shift'}] if char.isupper() or char in ':_?&' else [])+[{'type':'qcode','data':mapping.get(char,char.lower())}]
        guest.qmp('send-key',{'keys':keys,'hold-time':60});time.sleep(.09)
        prefix+=char;deadline=time.monotonic()+2
        while time.monotonic()<deadline:
            actual=i.memory(guest,address,512).split(b'\0')[0]
            if actual==prefix.encode():break
            time.sleep(.03)
        assert actual==prefix.encode(),('Native URL input differs from intended request',actual,prefix)


def expect(guest,wanted,timeout=60):
    deadline=time.monotonic()+timeout
    while time.monotonic()<deadline:
        raw=read_info(guest);current=struct.unpack_from('<I',raw)[0]
        if current==wanted:return raw
        if current in (4,5,6):
            message=i.memory(guest,struct.unpack_from('<Q',raw,24)[0],160).split(b'\0')[0].decode()
            h.shot(guest,ROOT/'evidence/M7/https-negative','unexpected-terminal-state')
            mapping=(ROOT/'dist/EngineOS-M7-installed-kernel.map').read_text(encoding='utf-8').split('kernel_network_download-kernel.o:(.bss)',1)[1]
            import re
            def address(name):return int(re.search(r'^\s*([0-9a-f]+)\s+[0-9a-f]+\s+[0-9a-f]+\s+\d+\s+'+name+r'$',mapping,re.M)[1],16)
            response=struct.unpack('<II',i.memory(guest,address('response'),8))
            request=i.memory(guest,address('request'),1024).split(b'\0')[0].decode()
            message+=' HTTP state/status '+str(response)+' request '+request
            raise AssertionError('Unexpected terminal download state '+str(current)+' error '+str(struct.unpack_from('<I',raw,8)[0])+' message '+message)
        time.sleep(.05)
    raise AssertionError('Download did not reach state '+str(wanted)+'; actual '+str(current))


def main():
    work=q.new_work('m7-https-negative');out=ROOT/'evidence/M7/https-negative';out.mkdir(parents=True,exist_ok=True)
    report={'passed':False,'m7_complete':False,'cases':[]}
    try:
        disk=q.sparse_disk(work/'owned.img',2*1024**3)
        with q.Guest(work/'install',disk,image_name='EngineOS-M7') as guest:
            h.next(guest);h.next(guest);h.next(guest,2);h.next(guest,3);h.next(guest,5);start=len(guest.log());h.next(guest,1);guest.wait('M6_INSTALL_PASS',start,240)
        extra=['-cpu','max,x2apic=off','-rtc','base=utc,clock=host','-netdev','user,id=engine-net','-device','e1000,id=engine-net-card,netdev=engine-net,mac=52:54:00:12:34:56','-object','filter-buffer,id=engine-delay,netdev=engine-net,interval=200000,status=off']
        with q.Guest(work/'desktop',disk,media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY',extra=extra) as guest:
            guest.wait('M7_DHCP_CONFIGURED',timeout=60);t.n.wait_state(guest,3)
            pointer=s.Pointer(guest);pointer.click(55,460);_,bounds=i.window(guest,5)
            for label,url,error in [('expired','https://expired.badssl.com/',54),('wrong-hostname','https://wrong.host.badssl.com/',56),('untrusted','https://self-signed.badssl.com/',62),('downgrade','http://example.com/',0)]:
                type_url(guest,url);pointer.click(bounds.x+80,bounds.y+355);raw=expect(guest,5)
                actual=struct.unpack_from('<I',raw,8)[0]
                h.shot(guest,out,label);assert actual==error,(label,actual,error)
                report['cases'].append({'scenario':label,'passed':True,'tls_error':actual})
            guest.qmp('qom-set',{'path':'/objects/engine-delay','property':'status','value':'on'})
            url='https://raw.githubusercontent.com/enginsoysal/codename-engine-os/main/third_party/mozilla-ca/cacert.pem'
            type_url(guest,url);pointer.click(bounds.x+80,bounds.y+355)
            # Cancel the actual pending/active request through the native control.
            assert state(guest) in (1,2,3)
            pointer.click(bounds.x+220,bounds.y+355);expect(guest,6)
            report['cases'].append({'scenario':'native cancellation leaves no completed partial file','passed':True})
            pointer.click(bounds.x+80,bounds.y+355)
            assert state(guest) in (1,2,3)
            guest.qmp('set_link',{'name':'engine-net-card','up':False});raw=expect(guest,5);t.n.wait_state(guest,1)
            h.shot(guest,out,'cable-loss')
            report['cases'].append({'scenario':'actual virtual cable loss fails transfer and preserves namespace','passed':True})
            guest.qmp('qom-set',{'path':'/objects/engine-delay','property':'status','value':'off'})
            start=len(guest.log());guest.qmp('set_link',{'name':'engine-net-card','up':True});guest.wait('M7_DHCP_CONFIGURED',start,60);t.n.wait_state(guest,3)
            type_url(guest,'https://raw.githubusercontent.com/enginsoysal/codename-engine-os/main/LICENSE')
            pointer.click(bounds.x+80,bounds.y+355);expect(guest,4)
            h.shot(guest,out,'recovered-download');(out/'serial.log').write_bytes(guest.log().encode())
            report['cases'].append({'scenario':'cable recovery renews DHCP and completes a fresh checked HTTPS transfer','passed':True})
        assert s.document(disk,'/Users/Owner/Downloads/Download1.bin')==(ROOT/'LICENSE').read_bytes()
        try:s.document(disk,'/Users/Owner/Downloads/Download2.bin')
        except AssertionError:pass
        else:raise AssertionError('Cancelled/failed transfer unexpectedly consumed a completed filename')
        bad_clock=[*extra];bad_clock[bad_clock.index('base=utc,clock=host')]='base=2010-01-01T00:00:00,clock=vm'
        with q.Guest(work/'invalid-clock',disk,media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY',extra=bad_clock) as guest:
            guest.wait('M7_DHCP_CONFIGURED',timeout=60);pointer=s.Pointer(guest);pointer.click(55,460);_,bounds=i.window(guest,5)
            pointer.click(bounds.x+80,bounds.y+355);raw=expect(guest,5)
            message=struct.unpack_from('<Q',raw,24)[0];assert b'UTC hardware clock is invalid' in i.memory(guest,message,100)
            h.shot(guest,out,'invalid-utc-clock');(out/'clock.log').write_bytes(guest.log().encode())
        assert s.document(disk,'/Users/Owner/Downloads/Download1.bin')==(ROOT/'LICENSE').read_bytes()
        report['cases'].append({'scenario':'actual invalid virtual RTC refuses secure download and preserves existing file','passed':True})
        manifest=json.loads((ROOT/'dist/m7-build-manifest.json').read_text(encoding='utf-8'))
        report.update(passed=True,build_id=manifest['build_id'],iso_sha256=manifest['artifacts']['EngineOS-M7.iso']['sha256'],scope='native negative-security/network slice; full M7 platform/resource qualification pending')
    except Exception:
        report['error']=q.clean(traceback.format_exc());print(report['error'])
        if 'guest' in locals():(out/'last-run.log').write_bytes(guest.log().encode())
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('M7 native HTTPS negative '+('PASS' if report['passed'] else 'FAIL'),flush=True)
    if not report['passed']:raise SystemExit(1)


if __name__=='__main__':main()
