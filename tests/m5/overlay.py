"""Prove the native live-memory cap, denial and complete test-page reclamation."""
from pathlib import Path
import json,struct,subprocess,sys
ROOT=Path(__file__).resolve().parents[2];sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/build')]
from vm import arguments,qemu_tools
from media import package

def main():
    work=ROOT/'build/m5/overlay';work.mkdir(parents=True,exist_ok=True);out=ROOT/'evidence/M5/overlay';out.mkdir(parents=True,exist_ok=True);build=ROOT/'build/m5';manifest=json.loads((ROOT/'dist/m5-build-manifest.json').read_text(encoding='utf-8'))
    package(work,(build/'BOOTX64.EFI').read_bytes(),(build/'KERNEL.ELF').read_bytes(),struct.pack('<8sII32s',b'ENGINIT1',48,7,manifest['build_id'].encode()),'M5')
    argv,serial=arguments(qemu_tools(),'iso',work/'EngineOS-M5.iso',work/'guest',ram=64);p=subprocess.run(argv,capture_output=True,timeout=90,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));log=serial.read_text(encoding='utf-8',errors='replace');passed=p.returncode==33 and all(s in log for s in ['M3_COMPLETE_PASS','M5_OVERLAY_PASS','M5_OVERLAY_RELEASE_BASELINE_RESTORED'])
    (out/'serial.log').write_text(log,encoding='utf-8');(out/'results.json').write_text(json.dumps({'passed':passed,'build_id':manifest['build_id'],'guest_ram_mib':64,'overlay_cap_bytes':524288,'checks':[{'name':'128-page cap, 129th-page denial, reuse without allocation and exact allocator baseline after cleanup','passed':passed}]},indent=2)+'\n',encoding='utf-8');print('Native overlay', 'PASS' if passed else 'FAIL',flush=True)
    if not passed:raise SystemExit(1)
if __name__=='__main__':main()
