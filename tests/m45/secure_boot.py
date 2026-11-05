"""Owner-enrolled Secure Boot in disposable TCG NVRAM only; never host firmware."""
from pathlib import Path
import hashlib,json,re,shutil,subprocess,sys,time
sys.path[:0]=[str(Path(__file__).resolve().parents[2]/'tools'),str(Path(__file__).resolve().parents[2]/'tools/build')]
from vm import ROOT,arguments,qemu_tools
from secure import prepare

def main():
    provenance=prepare();work=ROOT/'build/m45/secure';out=ROOT/'evidence/M4.5';out.mkdir(parents=True,exist_ok=True);files=qemu_tools();secure=files['ovmf-code'].parent/'edk2-x86_64-secure-code.fd'
    if not secure.is_file():raise RuntimeError('Secure firmware unavailable')
    lock=json.loads((ROOT/'tools/security-toolchain.json').read_text(encoding='utf-8'))
    if hashlib.sha256(secure.read_bytes()).hexdigest()!=lock['files']['ovmf-secure-code']['sha256']:raise RuntimeError('Secure firmware differs from lock')
    provenance['secure_firmware_sha256']=hashlib.sha256(secure.read_bytes()).hexdigest();checks=[]
    def launch(name,media,enrolled=False,persistent=False):
        case=work/name;a,serial=arguments(files,'iso',work/media/'EngineOS-M45.iso',case,ram=256)
        a[a.index('-machine')+1]='pc-q35-11.1,smm=on';a+=['-global','driver=cfi.pflash01,property=secure,value=on']
        for i,arg in enumerate(a):
            if arg.startswith('if=pflash,format=raw,unit=0,'):a[i]='if=pflash,format=raw,unit=0,readonly=on,file='+str(secure)
        if enrolled:shutil.copyfile(work/'enroll/vars.fd',case/'vars.fd')
        p=subprocess.Popen(a,stdout=subprocess.PIPE,stderr=subprocess.PIPE,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));end=time.monotonic()+(60 if persistent else 25)
        try:
            while time.monotonic()<end and p.poll() is None:
                log=serial.read_text(encoding='utf-8',errors='replace') if serial.exists() else ''
                if persistent and ('M4_CONSOLE_READY' in log or 'LOADER_REJECT:' in log or 'KERNEL_PANIC' in log):break
                time.sleep(.1)
            log=serial.read_text(encoding='utf-8',errors='replace') if serial.exists() else '';code=p.poll()
        finally:
            if p.poll() is None:p.terminate();p.wait(timeout=5)
        err=p.stderr.read().decode(errors='replace').replace(str(ROOT),'<project-root>');clean=re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]','',log);(out/(name+'-secure.log')).write_bytes(clean.encode())
        return log,code,err
    log,code,error=launch('enroll','enroll-media')
    checks.append({'name':'TCG-only enroller accepts only empty SetupMode policy; writes disposable VM db/KEK/PK','passed':code==33 and 'ENROLL_TEST_POLICY_PASS' in log,'exit':code})
    if not checks[-1]['passed']:
        (out/'secure-boot-results.json').write_bytes((json.dumps({'checks':checks,'error':error,'provenance':provenance,'full_m45_complete':False},indent=2)+'\n').encode());raise SystemExit('VM key enrollment failed')
    log,code,error=launch('existing-policy-refusal','enroll-refusal-media',enrolled=True)
    checks.append({'name':'signed test enroller refuses nonempty existing platform policy before any SetVariable','passed':code==101 and 'ENROLL_REFUSED_EXISTING_POLICY' in log,'exit':code})
    log,code,error=launch('disabled','unsigned-media',persistent=True)
    checks.append({'name':'verified loader with Secure Boot disabled still checks payload hashes','passed':'M4_CONSOLE_READY' in log and 'ENGINE_TRUST_POLICY SecureBoot=0x0000000000000000' in log and 'SIGNED_LOADER_PAYLOAD_SHA256_OK' in log})
    for name,media,expected in [('signed','signed-media','boot'),('unsigned','unsigned-media','firmware-reject'),('tampered-loader','bad-loader-media','firmware-reject'),('tampered-kernel','bad-kernel-media','loader-reject'),('tampered-init','bad-init-media','loader-reject')]:
        log,code,error=launch(name,media,enrolled=True,persistent=expected=='boot')
        if expected=='boot':passed='M4_CONSOLE_READY' in log and 'ENGINE_TRUST_POLICY SecureBoot=0x0000000000000001 SetupMode=0x0000000000000000' in log and 'SIGNED_LOADER_PAYLOAD_SHA256_OK' in log
        elif expected=='loader-reject':passed=code==69 and 'LOADER_REJECT: payload SHA256' in log and 'ENGINE_OS_KERNEL_ENTRY' not in log
        else:passed='ENGINE_OS_LOADER' not in log and bool(re.search(r'Security Violation|Access Denied|Security.*Violation',log,re.I))
        checks.append({'name':name,'passed':passed,'exit':code,'expected':expected});print(name,'PASS' if passed else 'FAIL',flush=True)
    (out/'secure-boot-results.json').write_bytes((json.dumps({'checks':checks,'provenance':provenance,'full_m45_complete':False,'scope':'owner-enrolled disposable QEMU/OVMF trust only; no factory/OEM trust or physical qualification'},indent=2)+'\n').encode())
    if not all(c['passed'] for c in checks):raise SystemExit(1)
if __name__=='__main__':main()
