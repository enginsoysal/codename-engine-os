"""M8 development compiler. Runtime acceptance is a separate, mandatory gate."""
from pathlib import Path
from m1 import ROOT, command, pinned_tools


def privacy_input(data):
    # These NUL-terminated generic compatibility prefixes contain no host user.
    # Full paths, including a username after the prefix, remain subject to the guard.
    for prefix in (b'C:/Users/', b'c:/Users/', b'C:\\Users\\'):
        data = data.replace(prefix + b'\0', b'VIRTUAL-PREFIX\0')
    return data


def runtime(work, tools, common):
    sources = ['runtime', 'pe', 'api', 'msi_db', 'cab', 'msi']
    objects = []
    for name in sources:
        source = ROOT / 'user/windows8' / (name + '.c')
        if not source.is_file():
            raise ValueError('M8 is not release-ready: missing runtime implementation ' + name)
        obj = work / ('windows8-' + name + '.o')
        command([tools['clang'], '--target=x86_64-unknown-none-elf', *common,
                 '-mcmodel=large', '-fno-pic', '-fno-pie', '-fno-asynchronous-unwind-tables',
                 '-DMSPACK_NO_DEFAULT_SYSTEM', '-DSIZEOF_OFF_T=8', '-DDEBUG=0',
                 '-I', ROOT / 'third_party/libmspack-port', '-I', ROOT / 'third_party/libmspack',
                 '-I', ROOT / 'user/windows8', '-c', source, '-o', obj])
        objects.append(obj)
    for source in ('runtime/sha256.c', 'third_party/libmspack/lzxd.c'):
        obj = work / ('windows8-' + Path(source).stem + '.o')
        command([tools['clang'], '--target=x86_64-unknown-none-elf', *common,
                 '-mcmodel=large', '-fno-pic', '-fno-pie', '-fno-asynchronous-unwind-tables',
                 '-DMSPACK_NO_DEFAULT_SYSTEM', '-DSIZEOF_OFF_T=8', '-DDEBUG=0',
                 '-I', ROOT / 'third_party/libmspack-port', '-I', ROOT / 'third_party/libmspack',
                 '-c', ROOT / source, '-o', obj])
        objects.append(obj)
    command([tools['ld.lld'], '-T', ROOT / 'user/windows8/linker.ld', '--build-id=none',
             '-o', work / 'WINDOWS8.ELF', *objects])
    from m5_layout import compact_elf
    image = work / 'WINDOWS8.ELF'
    (work / 'WINDOWS8-debug.ELF').write_bytes(image.read_bytes())
    image.write_bytes(compact_elf(image.read_bytes()))


def database_harness():
    tools = pinned_tools()
    work = ROOT / 'build/m8/db-host'
    work.mkdir(parents=True, exist_ok=True)
    objects = []
    for source in ('user/windows8/msi_db.c', 'user/windows8/cab.c', 'third_party/libmspack/lzxd.c', 'tests/m8/db_host.c'):
        obj = work / (Path(source).stem + '.obj')
        command([tools['clang'], '--target=x86_64-pc-windows-msvc', '-O2', '-std=c17',
                 '-ffreestanding', '-fno-builtin', '-fno-stack-protector', '-Wall', '-Wextra',
                 '-Werror', '-DMSPACK_NO_DEFAULT_SYSTEM', '-DSIZEOF_OFF_T=8', '-DDEBUG=0',
                 '-I', ROOT / 'third_party/libmspack-port', '-I', ROOT / 'third_party/libmspack',
                 '-I', ROOT / 'include', '-I', ROOT / 'user/windows8',
                 '-c', ROOT / source, '-o', obj])
        objects.append(obj)
    output = work / 'msi-db-test.dll'
    command([tools['lld-link'], '/dll', '/noentry', '/nodefaultlib', '/timestamp:0',
             '/out:' + str(output), *objects])
    return output


if __name__ == '__main__':
    import argparse
    parser=argparse.ArgumentParser(description='Build the isolated M8 original Windows-console profile')
    parser.add_argument('--profile',type=Path)
    parser.add_argument('--revision',default='standard')
    parser.add_argument('--database-harness',action='store_true')
    args=parser.parse_args()
    if args.database_harness:
        print(database_harness().relative_to(ROOT))
    else:
        from m6 import build
        build(args.revision,args.profile,windows=True)
