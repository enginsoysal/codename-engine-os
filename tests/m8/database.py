"""Host-only MSI/CAB format tests; never install or execute a package on the host."""
import ctypes
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/build'))
from m8 import database_harness

PACKAGE = ROOT / 'third_party/7zip/7z2606-x64.msi'
WORK = ROOT / 'build/m8/db-host'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def original_tables():
    """Windows Installer database access is explicitly read-only, without installation."""
    msi = ctypes.WinDLL('msi')
    handle = ctypes.c_uint()
    msi.MsiOpenDatabaseW.argtypes = [ctypes.c_wchar_p, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint)]
    msi.MsiDatabaseOpenViewW.argtypes = [ctypes.c_uint, ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_uint)]
    msi.MsiViewFetch.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_uint)]
    msi.MsiRecordGetStringW.argtypes = [ctypes.c_uint, ctypes.c_uint, ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_uint)]
    msi.MsiRecordGetInteger.restype = ctypes.c_int
    assert msi.MsiOpenDatabaseW(str(PACKAGE), None, ctypes.byref(handle)) == 0
    result = {}
    try:
        for name in ('Property', 'Directory', 'File', 'Component', 'FeatureComponents',
                     'Feature', 'Media', 'Registry', 'Shortcut', 'InstallExecuteSequence',
                     'RemoveFile', 'Upgrade', 'LaunchCondition'):
            view = ctypes.c_uint()
            opened = msi.MsiDatabaseOpenViewW(handle, 'SELECT * FROM `' + name + '`', ctypes.byref(view))
            if opened == 1615:
                result[name] = []
                continue
            assert opened == 0, name
            try:
                assert msi.MsiViewExecute(view, 0) == 0
                rows = []
                while True:
                    record = ctypes.c_uint()
                    status = msi.MsiViewFetch(view, ctypes.byref(record))
                    if status == 259:
                        break
                    assert status == 0
                    try:
                        row = []
                        for col in range(1, msi.MsiRecordGetFieldCount(record) + 1):
                            text = ctypes.create_unicode_buffer(8192)
                            size = ctypes.c_uint(8192)
                            assert msi.MsiRecordGetStringW(record, col, text, ctypes.byref(size)) == 0
                            row.append(text.value)
                        rows.append(row)
                    finally:
                        msi.MsiCloseHandle(record)
                result[name] = rows
            finally:
                msi.MsiCloseHandle(view)
    finally:
        msi.MsiCloseHandle(handle)
    return result


def worker(case):
    lib = ctypes.CDLL(str(WORK / 'msi-db-test.dll'))
    callback_type = ctypes.CFUNCTYPE(None, ctypes.c_char_p)

    @callback_type
    def failure(message):
        print(json.dumps({'rejected': message.decode('utf-8')}), flush=True)
        os._exit(23)

    lib.db8_open.argtypes = [ctypes.c_void_p, ctypes.c_uint, callback_type]
    lib.db8_table.argtypes = [ctypes.c_char_p]
    lib.db8_table.restype = ctypes.c_uint
    lib.db8_text.argtypes = [ctypes.c_uint, ctypes.c_uint]
    lib.db8_text.restype = ctypes.c_char_p
    lib.db8_int.argtypes = [ctypes.c_uint, ctypes.c_uint]
    lib.db8_int.restype = ctypes.c_int
    for name in ('db8_cab_name', 'db8_cab_data'):
        getattr(lib, name).argtypes = [ctypes.c_uint]
        getattr(lib, name).restype = ctypes.c_char_p if name.endswith('name') else ctypes.c_void_p
    raw = bytearray(PACKAGE.read_bytes())
    if case == 'signature':
        raw[0] ^= 1
    elif case == 'geometry':
        raw[30] = 12
    elif case == 'extended-fat':
        struct.pack_into('<I', raw, 72, 1)
    elif case == 'directory-cycle':
        directory = struct.unpack_from('<I', raw, 48)[0]
        fat = struct.unpack_from('<I', raw, 76)[0]
        assert directory < 128
        struct.pack_into('<I', raw, 512 + fat * 512 + directory * 4, directory)
    elif case == 'truncated':
        raw = raw[:-1]
    elif case != 'positive':
        raise ValueError(case)
    data = ctypes.create_string_buffer(bytes(raw))
    lib.db8_open(data, len(raw), failure)
    assert case == 'positive', 'Malformed input was accepted: ' + case
    reference = original_tables()
    counts = {}
    for table, expected in reference.items():
        count = lib.db8_table(table.encode())
        columns = lib.db8_columns()
        actual = []
        for row in range(count):
            cells = []
            for col in range(columns):
                if lib.db8_type(col) & 0x800:
                    value = lib.db8_text(row, col).decode('utf-8')
                else:
                    number = lib.db8_int(row, col)
                    value = '' if number == -2147483648 else str(number)
                cells.append(value)
            actual.append(cells)
        assert actual == expected, table
        counts[table] = count
    provenance = json.loads((ROOT / 'third_party/7zip/provenance.json').read_text(encoding='utf-8'))
    files = {}
    for i in range(lib.db8_cab()):
        name = lib.db8_cab_name(i).decode('utf-8')
        content = ctypes.string_at(lib.db8_cab_data(i), lib.db8_cab_size(i))
        files[name] = hashlib.sha256(content).hexdigest()
    assert files == provenance['original_payload_sha256']
    print(json.dumps({'tables': counts, 'original_payloads': len(files), 'payload_sha256': files}))


def main():
    harness = database_harness()
    results = {}
    for case in ('positive', 'signature', 'geometry', 'extended-fat', 'directory-cycle', 'truncated'):
        run = subprocess.run([sys.executable, __file__, '--worker', case], cwd=ROOT,
                             capture_output=True, text=True, timeout=30)
        assert run.returncode == (0 if case == 'positive' else 23), (case, run.returncode, run.stdout, run.stderr)
        results[case] = json.loads(run.stdout)
    paths = [PACKAGE, harness, Path(__file__), ROOT / 'tests/m8/db_host.c',
             ROOT / 'tools/build/m8.py', ROOT / 'user/windows8/msi_db.c',
             ROOT / 'user/windows8/msi_db.h', ROOT / 'user/windows8/cab.c',
             ROOT / 'user/windows8/cab.h']
    paths += sorted(p for p in (ROOT / 'third_party/libmspack').rglob('*') if p.is_file())
    paths += sorted(p for p in (ROOT / 'third_party/libmspack-port').rglob('*') if p.is_file())
    report = {'scope': 'host-only MSI/CAB format checks; no guest runtime acceptance',
              'inputs_sha256': {p.relative_to(ROOT).as_posix(): sha(p) for p in paths},
              'cases': results}
    (WORK / 'format-results.json').write_bytes((json.dumps(report, indent=2) + '\n').encode())
    print('PASS: 13 database tables, 106 original payload hashes, five malformed-package rejections; host format scope only.')


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--worker':
        worker(sys.argv[2])
    else:
        main()
