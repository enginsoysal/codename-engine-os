"""Sequential owned VMware controller/firmware lifecycle matrix on one frozen ISO."""
import argparse,hashlib,json,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--resume',action='store_true');a=p.parse_args();meta=json.loads((ROOT/'dist/m10-build-manifest.json').read_text(encoding='utf-8'));iso=meta['artifacts']['EngacerOS-M10.iso']['sha256'];assert hashlib.sha256((ROOT/'dist/EngacerOS-M10.iso').read_bytes()).hexdigest()==iso
 pairs=[(c,f) for c in ['sata','ide','lsilogic','lsisas1068','pvscsi','nvme'] for f in ['efi','bios']]+[('buslogic','bios')];failed=[]
 for controller,firmware in pairs+[('sata','efi-starter')]:
  label=controller+'-'+firmware;path=ROOT/'build/m10/vmware-results'/label/'results.json'
  if a.resume and path.is_file():
   r=json.loads(path.read_text(encoding='utf-8'))
   if r.get('passed') and r.get('build_id')==meta['build_id'] and r.get('iso_sha256')==iso and (firmware!='efi-starter' or r.get('starter_sha256')==hashlib.sha256((ROOT/'dist/EngacerOS-M10-VMware.zip').read_bytes()).hexdigest()):print('Already qualified',label,flush=True);continue
  command=[sys.executable,str(ROOT/'tests/m10/vmware.py'),'--controller',controller,'--firmware',firmware.split('-')[0]]+(['--starter'] if firmware=='efi-starter' else ['--storage-only'])
  print('Starting owned VMware lifecycle',label,flush=True);result=subprocess.run(command,cwd=ROOT)
  assert hashlib.sha256((ROOT/'dist/EngacerOS-M10.iso').read_bytes()).hexdigest()==iso,'Build changed during matrix; freeze inputs and artifacts'
  if result.returncode:failed.append(label)
 print('Controller matrix failures:',failed,flush=True)
 if failed:raise SystemExit(1)
if __name__=='__main__':main()
