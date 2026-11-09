"""Build M2/M3 native kernels, preserving the separate M1 build path."""
from pathlib import Path
import hashlib
import json
import re
import shutil
import struct
from m1 import ROOT,pinned_tools,command,sha
from media import package
BUILD=ROOT/'build/m2'
def main(milestone='M2',m5_live=False,distro=None,m6=False,m6_revision="standard",m7=False,m71=False,m72=False,m73=False,m74=False,m75=False,m8=False,m9=False,m92=False):
    global BUILD
    m9=m9 or m92
    m8=m8 or m9
    m75=m75 or m8
    m74=m74 or m75
    m73=m73 or m74
    m72=m72 or m73
    m71=m71 or m72
    m7=m7 or m71
    m6=m6 or m7
    flavor='m92' if m92 else 'm9' if m9 else 'm8' if m8 else 'm75' if m75 else 'm74' if m74 else 'm73' if m73 else 'm72' if m72 else 'm71' if m71 else 'm7' if m7 else 'm6'
    BUILD=ROOT/'build'/(flavor if m6 else milestone.lower())
    tools=pinned_tools();BUILD.mkdir(parents=True,exist_ok=True)
    sources=[]
    for folder in ('boot','kernel','include','runtime','user'):
        sources.extend(p for p in (ROOT/folder).rglob('*') if p.is_file())
    sources.extend(ROOT/p for p in ('include/engine/boot/boot_v0.h','include/engine/abi/syscall_v0.h','include/engine/boot/validate_v0.h',
                                   'tools/build/m2.py','tools/build/m4.py','tools/build/m45.py','tools/build/bios.py','tools/build/secure.py','tools/build/owner.py','tools/security-toolchain.json','tools/build/data_image.py','tools/build/ext2_image.py','tools/build/fsck.py','tools/build/m3.py','tools/build/m1.py','tools/build/media.py'))
    if milestone=='M5':sources.extend(sorted((ROOT/'tools/build').glob('m5*.py')))
    if m6:sources.extend(sorted((ROOT/'tools/build').glob('m6*.py')))
    if m7:
        sources.extend(sorted((ROOT/'tools/build').glob('m7*.py')))
        sources.extend(ROOT/p for p in ('tools/build/bearssl.py','tools/build/trust7.py'))
        for folder in ('third_party/bearssl','third_party/bearssl-port','third_party/mozilla-ca'):
            sources.extend(p for p in (ROOT/folder).rglob('*') if p.is_file())
    if distro:
        sources.extend(ROOT/p for p in distro['input_sha256'])
        sources.append(ROOT/'tools/build/distro.py')
    if m73:
        from font73 import prepare as prepare_font
        prepare_font(BUILD)
        sources.append(ROOT/'tools/build/font73.py')
        sources.extend(p for p in (ROOT/'third_party/inter').iterdir() if p.is_file())
    if m74:
        provenance=json.loads((ROOT/'third_party/miniz/provenance.json').read_text(encoding='utf-8'))
        for filename,expected in provenance['files_sha256'].items():
            if sha(ROOT/'third_party/miniz'/filename)!=expected:raise ValueError('Pinned miniz source changed: '+filename)
        for folder in ('third_party/miniz','third_party/miniz-port'):
            sources.extend(p for p in (ROOT/folder).iterdir() if p.is_file())
    if m8:
        for folder in ('7zip','libmspack'):
            provenance=json.loads((ROOT/'third_party'/folder/'provenance.json').read_text(encoding='utf-8'))
            pins={k:v['sha256'] for k,v in provenance['files'].items()} if folder=='7zip' else provenance['files_sha256']
            for filename,expected in pins.items():
                if sha(ROOT/'third_party'/folder/filename)!=expected:raise ValueError('Pinned M8 upstream bytes changed: '+filename)
        sources.extend(sorted((ROOT/'tools/build').glob('m8*.py')))
        for folder in ('third_party/7zip','third_party/libmspack','third_party/libmspack-port'):
            sources.extend(p for p in (ROOT/folder).rglob('*') if p.is_file())
    if m9:sources.extend(sorted((ROOT/'tools/build').glob('m9*.py')))
    hashes={p.relative_to(ROOT).as_posix():sha(p) for p in sorted(sources)}
    identity={'milestone':'M92' if m92 else 'M9' if m9 else 'M8' if m8 else 'M75' if m75 else 'M74' if m74 else 'M73' if m73 else 'M72' if m72 else 'M71' if m71 else 'M7' if m7 else 'M6' if m6 else milestone,'source_sha256':hashes}
    if m6:identity['revision']=m6_revision
    build_id=hashlib.sha256(json.dumps(identity,sort_keys=True).encode()).hexdigest()[:32]
    (BUILD/'build.h').write_text(f'#define BUILD_ID "{build_id}"\n',encoding='utf-8')
    debug=['-g','-fdebug-compilation-dir=.']
    for original,replacement in ((ROOT,'.'),(Path(tools['clang']).parent.parent,'/toolchain/llvm')):
        for spelling in (str(original),original.as_posix()):debug.extend((f'-ffile-prefix-map={spelling}={replacement}',f'-fdebug-prefix-map={spelling}={replacement}'))
    common=['-std=c17','-O2',*debug,'-ffreestanding','-fno-builtin','-fno-stack-protector','-mno-red-zone','-mgeneral-regs-only',
            '-Wall','-Wextra','-Werror','-DENGINE_M2','-I',ROOT/'include','-I',BUILD]
    if m6:common.append('-DENGINE_M6')
    if m71:common.append('-DENGINE_M71')
    if m72:common.append('-DENGINE_M72')
    if m73:common.append('-DENGINE_M73')
    if m74:common.append('-DENGINE_M74')
    if m75:common.append('-DENGINE_M75')
    if m8:common.append('-DENGINE_M8')
    if m9:common.append('-DENGINE_M9')
    if m92:common.append('-DENGINE_M92')
    if m75 and m5_live:common.append('-DENGINE_SETUP_MEDIA75')
    if m7:
        common.extend(('-DENGINE_M7','-I',ROOT/'third_party/bearssl/inc','-I',ROOT/'third_party/bearssl-port'))
        from trust7 import prepare as prepare_trust
        prepare_trust(BUILD/'trust7.h')
    if milestone in ('M3','M4','M45','M5'):common.append('-DENGINE_M3')
    if milestone in ('M4','M45','M5'):common.append('-DENGINE_M4')
    if milestone in ('M45','M5'):common.append('-DENGINE_M45')
    if distro:
        from distro import prepare
        prepare(BUILD,distro);common.append('-DENGINE_M51')
    elif (BUILD/'distro.json').exists():(BUILD/'distro.json').unlink()
    if m7:
        from m74 import applications
        for app,kind in (applications() if m74 else [('editor',2),('calculator',4)]):
            obj=BUILD/('app7-'+app+'.o')
            inflate=m74 and app=='archives'
            port=['-I',ROOT/'third_party/miniz-port','-I',ROOT/'third_party/miniz','-include',ROOT/'third_party/miniz-port/config74.h'] if inflate else []
            command([tools['clang'],'--target=x86_64-unknown-none-elf',*common,'-mcmodel=large',
                     '-fno-pic','-fno-pie','-fno-asynchronous-unwind-tables','-DAPP7_KIND='+str(kind),
                     *port,'-c',ROOT/('user/archive74.c' if inflate else 'user/browser74.c' if m74 and app=='browser' else 'user/desktop7.c'),'-o',obj])
            objects=[obj]
            if inflate:
                vendor=BUILD/'app74-inflate.o'
                command([tools['clang'],'--target=x86_64-unknown-none-elf',*common,'-mcmodel=large','-fno-pic','-fno-pie',*port,
                         '-c',ROOT/'third_party/miniz/miniz_tinfl.c','-o',vendor])
                objects.append(vendor)
            command([tools['ld.lld'],'-T',ROOT/'user/desktop7.ld','--build-id=none','-o',BUILD/('APP7-'+app+'.ELF'),*objects])
            if m74:
                from m5_layout import compact_elf
                image=BUILD/('APP7-'+app+'.ELF')
                shutil.copyfile(image,BUILD/('APP7-'+app+'-debug.ELF'))
                image.write_bytes(compact_elf(image.read_bytes()))
    if m8:
        from m8 import runtime
        runtime(BUILD,tools,common)
    if milestone=='M5':
        common.append('-DENGINE_M5')
        if m6:
            from m6 import embed
        else:
            from m5_layout import embed
        embed(BUILD,m5_live)
    def compile_file(path,kind,assembly=False):
        path=Path(path);obj=BUILD/(path.with_suffix('').as_posix().replace('/','_')+'-'+kind+'.o')
        target='x86_64-pc-windows-msvc' if kind=='efi' else 'x86_64-unknown-none-elf'
        flags=debug if assembly else common+([] if kind=='efi' else ['-fno-pic','-fno-pie','-fno-asynchronous-unwind-tables'])
        if kind=='user':flags=flags+['-mcmodel=large']
        command([tools['clang'],'--target='+target,*flags,'-c',path.as_posix(),'-o',obj]);return obj
    user=[compile_file('user/probe.c','user')]
    if milestone in ('M3','M4','M45','M5'):user.append(compile_file('user/m3.c','user'))
    command([tools['ld.lld'],'-T',ROOT/'user/linker.ld','--build-id=none','-o',BUILD/'USER.ELF',*user])
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
            f'build/{BUILD.name}/user-image.c',f'build/{BUILD.name}/interrupt-stubs.S')
    if milestone in ('M3','M4','M45','M5'):inputs+=('kernel/arch/x86_64/timer.c','kernel/scheduler/m3.c','kernel/arch/x86_64/smp.c','kernel/arch/x86_64/ap.S')
    if milestone in ('M4','M45','M5'):inputs+=(('kernel/storage/ahci45.c' if milestone in ('M45','M5') else 'kernel/storage/ahci.c'),'kernel/storage/ext2.c','kernel/storage/fs_test.c','kernel/input/legacy.c',('kernel/usb/xhci.c' if milestone in ('M45','M5') else 'kernel/input/xhci.c'),'kernel/core/console.c','kernel/m4/main.c')
    if milestone in ('M45','M5'):inputs+=('kernel/storage/registry.c','kernel/storage/partitions.c','kernel/storage/nvme.c','kernel/storage/ide.c','kernel/storage/virtio_blk.c','kernel/storage/fat32.c','kernel/storage/fat_test.c','kernel/storage/ext2_read.c','kernel/storage/ext2_read_test.c')
    if m92:inputs+=('kernel/storage/scsi92.c',)
    if m6:
        inputs=tuple(p for p in inputs if p not in ('kernel/storage/ext2.c','kernel/storage/fs_test.c','kernel/m4/main.c'))
        inputs+=('kernel/install/layout.c','kernel/storage/volume6.c','kernel/install/installer6.c','kernel/install/wizard6.c','build/'+flavor+'/installer-image.c','kernel/install/entry6.c')
    if m7:inputs+=('kernel/desktop/window.c','kernel/desktop/session.c','kernel/desktop/app.c','kernel/desktop/app-return.S','kernel/network/wire.c','kernel/network/e1000.c','kernel/network/network.c','kernel/network/tcp.c','kernel/network/tls.c','kernel/network/calendar.c','kernel/network/clock.c','kernel/network/http.c','kernel/network/download.c')
    if m71:inputs+=('kernel/install/disk71.c',)
    elif milestone=='M5' and not m6:inputs+=('kernel/install/m5.c','build/m5/installer-image.c')
    if m74:inputs+=('kernel/audio/hda74.c',)
    if m8:inputs+=('kernel/windows8/process.c','kernel/windows8/enter.S','runtime/sha256.c')
    if distro and not m6:inputs+=('kernel/install/guided.c',)
    kernel=[compile_file(p,'kernel',p.endswith('.S')) for p in inputs]
    if m7:
        from bearssl import objects as bearssl_objects
        dependency=bearssl_objects(BUILD/'bearssl-linked' if m8 else ROOT/'build/m7/bearssl-linked',tools,
            ([flag for flag in common if flag!='-DENGINE_SETUP_MEDIA75']+['-fno-pic','-fno-pie','-fno-asynchronous-unwind-tables']) if m8 else [*common,'-fno-pic','-fno-pie','-fno-asynchronous-unwind-tables'])
        kernel+=['--start-lib',*dependency,'--end-lib']
    link=['-T',ROOT/'kernel/arch/x86_64/linker.ld','--build-id=none','-Map='+str(BUILD/'kernel.map'),'-o',BUILD/'KERNEL.ELF',*kernel]
    if m7:
        import subprocess
        response=BUILD/'kernel-link.rsp'
        response.write_text(subprocess.list2cmdline([str(arg).replace('\\','/') for arg in link]),encoding='utf-8')
        command([tools['ld.lld'],'@'+str(response)])
    else:command([tools['ld.lld'],*link])
    nm=Path(tools['clang']).parent/'llvm-nm.exe'
    if command([nm,'-u',BUILD/'KERNEL.ELF']).strip():raise RuntimeError('Undefined kernel symbols')
    map_path=BUILD/'kernel.map';map_path.write_text(map_path.read_text(encoding='utf-8').replace(str(ROOT),'.').replace(ROOT.as_posix(),'.'),encoding='utf-8')
    if m6 and any(sha(ROOT/p)!=value for p,value in hashes.items()):raise RuntimeError('Build inputs changed during compilation; discard mixed outputs and rebuild')
    artifact_name=(('M92' if m92 else 'M9' if m9 else 'M8' if m8 else 'M75' if m75 else 'M74' if m74 else 'M73' if m73 else 'M72' if m72 else 'M71' if m71 else 'M7' if m7 else 'M6')+('' if m6_revision=='standard' else m6_revision.upper())) if m6 else milestone
    if m73:
        from m5_layout import compact_elf
        shutil.copyfile(BUILD/'KERNEL.ELF',BUILD/'KERNEL-debug.ELF')
        (BUILD/'KERNEL.ELF').write_bytes(compact_elf((BUILD/'KERNEL.ELF').read_bytes()))
    init=struct.pack('<8sII32s',b'ENGINIT1',48,0,build_id.encode())
    if m8 and m5_live:
        payload=(ROOT/'third_party/7zip/7z2606-x64.msi').read_bytes()
        init+=struct.pack('<8sQ32s',b'ENGMSI81',len(payload),hashlib.sha256(payload).digest())+payload
    (BUILD/'INIT.BIN').write_bytes(init)
    package(BUILD/'normal',(BUILD/'BOOTX64.EFI').read_bytes(),(BUILD/'KERNEL.ELF').read_bytes(),init,artifact_name)
    dist=ROOT/'dist';dist.mkdir(exist_ok=True)
    for name in (f'EngineOS-{artifact_name}.iso',f'EngineOS-{artifact_name}-usb.img'):shutil.copyfile(BUILD/'normal'/name,dist/name)
    for filename,name in (('KERNEL.ELF',f'EngineOS-{artifact_name}-kernel.elf'),('kernel.map',f'EngineOS-{artifact_name}-kernel.map'),('USER.ELF',f'EngineOS-{artifact_name}-user.elf')):
        shutil.copyfile(BUILD/filename,dist/name)
    if m7:
        for app,kind in (applications() if m74 else [('editor',2),('calculator',4)]):shutil.copyfile(BUILD/('APP7-'+app+'.ELF'),dist/('EngineOS-'+artifact_name+'-'+app+'.elf'))
    artifacts={p.name:{'sha256':sha(p),'size':p.stat().st_size} for p in sorted(dist.glob(f'EngineOS-{artifact_name}*')) if p.is_file()}
    for name in artifacts:
        payload=(dist/name).read_bytes()
        if m8:
            from m8 import privacy_input
            payload=privacy_input(payload)
        if re.search(rb'[Cc]:[\\/]+Users[\\/]+',payload):raise RuntimeError('Host user path in '+name)
    manifest={'milestone':milestone,'build_id':build_id,'source_sha256':hashes,'artifacts':artifacts,'mode':'normal',
              'limits':(['native VM scope: 1/2/4 active xAPIC CPUs, fixed process affinity, serialized kernel',
                         'ACPI MADT, xAPIC, 64-bit HPET and startup page 0x8000 required',
                         'x87/SSE state; no AVX, migration, file paging, Windows apps or installer',
                         '8 thread slots, 2 address spaces, 16 anonymous VM pages per space'] if milestone in ('M3','M4','M45','M5') else
                        ['one active CPU; GPR-only cooperative probes','no Windows apps, installer, threads or SMP'])+
                       ['early allocator manages conventional memory below 1 GiB']}
    (dist/f'{artifact_name.lower()}-build-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    (dist/f'{artifact_name}-SHA256SUMS.txt').write_text(''.join(f'{v["sha256"]}  {k}\n' for k,v in artifacts.items()),encoding='utf-8')
    print(json.dumps({'build_id':build_id,'iso':f'dist/EngineOS-{artifact_name}.iso'},indent=2));return manifest
if __name__=='__main__':main()
