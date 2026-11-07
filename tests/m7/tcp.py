"""Execute the actual original TCP state machine against independent peer packets.

The host adapter supplies time, entropy and packet delivery only. This is not
native NIC or TLS qualification.
"""
import ctypes as c
import importlib.util
import json
from pathlib import Path
import struct
import subprocess

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('wire_test',Path(__file__).with_name('wire.py'))
w=importlib.util.module_from_spec(spec);spec.loader.exec_module(w)


def main():
    work=ROOT/'build/m7/tcp-test';work.mkdir(parents=True,exist_ok=True)
    tools=w.pinned_tools()
    adapter=work/'adapter.c'
    adapter.write_text('''#include <engine/network/network.h>
#include <stdint.h>
uint64_t tick;
unsigned busy,entropy=1,connected=1,sends;
uint8_t last_packet[1500];unsigned last_size;
uint64_t m3_clock_ticks(void){return tick;}
void net7_schedule(void){}
net7_configuration net7_current(void){return (net7_configuration){connected?NET7_CONNECTED:NET7_DISCONNECTED,0,0x0a00020f,0xffffff00,0x0a000202,0x0a000203,0};}
unsigned net7_random(void *p,unsigned n){uint8_t *b=p;for(unsigned i=0;i<n;++i)b[i]=255;return entropy;}
unsigned net7_send_ipv4(uint32_t d,unsigned p,const void *b,unsigned n){
    (void)d;(void)p;if(busy)return 1;
    const uint8_t *s=b;for(unsigned i=0;i<n;++i)last_packet[i]=s[i];last_size=n;++sends;return 0;
}
''',encoding='utf-8')
    flags=['-std=c17','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin','-fno-stack-protector','-I',ROOT/'include']
    def run(args):
        result=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True)
        if result.returncode:raise RuntimeError(result.stderr.replace(str(ROOT),'<project-root>'))
    sources=[ROOT/'kernel/network/tcp.c',ROOT/'kernel/network/wire.c',ROOT/'runtime/memory.c',adapter]
    run([tools['clang'],'--target=x86_64-unknown-none-elf',*flags,'-mno-red-zone','-c',sources[0],'-o',work/'tcp.o'])
    objects=[]
    for n,path in enumerate(sources):
        obj=work/(str(n)+'.obj');run([tools['clang'],'--target=x86_64-pc-windows-msvc',*flags,'-c',path,'-o',obj]);objects.append(obj)
    functions=['connect','status','write','read','close','abort','receive','pump','active']
    globals_=['tick','busy','entropy','connected','sends','last_packet','last_size']
    run([tools['lld-link'],'/dll','/noentry','/nodefaultlib',*['/export:tcp7_'+n for n in functions],*['/export:'+n for n in globals_],*objects,'/out:'+str(work/'tcp.dll')])
    lib=c.CDLL(str(work/'tcp.dll'))
    lib.tcp7_connect.argtypes=[c.c_uint32,c.c_uint]
    for name in ['write','read']:getattr(lib,'tcp7_'+name).argtypes=[c.c_void_p,c.c_uint]
    lib.tcp7_receive.argtypes=[c.POINTER(w.IPv4)]
    variables={n:c.c_uint.in_dll(lib,n) for n in globals_ if n not in ['tick','last_packet']}
    tick=c.c_uint64.in_dll(lib,'tick');captured=(c.c_ubyte*1500).in_dll(lib,'last_packet')
    def sent():return bytes(captured[:variables['last_size'].value])
    remote=0x0a000202;local=0x0a00020f;port=0;ack=0;seq=9000
    def peer(flags=16,data=b'',sequence=None,acknowledgement=None,window=16384,options=b''):
        packet=bytearray(struct.pack('!HHIIBBHHH',443,port,seq if sequence is None else sequence,
            ack if acknowledgement is None else acknowledgement,((20+len(options))//4)<<4,flags,window,0,0)+options+data)
        pseudo=struct.pack('!IIBBH',remote,local,0,6,len(packet))
        struct.pack_into('!H',packet,16,w.checksum(pseudo+packet))
        buffer=c.create_string_buffer(bytes(packet));ip=w.IPv4(remote,local,c.cast(buffer,c.c_void_p),len(packet),6)
        lib.tcp7_receive(c.byref(ip))
    def connect():
        nonlocal port,ack,seq
        lib.tcp7_abort();tick.value=0;variables['busy'].value=0;variables['entropy'].value=1;variables['connected'].value=1
        assert not lib.tcp7_connect(remote,443);lib.tcp7_pump();packet=sent()
        port=struct.unpack_from('!H',packet)[0];initial=struct.unpack_from('!I',packet,4)[0]
        assert initial==0xffffffff and packet[13]==2 and packet[20:24]==b'\x02\x04\x05\xb4'
        ack=(initial+1)&0xffffffff;seq=9000
        peer(18,options=b'\x02\x04\x05\xb4');seq+=1;lib.tcp7_pump()
        assert lib.tcp7_status()==2 and sent()[13]==16
    connect()
    # Queue once; temporary device backpressure cannot consume or lose bytes.
    variables['busy'].value=1;assert lib.tcp7_write(b'hello',5)==5
    before=variables['sends'].value;lib.tcp7_pump();assert variables['sends'].value==before
    variables['busy'].value=0;lib.tcp7_pump();first=sent();assert first[20:]==b'hello'
    assert not lib.tcp7_write(b'x',1)
    tick.value=100;lib.tcp7_pump();assert sent()==first
    # Invalid ACKs and out-of-window ACKs cannot release the queued payload.
    peer(acknowledgement=6);assert not lib.tcp7_write(b'x',1)
    peer(sequence=seq+100,acknowledgement=5);assert not lib.tcp7_write(b'x',1)
    ack=5;peer();assert lib.tcp7_write(b'next',4)==4;lib.tcp7_pump();ack=9;peer()
    peer(data=b'abc');seq+=3;lib.tcp7_pump()
    peer(data=b'abc',sequence=seq-3);peer(data=b'future',sequence=seq+10)
    out=c.create_string_buffer(32);assert lib.tcp7_read(out,32)==3 and out.raw[:3]==b'abc'
    # A full receive ring advertises zero and reopening it sends a window update.
    for _ in range(16):peer(data=b'z'*1024);seq+=1024
    lib.tcp7_pump();assert struct.unpack_from('!H',sent(),14)[0]==0
    peer(data=b'overflow');lib.tcp7_pump();assert struct.unpack_from('!H',sent(),14)[0]==0
    out=c.create_string_buffer(16384);assert lib.tcp7_read(out,16384)==16384 and out.raw==b'z'*16384
    lib.tcp7_pump();assert struct.unpack_from('!H',sent(),14)[0]==16384
    # Invalid reset is challenged; a peer FIN supports final local writes.
    peer(20,sequence=seq+1);assert lib.tcp7_status()==2
    peer(17);seq+=1;assert lib.tcp7_status()==3
    assert lib.tcp7_write(b'bye',3)==3;lib.tcp7_pump();ack+=3;peer()
    lib.tcp7_close();lib.tcp7_pump();ack+=1;peer();assert lib.tcp7_status()==0
    connect();peer(20);assert lib.tcp7_status()==5
    connect();variables['connected'].value=0;lib.tcp7_pump();assert lib.tcp7_status()==5
    connect();tick.value=3000;lib.tcp7_pump();assert lib.tcp7_status()==5
    # No entropy means no SYN, not a predictable security-sensitive connection.
    lib.tcp7_abort();variables['entropy'].value=0;assert lib.tcp7_connect(remote,443)
    report={'passed':True,'m7_complete':False,'scope':'actual original C TCP state machine with host packet/time adapter; no native TCP/TLS qualification',
            'checks':['sequence wrap handshake and MSS','bounded queue/device backpressure/retransmission','invalid ACK and sequence refusal',
                      'duplicate/out-of-order receive','16 KiB flow control/window update','half-close/final write','reset/link loss/timeout','missing entropy refusal']}
    (ROOT/'evidence/M7/tcp-results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('TCP state machine PASS; native transport and TLS remain pending')


if __name__=='__main__':main()
