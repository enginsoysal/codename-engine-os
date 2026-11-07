"""Run actual bounded HTTP response parser on independent fragmented fixtures."""
import ctypes as c
import json
from pathlib import Path
import random
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools/build'))
from m1 import pinned_tools


class Response(c.Structure):
    _fields_=[(n,c.c_uint) for n in ['state','status','line_size','header_bytes','interim','seen_length','chunked','encoding','cr']]+[(n,c.c_uint64) for n in ['length','received','remaining']]+[('line',c.c_char*4096),('location',c.c_char*3072)]
Sink=c.CFUNCTYPE(c.c_uint,c.c_void_p,c.c_void_p,c.c_uint)


def main():
    work=ROOT/'build/m7/http-test';work.mkdir(parents=True,exist_ok=True);tools=pinned_tools()
    flags=['-std=c17','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin','-fno-stack-protector','-I',ROOT/'include']
    def run(args):
        result=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True)
        if result.returncode:raise RuntimeError(result.stderr.replace(str(ROOT),'<project-root>'))
    sources=[ROOT/'kernel/network/http.c',ROOT/'runtime/memory.c'];objs=[]
    run([tools['clang'],'--target=x86_64-unknown-none-elf',*flags,'-c',sources[0],'-o',work/'http.o'])
    for n,path in enumerate(sources):
        obj=work/(str(n)+'.obj');run([tools['clang'],'--target=x86_64-pc-windows-msvc',*flags,'-c',path,'-o',obj]);objs.append(obj)
    run([tools['lld-link'],'/dll','/noentry','/nodefaultlib',*['/export:http7_'+n for n in ['init','feed','finish']],*objs,'/out:'+str(work/'http.dll')])
    lib=c.CDLL(str(work/'http.dll'));lib.http7_init.argtypes=[c.POINTER(Response)];lib.http7_feed.argtypes=[c.POINTER(Response),c.c_void_p,c.c_uint,Sink,c.c_void_p];lib.http7_finish.argtypes=[c.POINTER(Response)]
    def parse(raw,step=1,finish=True):
        r=Response();lib.http7_init(c.byref(r));body=bytearray()
        @Sink
        def sink(ctx,p,n):body.extend(c.string_at(p,n));return 0
        for start in range(0,len(raw),step):
            part=raw[start:start+step]
            if lib.http7_feed(c.byref(r),part,len(part),sink,None):break
        if finish and r.state!=10:lib.http7_finish(c.byref(r))
        assert r.received==len(body) and len(body)<=64*1024**2
        return r,bytes(body)
    positives=[(b'HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello',b'hello'),
       (b'HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2;extension=value\r\nhe\r\n3\r\nllo\r\n0\r\nX-Note: done\r\n\r\n',b'hello'),
       (b'HTTP/1.0 200 OK\r\n\r\nhello',b'hello'),
       (b'HTTP/1.1 103 Early Hints\r\nLink: </style>\r\n\r\nHTTP/1.1 200 OK\r\ncontent-length: 0\r\n\r\n',b'')]
    for raw,expected in positives:
        for step in [1,2,7,31,4096]:
            r,body=parse(raw,step);assert r.state==8 and body==expected,(r.state,body)
    bad=[b'HTTP/1.1 200 OK\n\n',b'HTTP/1.1 200 OK\r\n Content-Length: 2\r\n\r\nxx',
         b'HTTP/1.1 200 OK\r\nContent-Length: 2\r\nContent-Length: 2\r\n\r\nxx',
         b'HTTP/1.1 200 OK\r\nContent-Length: 2\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n',
         b'HTTP/1.1 200 OK\r\nContent-Length: -1\r\n\r\n',b'HTTP/1.1 200 OK\r\nContent-Length: 67108865\r\n\r\n',
         b'HTTP/1.1 200 OK\r\nContent-Encoding: gzip\r\n\r\nx',b'HTTP/1.1 404 Missing\r\n\r\n',
         b'HTTP/1.1 200 OK\r\nTransfer-Encoding: gzip, chunked\r\n\r\n',
         b'HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\nFFFFFFFFFFFF\r\n',
         b'HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n0\r\nContent-Length: 2\r\n\r\n',
         b'HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nhi',
         b'HTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\nextra',
         b'HTTP/1.1 200 OK\r\nX: '+b'x'*4096+b'\r\n\r\n',
         b'HTTP/1.1 200 OK\r\n'+b'X: x\r\n'*3000+b'\r\n']
    for raw in bad:
        for step in [1,4096]:assert parse(raw,step)[0].state==9
    r,_=parse(b'HTTP/1.1 302 Found\r\nLocation: https://example.com/next\r\n\r\nignored')
    assert r.state==10 and r.location==b'https://example.com/next'
    long_location=b'https://example.com/download?signature='+b'a'*1500
    r,_=parse(b'HTTP/1.1 302 Found\r\nLocation: '+long_location+b'\r\n\r\n',31)
    assert r.state==10 and r.location==long_location
    rng=random.Random(703);base=positives[1][0]
    for _ in range(5000):
        raw=bytearray(base)
        for __ in range(rng.randrange(1,5)):raw[rng.randrange(len(raw))]=rng.randrange(256)
        parse(bytes(raw),rng.randrange(1,40))
    report={'passed':True,'m7_complete':False,'mutation_cases':5000,'scope':'actual HTTP parser C with host sinks; no native HTTPS task qualification',
            'checks':['fragmented fixed/chunked/close-delimited bodies','interim response','HTTPS redirect metadata','ambiguous framing/oversized/encoded/truncated refusal','bounded header/trailer/body sizes']}
    (ROOT/'evidence/M7/http-results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('HTTP parser PASS: fragmented fixtures and 5000 mutations')


if __name__=='__main__':main()
