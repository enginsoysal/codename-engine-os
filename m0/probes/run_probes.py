"""M0 toolchain/reference probes. Does not build or boot an operating system."""
import ctypes
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import threading

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'evidence' / 'M0'
OUT.mkdir(parents=True, exist_ok=True)
results = {'scope': 'cross-compile/link and Windows reference semantics; no OS runtime proof',
           'tools': {}, 'checks': [], 'commands': []}
for name in ('clang', 'ld.lld', 'lld-link', 'python', 'lldb', 'qemu-system-x86_64', 'xorriso', 'rustc'):
    path = shutil.which(name)
    results['tools'][name] = {'path': path}
    if path and name in ('clang', 'ld.lld', 'lld-link', 'python'):
        p = subprocess.run([path, '--version'], capture_output=True, text=True)
        results['tools'][name]['version'] = (p.stdout or p.stderr).splitlines()[0]

def check(name, condition, detail):
    results['checks'].append({'name': name, 'passed': bool(condition), 'detail': detail})

def run(args):
    p = subprocess.run(args, capture_output=True, text=True)
    results['commands'].append({'argv': args, 'exit_code': p.returncode,
                                'stdout': p.stdout, 'stderr': p.stderr})
    if p.returncode:
        raise RuntimeError(f'command failed: {args[0]}: {p.stderr}')

