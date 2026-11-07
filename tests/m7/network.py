"""Qualify real e1000 DMA/interrupt traffic, automatic DHCP and DNS in an owned native guest."""
import importlib.util
import argparse
import json
from pathlib import Path
import shutil
import struct
import time
import traceback

ROOT=Path(__file__).resolve().parents[2]
def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);value=importlib.util.module_from_spec(spec);spec.loader.exec_module(value);return value
s=module('m7session',Path(__file__).with_name('session.py'))
i=module('m7isolation',Path(__file__).with_name('isolation.py'))
h,q=s.h,s.q


def wait_state(guest,value):
    end=time.monotonic()+15
    while time.monotonic()<end:
        state=struct.unpack('<I',i.memory(guest,i.symbol('configuration')[0],4))[0]
        if state==value:return
        time.sleep(.05)
    raise AssertionError('Native network state did not become '+str(value))


def inspect_capture(path):
    raw=path.read_bytes();assert raw[:4]==b'\xd4\xc3\xb2\xa1'
    offset=24;types=[];queries=responses=arp=0;frames=0
    while offset<len(raw):
        assert len(raw)-offset>=16
        _,_,size,original=struct.unpack_from('<IIII',raw,offset);offset+=16
        assert size<=original and size<=len(raw)-offset
        packet=raw[offset:offset+size];offset+=size;frames+=1
        if len(packet)<14:continue
        eth=struct.unpack_from('!H',packet,12)[0]
        if eth==0x0806:arp+=1;continue
        if eth!=0x0800 or len(packet)<34 or packet[23]!=17:continue
        header=(packet[14]&15)*4;position=14+header
        if len(packet)-position<8:continue
        source,destination,length,_=struct.unpack_from('!HHHH',packet,position)
        body=packet[position+8:position+length]
        if {source,destination}=={67,68} and len(body)>=240 and body[236:240]==bytes.fromhex('63825363'):
            pos=240
            while pos<len(body):
                tag=body[pos];pos+=1
                if tag==255:break
                if tag==0:continue
                assert pos<len(body);count=body[pos];pos+=1;assert count<=len(body)-pos
                if tag==53 and count==1:types.append(body[pos])
                pos+=count
        if destination==53:queries+=1
        if source==53:responses+=1
    assert {1,2,3,5}<=set(types) and arp and queries and responses
    return {'frames':frames,'dhcp_message_types':types,'arp_frames':arp,'dns_queries':queries,'dns_responses':responses}


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--nic',choices=['e1000','e1000e'],default='e1000');args=parser.parse_args()
    work=q.new_work('m7-network-'+args.nic);out=ROOT/('evidence/M7/network' if args.nic=='e1000' else 'evidence/M7/network-e1000e');out.mkdir(parents=True,exist_ok=True)
    checks=[];result={'passed':False,'m7_complete':False,'checks':checks}
    try:
        disk=q.sparse_disk(work/'owned.img',2*1024**3)
        with q.Guest(work/'install',disk,image_name='EngineOS-M7') as guest:
            h.next(guest);h.next(guest);h.next(guest,2);h.next(guest,3);h.next(guest,5);start=len(guest.log());h.next(guest,1);guest.wait('M6_INSTALL_PASS',start,240)
        capture=work/'ethernet.pcap'
        extra=['-cpu','max,x2apic=off','-netdev','user,id=engine-net','-device',args.nic+',id=engine-net-card,netdev=engine-net,mac=52:54:00:12:34:56',
               '-object','filter-dump,id=engine-capture,netdev=engine-net,file='+str(capture)]
        with q.Guest(work/'desktop',disk,media='installed',image_name='EngineOS-M7',ready='M7_DESKTOP_READY',extra=extra) as guest:
            guest.wait('M7_NETWORK_E1000_READY');guest.wait('M7_DHCP_CONFIGURED',timeout=60);wait_state(guest,3)
            pointer=s.Pointer(guest);pointer.click(55,460)
            _,bounds=i.window(guest,5)
            pointer.click(bounds.x+185+90,bounds.y+200+15)
            guest.wait('M7_DNS_RESOLVED',timeout=30);h.shot(guest,out,'real-dhcp-and-dns')
            checks.append({'name':'actual native e1000 DMA/IRQ path performs DHCP discovery/offer/request/ack and real DNS through QEMU user networking','passed':True})
            guest.qmp('set_link',{'name':'engine-net-card','up':False});wait_state(guest,1)
            h.shot(guest,out,'network-disconnected')
            start=len(guest.log());guest.qmp('set_link',{'name':'engine-net-card','up':True});guest.wait('M7_DHCP_CONFIGURED',start,60);wait_state(guest,3)
            start=len(guest.log());pointer.click(bounds.x+185+90,bounds.y+200+15);guest.wait('M7_DNS_RESOLVED',start,30)
            h.shot(guest,out,'network-recovered')
            time.sleep(.5)
            address,_=i.symbol('paints');before=i.memory(guest,address,8);time.sleep(.3);after=i.memory(guest,address,8)
            assert before==after
            assert guest.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==2
            (out/'serial.log').write_bytes(guest.log().encode())
            checks.append({'name':'actual link loss/recovery renews configuration and DNS; completed work disables its timer, idle CPUs halt and painting stops','passed':True})
        traffic=inspect_capture(capture);shutil.copyfile(capture,out/'ethernet.pcap')
        manifest=json.loads((ROOT/'dist/m7-build-manifest.json').read_text(encoding='utf-8'))
        result.update(passed=True,nic=args.nic,cpu_profile="max,x2apic=off; RDRAND exposed",build_id=manifest['build_id'],iso_sha256=manifest['artifacts']['EngineOS-M7.iso']['sha256'],
                      packet_capture=traffic,https_qualified=False,scope='native Ethernet/DHCP/ARP/DNS slice only; TCP/HTTPS and full M7 remain pending')
    except Exception:
        result['error']=q.clean(traceback.format_exc());print(result['error'])
        if 'guest' in locals():(out/'last-run.log').write_bytes(guest.log().encode())
    (out/'results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode())
    print('M7 native network slice '+('PASS' if result['passed'] else 'FAIL'),flush=True)
    if not result['passed']:raise SystemExit(1)


if __name__=='__main__':main()
