"""Execute the original native window model, including hostile geometry and stale tokens."""
import ctypes as c
import hashlib
import json
from pathlib import Path
import random
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/build'))
from m1 import pinned_tools


class Rect(c.Structure):
    _fields_ = [(n, c.c_int32) for n in ('x', 'y', 'width', 'height')]


class Window(c.Structure):
    _fields_ = [('bounds', Rect), ('generation', c.c_uint32), ('app', c.c_uint32),
                ('live', c.c_uint8), ('minimized', c.c_uint8)]


class Model(c.Structure):
    _fields_ = [('windows', Window * 8), ('order', c.c_uint8 * 8),
                *[(n, c.c_uint32) for n in ('count', 'focused', 'width', 'height')],
                ('damage', Rect), ('damaged', c.c_uint8), ('changes', c.c_uint64)]


def main():
    tools = pinned_tools()
    work = ROOT / 'build/m7/windows'
    work.mkdir(parents=True, exist_ok=True)
    source = ROOT / 'kernel/desktop/window.c'
    flags = ['-std=c17', '-O2', '-Wall', '-Wextra', '-Werror', '-ffreestanding',
             '-fno-builtin', '-fno-stack-protector', '-I', ROOT / 'include']

    def run(args):
        result = subprocess.run(list(map(str, args)), cwd=ROOT, capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stderr.replace(str(ROOT), '<project-root>'))

    run([tools['clang'], '--target=x86_64-unknown-none-elf', *flags, '-mno-red-zone',
         '-c', source, '-o', work / 'window.o'])
    objects = []
    for name, path in [('window', source), ('memory', ROOT / 'runtime/memory.c')]:
        obj = work / (name + '.obj')
        run([tools['clang'], '--target=x86_64-pc-windows-msvc', *flags, '-c', path, '-o', obj])
        objects.append(obj)
    names = ['init', 'open', 'close', 'focus', 'place', 'minimize', 'hit',
             'lookup', 'damage', 'take_damage', 'token']
    run([tools['lld-link'], '/dll', '/noentry', '/nodefaultlib',
         *['/export:desktop_' + name for name in names], *objects,
         '/out:' + str(work / 'window.dll')])
    dll = c.CDLL(str(work / 'window.dll'))
    functions = {
        'init': ([c.POINTER(Model), c.c_uint32, c.c_uint32], None),
        'open': ([c.POINTER(Model), c.c_uint32, Rect], c.c_uint64),
        'close': ([c.POINTER(Model), c.c_uint64], c.c_int),
        'focus': ([c.POINTER(Model), c.c_uint64], c.c_int),
        'place': ([c.POINTER(Model), c.c_uint64, Rect], c.c_int),
        'minimize': ([c.POINTER(Model), c.c_uint64, c.c_int], c.c_int),
        'hit': ([c.POINTER(Model), c.c_int32, c.c_int32], c.c_uint64),
        'take_damage': ([c.POINTER(Model), c.POINTER(Rect)], c.c_int),
        'token': ([c.POINTER(Model), c.c_uint], c.c_uint64),
    }
    for name, (args, result) in functions.items():
        function = getattr(dll, 'desktop_' + name)
        function.argtypes, function.restype = args, result

    model, damage = Model(), Rect()
    dll.desktop_init(c.byref(model), 1280, 800)
    assert dll.desktop_take_damage(c.byref(model), c.byref(damage))
    assert tuple(getattr(damage, n) for n in ('x', 'y', 'width', 'height')) == (0, 0, 1280, 800)
    assert not dll.desktop_take_damage(c.byref(model), c.byref(damage))
    first = dll.desktop_open(c.byref(model), 1, Rect(30, 40, 640, 400))
    second = dll.desktop_open(c.byref(model), 2, Rect(30, 40, 640, 400))
    assert first and second and dll.desktop_hit(c.byref(model), 40, 50) == second
    assert dll.desktop_focus(c.byref(model), first)
    assert dll.desktop_hit(c.byref(model), 40, 50) == first
    assert dll.desktop_minimize(c.byref(model), first, 1)
    assert dll.desktop_hit(c.byref(model), 40, 50) == second
    assert dll.desktop_close(c.byref(model), first)
    replacement = dll.desktop_open(c.byref(model), 3, Rect(0, 0, 400, 300))
    assert replacement != first and replacement & 0xffffffff == first & 0xffffffff
    assert not dll.desktop_close(c.byref(model), first)
    assert not dll.desktop_place(c.byref(model), first, Rect(1, 1, 1000, 700))

    rng = random.Random(7001)
    extremes = [-(2**31), -100000, -1, 0, 1, 199, 320, 8192, 2**31 - 1]
    checks = [{'name': 'stacking/focus, minimize, close/reuse and stale-token rejection', 'passed': True}]
    for _ in range(10000):
        active = [dll.desktop_token(c.byref(model), i) for i in range(8) if model.windows[i].live]
        action = rng.randrange(5)
        token = rng.choice(active) if active else 0
        rect = Rect(*(rng.choice(extremes) for _ in range(4)))
        if action == 0:
            created = dll.desktop_open(c.byref(model), rng.randrange(4), rect)
            assert bool(created) == (len(active) < 8)
        elif token:
            if action == 1:
                assert dll.desktop_place(c.byref(model), token, rect)
            elif action == 2:
                assert dll.desktop_focus(c.byref(model), token)
            elif action == 3:
                assert dll.desktop_minimize(c.byref(model), token, rng.randrange(2))
            else:
                assert dll.desktop_close(c.byref(model), token)
        live = [i for i in range(8) if model.windows[i].live]
        assert model.count == len(live) <= 8
        assert sorted(model.order[:model.count]) == live
        for i in live:
            r = model.windows[i].bounds
            assert 0 <= r.x <= 1280 - r.width and 320 <= r.width <= 1280
            assert 0 <= r.y <= 740 - r.height and 200 <= r.height <= 740
        if model.focused != 8:
            assert model.focused in live and not model.windows[model.focused].minimized
        x, y = rng.randrange(1280), rng.randrange(800)
        expected = 0
        for i in reversed(model.order[:model.count]):
            w = model.windows[i]
            r = w.bounds
            if not w.minimized and r.x <= x < r.x + r.width and r.y <= y < r.y + r.height:
                expected = dll.desktop_token(c.byref(model), i)
                break
        assert dll.desktop_hit(c.byref(model), x, y) == expected
        if dll.desktop_take_damage(c.byref(model), c.byref(damage)):
            assert 0 <= damage.x < damage.x + damage.width <= 1280
            assert 0 <= damage.y < damage.y + damage.height <= 800
        assert not dll.desktop_take_damage(c.byref(model), c.byref(damage))
    checks.append({'name': '10000 seeded lifecycle/hostile-geometry/hit-order cases, bounded damage and idle consumption',
                   'passed': True, 'cases': 10000})
    result = {'passed': True, 'scope': 'actual native C host execution and freestanding compilation only',
              'native_desktop_qualified': False, 'm7_complete': False,
              'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
              'window_model_bytes': c.sizeof(Model), 'checks': checks}
    out = ROOT / 'evidence/M7'
    out.mkdir(parents=True, exist_ok=True)
    (out / 'window-model-results.json').write_bytes((json.dumps(result, indent=2) + '\n').encode())
    print('M7 window model PASS: 10000 cases; no desktop runtime claim')


if __name__ == '__main__':
    main()
