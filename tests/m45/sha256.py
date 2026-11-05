"""Run the exact freestanding C SHA-256 through a no-CRT Windows DLL and compare independent digests."""
from pathlib import Path
import ctypes,hashlib,json,shutil,subprocess
ROOT=Path(__file__).resolve().parents[2]
def main():
    work=ROOT/'build/m45/sha-test';work.mkdir(parents=True,exist_ok=True);clang=Path(shutil.which('clang'));link=clang.parent/'lld-link.exe';objects=[]
    for name in ('sha256','memory'):
        obj=work/(name+'.obj');subprocess.run([str(clang),'--target=x86_64-pc-windows-msvc','-O2','-ffreestanding','-fno-builtin','-fno-stack-protector','-Iinclude','-c','runtime/'+name+'.c','-o',str(obj)],cwd=ROOT,check=True,capture_output=True);objects.append(obj)
    dll=work/'sha.dll';subprocess.run([str(link),'/dll','/noentry','/nodefaultlib','/export:engine_sha256','/out:'+str(dll),*map(str,objects)],cwd=ROOT,check=True,capture_output=True)
    api=ctypes.CDLL(str(dll));api.engine_sha256.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_void_p];results=[]
    cases=[b'',b'abc',b'a'*1000000]+[bytes((i*17+3)&255 for i in range(n)) for n in (1,55,56,63,64,65,119,120,127,128,129,4096)]
    for index,data in enumerate(cases):
        output=ctypes.create_string_buffer(32);source=ctypes.create_string_buffer(data);api.engine_sha256(source,len(data),output);passed=output.raw==hashlib.sha256(data).digest();results.append({'case':index,'bytes':len(data),'passed':passed})
    out=ROOT/'evidence/M4.5';out.mkdir(parents=True,exist_ok=True);(out/'sha256-results.json').write_bytes((json.dumps({'checks':results,'source_sha256':hashlib.sha256((ROOT/'runtime/sha256.c').read_bytes()).hexdigest()},indent=2)+'\n').encode())
    assert all(r['passed'] for r in results);print('SHA-256 PASS: '+str(len(results))+' exact-C boundary/known-vector comparisons')
if __name__=='__main__':main()
