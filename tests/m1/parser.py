"""Exercise the exact C ELF parser used by the loader, with adversarial disk inputs."""
import ctypes
import json
from pathlib import Path
import shutil
import struct
import subprocess
ROOT=Path(__file__).resolve().parents[2]
def main():
    work=ROOT/'build/m1/elf-tests';work.mkdir(parents=True,exist_ok=True)
    source=work/'wrapper.c';source.write_text('#include <engine/elf.h>\n'
        '__declspec(dllexport) int validate(const uint8_t *p,uint64_t n){elf_plan plan;return elf_validate(p,n,&plan);}\n',encoding='utf-8')
    def command(args):subprocess.run([str(a) for a in args],check=True,capture_output=True,text=True,timeout=20)
    objects=[]
    for path in (source,ROOT/'runtime/memory.c'):
        obj=work/(path.stem+'.obj')
        command([shutil.which('clang'),'--target=x86_64-pc-windows-msvc','-ffreestanding','-fno-builtin',
                 '-fno-stack-protector','-std=c17','-Wall','-Wextra','-Werror','-I',ROOT/'include','-c',path,'-o',obj]);objects.append(obj)
    command([shutil.which('lld-link'),'/dll','/noentry','/nodefaultlib','/timestamp:0',f'/out:{work/"elf.dll"}',*objects])
    dll=ctypes.CDLL(str(work/'elf.dll'));dll.validate.argtypes=[ctypes.c_void_p,ctypes.c_uint64];dll.validate.restype=ctypes.c_int
    checks=[]
    def check(name,condition):checks.append({'name':name,'passed':bool(condition)})
    def validate(value,n=None):
        buf=ctypes.create_string_buffer(bytes(value));return dll.validate(buf,len(value) if n is None else n)
    base=bytearray(4097)
    struct.pack_into('<16sHHIQQQIHHHHHH',base,0,b'\x7fELF\x02\x01\x01'+bytes(9),2,62,1,0x200000,64,0,0,64,56,1,0,0,0)
    struct.pack_into('<IIQQQQQQ',base,64,1,5,4096,0x200000,0x200000,1,4096,4096);base[4096]=0xc3
    check('valid minimal ELF64',validate(base)==0)
    check('actual built kernel accepted',validate((ROOT/'build/m1/KERNEL.ELF').read_bytes())==0)
    check('null input rejected',dll.validate(None,64)==1)
    check('all truncated ELF headers rejected',all(validate(base,n)==1 for n in range(64)))
    cases=[('bad magic',0,'I',0,2),('wrong architecture',18,'H',183,2),('ET_DYN not supported',16,'H',3,2),
           ('ph table out of file',32,'Q',2**64-1,2),('too many segments',56,'H',33,2),
           ('entry outside executable bytes',24,'Q',0x200010,6),('RWX segment rejected',68,'I',7,4),
           ('file offset overflow',72,'Q',2**64-1,4),('filesz greater than memsz',96,'Q',4097,4),
           ('kernel outside bootstrap area',88,'Q',0x1000000,4),('mismatched physical virtual',80,'Q',0x300000,4),
           ('invalid alignment',112,'Q',3,4),('dynamic segment rejected',64,'I',2,3)]
    for name,offset,fmt,value,expected in cases:
        mutated=bytearray(base);struct.pack_into('<'+fmt,mutated,offset,value);check(name,validate(mutated)==expected)
    overlap=bytearray(base);struct.pack_into('<H',overlap,56,2);overlap[120:176]=overlap[64:120]
    check('page-overlapping segments rejected',validate(overlap)==5)
    check('truncated segment data rejected',validate(base,4096)==4)
    result={'scope':'actual loader ELF C parser on Windows host; not general ELF/Linux support',
            'passed':all(c['passed'] for c in checks),'checks':checks}
    evidence=ROOT/'evidence/M1';evidence.mkdir(parents=True,exist_ok=True)
    (evidence/'elf-results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(f'ELF parser: {len(checks)} checks, passed={result["passed"]}')
    if not result['passed']:print(json.dumps(result,indent=2));raise SystemExit(1)
if __name__=='__main__':main()
