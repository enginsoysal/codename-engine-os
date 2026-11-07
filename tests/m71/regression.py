"""Reuse unchanged M7 tasks on M7.1 media without overwriting historical proof.

Only artifact/output roots and the two new disk-screen buttons are adapted.
The original harness inputs and transformed copies are digest-recorded.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import types
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'evidence/M7.1/regression'


def patch_helpers(module,seen=None):
    seen=set() if seen is None else seen
    if id(module) in seen:return
    seen.add(id(module))
    path=str(getattr(module,'__file__',''))
    if not path or not any(part in path.replace('\\','/') for part in ['/tests/','/regression-harness/']):return
    if path.replace('\\','/').endswith('/tests/m6/guided.py') and not getattr(module,'_m71_adapter',False):
        original=module.next
        def next_step(guest,tabs=0):
            count=getattr(guest,'_m71_initial_steps',0)+1;guest._m71_initial_steps=count
            # M7 already explicitly selects the row; M7.1 adds Manage disks and Erase/install before Next.
            if count==3 and tabs in [2,3]:tabs+=2
            return original(guest,tabs)
        module.next=next_step;module._m71_adapter=True
    for value in list(vars(module).values()):
        if isinstance(value,types.ModuleType):patch_helpers(value,seen)


def main():
    parser=argparse.ArgumentParser();parser.add_argument('task',choices=['isolation','resources','distribution','https_negative','firmware']);parser.add_argument('--firmware',choices=['uefi','bios'],default='uefi');parser.add_argument('--media',choices=['iso','usb'],default='usb');a=parser.parse_args()
    stage=ROOT/'build/m71/regression-harness'/a.task;stage.mkdir(parents=True,exist_ok=True);snapshots={}
    for p in sorted((ROOT/'tests/m7').glob('*.py')):
        source=p.read_text(encoding='utf-8')
        adapted=source.replace("ROOT=Path(__file__).resolve().parents[2]","ROOT=Path(__import__('os').environ['ENGINEOS_TEST_ROOT'])")
        adapted=adapted.replace('EngineOS-M7','EngineOS-M71').replace('m7night-build-manifest','m71night-build-manifest').replace('m7-build-manifest','m71-build-manifest').replace('evidence/M7','evidence/M7.1/regression')
        target=stage/p.name;target.write_bytes(adapted.encode())
        snapshots[p.relative_to(ROOT).as_posix()]={'original_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'adapted_sha256':hashlib.sha256(target.read_bytes()).hexdigest()}
    os.environ['ENGINEOS_TEST_ROOT']=str(ROOT)
    path=stage/(a.task+'.py');spec=importlib.util.spec_from_file_location('m71_'+a.task,path);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);patch_helpers(module)
    label=a.task+('-'+a.firmware+'-'+a.media if a.task=='firmware' else '')
    OUT.mkdir(parents=True,exist_ok=True);(OUT/(label+'-harness.json')).write_bytes((json.dumps({'original_inputs':snapshots,'adapter':'artifact/output roots plus two extra disk-screen tabs; no native source changes'},indent=2)+'\n').encode())
    sys.argv=[str(path)]+(['--firmware',a.firmware,'--media',a.media] if a.task=='firmware' else [])
    module.main()


if __name__=='__main__':main()
