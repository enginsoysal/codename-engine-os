"""Repeat native pressure checks with four guest CPUs sharing one owned host CPU."""
from pathlib import Path
import argparse,ctypes,hashlib,json,re,struct,subprocess,sys
from ctypes import wintypes as w
sys.path[:0]=[str(Path(__file__).resolve().parents[2]/'tools'),str(Path(__file__).resolve().parents[2]/'tools/build')]
from vm import ROOT,arguments,qemu_tools
from media import package

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--runs',type=int,default=8);parser.add_argument('--label',default='baseline');options=parser.parse_args()
    if not 1<=options.runs<=32 or not re.fullmatch('[a-z0-9-]+',options.label):raise ValueError('Bounded test arguments required')
    work=ROOT/'build/m45/smp-pressure'/options.label;work.mkdir(parents=True,exist_ok=True);out=ROOT/'evidence/M4.5/smp-pressure'/options.label;out.mkdir(parents=True,exist_ok=True);build=ROOT/'build/m45';manifest=json.loads((ROOT/'dist/m45-build-manifest.json').read_text(encoding='utf-8'));package(work,(build/'BOOTX64.EFI').read_bytes(),(build/'KERNEL.ELF').read_bytes(),struct.pack('<8sII32s',b'ENGINIT1',48,1,manifest['build_id'].encode()),'M45');checks=[]
    api=ctypes.WinDLL('kernel32',use_last_error=True);size=ctypes.c_size_t;api.GetProcessAffinityMask.argtypes=[w.HANDLE,ctypes.POINTER(size),ctypes.POINTER(size)];api.GetProcessAffinityMask.restype=w.BOOL;api.SetProcessAffinityMask.argtypes=[w.HANDLE,size];api.SetProcessAffinityMask.restype=w.BOOL
    for index in range(options.runs):
        args,serial=arguments(qemu_tools(),'iso',work/'EngineOS-M45.iso',work/str(index),ram=128);args[args.index('-smp')+1]='4';process=subprocess.Popen(args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));mask=system=size();assigned=0
        try:
            mask=size();system=size()
            if not api.GetProcessAffinityMask(int(process._handle),ctypes.byref(mask),ctypes.byref(system)):raise ctypes.WinError(ctypes.get_last_error())
            assigned=mask.value&-mask.value
            if not api.SetProcessAffinityMask(int(process._handle),assigned):raise ctypes.WinError(ctypes.get_last_error())
            stdout,stderr=process.communicate(timeout=90)
        finally:
            if process.poll() is None:process.terminate();process.wait(timeout=5)
        log=serial.read_text(encoding='utf-8',errors='replace') if serial.exists() else '';log=re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]','',log);(out/(str(index)+'.log')).write_bytes(('\n'.join(line.rstrip() for line in log.splitlines())+'\n').encode());passed=process.returncode==33 and 'M3_COMPLETE_PASS' in log and 'pressure_checks=0x0000000000000005' in log;checks.append({'name':f'four guest CPUs on one host logical CPU, run {index}','passed':passed,'exit':process.returncode,'owned_process_affinity_mask':assigned,'error':stderr.decode(errors='replace').replace(str(ROOT),'<project-root>') if process.returncode!=33 else ''});(out/'results.json').write_bytes((json.dumps({'build_id':manifest['build_id'],'full_m45_complete':False,'checks':checks},indent=2)+'\n').encode());print(index,'PASS' if passed else 'FAIL',flush=True)
    if not all(c['passed'] for c in checks):raise SystemExit(1)
if __name__=='__main__':main()
