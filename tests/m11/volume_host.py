"""Execute production native subtree and verified-stream code with fault injection."""
import ctypes as c
import hashlib
import importlib.util
import json
import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / 'tools/build'), str(ROOT / 'tests/m6')]
from m1 import pinned_tools, command

spec = importlib.util.spec_from_file_location('types11', ROOT / 'tests/m6/volume.py')
types = importlib.util.module_from_spec(spec)
spec.loader.exec_module(types)
Volume, IO, FLUSH, SOURCE = types.Volume, types.IO, types.FLUSH, types.SOURCE


class Disk:
    def __init__(self, content=None):
        self.data = bytearray(4 * 1024**2) if content is None else bytearray(content)
        self.calls = self.fail = self.corrupt = 0
        self.torn = False
        self.read = IO(self._read)
        self.write = IO(self._write)
        self.flush = FLUSH(self._flush)

    def trip(self):
        self.calls += 1
        return self.fail == self.calls

    def _read(self, ctx, block, out, size):
        if self.trip() or size != 4096 or block >= len(self.data) // 4096:
            return 1
        c.memmove(out, bytes(self.data[block * 4096:(block + 1) * 4096]), size)
        return 0

    def _write(self, ctx, block, source, size):
        if size != 4096 or block >= len(self.data) // 4096:
            return 1
        bad = self.trip()
        if bad and not self.torn:
            return 1
        raw = c.string_at(source, 127 if bad else size)
        self.data[block * 4096:block * 4096 + len(raw)] = raw
        if self.corrupt and block >= 160:
            self.data[block * 4096] ^= 1
            self.corrupt = 0
        return int(bad)

    def _flush(self, ctx):
        return int(self.trip())

    def volume(self):
        return Volume(read=self.read, write=self.write, flush=self.flush, blocks=len(self.data) // 4096)


def main():
    work = ROOT / 'build/m11/volume-host'
    work.mkdir(parents=True, exist_ok=True)
    tools = pinned_tools()
    command([tools['clang'], '--target=x86_64-pc-windows-msvc', '-std=c17', '-O2', '-Wall', '-Wextra', '-Werror', '-ffreestanding', '-fno-builtin', '-fno-stack-protector', '-mno-stack-arg-probe', '-DENGINE_M7', '-DENGINE_M8', '-DENGINE_M11', '-I', ROOT / 'include', '-c', ROOT / 'kernel/storage/volume6.c', '-o', work / 'volume.obj'])
    names = ['format', 'mount', 'mkdir', 'put', 'read', 'verify', 'move_tree11', 'remove_tree11', 'stream_begin', 'stream_append', 'stream_abort', 'stream_seal11', 'stream_check11', 'stream_finish_expected11']
    command([tools['lld-link'], '/dll', '/noentry', '/nodefaultlib', '/timestamp:0', *['/export:v6_' + n for n in names], work / 'volume.obj', '/out:' + str(work / 'volume.dll')])
    lib = c.CDLL(str(work / 'volume.dll'))
    vp = c.POINTER(Volume)
    for name in ['format', 'mount', 'stream_abort', 'stream_seal11']:
        getattr(lib, 'v6_' + name).argtypes = [vp]
    for name in ['mkdir', 'verify', 'remove_tree11']:
        getattr(lib, 'v6_' + name).argtypes = [vp, c.c_char_p]
    lib.v6_put.argtypes = [vp, c.c_char_p, c.c_uint64, SOURCE, c.c_void_p]
    lib.v6_read.argtypes = [vp, c.c_char_p, c.c_uint64, c.c_void_p, c.c_uint, c.POINTER(c.c_uint)]
    lib.v6_move_tree11.argtypes = [vp, c.c_char_p, c.c_char_p, c.c_uint]
    lib.v6_stream_begin.argtypes = [vp, c.c_char_p, c.c_uint64]
    lib.v6_stream_append.argtypes = [vp, c.c_void_p, c.c_uint]
    lib.v6_stream_check11.argtypes = [vp, c.POINTER(c.c_uint)]
    lib.v6_stream_finish_expected11.argtypes = [vp, c.c_uint64, c.c_uint32]

    def entries(v):
        return {bytes(e.path): e for e in v.entries if e.kind}

    def read(v, path):
        e = entries(v)[path]
        out = c.create_string_buffer(e.size)
        size = c.c_uint()
        assert lib.v6_read(c.byref(v), path, 0, out, e.size, c.byref(size)) == 0
        return out.raw[:size.value]

    def put(v, path, data):
        @SOURCE
        def source(ctx, offset, out, size):
            c.memmove(out, data[offset:offset + size], size)
            return 0
        assert lib.v6_put(c.byref(v), path, len(data), source, None) == 0

    disk = Disk()
    v = disk.volume()
    assert lib.v6_format(c.byref(v)) == 0
    for path in [b'/Users', b'/Users/Owner', b'/Users/Owner/A', b'/Users/Owner/A/Sub', b'/Users/Owner/B', b'/Users/Owner/A2']:
        assert lib.v6_mkdir(c.byref(v), path) == 0
    payload = bytes((i * 37 + 11) & 255 for i in range(65536 + 113))
    put(v, b'/Users/Owner/A/Sub/Original.bin', payload)
    put(v, b'/Users/Owner/A/Empty.txt', b'')
    put(v, b'/Users/Owner/A2/Keep.txt', b'Unrelated data')
    assert entries(v)[b'/Users/Owner/A/Sub/Original.bin'].crc == zlib.crc32(payload)
    baseline = bytes(disk.data)
    checks = ['Unchanged IEEE/native disk format checked against independent zlib CRCs']

    for target in [b'/Users/Owner/A/Sub/Bad', b'/Users/Owner/B', b'/Users/Owner/' + b'x' * 64]:
        before = bytes(disk.data)
        assert lib.v6_move_tree11(c.byref(v), b'/Users/Owner/A', target, 0) != 0
        assert disk.data == before
    checks.append('Self-descendant, collision and overlong component refusals perform no writes')

    disk.calls = 0
    assert lib.v6_move_tree11(c.byref(v), b'/Users/Owner/A', b'/Users/Owner/B/Moved', 0) == 0
    operation_calls = disk.calls
    assert read(v, b'/Users/Owner/B/Moved/Sub/Original.bin') == payload
    assert b'/Users/Owner/A' not in entries(v)
    assert read(v, b'/Users/Owner/A2/Keep.txt') == b'Unrelated data'
    checks.append('One subtree metadata transaction preserves descendant bytes and unrelated prefix neighbors')

    for torn in (False, True):
        for point in range(1, operation_calls + 1):
            d = Disk(baseline)
            tree = d.volume()
            assert lib.v6_mount(c.byref(tree)) == 0
            d.calls, d.fail, d.torn = 0, point, torn
            assert lib.v6_move_tree11(c.byref(tree), b'/Users/Owner/A', b'/Users/Owner/B/Moved', 0) != 0
            d.fail = 0
            recovered = d.volume()
            assert lib.v6_mount(c.byref(recovered)) == 0
            paths = entries(recovered)
            old, new = b'/Users/Owner/A', b'/Users/Owner/B/Moved'
            assert (old in paths) != (new in paths), (point, torn)
            root = old if old in paths else new
            assert read(recovered, root + b'/Sub/Original.bin') == payload, (point, torn)
            assert root + b'/Empty.txt' in paths
            assert read(recovered, b'/Users/Owner/A2/Keep.txt') == b'Unrelated data'
    checks.append(f'All {operation_calls} I/O acknowledgement points, with ordinary and torn failures, remount as complete old or new subtree')

    d = Disk(baseline)
    tree = d.volume()
    assert lib.v6_mount(c.byref(tree)) == 0
    target = b'/Users/Owner/B/Copy.bin'

    def append_copy():
        assert lib.v6_stream_begin(c.byref(tree), target, len(payload)) == 0
        for start in range(0, len(payload), 4096):
            part = payload[start:start + 4096]
            assert lib.v6_stream_append(c.byref(tree), c.c_char_p(part), len(part)) == 0
        assert target not in entries(tree)

    append_copy()
    lib.v6_stream_abort(c.byref(tree))
    assert target not in entries(tree) and read(tree, b'/Users/Owner/A/Sub/Original.bin') == payload
    checks.append('Cancellation aborts unpublished destination data and preserves source bytes')
    append_copy()
    assert lib.v6_stream_seal11(c.byref(tree)) == 0
    assert lib.v6_stream_finish_expected11(c.byref(tree), len(payload), zlib.crc32(payload)) != 0
    done = c.c_uint()
    steps = 0
    while not done.value:
        before = d.calls
        assert lib.v6_stream_check11(c.byref(tree), c.byref(done)) == 0
        assert d.calls - before == 1
        steps += 1
    assert steps == (len(payload) + 4095) // 4096
    assert target not in entries(tree)
    assert lib.v6_stream_finish_expected11(c.byref(tree), len(payload), zlib.crc32(payload) ^ 1) != 0
    assert target not in entries(tree)
    checks.append('One-page verification is bounded; premature finish and wrong source CRC cannot publish')
    append_copy()
    assert lib.v6_stream_seal11(c.byref(tree)) == 0
    done.value = 0
    while not done.value:
        assert lib.v6_stream_check11(c.byref(tree), c.byref(done)) == 0
    assert lib.v6_stream_finish_expected11(c.byref(tree), len(payload), zlib.crc32(payload)) == 0
    assert read(tree, target) == payload and lib.v6_verify(c.byref(tree), target) == 0
    checks.append('Verified copy publishes exact bytes only after sealing, readback and source-CRC agreement')

    assert lib.v6_remove_tree11(c.byref(tree), b'/Users/Owner/A') == 0
    assert b'/Users/Owner/A' not in entries(tree) and read(tree, b'/Users/Owner/A2/Keep.txt') == b'Unrelated data'
    checks.append('Subtree removal preserves similarly prefixed unrelated files')

    bad = Disk(baseline)
    tree = bad.volume()
    assert lib.v6_mount(c.byref(tree)) == 0
    bad.corrupt = 1
    append_copy()
    assert lib.v6_stream_seal11(c.byref(tree)) == 0
    done.value = 0
    status = 0
    while not done.value and not status:
        status = lib.v6_stream_check11(c.byref(tree), c.byref(done))
    assert status != 0 and tree.readonly
    lib.v6_stream_abort(c.byref(tree))
    assert target not in entries(tree) and read(tree, b'/Users/Owner/A/Sub/Original.bin') == payload
    checks.append('A falsely successful corrupted data write is detected before publication; source remains intact')

    inputs = ['kernel/storage/volume6.c', 'include/engine/storage/volume6.h', 'tests/m11/volume_host.py']
    result = {'passed': True, 'scope': 'Production native storage on host; actual GUI and reboot tasks are separate required gates', 'checks': checks, 'fault_cases': operation_calls * 2, 'source_sha256': {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in inputs}}
    (work / 'results.json').write_bytes((json.dumps(result, indent=2) + '\n').encode())
    print('M11 production volume PASS:', len(checks), 'checks;', operation_calls * 2, 'fault cases')


if __name__ == '__main__':
    main()
