"""Close M11.1 only from matching production and native candidate evidence."""
import hashlib
import json
import os
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'evidence/M11/1'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    meta = json.loads((ROOT / 'dist/m11stage1-build-manifest.json').read_text(encoding='utf-8'))
    native = json.loads((ROOT / 'build/m11/selection/results.json').read_text(encoding='utf-8'))
    host = json.loads((ROOT / 'build/m11/input-host/results.json').read_text(encoding='utf-8'))
    assert native['passed'] and host['passed'], 'Both production and actual native checks must pass'
    assert meta['milestone'] == 'M11' and meta['build_revision'] == 'stage1'
    assert native['iso_sha256'] == meta['artifacts']['EngacerOS-M11STAGE1.iso']['sha256']
    for name, expected in meta['source_sha256'].items():
        assert digest(ROOT / name) == expected, ('Changed candidate source', name)
    for name, expected in meta['artifacts'].items():
        assert digest(ROOT / 'dist' / name) == expected['sha256'], ('Changed artifact', name)
    for name, expected in host['source_sha256'].items():
        assert digest(ROOT / name) == expected, ('Changed production test input', name)

    owned = Path(os.environ['LOCALAPPDATA']) / 'EngineOS/Tests/M6/m11-selection'
    latest = max(owned.iterdir(), key=lambda p: p.stat().st_mtime_ns)
    logs = {}
    for folder in ('install', 'disk-only'):
        log = (latest / folder / 'serial.log').read_text(encoding='utf-8', errors='replace')
        assert 'M7_DESKTOP_READY build=' + meta['build_id'] in log, folder
        assert 'KERNEL_PANIC' not in log and 'KERNEL_REJECT:' not in log, folder
        logs[folder] = log

    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / 'native').mkdir(exist_ok=True)
    for path in (ROOT / 'build/m11/selection').glob('*.png'):
        shutil.copyfile(path, OUT / 'native' / path.name)
        capture = path.with_name(path.stem + '-capture.json')
        if capture.exists():
            shutil.copyfile(capture, OUT / 'native' / capture.name)
    for folder, log in logs.items():
        (OUT / 'native' / (folder + '.log')).write_bytes(log.encode('utf-8'))
    for filename, value in [('candidate-manifest.json', meta), ('native-results.json', native), ('host-results.json', host)]:
        (OUT / filename).write_bytes((json.dumps(value, indent=2) + '\n').encode('utf-8'))
    report = {
        'passed': True,
        'stage': 'M11.1',
        'scope': 'Native interaction stage; not final M11 media qualification',
        'build_id': meta['build_id'],
        'iso_sha256': native['iso_sha256'],
        'source_inputs_verified': len(meta['source_sha256']),
        'artifacts_verified': len(meta['artifacts']),
        'native_tasks': native['checks'],
        'host_checks': host['checks'],
        'independent_boot': True,
        'publication': 'Separate required public website gate before M11.2',
    }
    (OUT / 'acceptance-results.json').write_bytes((json.dumps(report, indent=2) + '\n').encode('utf-8'))
    print('M11.1 matching-source/native acceptance PASS')


if __name__ == '__main__':
    main()
