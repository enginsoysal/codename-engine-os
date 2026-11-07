"""Qualify an actual installed native HTTPS download and persisted exact bytes."""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import struct
import time
import traceback

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('m7network',Path(__file__).with_name('network.py'))
n=importlib.util.module_from_spec(spec);spec.loader.exec_module(n)
s,i,h,q=n.s,n.i,n.h,n.q


def download_address():
    mapping=(ROOT/'dist/EngineOS-M7-installed-kernel.map').read_text(encoding='utf-8')
    # Match the owning translation unit rather than another driver's static info.
    block=mapping.split('kernel_network_download-kernel.o:(.bss)',1)[1]
    found=re.search(r'^\s*([0-9a-f]+)\s+[0-9a-f]+\s+[0-9a-f]+\s+\d+\s+info$',block,re.M)
    assert found
    return int(found[1],16)


def wait_download(guest,wanted=4):
    address=download_address();deadline=time.monotonic()+120
    while time.monotonic()<deadline:
        raw=i.memory(guest,address,288);state,revision,error=struct.unpack_from('<III',raw)
        if state==wanted:return raw
        if state in (4,5,6):raise AssertionError('Download state '+str(state)+' TLS error '+str(error))
        time.sleep(.15)
    raise AssertionError('Native download timed out, state '+str(state)+' TLS error '+str(error))


def main():
    work=q.new_work('m7-https');out=ROOT/'evidence/M7/https';out.mkdir(parents=True,exist_ok=True)
    result={'passed':False,'m7_complete':False,'checks':[]}
    try:
        disk=q.sparse_disk(work/'owned.img',2*1024**3)
        with q.Guest(work/'install',disk,image_name='EngineOS-M7') as guest:
            h.next(guest);h.next(guest);h.next(guest,2);h.next(guest,3);h.next(guest,5);start=len(guest.log());h.next(guest,1);guest.wait('M6_INSTALL_PASS',start,240)
        capture=work/'https.pcap'
        extra=['-cpu','max,x2apic=off','-rtc','base=utc,clock=host','-netdev','user,id=engine-net','-device','e1000,id=engine-net-card,netdev=engine-net,mac=52:54:00:12:34:56',
               '-object','filter-dump,id=engine-capture,netdev=engine-net,file='+str(capture)]
        with q.Guest(work/'desktop',disk,media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY',extra=extra) as guest:
            guest.wait('M7_DHCP_CONFIGURED',timeout=60);n.wait_state(guest,3)
            pointer=s.Pointer(guest);pointer.click(55,460);_,bounds=i.window(guest,5)
            pointer.click(bounds.x+80,bounds.y+355)
            try:raw=wait_download(guest)
            finally:h.shot(guest,out,'native-https-result')
            path=raw[32:288].split(b'\0')[0].decode();size=struct.unpack_from('<Q',raw,16)[0]
            assert path=='/Users/Owner/Downloads/Download1.bin' and size>0
            guest.wait('M7_HTTPS_DOWNLOAD_SAVED')
            time.sleep(.5)
            paints=i.symbol('paints')[0];before=i.memory(guest,paints,8);time.sleep(.3);assert before==i.memory(guest,paints,8)
            assert guest.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==2
            (out/'serial.log').write_bytes(guest.log().encode())
        payload=s.document(disk,path);expected=(ROOT/'LICENSE').read_bytes()
        assert payload==expected and len(payload)==size
        result['checks'].append({'name':'real native DHCP DNS TCP TLS 1.2 certificate/UTC checked GET from raw.githubusercontent.com; exact public MIT license bytes persisted on installed disk','passed':True})
        with q.Guest(work/'restart',disk,media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY') as guest:
            pointer=s.Pointer(guest);pointer.click(55,460);_,bounds=i.window(guest,5);pointer.click(bounds.x+360,bounds.y+355)
            h.shot(guest,out,'downloads-after-restart');(out/'restart.log').write_bytes(guest.log().encode())
        assert s.document(disk,path)==expected
        shutil.copyfile(capture,out/'https.pcap')
        manifest=json.loads((ROOT/'dist/m7-build-manifest.json').read_text(encoding='utf-8'))
        result.update(passed=True,build_id=manifest['build_id'],iso_sha256=manifest['artifacts']['EngineOS-M7.iso']['sha256'],download_sha256=hashlib.sha256(payload).hexdigest(),download_bytes=len(payload),
                      scope='native positive HTTPS and restart persistence slice; negative native TLS/network/resource/full M7 gates still pending')
    except Exception:
        result['error']=q.clean(traceback.format_exc());print(result['error'])
        if 'guest' in locals():(out/'last-run.log').write_bytes(guest.log().encode())
        if 'capture' in locals() and capture.exists():shutil.copyfile(capture,out/'https.pcap')
    (out/'results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('M7 native HTTPS '+('PASS' if result['passed'] else 'FAIL'),flush=True)
    if not result['passed']:raise SystemExit(1)


if __name__=='__main__':main()
