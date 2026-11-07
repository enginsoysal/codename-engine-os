"""Download a real public Windows installer as data, never execute it.

Qualifies long checked HTTPS redirects, streamed persistence and unchanged
managed allocation during a useful binary transfer. Windows execution is M8.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import time
import traceback
ROOT=Path(__file__).resolve().parents[2]
def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);value=importlib.util.module_from_spec(spec);spec.loader.exec_module(value);return value
t=module('m7negative',Path(__file__).with_name('https_negative.py'))
r=module('m7resources',Path(__file__).with_name('resources.py'))
s,i,h,q=t.s,t.i,t.h,t.q


def main():
    fixture=json.loads((ROOT/'tests/m7/fixtures/download-7zip.json').read_text(encoding='utf-8'))
    work=q.new_work('m7-binary-download');out=ROOT/'evidence/M7/binary-download';out.mkdir(parents=True,exist_ok=True)
    report={'passed':False,'m7_complete':False,'installer_executed':False,'windows_compatibility_claimed':False}
    try:
        disk=q.sparse_disk(work/'owned.img',2*1024**3)
        with q.Guest(work/'install',disk,image_name='EngineOS-M7') as guest:
            h.next(guest);h.next(guest);h.next(guest,2);h.next(guest,3);h.next(guest,5);start=len(guest.log());h.next(guest,1);guest.wait('M6_INSTALL_PASS',start,240)
        extra=['-cpu','max,x2apic=off','-netdev','user,id=engine-net','-device','e1000e,id=engine-net-card,netdev=engine-net,mac=52:54:00:12:34:56']
        with q.Guest(work/'desktop',disk,media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY',extra=extra) as guest:
            guest.wait('M7_DHCP_CONFIGURED',timeout=60);pointer=s.Pointer(guest);pointer.click(55,460);_,bounds=i.window(guest,5)
            t.type_url(guest,fixture['url']);baseline=r.stats(guest);pointer.click(bounds.x+80,bounds.y+355)
            deadline=time.monotonic()+240;states=[];max_managed=baseline[0]
            while time.monotonic()<deadline:
                raw=t.read_info(guest);state=struct.unpack_from('<I',raw)[0]
                if not states or states[-1]!=state:states.append(state)
                max_managed=max(max_managed,r.stats(guest)[0])
                if state==4:break
                if state in (5,6):
                    message=i.memory(guest,struct.unpack_from('<Q',raw,24)[0],160).split(b'\0')[0].decode()
                    h.shot(guest,out,'failed-transfer');raise AssertionError('Actual binary download failed: '+message+' TLS='+str(struct.unpack_from('<I',raw,8)[0]))
                time.sleep(.1)
            assert state==4 and struct.unpack_from('<Q',raw,16)[0]==fixture['size']
            assert max_managed==baseline[0] and r.stats(guest)[0]==baseline[0]
            path=raw[32:288].split(b'\0')[0].decode();h.shot(guest,out,'native-binary-download-complete')
            time.sleep(.3);assert guest.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==2
            (out/'serial.log').write_bytes(guest.log().encode())
        payload=s.document(disk,path,limit=fixture['size']);assert payload[:2]==b'MZ' and len(payload)==fixture['size']
        assert hashlib.sha256(payload).hexdigest()==fixture['sha256']
        with q.Guest(work/'restart',disk,media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY') as guest:
            (out/'restart.log').write_bytes(guest.log().encode())
        assert hashlib.sha256(s.document(disk,path,limit=fixture['size'])).hexdigest()==fixture['sha256']
        manifest=json.loads((ROOT/'dist/m7-build-manifest.json').read_text(encoding='utf-8'))
        report.update(passed=True,build_id=manifest['build_id'],iso_sha256=manifest['artifacts']['EngineOS-M7.iso']['sha256'],
                      fixture=fixture,observed_states=states,download_bytes=len(payload),download_sha256=hashlib.sha256(payload).hexdigest(),
                      managed_baseline_bytes=baseline[0]*4096,managed_max_during_transfer_bytes=max_managed*4096,
                      scope='actual public binary HTTPS download and persisted byte/hash verification; no installer or Windows program execution')
    except Exception:
        report['error']=q.clean(traceback.format_exc());print(report['error'])
        if 'guest' in locals():(out/'last-run.log').write_bytes(guest.log().encode())
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('M7 native binary download '+('PASS' if report['passed'] else 'FAIL'),flush=True)
    if not report['passed']:raise SystemExit(1)


if __name__=='__main__':main()
