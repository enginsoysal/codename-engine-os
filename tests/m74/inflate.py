"""Check the freestanding decoder's bounded event ABI against independent zlib."""
import ctypes as c
import hashlib
import json
from pathlib import Path
import random
import subprocess
import sys
import zlib
from calculator import View

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/build'))
from m1 import pinned_tools

class Mailbox(c.Structure):
    _fields_=[('magic',c.c_uint),('command',c.c_uint),('key',c.c_uint),('bytes',c.c_uint),('input',c.c_char*8192),('view',View)]

def main():
    work=ROOT/'build/m74/inflate-host';work.mkdir(parents=True,exist_ok=True);tools=pinned_tools()
    flags=['--target=x86_64-pc-windows-msvc','-std=c17','-O2','-ffreestanding','-fno-builtin','-fno-stack-protector','-mno-stack-arg-probe',
           '-DENGINE_M74','-DAPP74_HOST','-I',str(ROOT/'include'),'-I',str(ROOT/'third_party/miniz-port'),'-I',str(ROOT/'third_party/miniz'),
           '-include',str(ROOT/'third_party/miniz-port/config74.h')]
    objects=[]
    for source,name in [('user/archive74.c','worker'),('third_party/miniz/miniz_tinfl.c','decoder')]:
        obj=work/(name+'.obj');subprocess.run([tools['clang'],*flags,'-c',str(ROOT/source),'-o',str(obj)],check=True,capture_output=True);objects.append(str(obj))
    subprocess.run([tools['lld-link'],'/dll','/noentry','/nodefaultlib','/machine:x64','/out:'+str(work/'test.dll'),*objects],check=True,capture_output=True)
    dll=c.CDLL(str(work/'test.dll'));dll.archive74_event.argtypes=[c.POINTER(Mailbox),c.c_void_p]
    def decode(data,chunk):
        m=Mailbox();m.magic=0x37415050;m.command=20;state=c.create_string_buffer(12*4096);dll.archive74_event(c.byref(m),state)
        offset=0;output=bytearray()
        for step in range(200000):
            size=0 if m.view.number else min(chunk,len(data)-offset)
            m.command=21;m.bytes=size;m.key=int(offset+size<len(data))
            if size:c.memmove(c.addressof(m)+Mailbox.input.offset,data[offset:offset+size],size)
            dll.archive74_event(c.byref(m),state)
            if m.view.calc_error:raise ValueError(f'Decoder rejected compressed input: status={m.view.operation}, offset={offset}, bytes={size}, chunk={chunk}, total={len(data)}')
            assert 0<=m.view.operand<=size and 0<=m.view.number<=32768 and m.view.length<=4096
            offset+=m.view.operand;output+=c.string_at(c.addressof(m.view),m.view.length)
            if not m.view.operation:
                assert offset==len(data);return bytes(output),step+1
            if not m.view.operand and not m.view.length and not size:raise ValueError('Truncated stream made no progress')
        raise AssertionError('Decoder did not terminate within its bounded test budget')
    rng=random.Random(74);cases=[]
    for original,chunk in [(b'',1),(bytes(range(256))*16,1),(b'Engine OS\n'*12000,17),(rng.randbytes(90000),8191),
                            (bytes(range(256))*700,8191)]:
        stream=zlib.compressobj(level=9,wbits=-15);encoded=stream.compress(original)+stream.flush()
        decoded,events=decode(encoded,chunk);assert decoded==original
        cases.append({'source_sha256':hashlib.sha256(original).hexdigest(),'bytes':len(original),'input_chunk':chunk,'events':events})
    for data in [b'\x06',b'\xff'*24,zlib.compress(b'not a raw deflate stream')]:
        try:decode(data,17)
        except ValueError:pass
        else:raise AssertionError('Malformed stream was accepted')
    out=ROOT/'evidence/M7.4';out.mkdir(exist_ok=True)
    (out/'inflate-host-results.json').write_bytes((json.dumps({'passed':True,'host_only':True,'cases':cases,'malformed_cases':3},indent=2)+'\n').encode('utf-8'))
    print('Bounded inflate event ABI PASS; native extraction acceptance remains separate')

if __name__=='__main__':main()