try:
    with tempfile.TemporaryDirectory(prefix='os-m0-') as temp:
        d = Path(temp)
        src = d / 'probe.c'
        src.write_text('typedef __UINT64_TYPE__ u64;\n'
                       '_Static_assert(sizeof(void*) == 8, "64-bit target");\n'
                       '__attribute__((visibility("default"))) u64 probe(u64 x) { return x + 7; }\n', encoding='utf-8')
        clang = results['tools']['clang']['path']
        linker = results['tools']['ld.lld']['path']
        layout = d / 'layout.c'
        layout.write_text('#include "boot_v0.h"\n#include "syscall_v0.h"\n', encoding='utf-8')
        for target in ('x86_64-unknown-none-elf', 'aarch64-unknown-none-elf', 'i686-unknown-none-elf'):
            run([clang, f'--target={target}', '-std=c17', '-ffreestanding', '-Wall', '-Wextra',
                 '-Werror', '-I', str(ROOT / 'm0' / 'contracts'), '-fsyntax-only', str(layout)])
            check(f'{target} contract layout assertions', True, 'boot=160, request=24; fixed-width fields')
        for target, machine in [('x86_64-unknown-none-elf', 62), ('aarch64-unknown-none-elf', 183)]:
            obj, elf = d / f'{machine}.o', d / f'{machine}.elf'
            flags = ['-mno-red-zone', '-mgeneral-regs-only'] if machine == 62 else ['-mgeneral-regs-only']
            run([clang, f'--target={target}', '-std=c17', '-ffreestanding', '-fno-stack-protector',
                 '-fno-pic', '-fno-pie', '-Wall', '-Wextra', '-Werror', *flags, '-c', str(src), '-o', str(obj)])
            run([linker, '-e', 'probe', '-Ttext=0x200000', str(obj), '-o', str(elf)])
            data = elf.read_bytes()
            check(f'{target} ELF header', data[:4] == b'\x7fELF' and data[4:6] == b'\x02\x01'
                  and struct.unpack_from('<H', data, 18)[0] == machine,
                  {'machine': machine, 'sha256': hashlib.sha256(data).hexdigest(), 'size': len(data)})
        efi_src, efi_obj, efi = d / 'efi.c', d / 'efi.obj', d / 'probe.efi'
        efi_src.write_text('typedef __UINTPTR_TYPE__ status;\n'
                          'status efi_main(void *image, void *table) { (void)image; (void)table; return 0; }\n', encoding='utf-8')
        run([clang, '--target=x86_64-pc-windows-msvc', '-std=c17', '-ffreestanding',
             '-fno-stack-protector', '-mno-red-zone', '-Wall', '-Wextra', '-Werror',
             '-c', str(efi_src), '-o', str(efi_obj)])
        run([results['tools']['lld-link']['path'], '/subsystem:efi_application', '/entry:efi_main',
             '/nodefaultlib', '/machine:x64', '/timestamp:0', f'/out:{efi}', str(efi_obj)])
        data = efi.read_bytes()
        pe = struct.unpack_from('<I', data, 0x3c)[0]
        check('UEFI PE32+ image header', data[pe:pe+4] == b'PE\0\0'
              and struct.unpack_from('<H', data, pe+4)[0] == 0x8664
              and struct.unpack_from('<H', data, pe+24)[0] == 0x20b
              and struct.unpack_from('<H', data, pe+24+68)[0] == 10,
              {'sha256': hashlib.sha256(data).hexdigest(), 'size': len(data),
               'note': 'format only; not loaded by firmware'})

        if os.name == 'nt':
            from ctypes import wintypes as w
            k = ctypes.WinDLL('kernel32', use_last_error=True)
            k.CreateEventW.argtypes = [ctypes.c_void_p, w.BOOL, w.BOOL, w.LPCWSTR]
            k.CreateEventW.restype = w.HANDLE
            k.WaitForSingleObject.argtypes = [w.HANDLE, w.DWORD]
            k.WaitForSingleObject.restype = w.DWORD
            k.CloseHandle.argtypes = [w.HANDLE]
            k.CloseHandle.restype = w.BOOL
            k.SetEvent.argtypes = [w.HANDLE]
            k.SetEvent.restype = w.BOOL
            k.WaitForMultipleObjects.argtypes = [w.DWORD, ctypes.POINTER(w.HANDLE), w.BOOL, w.DWORD]
            k.WaitForMultipleObjects.restype = w.DWORD
            for manual in (False, True):
                event = k.CreateEventW(None, manual, True, None)
                if not event:
                    raise ctypes.WinError(ctypes.get_last_error())
                try:
                    first = k.WaitForSingleObject(event, 0)
                    second = k.WaitForSingleObject(event, 0)
                    expected = 0 if manual else 258
                    check(f'Windows {"manual" if manual else "auto"}-reset event',
                          first == 0 and second == expected, {'first': first, 'second': second})
                finally:
                    k.CloseHandle(event)
            events = [k.CreateEventW(None, True, False, None) for _ in range(2)]
            try:
                if not all(events):
                    raise ctypes.WinError(ctypes.get_last_error())
                array = (w.HANDLE * 2)(*events)
                empty = k.WaitForMultipleObjects(2, array, False, 0)
                if not k.SetEvent(events[1]):
                    raise ctypes.WinError(ctypes.get_last_error())
                any_second = k.WaitForMultipleObjects(2, array, False, 0)
                all_partial = k.WaitForMultipleObjects(2, array, True, 0)
                if not k.SetEvent(events[0]):
                    raise ctypes.WinError(ctypes.get_last_error())
                all_ready = k.WaitForMultipleObjects(2, array, True, 0)
                check('Windows multiwait any/all', (empty, any_second, all_partial, all_ready) == (258, 1, 258, 0),
                      {'empty': empty, 'any_second': any_second, 'all_partial': all_partial, 'all_ready': all_ready})
            finally:
                for event in events:
                    if event:
                        k.CloseHandle(event)
            k.TlsAlloc.restype = w.DWORD
            k.TlsSetValue.argtypes = [w.DWORD, ctypes.c_void_p]
            k.TlsSetValue.restype = w.BOOL
            k.TlsGetValue.argtypes = [w.DWORD]
            k.TlsGetValue.restype = ctypes.c_void_p
            k.TlsFree.argtypes = [w.DWORD]
            k.TlsFree.restype = w.BOOL
            slot = k.TlsAlloc()
            if slot == 0xffffffff:
                raise ctypes.WinError(ctypes.get_last_error())
            worker_values = []
            try:
                if not k.TlsSetValue(slot, ctypes.c_void_p(0x1111)):
                    raise ctypes.WinError(ctypes.get_last_error())
                def worker():
                    worker_values.append(k.TlsGetValue(slot))
                    worker_values.append(bool(k.TlsSetValue(slot, ctypes.c_void_p(0x2222))))
                    worker_values.append(k.TlsGetValue(slot))
                thread = threading.Thread(target=worker)
                thread.start()
                thread.join(timeout=5)
                main_value = k.TlsGetValue(slot)
                check('Windows per-thread TLS', not thread.is_alive() and worker_values == [None, True, 0x2222]
                      and main_value == 0x1111, {'worker': worker_values, 'main': main_value})
            finally:
                k.TlsFree(slot)
            k.CreateFileW.argtypes = [w.LPCWSTR, w.DWORD, w.DWORD, ctypes.c_void_p,
                                      w.DWORD, w.DWORD, w.HANDLE]
            k.CreateFileW.restype = w.HANDLE
            path = d / 'sharing-reference.txt'
            path.write_text('M0 reference', encoding='utf-8')
            invalid = ctypes.c_void_p(-1).value
            first = k.CreateFileW(str(path), 0x80000000, 0, None, 3, 0x80, None)
            if first == invalid:
                raise ctypes.WinError(ctypes.get_last_error())
            try:
                ctypes.set_last_error(0)
                second = k.CreateFileW(str(path), 0x80000000, 1, None, 3, 0x80, None)
                error = ctypes.get_last_error()
                if second != invalid:
                    k.CloseHandle(second)
                check('Windows exclusive file sharing', second == invalid and error == 32,
                      {'second_open_error': error})
            finally:
                k.CloseHandle(first)
except Exception as exc:
    results['error'] = str(exc)
finally:
    results['passed'] = not results.get('error') and bool(results['checks']) and all(c['passed'] for c in results['checks'])
    # Redact path values recursively before serialization, preserving JSON escaping.
    replacements = [(str(ROOT), '<project-root>'), (str(Path.home()), '<user-home>')]
    if 'temp' in globals():
        replacements.insert(0, (str(temp), '<temporary-probe-directory>'))
    def public_value(value):
        if isinstance(value, str):
            for original, replacement in replacements:
                value = value.replace(original, replacement).replace(original.replace('\\', '/'), replacement)
            return value
        if isinstance(value, list):
            return [public_value(item) for item in value]
        if isinstance(value, dict):
            return {key: public_value(item) for key, item in value.items()}
        return value
    (OUT / 'probe-results.json').write_text(json.dumps(public_value(results), indent=2), encoding='utf-8')
    print(json.dumps({'passed': results['passed'], 'checks': results['checks'], 'error': results.get('error')}, indent=2))
if not results['passed']:
    raise SystemExit(1)
