"""Pin observed host tools and test real UEFI load via disposable ISO/USB fixtures."""
from pathlib import Path
import argparse
import hashlib
import importlib.metadata
import json
import os
import shutil
import struct
import subprocess
import sys
import uuid
import zlib

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/local/python'))
import pycdlib
if importlib.metadata.version('pycdlib') != '1.22.0':
    raise RuntimeError('The pinned pycdlib 1.22.0 is required')

def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def fat_fixture(payload):
    """Restricted FAT32 EFI fixture, short names only; not a general filesystem writer."""
    sectors, reserved, fat_sectors = 131072, 32, 1024
    data_start = reserved + 2 * fat_sectors
    count = sectors - data_start
    assert 65525 <= count and (count + 2) * 4 <= fat_sectors * 512
    image = bytearray(sectors * 512)
    image[:3], image[3:11] = b'\xeb\x58\x90', b'ENGINEOS'
    struct.pack_into('<HBHBHHBHHHII', image, 11, 512, 1, reserved, 2, 0, 0, 0xf8, 0, 63, 255, 0, sectors)
    struct.pack_into('<IHHIHH', image, 36, fat_sectors, 0, 0, 2, 1, 6)
    image[64], image[66] = 0x80, 0x29
    struct.pack_into('<I', image, 67, 0x454e474e)
    image[71:82], image[82:90], image[510:512] = b'ENGINE M0  ', b'FAT32   ', b'\x55\xaa'
    struct.pack_into('<I', image, 512, 0x41615252)
    struct.pack_into('<I', image, 512 + 484, 0x61417272)
    struct.pack_into('<II', image, 512 + 488, 0xffffffff, 0xffffffff)
    struct.pack_into('<I', image, 512 + 508, 0xaa550000)
    image[6*512:7*512] = image[:512]
    image[7*512:8*512] = image[512:1024]
    table = bytearray(fat_sectors * 512)
    chunks = (len(payload) + 511) // 512
    assert chunks > 0 and 5 + chunks <= count
    for index, value in [(0, 0x0ffffff8), (1, 0xffffffff), (2, 0x0fffffff), (3, 0x0fffffff), (4, 0x0fffffff)]:
        struct.pack_into('<I', table, index*4, value)
    for index in range(5, 5 + chunks):
        struct.pack_into('<I', table, index*4, index+1 if index < 4 + chunks else 0x0fffffff)
    image[reserved*512:(reserved+fat_sectors)*512] = table
    image[(reserved+fat_sectors)*512:data_start*512] = table
    def entry(name, cluster, directory, length=0):
        value = bytearray(32)
        value[:11], value[11] = name, 0x10 if directory else 0x20
        struct.pack_into('<H', value, 20, cluster >> 16)
        struct.pack_into('<HI', value, 26, cluster & 0xffff, length)
        return value
    def directory(cluster, entries):
        start = (data_start + cluster - 2) * 512
        for i, item in enumerate(entries):
            image[start+i*32:start+(i+1)*32] = item
    directory(2, [entry(b'EFI        ', 3, True)])
    directory(3, [entry(b'.          ', 3, True), entry(b'..         ', 0, True), entry(b'BOOT       ', 4, True)])
    directory(4, [entry(b'.          ', 4, True), entry(b'..         ', 3, True), entry(b'BOOTX64 EFI', 5, False, len(payload))])
    start = (data_start + 3) * 512
    image[start:start+len(payload)] = payload
    return image

