"""Owner-enrolled Secure Boot in disposable TCG NVRAM only; never host firmware."""
from pathlib import Path
import hashlib,json,re,secrets,shutil,struct,subprocess,sys,time
from datetime import datetime,timedelta,timezone
sys.path[:0]=[str(Path(__file__).resolve().parents[2]/'tools'),str(Path(__file__).resolve().parents[2]/'tools/build')]
from vm import ROOT,arguments,qemu_tools
from secure import prepare
from owner import load_identity,sign_image
from policy_media import esl,authenticated_db,build_policy
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.serialization import pkcs12

def main():
    out=ROOT/'evidence/M4.5';out.mkdir(parents=True,exist_ok=True)
    (out/'secure-boot-results.json').write_bytes(b'{"checks": [], "state": "checking", "full_m45_complete": false}\n')
    provenance=prepare();work=ROOT/'build/m45/secure';files=qemu_tools();secure=files['ovmf-code'].parent/'edk2-x86_64-secure-code.fd'
    if not secure.is_file():raise RuntimeError('Secure firmware unavailable')
    lock=json.loads((ROOT/'tools/security-toolchain.json').read_text(encoding='utf-8'))
    if hashlib.sha256(secure.read_bytes()).hexdigest()!=lock['files']['ovmf-secure-code']['sha256']:raise RuntimeError('Secure firmware differs from lock')
    provenance['secure_firmware_sha256']=hashlib.sha256(secure.read_bytes()).hexdigest();checks=[]
    def launch(name,media,enrolled=False,persistent=False,policy=None):
        case=work/name;a,serial=arguments(files,'iso',work/media/'EngineOS-M45.iso',case,ram=256)
        a[a.index('-machine')+1]='pc-q35-11.1,smm=on';a+=['-global','driver=cfi.pflash01,property=secure,value=on']
        for i,arg in enumerate(a):
            if arg.startswith('if=pflash,format=raw,unit=0,'):a[i]='if=pflash,format=raw,unit=0,readonly=on,file='+str(secure)
        if enrolled:shutil.copyfile(policy if policy is not None else work/'enroll/vars.fd',case/'vars.fd')
        p=subprocess.Popen(a,stdout=subprocess.PIPE,stderr=subprocess.PIPE,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));end=time.monotonic()+(60 if persistent else 25)
        try:
            while time.monotonic()<end and p.poll() is None:
                log=serial.read_text(encoding='utf-8',errors='replace') if serial.exists() else ''
                if persistent and ('M4_CONSOLE_READY' in log or 'LOADER_REJECT:' in log or 'KERNEL_PANIC' in log):break
                time.sleep(.1)
            log=serial.read_text(encoding='utf-8',errors='replace') if serial.exists() else '';code=p.poll()
        finally:
            if p.poll() is None:p.terminate();p.wait(timeout=5)
        err=p.stderr.read().decode(errors='replace').replace(str(ROOT),'<project-root>');clean=re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]','',log);(out/(name+'-secure.log')).write_bytes(('\n'.join(line.rstrip() for line in clean.splitlines())+'\n').encode())
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
    # Reuse an encrypted identity across a changed authenticated payload.
    pin=provenance['public_certificate_sha256'];owner_file=work/'test-owner-private.pfx';identity=load_identity(owner_file,None,pin)
    for label,pfx_arg,pin_arg in [('missing-owner-pfx',None,pin),('missing-owner-pin',owner_file,None)]:
        refused=False
        try:prepare(owner_pfx=pfx_arg,owner_sha256=pin_arg,work_name='secure-incomplete-owner')
        except ValueError:refused=True
        checks.append({'name':label,'passed':refused and not (ROOT/'build/m45/secure-incomplete-owner').exists()})
    password=secrets.token_urlsafe(32).encode();encrypted=work/'persistent-owner-encrypted.pfx'
    encrypted.write_bytes(pkcs12.serialize_key_and_certificates(b'owner',*identity,None,serialization.BestAvailableEncryption(password)))
    for label,secret,expected in [('wrong-owner-pin',password,'0'*64),('wrong-owner-password',b'incorrect',pin)]:
        refused=False
        try:load_identity(encrypted,secret,expected)
        except ValueError:refused=True
        checks.append({'name':label,'passed':refused})
    invalid_image=work/'invalid-pe.efi';invalid_image.write_bytes(b'not a PE image');refused=False
    try:sign_image(Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64/signtool.exe'),invalid_image,*identity,work)
    except subprocess.CalledProcessError as exception:refused=Path(exception.cmd[0]).name.lower()=='signtool.exe'
    checks.append({'name':'failed PE signing removes temporary plaintext PFX and owner-only directory','passed':refused and not list(work.glob('private-sign-*'))})
    changed=struct.pack('<8sII32s',b'ENGINIT1',48,1,provenance['build_id'].encode())
    update=prepare(encrypted,password,pin,'secure-update',changed)
    checks.append({'name':'encrypted persistent owner signs changed init; same enrolled policy accepts update','passed':update['identity_reused'] and update['public_certificate_sha256']==pin and update['init_sha256']!=provenance['init_sha256']})
    log,code,error=launch('owner-update',ROOT/'build/m45/secure-update/signed-media',enrolled=True,persistent=True)
    checks[-1]['temporary_private_key_files_removed']=not list((ROOT/'build/m45/secure-update').glob('private-sign-*'))
    checks[-1]['passed']=checks[-1]['passed'] and checks[-1]['temporary_private_key_files_removed'] and code==33 and 'SIGNED_LOADER_PAYLOAD_SHA256_OK' in log and 'M45_DISCOVERY_PASS' in log
    foreign=prepare(work_name='secure-foreign');foreign_identity=load_identity(ROOT/'build/m45/secure-foreign/test-owner-private.pfx',None,foreign['public_certificate_sha256'])
    log,code,error=launch('foreign-owner',ROOT/'build/m45/secure-foreign/signed-media',enrolled=True)
    checks.append({'name':'foreign signing identity denied by existing policy','passed':'ENGINE_OS_LOADER' not in log and bool(re.search(r'Security Violation|Access Denied',log,re.I))})
    from media import package
    package(work/'stale-loader-update', (work/'verified-signed.efi').read_bytes(),(ROOT/'build/m45/KERNEL.ELF').read_bytes(),changed,'M45')
    log,code,error=launch('stale-loader-update','stale-loader-update',enrolled=True)
    checks.append({'name':'old signed loader rejects changed init without regenerated signed digest','passed':code==69 and 'LOADER_REJECT: payload SHA256' in log})
    moment=datetime.now(timezone.utc)+timedelta(seconds=2)
    bad_auth=authenticated_db(foreign_identity,esl(foreign_identity[1]),moment)
    bad_media=build_policy('policy-unauthorized',identity,identity,identity,identity,bad_auth,True)
    log,code,error=launch('unauthorized-policy',bad_media,enrolled=True)
    checks.append({'name':'foreign authenticated db update denied; PK KEK and db unchanged','passed':code==33 and 'POLICY_UPDATE_TEST_PASS' in log})
    auth=authenticated_db(identity,esl(foreign_identity[1]),moment)
    rotated=build_policy('policy-rotation',identity,identity,identity,foreign_identity,auth)
    log,code,error=launch('rotate-policy',rotated,enrolled=True)
    checks.append({'name':'KEK-authorized db replacement; exact new db and unchanged PK/KEK read back','passed':code==33 and 'POLICY_UPDATE_TEST_PASS' in log})
    rotated_vars=work/'rotate-policy/vars.fd'
    log,code,error=launch('rotated-owner',ROOT/'build/m45/secure-foreign/signed-media',enrolled=True,persistent=True,policy=rotated_vars)
    checks.append({'name':'new owner boots after authenticated db rotation with Secure Boot enabled','passed':'M4_CONSOLE_READY' in log and 'SecureBoot=0x0000000000000001 SetupMode=0x0000000000000000' in log})
    log,code,error=launch('retired-owner','signed-media',enrolled=True,policy=rotated_vars)
    checks.append({'name':'retired image signer no longer boots under replacement db','passed':'ENGINE_OS_LOADER' not in log and bool(re.search(r'Security Violation|Access Denied',log,re.I))})
    escalation=authenticated_db(foreign_identity,esl(identity[1]),moment+timedelta(seconds=2))
    limited=build_policy('policy-image-role',foreign_identity,identity,foreign_identity,foreign_identity,escalation,True)
    log,code,error=launch('image-key-not-kek',limited,enrolled=True,policy=rotated_vars)
    checks.append({'name':'trusted image signer cannot replace db without KEK authority','passed':code==33 and 'POLICY_UPDATE_TEST_PASS' in log})
    replay=build_policy('policy-replay',foreign_identity,identity,foreign_identity,foreign_identity,auth,True)
    log,code,error=launch('replayed-policy',replay,enrolled=True,policy=rotated_vars)
    checks.append({'name':'replayed authenticated db update denied; new db and authority unchanged','passed':code==33 and 'POLICY_UPDATE_TEST_PASS' in log})
    log,code,error=launch('policy-backup-recovery','signed-media',enrolled=True,persistent=True)
    checks.append({'name':'explicit original VM policy snapshot restores original signed boot','passed':'M4_CONSOLE_READY' in log and 'SecureBoot=0x0000000000000001 SetupMode=0x0000000000000000' in log})
    provenance['persistent_owner_and_rotation_sources']={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in ('tools/build/owner.py','tests/m45/policy_media.py','tests/m45/firmware/policy.c')}
    provenance['updated_payload']=update;provenance['rotated_owner_certificate_sha256']=foreign['public_certificate_sha256']
    for item in checks:print(item['name'],'PASS' if item['passed'] else 'FAIL',flush=True)
    (out/'secure-boot-results.json').write_bytes((json.dumps({'checks':checks,'provenance':provenance,'full_m45_complete':False,'scope':'persistent pinned encrypted owner, payload update and authenticated db rotation/replay/recovery in disposable QEMU/OVMF only; no factory/OEM trust or physical qualification'},indent=2)+'\n').encode())
    if len(checks)!=23 or not all(c['passed'] for c in checks):raise SystemExit(1)
if __name__=='__main__':main()
