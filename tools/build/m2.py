"""Build protected native usermode M2, preserving the separate M1 build path."""
from pathlib import Path
import hashlib
import json
import re
import shutil
import struct
from m1 import ROOT,pinned_tools,command,sha
from media import package
BUILD=ROOT/'build/m2'
def main():
    tools=pinned_tools();BUILD.mkdir(parents=True,exist_ok=True)
    sources=[]
    for folder in ('boot','kernel','include','runtime','user'):
        sources.extend(p for p in (ROOT/folder).rglob('*') if p.is_file())
    sources.extend(ROOT/p for p in ('include/engine/boot/boot_v0.h','include/engine/abi/syscall_v0.h','include/engine/boot/validate_v0.h',
                                   'tools/build/m2.py','tools/build/m1.py','tools/build/media.py'))
    hashes={p.relative_to(ROOT).as_posix():sha(p) for p in sorted(sources)}
    build_id=hashlib.sha256(json.dumps(hashes,sort_keys=True).encode()).hexdigest()[:32]
    (BUILD/'build.h').write_text(f'#define BUILD_ID "{build_id}"\n',encoding='utf-8')
    debug=['-g','-fdebug-compilation-dir=.']
    for original,replacement in ((ROOT,'.'),(Path(tools['clang']).parent.parent,'/toolchain/llvm')):
        for spelling in (str(original),original.as_posix()):debug.extend((f'-ffile-prefix-map={spelling}={replacement}',f'-fdebug-prefix-map={spelling}={replacement}'))
    common=['-std=c17','-O2',*debug,'-ffreestanding','-fno-builtin','-fno-stack-protector','-mno-red-zone','-mgeneral-regs-only',
            '-Wall','-Wextra','-Werror','-DENGINE_M2','-I',ROOT/'include','-I',BUILD]
    def compile_file(path,kind,assembly=False):
        path=Path(path);obj=BUILD/(path.stem+'-'+kind+'.o')
        target='x86_64-pc-windows-msvc' if kind=='efi' else 'x86_64-unknown-none-elf'
        flags=debug if assembly else common+([] if kind=='efi' else ['-fno-pic','-fno-pie','-fno-asynchronous-unwind-tables'])
        if kind=='user':flags=flags+['-mcmodel=large']
        command([tools['clang'],'--target='+target,*flags,'-c',path.as_posix(),'-o',obj]);return obj
    user=compile_file('user/probe.c','user')
    command([tools['ld.lld'],'-T',ROOT/'user/linker.ld','--build-id=none','-o',BUILD/'USER.ELF',user])
    data=(BUILD/'USER.ELF').read_bytes()
    # Keep the app as an independent ELF artifact; only its exact bytes are embedded in rodata.
    embed='#include <stdint.h>\nconst uint8_t user_program_elf[] __attribute__((aligned(8)))={\n'
    embed+='\n'.join(','.join(str(x) for x in data[i:i+32])+',' for i in range(0,len(data),32))
    embed+=f'\n}};\nconst uint64_t user_program_size={len(data)};\n'
    (BUILD/'user-image.c').write_text(embed,encoding='utf-8')
    efi=[compile_file(p,'efi',p.endswith('.S')) for p in ('boot/uefi/loader.c','runtime/memory.c','boot/uefi/handoff.S')]
    command([tools['lld-link'],'/subsystem:efi_application','/entry:efi_main','/nodefaultlib','/timestamp:0',
             '/machine:x64',f'/out:{BUILD/"BOOTX64.EFI"}',*efi])
    lines=['.text']
    for i in range(256):
        lines.extend((f'.globl isr_{i}',f'isr_{i}:'))
        if i not in (8,10,11,12,13,14,17,21,29,30):lines.append('pushq $0')
        lines.extend((f'pushq ${i}','jmp exception_common'))
    lines.extend(('exception_common:','cld'))
    regs=('rax','rbx','rcx','rdx','rsi','rdi','rbp','r8','r9','r10','r11','r12','r13','r14','r15')
    lines.extend('pushq %'+r for r in regs)
    lines.extend(('movq %rsp,%rdi','andq $-16,%rsp','call exception_dispatch','movq %rax,%rsp','jmp restore_user',
                  '.globl arch_schedule_first','arch_schedule_first:','movabsq $0xffff800000005000,%rsp','xorq %rbp,%rbp',
                  'call process_first_context','movq %rax,%rdi',
                  '.globl arch_enter_user','arch_enter_user:','movq %rdi,%rsp','restore_user:'))
    lines.extend('popq %'+r for r in reversed(regs));lines.extend(('addq $16,%rsp','iretq','.section .rodata','.p2align 3',
                                                                 '.globl exception_stubs','exception_stubs:'))
    lines.extend(f'.quad isr_{i}' for i in range(256));lines.append('.section .note.GNU-stack,"",@progbits')
    (BUILD/'interrupt-stubs.S').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    inputs=('kernel/core/main.c','kernel/core/display.c','kernel/arch/x86_64/interrupts.c','kernel/arch/x86_64/fault.S',
            'runtime/memory.c','kernel/memory/pages.c','kernel/memory/space.c','kernel/process/process.c',
            'build/m2/user-image.c','build/m2/interrupt-stubs.S')
    kernel=[compile_file(p,'kernel',p.endswith('.S')) for p in inputs]
    command([tools['ld.lld'],'-T',ROOT/'kernel/arch/x86_64/linker.ld','--build-id=none','-Map='+str(BUILD/'kernel.map'),
             '-o',BUILD/'KERNEL.ELF',*kernel])
    nm=Path(tools['clang']).parent/'llvm-nm.exe'
    if command([nm,'-u',BUILD/'KERNEL.ELF']).strip():raise RuntimeError('Undefined kernel symbols')
    map_path=BUILD/'kernel.map';map_path.write_text(map_path.read_text(encoding='utf-8').replace(str(ROOT),'.').replace(ROOT.as_posix(),'.'),encoding='utf-8')
    init=struct.pack('<8sII32s',b'ENGINIT1',48,0,build_id.encode());(BUILD/'INIT.BIN').write_bytes(init)
    package(BUILD/'normal',(BUILD/'BOOTX64.EFI').read_bytes(),(BUILD/'KERNEL.ELF').read_bytes(),init,'M2')
    dist=ROOT/'dist';dist.mkdir(exist_ok=True)
    for name in ('EngineOS-M2.iso','EngineOS-M2-usb.img'):shutil.copyfile(BUILD/'normal'/name,dist/name)
    for filename,name in (('KERNEL.ELF','EngineOS-M2-kernel.elf'),('kernel.map','EngineOS-M2-kernel.map'),('USER.ELF','EngineOS-M2-user.elf')):
        shutil.copyfile(BUILD/filename,dist/name)
    artifacts={p.name:{'sha256':sha(p),'size':p.stat().st_size} for p in sorted(dist.glob('EngineOS-M2*')) if p.is_file()}
    for name in artifacts:
        if re.search(rb'[Cc]:[\\/]+Users[\\/]+',(dist/name).read_bytes()):raise RuntimeError('Host user path in '+name)
    manifest={'milestone':'M2','build_id':build_id,'source_sha256':hashes,'artifacts':artifacts,'mode':'normal',
              'limits':['one active CPU; cooperative trusted native probes','no Windows apps, installer or preemption',
                        'early allocator manages conventional memory below 1 GiB']}
    (dist/'m2-build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    (dist/'M2-SHA256SUMS.txt').write_text(''.join(f'{v["sha256"]}  {k}\n' for k,v in artifacts.items()),encoding='utf-8')
    print(json.dumps({'build_id':build_id,'iso':'dist/EngineOS-M2.iso'},indent=2));return manifest
if __name__=='__main__':main()
