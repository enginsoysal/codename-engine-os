"""Build real M1 loader/kernel and normal boot media, without a host CRT."""
from pathlib import Path
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import sys
from media import package

ROOT=Path(__file__).resolve().parents[2]
BUILD=ROOT/'build/m1'

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def pinned_tools():
    manifest=json.loads((ROOT/'tools/toolchain-lock.json').read_text(encoding='utf-8'))
    tools={}
    for tool in ('clang','lld-link','ld.lld'):
        path=shutil.which(tool)
        if not path or sha(path)!=manifest['files'][tool]['sha256']:raise RuntimeError(f'Pinned {tool} missing or changed')
        tools[tool]=path
    return tools
def command(argv):
    process=subprocess.run([str(a) for a in argv],cwd=ROOT,capture_output=True,text=True,timeout=60)
    if process.returncode:raise RuntimeError(process.stderr.replace(str(ROOT),'<project-root>'))
    return process.stdout
def main():
    tools=pinned_tools();BUILD.mkdir(parents=True,exist_ok=True)
    sources=[]
    for folder in ('boot','kernel','include','runtime'):
        sources.extend(p for p in (ROOT/folder).rglob('*') if p.is_file())
    sources.extend(ROOT/p for p in ('include/engine/boot/boot_v0.h','include/engine/abi/syscall_v0.h','include/engine/boot/validate_v0.h','tools/build/m1.py','tools/build/media.py'))
    source_hashes={p.relative_to(ROOT).as_posix():sha(p) for p in sorted(sources)}
    build_id=hashlib.sha256(json.dumps(source_hashes,sort_keys=True).encode()).hexdigest()[:32]
    (BUILD/'build.h').write_text(f'#define BUILD_ID "{build_id}"\n',encoding='utf-8')
    debug_flags=['-g','-fdebug-compilation-dir=.']
    for original,replacement in [(ROOT,'.'),(Path(tools['clang']).parent.parent,'/toolchain/llvm')]:
        for spelling in (str(original),original.as_posix()):
            debug_flags += [f'-ffile-prefix-map={spelling}={replacement}',f'-fdebug-prefix-map={spelling}={replacement}']
    common=['-std=c17','-O2',*debug_flags,'-ffreestanding','-fno-builtin','-fno-stack-protector','-mno-red-zone','-mgeneral-regs-only',
            '-Wall','-Wextra','-Werror','-I',ROOT/'include','-I',BUILD]
    loader_objects=[]
    for filename in ('boot/uefi/loader.c','runtime/memory.c','boot/uefi/handoff.S'):
        obj=BUILD/(Path(filename).stem+'-efi.obj')
        flags=common if filename.endswith('.c') else debug_flags
        command([tools['clang'],'--target=x86_64-pc-windows-msvc',*flags,'-c',filename,'-o',obj]);loader_objects.append(obj)
    command([tools['lld-link'],'/subsystem:efi_application','/entry:efi_main','/nodefaultlib','/timestamp:0',
             '/machine:x64',f'/out:{BUILD/"BOOTX64.EFI"}',*loader_objects])
    stubs=['.text']
    errors={8,10,11,12,13,14,17,21,29,30}
    for i in range(256):
        stubs += [f'.globl isr_{i}',f'isr_{i}:']
        if i not in errors:stubs+=['pushq $0']
        stubs += [f'pushq ${i}','jmp exception_common']
    stubs+=['exception_common:','cld']
    for reg in ('rax','rbx','rcx','rdx','rsi','rdi','rbp','r8','r9','r10','r11','r12','r13','r14','r15'):stubs += [f'pushq %{reg}']
    stubs+=['movq %rsp,%rdi','andq $-16,%rsp','call exception_dispatch','cli','1: hlt','jmp 1b',
            '.section .rodata','.p2align 3','.globl exception_stubs','exception_stubs:']
    stubs += [f'.quad isr_{i}' for i in range(256)];stubs+=['.section .note.GNU-stack,"",@progbits']
    (BUILD/'interrupt-stubs.S').write_text('\n'.join(stubs)+'\n',encoding='utf-8')
    kernel_objects=[]
    inputs=[ROOT/p for p in ('kernel/core/main.c','kernel/core/display.c','kernel/arch/x86_64/interrupts.c',
                             'runtime/memory.c','kernel/arch/x86_64/fault.S')]+[BUILD/'interrupt-stubs.S']
    for path in inputs:
        obj=BUILD/(path.stem+'-kernel.o')
        flags=[*common,'-fno-pic','-fno-pie','-fno-asynchronous-unwind-tables'] if path.suffix=='.c' else debug_flags
        command([tools['clang'],'--target=x86_64-unknown-none-elf',*flags,'-c',path.relative_to(ROOT).as_posix(),'-o',obj]);kernel_objects.append(obj)
    command([tools['ld.lld'],'-T',ROOT/'kernel/arch/x86_64/linker.ld','-Map='+str(BUILD/'kernel.map'),
             '--build-id=none','-o',BUILD/'KERNEL.ELF',*kernel_objects])
    undefined=command([str(Path(tools['clang']).parent/'llvm-nm.exe'),'-u',BUILD/'KERNEL.ELF']).strip()
    if undefined:raise RuntimeError('Undefined kernel symbols: '+undefined)
    map_path=BUILD/'kernel.map'
    map_text=map_path.read_text(encoding='utf-8').replace(str(ROOT),'.').replace(ROOT.as_posix(),'.')
    map_path.write_text(map_text,encoding='utf-8')
    for path in (BUILD/'KERNEL.ELF',BUILD/'BOOTX64.EFI',map_path):
        contents=path.read_bytes().lower()
        if re.search(rb'c:[\\/]+users[\\/]+',contents):raise RuntimeError('Host user path remains in build artifact')
    init=struct.pack('<8sII32s',b'ENGINIT1',48,0,build_id.encode('ascii'))
    (BUILD/'INIT.BIN').write_bytes(init)
    package(BUILD/'normal',(BUILD/'BOOTX64.EFI').read_bytes(),(BUILD/'KERNEL.ELF').read_bytes(),init)
    dist=ROOT/'dist';dist.mkdir(exist_ok=True)
    for name in ('EngineOS-M1.iso','EngineOS-M1-usb.img'):
        shutil.copyfile(BUILD/'normal'/name,dist/name)
    for source,name in [('KERNEL.ELF','EngineOS-M1-kernel.elf'),('kernel.map','EngineOS-M1-kernel.map')]:shutil.copyfile(BUILD/source,dist/name)
    manifest={'milestone':'M1','build_id':build_id,'mode':'normal','source_sha256':source_hashes,
              'artifacts':{p.name:{'sha256':sha(p),'size':p.stat().st_size} for p in sorted(dist.glob('EngineOS-M1*')) if p.is_file()},
              'limits':['UEFI x86-64 only','bootstrap identity map below 4 GiB','no installer, user processes, drivers or Windows apps']}
    (dist/'build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'build_id':build_id,'iso':'dist/EngineOS-M1.iso','kernel':'dist/EngineOS-M1-kernel.elf'},indent=2))
    return manifest
if __name__=='__main__':main()
