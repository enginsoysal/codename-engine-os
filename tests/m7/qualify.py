"""Run bounded M7 gate groups against already-built, immutable candidate artifacts."""
from concurrent.futures import ThreadPoolExecutor,as_completed
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'evidence/M7'
HOST=[('windows',[],'window-model-results.json'),('media',[],'media-authoring-results.json'),('wire',[],'wire-results.json'),('tcp',[],'tcp-results.json'),('tls',[],'tls-policy-results.json'),('clock',[],'clock-results.json'),('http',[],'http-results.json'),('stream',[],'stream-results.json'),('profiles',[],'profile-results.json')]
NATIVE=[('usability',[],'usability/results.json'),('resources',[],'resources/results.json'),('session',[],'session/results.json'),('isolation',[],'isolation/results.json'),
        ('network',['--nic','e1000'],'network/results.json'),('network',['--nic','e1000e'],'network-e1000e/results.json'),('https',[],'https/results.json'),('https_negative',[],'https-negative/results.json'),('binary_download',[],'binary-download/results.json')]
NATIVE += [('firmware',['--firmware',firmware,'--media',media],'firmware/'+firmware+'-'+media+'/results.json') for firmware in ['uefi','bios'] for media in ['iso','usb']]
CUSTOM=[('distribution',[],'distribution/results.json'),('maintenance',[],'maintenance/results.json')]
VMWARE=[('vmware',['--firmware',firmware,'--visual'],'vmware/'+firmware+'-visual/results.json') for firmware in ['efi','bios']]


def sha(path):
    digest=hashlib.sha256()
    with Path(path).open('rb') as file:
        for block in iter(lambda:file.read(1024*1024),b''):digest.update(block)
    return digest.hexdigest()


def main():
    parser=argparse.ArgumentParser();parser.add_argument('group',choices=['host','native','custom','vmware']);args=parser.parse_args()
    manifest=json.loads((ROOT/'dist/m7-build-manifest.json').read_text(encoding='utf-8'))
    sources=manifest['source_sha256']
    def verify_inputs():
        for name,digest in sources.items():assert sha(ROOT/name)==digest,'Native input changed: '+name
        for name,value in manifest['artifacts'].items():assert sha(ROOT/'dist'/name)==value['sha256'],'Candidate artifact changed: '+name
    verify_inputs()
    harness=sorted((ROOT/'tests/m7').rglob('*.py'))+sorted((ROOT/'tests/m7/fixtures').glob('*.json'))
    harness += [ROOT/'tests/m6'/n for n in ['vm.py','guided.py','workspace.py','volume.py','vmware.py']]
    harness += [ROOT/'tools/vm/config.py',ROOT/'tools/toolchain-lock.json',ROOT/'tools/security-toolchain.json']
    harness_sha={p.relative_to(ROOT).as_posix():sha(p) for p in harness}
    tasks={'host':HOST,'native':NATIVE,'custom':CUSTOM,'vmware':VMWARE}[args.group]
    logs=OUT/'gate-logs';logs.mkdir(parents=True,exist_ok=True)
    def execute(task):
        name,parameters,report=task;label=name+('-'+'-'.join(p.strip('-') for p in parameters) if parameters else '')
        command=[sys.executable,str(ROOT/'tests/m7'/(name+'.py')),*parameters]
        result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=1800,
                              creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
        text=(result.stdout+result.stderr).replace(str(ROOT),'<project-root>')
        log=logs/(label+'.log');log.write_bytes(text.replace('\r\n','\n').encode('utf-8'))
        path=OUT/report;data=json.loads(path.read_text(encoding='utf-8')) if path.exists() else {}
        if path.exists():path.write_bytes(path.read_bytes().replace(b'\r\n',b'\n'))
        passed=result.returncode==0 and data.get('passed') is True
        print(args.group+' '+label+' '+('PASS' if passed else 'FAIL'),flush=True)
        return {'test':name,'arguments':parameters,'report':report,'passed':passed,'returncode':result.returncode,'report_sha256':sha(path) if path.exists() else None,'log':log.relative_to(OUT).as_posix()}
    # VMware inventory checks must be serialized. Host compilers share bounded caches.
    workers=2 if args.group in ['native','custom'] else 1
    with ThreadPoolExecutor(max_workers=workers) as pool:
        futures=[pool.submit(execute,task) for task in tasks];results=[future.result() for future in as_completed(futures)]
    verify_inputs()
    assert all(sha(ROOT/name)==digest for name,digest in harness_sha.items()),'Test harness changed during qualification'
    result={'passed':all(item['passed'] for item in results),'m7_complete':False,'group':args.group,
            'build_id':manifest['build_id'],'iso_sha256':manifest['artifacts']['EngineOS-M7.iso']['sha256'],
            'native_source_snapshot_sha256':hashlib.sha256(json.dumps(sources,sort_keys=True).encode()).hexdigest(),
            'test_harness_sha256':harness_sha,'gates':results,
            'scope':'host checks are not native proof; this group alone never establishes full M7 acceptance'}
    (OUT/(args.group+'-gate-results.json')).write_bytes((json.dumps(result,indent=2)+'\n').encode('utf-8'))
    if not result['passed']:raise SystemExit(1)


if __name__=='__main__':main()
