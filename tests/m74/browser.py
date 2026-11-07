"""Exercise bounded HTML events; this is not a modern-web compatibility claim."""
import ctypes as c
import json
from pathlib import Path
import subprocess
import sys
from inflate import Mailbox

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/build'))
from m1 import pinned_tools

def main():
    work=ROOT/'build/m74/browser-host';work.mkdir(parents=True,exist_ok=True);tools=pinned_tools()
    subprocess.run([tools['clang'],'--target=x86_64-pc-windows-msvc','-std=c17','-O2','-ffreestanding','-fno-builtin','-fno-stack-protector',
                    '-DENGINE_M74','-DAPP74_HOST','-I',str(ROOT/'include'),'-c',str(ROOT/'user/browser74.c'),'-o',str(work/'test.obj')],check=True,capture_output=True)
    subprocess.run([tools['lld-link'],'/dll','/noentry','/nodefaultlib','/machine:x64','/out:'+str(work/'test.dll'),str(work/'test.obj')],check=True,capture_output=True)
    dll=c.CDLL(str(work/'test.dll'));dll.browser74_event.argtypes=[c.POINTER(Mailbox),c.c_void_p]
    def session():
        m=Mailbox();m.magic=0x37415050;state=c.create_string_buffer(3*4096)
        def call(command,data=b'',key=0):
            m.command=command;m.bytes=len(data);m.key=key
            if data:c.memmove(c.addressof(m)+Mailbox.input.offset,data,len(data))
            dll.browser74_event(c.byref(m),state);assert not m.view.calc_error
            assert m.view.length<=7679 and m.view.cursor<=m.view.length and 0<=m.view.number<=16
            return c.string_at(c.addressof(m.view),m.view.length)
        return m,call
    source=b'<h1>Hello &amp; Engine OS</h1><style>hidden style</style><script>hidden script</script><p>A <a HREF="/next?a=1&amp;b=2">useful link</a>.</p><p>&#65; &#x42; &lt;text&gt;</p>'
    for size in [1,7,4096]:
        m,call=session();call(30,b'https://example.com/',2)
        for at in range(0,len(source),size):body=call(31,source[at:at+size],4 if at+size>=len(source) else 0)
        assert b'Hello & Engine OS' in body and b'useful link' in body and b'A B <text>' in body
        assert b'hidden' not in body and m.view.number==1
        original=body;call(32,key=1)
        link=c.string_at(c.addressof(m.view)+7680).split(b'\0')[0];assert link==b'/next?a=1&b=2'
        assert c.string_at(c.addressof(m.view),m.view.length)==original
        call(35);assert c.string_at(c.addressof(m.view)+7680)==b'useful link'
        call(30,b'https://example.com/next',1);call(33)
        assert c.string_at(c.addressof(m.view)+7680)==b'https://example.com/'
    m,call=session();call(30,b'engine://home',2)
    for _ in range(4):call(31,b'a'*4096)
    assert m.view.length==7679 and m.view.operation==1
    call(30,b'https://example.com/plain',1);text=call(31,b'<literal plain text>\nSecond line',6)
    assert text==b'<literal plain text>\nSecond line'
    out=ROOT/'evidence/M7.4';out.mkdir(exist_ok=True)
    (out/'browser-host-results.json').write_bytes((json.dumps({'passed':True,'host_only':True,'chunk_sizes':[1,7,4096],
        'checks':['entities and hrefs across chunk boundaries','script/style omission','labels and URLs without replacing page text','bounded history','visible truncation and plain text'],
        'limits':'basic static HTML/text, no JavaScript, forms, CSS layout or general modern-web compatibility'},indent=2)+'\n').encode('utf-8'))
    print('Basic HTML event worker PASS; native HTTPS navigation remains separate')

if __name__=='__main__':main()
