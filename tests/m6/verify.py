"""Freeze exact M6 artifact acceptance only after all real runtime gates pass."""
import hashlib,json,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'evidence/M6'
REPORTS=['layout-results.json','volume-results.json','repository-results.json','qemu-uefi/results.json','qemu-bios/results.json','qemu-uefi-usb/results.json','qemu-bios-usb/results.json','vmware/efi/results.json','vmware/bios/results.json','maintenance/results.json','failures/results.json','resources/results.json','mouse/results.json','terminal/results.json','distribution/results.json']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def load(p):return json.loads(p.read_text(encoding='utf-8'))
def main(code_commit):
    code_commit=subprocess.check_output(['git','rev-parse',code_commit],cwd=ROOT,text=True).strip()
    manifest=load(ROOT/'dist/m6-build-manifest.json');update=load(ROOT/'dist/m6u1-build-manifest.json');custom=load(ROOT/'dist/m6minimal-build-manifest.json');errors=[];gates=[];frozen={}
    for info in [manifest,update,custom]:
        for rel,value in info['source_sha256'].items():
            if sha(ROOT/rel)!=value:errors.append('Changed source input: '+rel)
            committed=subprocess.run(['git','show',code_commit+':'+rel],cwd=ROOT,capture_output=True)
            if committed.returncode or hashlib.sha256(committed.stdout).hexdigest()!=value:errors.append('Build input is not bound to the declared source commit: '+rel)
        for name,item in info['artifacts'].items():
            path=ROOT/'dist'/name
            if not path.is_file() or path.stat().st_size!=item['size'] or sha(path)!=item['sha256']:errors.append('Artifact differs: '+name)
    if manifest['source_sha256']!=update['source_sha256'] or manifest['build_id']==update['build_id']:errors.append('Update variant must be a distinct build of the same frozen sources')
    old_count=0
    for milestone in ['M4.5','M5','M5.1']:
        info=load(ROOT/'evidence'/milestone/'acceptance-results.json')
        if not info['passed']:errors.append('Prior milestone acceptance failed: '+milestone)
        for rel,value in info['evidence_sha256'].items():
            if sha(ROOT/'evidence'/milestone/rel)!=value:errors.append('Prior accepted proof changed: '+milestone+'/'+rel)
            old_count+=1
    # Canonical public transcript formatting, only for this new milestone.
    # Runtime checks consume original serial output. Do not touch accepted older evidence.
    for path in OUT.rglob('*'):
        if path.suffix=='.log':
            text=path.read_text(encoding='utf-8').replace('\r\n','\n');path.write_bytes(('\n'.join(line.rstrip(' \t') for line in text.splitlines())+'\n').encode())
        elif path.suffix=='.json':path.write_bytes(path.read_bytes().replace(b'\r\n',b'\n'))
    for rel in REPORTS:
        path=OUT/rel
        if not path.exists():errors.append('Missing required report: '+rel);continue
        report=load(path)
        if not report.get('passed'):errors.append('Failed required report: '+rel)
        if 'build_id' in report and report['build_id']!=manifest['build_id']:errors.append('Report belongs to another base build: '+rel)
        if 'iso_sha256' in report:
            expected=custom['artifacts']['EngineOS-M6MINIMAL.iso']['sha256'] if rel.startswith('distribution/') else manifest['artifacts']['EngineOS-M6.iso']['sha256']
            if report['iso_sha256']!=expected:errors.append('Report media differs: '+rel)
        if rel=='volume-results.json':
            if report['source_sha256']!=sha(ROOT/'kernel/storage/volume6.c') or report['header_sha256']!=sha(ROOT/'include/engine/storage/volume6.h'):errors.append('Host storage report does not match native C/header')
        if rel=='layout-results.json' and report['source_sha256']!=sha(ROOT/'kernel/install/layout.c'):errors.append('Planner report does not match native source')
        if rel=='maintenance/results.json' and report['update_build_id']!=update['build_id']:errors.append('Maintenance report update version differs')
        if rel=='distribution/results.json' and report['custom_build_id']!=custom['build_id']:errors.append('Custom distribution report differs')
        checks=report.get('checks',[])
        if any(isinstance(c,dict) and not c.get('passed',False) for c in checks):errors.append('A required subcheck failed: '+rel)
        gates.append({'report':rel,'checks':len(checks),'passed':bool(report.get('passed'))});frozen[rel]=sha(path)
    for path in OUT.rglob('*'):
        if path.suffix.lower() in ['.png','.jpg','.log'] and 'history' not in path.relative_to(OUT).parts and path.name!='wizard-preview.jpg':frozen[path.relative_to(OUT).as_posix()]=sha(path)
    result={'passed':not errors,'scope_id':'M6-VM-v1','code_source_commit':code_commit,'build_id':manifest['build_id'],'iso_sha256':manifest['artifacts']['EngineOS-M6.iso']['sha256'],'update_build_id':update['build_id'],'distribution_build_id':custom['build_id'],'gates':gates,'gate_count':len(gates),'check_count':sum(g['checks'] for g in gates),'evidence_sha256':frozen,'prior_frozen_proofs_verified':old_count,'human_usability_study':False,'physical_hardware_qualified':False,'universal_hardware_support':False,'desktop_implemented':False,'windows_dos_runtime_implemented':False,'public_log_normalization':'UTF-8/LF; ANSI and private-root redaction in capture; trailing terminal prompt whitespace trimmed before freezing','errors':errors}
    (OUT/'acceptance-results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print('M6 acceptance '+('PASS' if result['passed'] else 'FAIL')+f": {len(gates)} gates, {result['check_count']} checks")
    for error in errors:print(error)
    if errors:raise SystemExit(1)
if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser();parser.add_argument('--source-commit',required=True);args=parser.parse_args();main(args.source_commit)
