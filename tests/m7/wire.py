"""Execute original bounded network parsers against independent wire fixtures and mutations."""
import ctypes as c
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/build'))
from m1 import pinned_tools

class IPv4(c.Structure):
    _fields_=[('source',c.c_uint32),('destination',c.c_uint32),('data',c.c_void_p),('bytes',c.c_uint),('protocol',c.c_uint)]
class UDP(c.Structure):
    _fields_=[('source',c.c_uint16),('destination',c.c_uint16),('data',c.c_void_p),('bytes',c.c_uint)]
class TCP(c.Structure):
    _fields_=[('source',c.c_uint16),('destination',c.c_uint16),('window',c.c_uint16),
              ('sequence',c.c_uint32),('acknowledgement',c.c_uint32),('data',c.c_void_p),('bytes',c.c_uint),('flags',c.c_uint)]
class Lease(c.Structure):
    _fields_=[(n,c.c_uint32) for n in ['address','mask','router','dns','server','seconds','type']]
class URL(c.Structure):
    _fields_=[('host',c.c_char*254),('path',c.c_char*2048),('port',c.c_uint16)]

MAC=b'\x52\x54\x00\x12\x34\x56'
SOURCE=0x0a000202
DESTINATION=0x0a00020f

def checksum(data):
    data+=b'\0'*(len(data)%2)
    total=sum(struct.unpack('!'+str(len(data)//2)+'H',data))
    while total>>16:total=(total&65535)+(total>>16)
    return (~total)&65535

def packet(protocol,payload):
    ip=bytearray(struct.pack('!BBHHHBBHII',0x45,0,20+len(payload),1337,0x4000,64,protocol,0,SOURCE,DESTINATION))
    struct.pack_into('!H',ip,10,checksum(bytes(ip)))
    return MAC+b'\x02'*6+b'\x08\x00'+ip+payload

def udp(payload):
    data=bytearray(struct.pack('!HHHH',67,68,len(payload)+8,0)+payload)
    pseudo=struct.pack('!IIBBH',SOURCE,DESTINATION,0,17,len(data))
    struct.pack_into('!H',data,6,checksum(pseudo+data) or 65535)
    return packet(17,data)

def domain(value):
    return b''.join(bytes([len(p)])+p.encode() for p in value.split('.'))+b'\0'

def main():
    work=ROOT/'build/m7/wire';work.mkdir(parents=True,exist_ok=True)
    out=ROOT/'evidence/M7/wire-results.json'
    tools=pinned_tools();source=ROOT/'kernel/network/wire.c'
    flags=['-std=c17','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin','-fno-stack-protector','-I',ROOT/'include']
    def run(args):
        result=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True)
        if result.returncode:raise RuntimeError(result.stderr.replace(str(ROOT),'<project-root>'))
    run([tools['clang'],'--target=x86_64-unknown-none-elf',*flags,'-mno-red-zone','-c',source,'-o',work/'wire.o'])
    objects=[]
    for name,path in [('wire',source),('memory',ROOT/'runtime/memory.c')]:
        obj=work/(name+'.obj');run([tools['clang'],'--target=x86_64-pc-windows-msvc',*flags,'-c',path,'-o',obj]);objects.append(obj)
    names=['read_ipv4','read_udp','read_tcp','read_dhcp','dns_answer','parse_url']
    run([tools['lld-link'],'/dll','/noentry','/nodefaultlib',*['/export:net7_'+n for n in names],*objects,'/out:'+str(work/'wire.dll')])
    lib=c.CDLL(str(work/'wire.dll'))
    types={
        'read_ipv4':[c.c_void_p,c.c_uint,c.c_void_p,c.POINTER(IPv4)],
        'read_udp':[c.POINTER(IPv4),c.POINTER(UDP)],
        'read_tcp':[c.POINTER(IPv4),c.POINTER(TCP)],
        'read_dhcp':[c.c_void_p,c.c_uint,c.c_uint32,c.c_void_p,c.POINTER(Lease)],
        'dns_answer':[c.c_void_p,c.c_uint,c.c_uint16,c.c_char_p,c.POINTER(c.c_uint32)],
        'parse_url':[c.c_char_p,c.POINTER(URL)]}
    for name,args in types.items():getattr(lib,'net7_'+name).argtypes=args;getattr(lib,'net7_'+name).restype=c.c_uint
    checks=[]
    frame=udp(b'payload');buffer=c.create_string_buffer(frame);ip=IPv4();result=UDP()
    assert not lib.net7_read_ipv4(buffer,len(frame),MAC,c.byref(ip))
    assert (ip.source,ip.destination,ip.protocol)==(SOURCE,DESTINATION,17)
    assert not lib.net7_read_udp(c.byref(ip),c.byref(result)) and c.string_at(result.data,result.bytes)==b'payload'
    tcp=bytearray(struct.pack('!HHIIBBHHH',443,50000,17,41,0x50,0x18,16384,0,0)+b'tls record bytes')
    pseudo=struct.pack('!IIBBH',SOURCE,DESTINATION,0,6,len(tcp));struct.pack_into('!H',tcp,16,checksum(pseudo+tcp))
    tcp_frame=packet(6,tcp);tcp_buffer=c.create_string_buffer(tcp_frame);tcp_result=TCP()
    assert not lib.net7_read_ipv4(tcp_buffer,len(tcp_frame),MAC,c.byref(ip))
    assert not lib.net7_read_tcp(c.byref(ip),c.byref(tcp_result))
    assert (tcp_result.source,tcp_result.sequence,tcp_result.acknowledgement,tcp_result.flags)==(443,17,41,0x18)
    assert c.string_at(tcp_result.data,tcp_result.bytes)==b'tls record bytes'
    for value in [frame[:i] for i in range(len(frame))]:
        obj=IPv4();c.memset(c.byref(obj),0xa5,c.sizeof(obj));before=bytes(obj)
        status=lib.net7_read_ipv4(c.create_string_buffer(value),len(value),MAC,c.byref(obj))
        assert status and bytes(obj)==before
    broken=bytearray(frame);broken[-1]^=1;buf=c.create_string_buffer(bytes(broken))
    assert not lib.net7_read_ipv4(buf,len(broken),MAC,c.byref(ip)) and lib.net7_read_udp(c.byref(ip),c.byref(result))
    checks.append({'name':'independent IPv4/UDP/TCP checksums, real fields/payload, truncations and corrupted transport rejection','passed':True})

    dhcp=bytearray(240);dhcp[:4]=bytes([2,1,6,0]);struct.pack_into('!I',dhcp,4,0x12345678)
    struct.pack_into('!I',dhcp,16,DESTINATION);dhcp[28:34]=MAC;dhcp[236:240]=bytes.fromhex('63825363')
    options=bytes([53,1,5,54,4])+struct.pack('!I',SOURCE)+bytes([1,4])+bytes.fromhex('ffffff00')+bytes([3,4])+struct.pack('!I',SOURCE)+bytes([6,4])+struct.pack('!I',0x0a000203)+bytes([51,4])+struct.pack('!I',3600)+b'\xff'
    dhcp+=options;lease=Lease()
    assert not lib.net7_read_dhcp(c.create_string_buffer(bytes(dhcp)),len(dhcp),0x12345678,MAC,c.byref(lease))
    assert (lease.address,lease.mask,lease.dns,lease.seconds,lease.type)==(DESTINATION,0xffffff00,0x0a000203,3600,5)
    for bad in [bytes(dhcp[:-1]),bytes(dhcp[:-1])+bytes([53,1,2,255]),bytes(dhcp[:242])+b'\xff']:
        obj=Lease();c.memset(c.byref(obj),0xa5,c.sizeof(obj));before=bytes(obj)
        assert lib.net7_read_dhcp(c.create_string_buffer(bad),len(bad),0x12345678,MAC,c.byref(obj)) and bytes(obj)==before
    assert lib.net7_read_dhcp(c.create_string_buffer(bytes(dhcp)),len(dhcp),123,MAC,c.byref(lease))
    checks.append({'name':'DHCP transaction/MAC binding, exact options, lease accounting and duplicate/truncated refusal','passed':True})

    query=domain('download.example')+struct.pack('!HH',1,1)
    answer=b'\xc0\x0c'+struct.pack('!HHIH',1,1,60,4)+bytes.fromhex('c0000201')
    dns=struct.pack('!HHHHHH',1234,0x8180,1,1,0,0)+query+answer;address=c.c_uint32()
    assert not lib.net7_dns_answer(c.create_string_buffer(dns),len(dns),1234,b'download.example',c.byref(address)) and address.value==0xc0000201
    cname=domain('cdn.example')
    dns_chain=struct.pack('!HHHHHH',1234,0x8180,1,2,0,0)+query+b'\xc0\x0c'+struct.pack('!HHIH',5,1,60,len(cname))+cname+domain('cdn.example')+struct.pack('!HHIH',1,1,60,4)+bytes.fromhex('c0000202')
    assert not lib.net7_dns_answer(c.create_string_buffer(dns_chain),len(dns_chain),1234,b'DOWNLOAD.EXAMPLE',c.byref(address)) and address.value==0xc0000202
    looping=bytearray(dns);looping[12:14]=b'\xc0\x0c'
    assert lib.net7_dns_answer(c.create_string_buffer(bytes(looping)),len(looping),1234,b'download.example',c.byref(address))
    assert lib.net7_dns_answer(c.create_string_buffer(dns),len(dns),1234,b'other.example',c.byref(address))
    checks.append({'name':'DNS ID/question binding, compressed names, CNAME resolution, case folding and compression-loop/mismatch rejection','passed':True})

    for url,host,path,port in [('https://example.com','example.com','/',443),('HTTPS://Example.COM/a?b=1','example.com','/a?b=1',443),('https://example.com:8443?file=1','example.com','/?file=1',8443)]:
        parsed=URL();assert not lib.net7_parse_url(url.encode(),c.byref(parsed))
        assert (parsed.host.decode(),parsed.path.decode(),parsed.port)==(host,path,port)
    for url in ['http://example.com','https://user@example.com/a','https://-bad.example','https://a..b','https://example.com:0','https://example.com:65536','https://example.com/#fragment','https://example.com/a b','https://'+('a'*64)+'.com']:
        parsed=URL();c.memset(c.byref(parsed),0xa5,c.sizeof(parsed));before=bytes(parsed)
        assert lib.net7_parse_url(url.encode(),c.byref(parsed)) and bytes(parsed)==before
    parsed=URL();assert not lib.net7_parse_url(('https://example.com/'+('x'*2046)).encode(),c.byref(parsed))
    assert lib.net7_parse_url(('https://example.com/'+('x'*2047)).encode(),c.byref(parsed))
    checks.append({'name':'bounded HTTPS URL/port/path parsing, no credentials/downgrade/control bytes and unchanged outputs on refusal','passed':True})

    rng=random.Random(7002)
    for _ in range(30000):
        original=rng.choice([frame,tcp_frame,bytes(dhcp),dns,dns_chain])
        data=bytearray(original)
        for _ in range(rng.randrange(1,6)):
            if data:data[rng.randrange(len(data))]=rng.randrange(256)
        data=bytes(data[:rng.randrange(len(data)+1)]);buf=c.create_string_buffer(data)
        obj=IPv4();status=lib.net7_read_ipv4(buf,len(data),MAC,c.byref(obj))
        if not status:
            assert obj.bytes<=len(data)-34 and obj.protocol<=255
            lib.net7_read_udp(c.byref(obj),c.byref(UDP()));lib.net7_read_tcp(c.byref(obj),c.byref(TCP()))
        lib.net7_read_dhcp(buf,len(data),0x12345678,MAC,c.byref(Lease()))
        address=c.c_uint32(0xa5a5a5a5)
        status=lib.net7_dns_answer(buf,len(data),1234,b'download.example',c.byref(address))
        if status:assert address.value==0xa5a5a5a5
    checks.append({'name':'30000 deterministic mutation/truncation cases execute actual bounded C parsers','passed':True,'cases':30000})
    report={'passed':True,'m7_complete':False,'native_network_qualified':False,
            'scope':'actual C parser host execution and freestanding compile only; no NIC/DHCP/TCP/TLS runtime claim',
            'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
            'header_sha256':hashlib.sha256((ROOT/'include/engine/network/wire.h').read_bytes()).hexdigest(),'checks':checks}
    out.write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print('M7 wire parsers PASS: protocol fixtures, refusals and 30000 mutations')


if __name__=='__main__':main()