def gpt_fixture(volume):
    """One disposable EFI partition. Never opens a host block device."""
    first, total = 2048, 2048 + len(volume)//512 + 2048
    image = bytearray(total * 512)
    image[446+4] = 0xee
    struct.pack_into('<II', image, 446+8, 1, total-1)
    image[510:512] = b'\x55\xaa'
    entries = bytearray(128 * 128)
    entries[:16] = uuid.UUID('c12a7328-f81f-11d2-ba4b-00a0c93ec93b').bytes_le
    entries[16:32] = uuid.UUID('1432896c-811c-4a61-bc1f-a12000000001').bytes_le
    struct.pack_into('<QQQ', entries, 32, first, first+len(volume)//512-1, 0)
    label = 'Engine OS M0 fixture'.encode('utf-16le')
    entries[56:56+len(label)] = label
    entries_crc = zlib.crc32(entries)
    disk_id = uuid.UUID('1432896c-811c-4a61-bc1f-a12000000002').bytes_le
    def header(current, backup, table):
        value = bytearray(512)
        struct.pack_into('<8sIIIIQQQQ16sQIII', value, 0, b'EFI PART', 0x10000, 92, 0, 0,
                         current, backup, 34, total-34, disk_id, table, 128, 128, entries_crc)
        struct.pack_into('<I', value, 16, zlib.crc32(value[:92]))
        return value
    image[512:1024] = header(1, total-1, 2)
    image[1024:1024+len(entries)] = entries
    image[(total-33)*512:(total-1)*512] = entries
    image[(total-1)*512:total*512] = header(total-1, 1, total-33)
    image[first*512:first*512+len(volume)] = volume
    return image

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--qemu-dir', type=Path, default=Path(os.environ.get('ProgramFiles', 'C:/Program Files')) / 'qemu')
    args = parser.parse_args()
    qemu = args.qemu_dir / 'qemu-system-x86_64.exe'
    code = args.qemu_dir / 'share/edk2-x86_64-code.fd'
    variables = args.qemu_dir / 'share/edk2-i386-vars.fd'
    work = ROOT / 'build/m0-environment'
    work.mkdir(parents=True, exist_ok=True)
    evidence = ROOT / 'evidence/M0'
    manifest_path = ROOT / 'tools/toolchain-lock.json'
    def command(argv):
        return subprocess.run([str(x) for x in argv], capture_output=True, text=True, timeout=45, check=True).stdout
    clang, link = shutil.which('clang'), shutil.which('lld-link')
    if not clang or not link:
        raise RuntimeError('Clang and lld-link must be on PATH')
    files = {'clang': Path(clang), 'lld-link': Path(link), 'ld.lld': Path(shutil.which('ld.lld') or ''),
             'qemu-system-x86_64': qemu, 'ovmf-code': code, 'ovmf-vars-template': variables,
             'ovmf-notices': args.qemu_dir / 'share/edk2-licenses.txt'}
    observed = {name: {'filename': path.name, 'sha256': digest(path), 'size': path.stat().st_size}
                for name, path in files.items()}
    # Record every adjacent QEMU DLL, since the executable hash alone is insufficient.
    observed['qemu-dlls'] = {path.name: digest(path) for path in sorted(args.qemu_dir.glob('*.dll'))}
    manifest = {'schema': 1, 'qemu_version': command([qemu, '--version']).splitlines()[0],
                'clang_version': command([clang, '--version']).splitlines()[0],
                'python_version': sys.version.split()[0], 'machine': 'pc-q35-11.1', 'accel': 'tcg',
                'ram_mib': 512, 'cpus': 2, 'files': observed,
                'iso_library': {'name': 'pycdlib', 'version': '1.22.0', 'license': 'LGPL-2.1-only'},
                'origin': {'qemu': 'pre-existing host installation; https://qemu.weilnetz.de/w64/',
                           'firmware': 'EDK2 files bundled with the observed QEMU installation',
                           'llvm': 'pre-existing Swift toolchain; observed local hashes, not upstream authenticity proof'}}
    if manifest_path.exists():
        pinned = json.loads(manifest_path.read_text(encoding='utf-8'))
        if pinned != manifest:
            raise RuntimeError('Toolchain differs from tools/toolchain-lock.json; inspect and explicitly review a pin change')
    else:
        manifest_path.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    command([clang, '--target=x86_64-pc-windows-msvc', '-ffreestanding', '-std=c17', '-fno-stack-protector',
             '-mno-red-zone', '-Wall', '-Wextra', '-Werror', '-c', ROOT / 'm0/probes/efi_environment.c', '-o', work / 'fixture.obj'])
    command([link, '/subsystem:efi_application', '/entry:efi_main', '/nodefaultlib', '/timestamp:0',
             '/machine:x64', f'/out:{work / "fixture.efi"}', work / 'fixture.obj'])
    fat = fat_fixture((work / 'fixture.efi').read_bytes())
    (work / 'efi-fat.img').write_bytes(fat)
    (work / 'usb-gpt.img').write_bytes(gpt_fixture(fat))
    iso = pycdlib.PyCdlib()
    iso.new(interchange_level=3, vol_ident='ENGINE_OS_M0_FIXTURE')
    iso.add_file(str(work / 'efi-fat.img'), iso_path='/EFI.IMG;1')
    iso.add_eltorito('/EFI.IMG;1', platform_id=0xef, efi=True, boot_load_size=1)
    iso.write(str(work / 'environment.iso'))
    iso.close()
    results = {'scope': 'UEFI fixture only; no kernel, installer or Windows compatibility', 'checks': []}
    for media in ('iso', 'usb'):
        varcopy = work / f'{media}-vars.fd'
        shutil.copyfile(variables, varcopy)
        serial = work / f'{media}-serial.log'
        if serial.exists():
            serial.unlink()
        argv = [str(qemu), '-machine', manifest['machine'], '-accel', 'tcg', '-m', '512', '-smp', '2',
                '-display', 'none', '-monitor', 'none', '-no-reboot', '-net', 'none',
                '-drive', f'if=pflash,format=raw,unit=0,readonly=on,file={code}',
                '-drive', f'if=pflash,format=raw,unit=1,file={varcopy}',
                '-serial', f'file:{serial}', '-device', 'isa-debug-exit,iobase=0xf4,iosize=0x04',
                '-device', 'qemu-xhci,id=xhci', '-device', 'usb-kbd,bus=xhci.0', '-device', 'usb-mouse,bus=xhci.0']
        if media == 'iso':
            argv += ['-cdrom', str(work / 'environment.iso'), '-boot', 'order=d']
        else:
            argv += ['-drive', f'if=none,id=usbfixture,format=raw,file={work / "usb-gpt.img"}',
                     '-device', 'usb-storage,bus=xhci.0,drive=usbfixture,bootindex=1']
        try:
            process = subprocess.run(argv, capture_output=True, text=True, timeout=45,
                                     creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
            log = serial.read_text(encoding='utf-8', errors='replace') if serial.exists() else ''
            passed = process.returncode == 33 and 'ENGINE_OS_M0_ENVIRONMENT_OK' in log
            detail = {'exit_code': process.returncode, 'marker_seen': 'ENGINE_OS_M0_ENVIRONMENT_OK' in log,
                      'stderr': process.stderr.replace(str(ROOT), '<project-root>').replace(str(args.qemu_dir), '<qemu-dir>')}
        except subprocess.TimeoutExpired:
            log = serial.read_text(encoding='utf-8', errors='replace') if serial.exists() else ''
            passed, detail = False, {'error': 'QEMU firmware fixture timeout after 45 seconds'}
        (evidence / f'environment-{media}-serial.log').write_text(log, encoding='utf-8')
        results['checks'].append({'name': f'UEFI {media} firmware load', 'passed': passed, 'detail': detail})
    results['passed'] = all(item['passed'] for item in results['checks'])
    results['artifacts'] = {name: digest(work / name) for name in ('fixture.efi', 'efi-fat.img', 'usb-gpt.img', 'environment.iso')}
    (evidence / 'environment-results.json').write_text(json.dumps(results, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(results, indent=2))
    if not results['passed']:
        raise SystemExit(1)

if __name__ == '__main__':
    main()
