"""Exercise actual freestanding TLS policy with independent Python/OpenSSL peers.

Private fixture keys and test trust anchors stay in ignored build output. This
host adapter does not qualify native TCP, clock hardware or actual downloads.
"""
import ctypes as c
from datetime import datetime,timedelta,timezone
import importlib.util
import json
from pathlib import Path
import ssl
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/build'))
from m1 import pinned_tools
from bearssl import objects


def main():
    import cryptography
    from cryptography import x509
    from cryptography.hazmat.primitives import hashes,serialization
    from cryptography.hazmat.primitives.asymmetric import rsa
    from cryptography.x509.oid import NameOID
    lock=json.loads((ROOT/'tools/security-toolchain.json').read_text(encoding='utf-8'))
    assert cryptography.__version__==lock['cryptography_version']
    work=ROOT/'build/m7/tls-test';work.mkdir(parents=True,exist_ok=True)
    now=datetime(2026,11,7,12,tzinfo=timezone.utc)
    root_key=rsa.generate_private_key(public_exponent=65537,key_size=2048)
    name=x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'Engine OS disposable TLS test CA')])
    root=x509.CertificateBuilder().subject_name(name).issuer_name(name).public_key(root_key.public_key()).serial_number(1).not_valid_before(now-timedelta(days=5)).not_valid_after(now+timedelta(days=5)).add_extension(x509.BasicConstraints(ca=True,path_length=0),critical=True).sign(root_key,hashes.SHA256())
    numbers=root_key.public_key().public_numbers()
    arrays=[]
    def array(name,data):
        arrays.append('static unsigned char '+name+'[]={'+','.join(map(str,data))+'};')
        return name+','+str(len(data))
    dn=array('dn',root.subject.public_bytes())
    modulus=array('modulus',numbers.n.to_bytes(256,'big'));exponent=array('exponent',b'\x01\x00\x01')
    arrays.append('static const br_x509_trust_anchor root7_anchors[]={{{'+dn+'},BR_X509_TA_CA,{BR_KEYTYPE_RSA,{.rsa={'+modulus+','+exponent+'}}}}};\n#define ROOT7_COUNT 1\n')
    (work/'trust7.h').write_bytes(('\n'.join(arrays)).encode())
    adapter=work/'adapter.c'
    adapter.write_text('''#include <engine/network/tcp.h>
#include <engine/base.h>
void *memmove(void *,const void *,size_t);
unsigned transport=2,entropy=1,out_size,in_size,closed;
uint8_t outgoing[65536],incoming[65536];
void net7_schedule(void){}
unsigned net7_random(void *p,unsigned n){uint8_t *b=p;for(unsigned i=0;i<n;++i)b[i]=(uint8_t)(i*131+17);return entropy;}
unsigned tcp7_connect(uint32_t a,unsigned p){(void)a;(void)p;transport=2;closed=0;return 0;}
unsigned tcp7_status(void){return transport;}
void tcp7_abort(void){out_size=in_size=0;closed=1;}
void tcp7_close(void){closed=1;}
unsigned tcp7_write(const void *p,unsigned n){if(n>1460)n=1460;if(n>65536-out_size)return 0;memcpy(outgoing+out_size,p,n);out_size+=n;return n;}
unsigned tcp7_read(void *p,unsigned n){if(n>in_size)n=in_size;memcpy(p,incoming,n);memmove(incoming,incoming+n,in_size-n);in_size-=n;return n;}
''',encoding='utf-8')
    tools=pinned_tools()
    flags=['-std=c17','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin','-fno-stack-protector','-DENGINE_M7',
           '-I',ROOT/'include','-I',ROOT/'third_party/bearssl/inc','-I',work]
    target='x86_64-pc-windows-msvc'
    dependency=objects(work/'bearssl',tools,flags,target=target)
    def run(args):
        result=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True)
        if result.returncode:raise RuntimeError(result.stderr.replace(str(ROOT),'<project-root>'))
    own=[]
    for n,path in enumerate([ROOT/'kernel/network/tls.c',ROOT/'runtime/memory.c',adapter]):
        obj=work/('own'+str(n)+'.obj');run([tools['clang'],'--target='+target,*flags,'-c',path,'-o',obj]);own.append(obj)
    exports=['tls7_'+n for n in ['start','status','error','pump','write','read','close','abort']]+['transport','entropy','outgoing','incoming','out_size','in_size','closed']
    link=['/dll','/noentry','/nodefaultlib',*['/export:'+n for n in exports],*own,*dependency,'/out:'+str(work/'tls.dll')]
    response=work/'link.rsp';response.write_text(subprocess.list2cmdline(list(map(str,link))),encoding='utf-8')
    run([tools['lld-link'],'@'+str(response)])
    lib=c.CDLL(str(work/'tls.dll'));lib.tls7_start.argtypes=[c.c_char_p,c.c_uint32,c.c_uint,c.c_uint32,c.c_uint32]
    for n in ['write','read']:getattr(lib,'tls7_'+n).argtypes=[c.c_void_p,c.c_uint]
    globals_={n:c.c_uint.in_dll(lib,n) for n in ['transport','entropy','out_size','in_size','closed']}
    outgoing=(c.c_ubyte*65536).in_dll(lib,'outgoing');incoming=(c.c_ubyte*65536).in_dll(lib,'incoming')
    days=now.date().toordinal()+365
    results=[]
    for scenario in ['valid','wrong-hostname','untrusted','expired','future-clock','missing-clock','missing-entropy','truncated-transport']:
        lib.tls7_abort();globals_['entropy'].value=scenario!='missing-entropy'
        key=rsa.generate_private_key(public_exponent=65537,key_size=2048)
        issuer_key=key if scenario=='untrusted' else root_key
        subject=x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'downloads.engine.test')])
        cert=x509.CertificateBuilder().subject_name(subject).issuer_name(subject if scenario=='untrusted' else name).public_key(key.public_key()).serial_number(x509.random_serial_number()).not_valid_before(now-timedelta(days=3)).not_valid_after(now-timedelta(days=1) if scenario=='expired' else now+timedelta(days=2)).add_extension(x509.BasicConstraints(ca=False,path_length=None),critical=True).add_extension(x509.SubjectAlternativeName([x509.DNSName('downloads.engine.test')]),critical=False).sign(issuer_key,hashes.SHA256())
        certfile=work/'peer.pem';keyfile=work/'peer-private.pem'
        certfile.write_bytes(cert.public_bytes(serialization.Encoding.PEM))
        keyfile.write_bytes(key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.NoEncryption()))
        context=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);context.minimum_version=context.maximum_version=ssl.TLSVersion.TLSv1_2
        context.load_cert_chain(certfile,keyfile);recv=ssl.MemoryBIO();send=ssl.MemoryBIO();peer=context.wrap_bio(recv,send,server_side=True)
        host=b'wrong.engine.test' if scenario=='wrong-hostname' else b'downloads.engine.test'
        start=lib.tls7_start(host,0x0a000202,443,0 if scenario=='missing-clock' else days+(10 if scenario=='future-clock' else 0),43200)
        established=False;received=bytearray();response_sent=False;reply=bytearray();request=b'GET /file.txt HTTP/1.1\r\nHost: downloads.engine.test\r\n\r\n'
        remaining=request
        for step in range(1000):
            lib.tls7_pump()
            n=globals_['out_size'].value
            if n:recv.write(bytes(outgoing[:n]));globals_['out_size'].value=0
            try:
                if not established:peer.do_handshake();established=True
                if established:
                    try:received.extend(peer.read(8192))
                    except ssl.SSLWantReadError:pass
                    if bytes(received)==request and not response_sent:
                        peer.write(b'HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nhello engine');response_sent=True
            except (ssl.SSLWantReadError,ssl.SSLWantWriteError):pass
            except ssl.SSLError:
                # A rejected peer may respond to the TLS alert. Client state is authoritative.
                pass
            data=send.read()
            if data:
                n=globals_['in_size'].value;assert n+len(data)<=65536
                c.memmove(c.addressof(incoming)+n,data,len(data));globals_['in_size'].value+=len(data)
            if lib.tls7_status()==2:
                if scenario=='truncated-transport':globals_['transport'].value=3
                if remaining:
                    n=lib.tls7_write(remaining,len(remaining));remaining=remaining[n:]
                out=c.create_string_buffer(8192);n=lib.tls7_read(out,8192);reply.extend(out.raw[:n])
            if scenario=='valid' and reply.endswith(b'hello engine'):break
            if lib.tls7_status()==4:break
        if scenario=='valid':
            assert lib.tls7_status()==2 and bytes(received)==request and reply.endswith(b'hello engine'),(scenario,step,lib.tls7_status(),lib.tls7_error())
        else:assert lib.tls7_status()==4,(scenario,step,lib.tls7_status(),lib.tls7_error())
        results.append({'scenario':scenario,'passed':True,'tls_error':lib.tls7_error(),'start_refused':bool(start)})
    report={'passed':True,'m7_complete':False,'scope':'actual TLS wrapper and BearSSL with disposable test-only trust, independent Python/OpenSSL memory peer; no native TCP/RTC/download claim','production_test_roots_added':False,'cases':results}
    (ROOT/'evidence/M7/tls-policy-results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('TLS policy PASS: '+str(len(results))+' actual cryptographic cases; native HTTPS remains pending')


if __name__=='__main__':main()
