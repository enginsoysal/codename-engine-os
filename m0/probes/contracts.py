"""Compile actual contract validator to a CRT-free DLL, exercise malformed boundaries."""
import ctypes
import json
from pathlib import Path
import random
import shutil
import struct
import subprocess
import zlib

ROOT = Path(__file__).resolve().parents[2]

def main():
    work = ROOT / 'build/m0-contracts'
    work.mkdir(parents=True, exist_ok=True)
    source = work / 'wrapper.c'
    source.write_text('#include "validate_v0.h"\n'
                      '__declspec(dllexport) int validate_boot(const uint8_t *p, uint64_t n) { return os_boot_validate(p,n); }\n'
                      '__declspec(dllexport) unsigned validate_buffer(const uint8_t *p, uint64_t n, uint64_t limit) { return os_user_buffer_validate(p,n,limit); }\n', encoding='utf-8')
    def run(args):
        subprocess.run([str(a) for a in args], check=True, capture_output=True, text=True, timeout=30)
    for target in ('x86_64-unknown-none-elf', 'aarch64-unknown-none-elf', 'i686-unknown-none-elf'):
        layout = work / 'layout.c'
        layout.write_text('#include "validate_v0.h"\n', encoding='utf-8')
        run([shutil.which('clang'), f'--target={target}', '-std=c17', '-ffreestanding', '-Wall', '-Wextra',
             '-Werror', '-I', ROOT / 'm0/contracts', '-fsyntax-only', layout])
    run([shutil.which('clang'), '--target=x86_64-pc-windows-msvc', '-std=c17', '-ffreestanding',
         '-fno-stack-protector', '-Wall', '-Wextra', '-Werror', '-I', ROOT / 'm0/contracts',
         '-c', source, '-o', work / 'wrapper.obj'])
    run([shutil.which('lld-link'), '/dll', '/noentry', '/nodefaultlib', '/timestamp:0',
         f'/out:{work / "contracts.dll"}', work / 'wrapper.obj'])
    dll = ctypes.CDLL(str(work / 'contracts.dll'))
    dll.validate_boot.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
    dll.validate_boot.restype = ctypes.c_int
    dll.validate_buffer.argtypes = [ctypes.c_void_p, ctypes.c_uint64, ctypes.c_uint64]
    dll.validate_buffer.restype = ctypes.c_uint
    checks = []
    def check(name, condition):
        checks.append({'name': name, 'passed': bool(condition)})
    def seal(value):
        struct.pack_into('<I', value, 24, 0)
        struct.pack_into('<I', value, 24, zlib.crc32(value))
        return value
    def boot(value, n=None):
        data = ctypes.create_string_buffer(bytes(value))
        return dll.validate_boot(data, len(value) if n is None else n)
    base = bytearray(160)
    struct.pack_into('<QII', base, 0, 0x304f464e49534f42, 0, 160)
    for i, (address, length) in enumerate([(0x1000,80),(0x200000,4096),(0x300000,512),(0,0),(0,0),(0x4000,32)]):
        struct.pack_into('<QQ', base, 32+i*16, address, length)
    struct.pack_into('<II', base, 128, 40, 1)
    seal(base)
    check('valid boot header', boot(base) == 0)
    check('CRC agrees with standard zlib vector', struct.unpack_from('<I', base,24)[0] == zlib.crc32(base[:24]+b'\0'*4+base[28:]))
    check('null boot pointer', dll.validate_boot(None,160) == 1)
    check('every truncated header rejected', all(boot(base, n) == 1 for n in range(160)))
    for name, offset, fmt, value, expected in [
        ('bad magic',0,'Q',0,2),('future version',8,'I',1,2),('wrong size',12,'I',159,2),
        ('unknown flags',16,'Q',1,2),('reserved field',28,'I',1,2),('range overflow',48,'Q',2**64-1,4),
        ('missing kernel',48,'Q',0,4),('overlapping kernel',48,'Q',0x1000,5),
        ('bad descriptor stride',128,'I',0,6),('bad descriptor version',132,'I',2,6),
        ('descriptor remainder',40,'Q',79,6),('reservation remainder',120,'Q',31,6),
        ('framebuffer fields without range',144,'I',1,7)]:
        value_bytes = bytearray(base)
        struct.pack_into('<'+fmt,value_bytes,offset,value)
        check(name, boot(seal(value_bytes)) == expected)
    corrupt = bytearray(base); corrupt[35] ^= 1
    check('corrupted checksum rejected',boot(corrupt)==3)
    video = bytearray(base)
    struct.pack_into('<QQ', video, 80, 0x800000, 640*480*4)
    struct.pack_into('<IIII', video, 136, 640*4, 2, 640, 480)
    check('valid framebuffer geometry',boot(seal(video))==0)
    struct.pack_into('<I',video,136,1)
    check('short framebuffer stride rejected',boot(seal(video))==7)
    def buffer(address,length,n=24,version=0,limit=2**47):
        value = ctypes.create_string_buffer(struct.pack('<IIQQ',24,version,address,length))
        return dll.validate_buffer(value,n,limit)
    check('valid user buffer',buffer(0x1000,4096)==0)
    check('user boundary exact end',buffer(2**47-1,1)==0)
    check('user boundary overflow',buffer(2**47-1,2)==1)
    check('zero length allowed',buffer(0,0)==0)
    check('null nonempty buffer rejected',buffer(0,1)==1)
    check('wraparound rejected',buffer(2**64-4,16)==1)
    check('32-bit profile bound',buffer(2**32-1,2,limit=2**32)==1)
    check('request version rejected',buffer(1,1,version=1)==1)
    check('request truncation rejected',all(buffer(1,1,n=n)==1 for n in range(24)))
    rng = random.Random(20261105)
    mutations = 0
    for _ in range(1000):
        value = bytearray(base)
        index = rng.randrange(160)
        value[index] ^= 1 << rng.randrange(8)
        if boot(value) == 0:
            raise AssertionError('Single-bit corruption accepted')
        mutations += 1
    check('1000 deterministic single-bit corruptions rejected',mutations==1000)
    result = {'scope':'actual C header validators; host execution, not kernel address validation',
              'passed':all(c['passed'] for c in checks),'checks':checks,
              'additional_inputs':{'truncated_boot':160,'truncated_requests':24,'mutations':mutations}}
    (ROOT/'evidence/M0/contract-results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result,indent=2))
    if not result['passed']:
        raise SystemExit(1)

if __name__=='__main__':
    main()
