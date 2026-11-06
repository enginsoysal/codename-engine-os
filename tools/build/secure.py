"""Build an owner-signed test loader with pinned payload hashes. Private keys stay ignored.

The enrollment helper is TCG-only and is never copied to dist or a release.
This does not obtain trust from Microsoft/OEM factory databases.
"""
from pathlib import Path
from datetime import datetime,timedelta,timezone
import hashlib,json,struct,subprocess,sys,uuid
from m1 import ROOT,pinned_tools
from media import package
from owner import load_identity,sign_image

def prepare(owner_pfx=None,owner_password=None,owner_sha256=None,work_name="secure",init_override=None):
    if (owner_pfx is None)!=(owner_sha256 is None) or (owner_pfx is None and owner_password is not None):raise ValueError('Owner PFX and approved certificate pin must be supplied together')
    import cryptography
    lock=json.loads((ROOT/'tools/security-toolchain.json').read_text(encoding='utf-8'))
    if cryptography.__version__!=lock['cryptography_version']:raise RuntimeError('Security test package differs from lock')
    from cryptography import x509
    from cryptography.hazmat.primitives import hashes,serialization
    from cryptography.hazmat.primitives.asymmetric import rsa
    from cryptography.hazmat.primitives.serialization import pkcs12
    from cryptography.hazmat.primitives.serialization import pkcs7
    from cryptography.x509.oid import NameOID,ExtendedKeyUsageOID
    build=ROOT/'build/m45'
    if not work_name or any(c not in 'abcdefghijklmnopqrstuvwxyz0123456789-' for c in work_name):raise ValueError('Invalid isolated signing work name')
    work=build/work_name;work.mkdir(parents=True,exist_ok=True);tools=pinned_tools();manifest=json.loads((ROOT/'dist/m45-build-manifest.json').read_text(encoding='utf-8'))
    now=datetime.now(timezone.utc)
    if owner_pfx is not None:key,cert=load_identity(owner_pfx,owner_password,owner_sha256 or '')
    else:
        key=rsa.generate_private_key(public_exponent=65537,key_size=2048);name=x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'Engine OS disposable VM test owner')])
        cert=x509.CertificateBuilder().subject_name(name).issuer_name(name).public_key(key.public_key()).serial_number(x509.random_serial_number()).not_valid_before(now-timedelta(days=1)).not_valid_after(now+timedelta(days=365)).add_extension(x509.BasicConstraints(ca=True,path_length=None),critical=True).add_extension(x509.ExtendedKeyUsage([ExtendedKeyUsageOID.CODE_SIGNING]),critical=False).sign(key,hashes.SHA256())
    certificate=cert.public_bytes(serialization.Encoding.DER);(work/'test-owner.cer').write_bytes(certificate);pfx=work/'test-owner-private.pfx'
    if owner_pfx is None:pfx.write_bytes(pkcs12.serialize_key_and_certificates(b'engine-test',key,cert,None,serialization.NoEncryption()))
    owner=uuid.UUID('8be4df61-93ca-11d2-aa0d-00e098032b8c').bytes_le;x509_guid=uuid.UUID('a5c059a1-94e4-4aa7-87b5-ab155c2bf072').bytes_le;pkcs7_guid=uuid.UUID('4aafd29d-68df-49ee-8aa9-347d375665a7').bytes_le
    timestamp=struct.pack('<HBBBBBBIhBB',now.year,now.month,now.day,now.hour,now.minute,now.second,0,0,0,0,0);esl=x509_guid+struct.pack('<III',28+16+len(certificate),0,16+len(certificate))+owner+certificate
    to_sign='PK'.encode('utf-16le')+owner+struct.pack('<I',0x27)+timestamp+esl
    cms=pkcs7.PKCS7SignatureBuilder().set_data(to_sign).add_signer(cert,key,hashes.SHA256()).sign(serialization.Encoding.DER,[pkcs7.PKCS7Options.DetachedSignature,pkcs7.PKCS7Options.Binary])
    pk_auth=timestamp+struct.pack('<IHH',24+len(cms),0x200,0xef1)+pkcs7_guid+cms+esl
    (work/'test-certificate.h').write_bytes(('static const unsigned char test_certificate[]={'+','.join(map(str,certificate))+'};\nstatic const unsigned char test_pk_auth[]={'+','.join(map(str,pk_auth))+'};\n').encode())
    kernel=(build/'KERNEL.ELF').read_bytes();init=(build/'INIT.BIN').read_bytes() if init_override is None else init_override;trusted=''
    for name,expected in manifest['source_sha256'].items():
        if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=expected:raise RuntimeError('Rebuild before signing: source changed')
    if hashlib.sha256(kernel).hexdigest()!=manifest['artifacts']['EngineOS-M45-kernel.elf']['sha256']:raise RuntimeError('Kernel differs from build manifest')
    if len(init)!=48 or init[:12]!=struct.pack('<8sI',b'ENGINIT1',48) or struct.unpack_from('<I',init,12)[0]>6 or init[16:]!=manifest['build_id'].encode():raise RuntimeError('Init does not match the native build contract')
    for label,data in (('kernel',kernel),('init',init)):trusted+='static const unsigned char trusted_'+label+'_sha256[]={'+','.join(map(str,hashlib.sha256(data).digest()))+'};\n'
    (work/'payload-digests.h').write_bytes(trusted.encode());(work/'build.h').write_bytes(('#define BUILD_ID "'+manifest['build_id']+'"\n').encode())
    def compile(source,stem,defines=()):
        obj=work/(stem+'.obj');args=[tools['clang'],'--target=x86_64-pc-windows-msvc','-std=c17','-O2','-ffreestanding','-fno-builtin','-fno-stack-protector','-mno-red-zone','-mgeneral-regs-only','-Wall','-Wextra','-Werror','-Iinclude','-Iboot/uefi','-I'+str(work),*defines,'-c',source,'-o',str(obj)]
        subprocess.run(args,cwd=ROOT,check=True,capture_output=True);return obj
    memory=compile('runtime/memory.c','memory');handoff=compile('boot/uefi/handoff.S','handoff');digest=compile('runtime/sha256.c','sha256');trust=compile('boot/uefi/payload_trust.c','trust')
    loader=compile('boot/uefi/loader.c','loader',('-DENGINE_M2','-DENGINE_M3','-DENGINE_M4','-DENGINE_M45','-DENGINE_SECURE_PAYLOAD'))
    verified=work/'verified-unsigned.efi';subprocess.run([tools['lld-link'],'/subsystem:efi_application','/entry:efi_main','/nodefaultlib','/timestamp:0','/machine:x64','/out:'+str(verified),*map(str,(loader,memory,handoff,digest,trust))],cwd=ROOT,check=True,capture_output=True)
    signer=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64/signtool.exe')
    if not signer.is_file():raise RuntimeError('Pinned SDK SignTool path unavailable')
    if hashlib.sha256(signer.read_bytes()).hexdigest()!=lock['files']['signtool']['sha256']:raise RuntimeError('SignTool differs from lock')
    signed=work/'verified-signed.efi';signed.write_bytes(verified.read_bytes());sign_image(signer,signed,key,cert,work)
    enroll=compile('tests/m45/firmware/enroll.c','enroll');enroller=work/'enroll-test.efi';subprocess.run([tools['lld-link'],'/subsystem:efi_application','/entry:efi_main','/nodefaultlib','/timestamp:0','/machine:x64','/out:'+str(enroller),str(enroll),str(memory)],cwd=ROOT,check=True,capture_output=True)
    signed_enroller=work/'enroll-refusal-test.efi';signed_enroller.write_bytes(enroller.read_bytes());sign_image(signer,signed_enroller,key,cert,work)
    package(work/'enroll-refusal-media',signed_enroller.read_bytes(),kernel,init,'M45')
    package(work/'enroll-media',enroller.read_bytes(),kernel,init,'M45');package(work/'signed-media',signed.read_bytes(),kernel,init,'M45');package(work/'unsigned-media',verified.read_bytes(),kernel,init,'M45')
    bad_kernel=bytearray(kernel);bad_kernel[-1]^=1;package(work/'bad-kernel-media',signed.read_bytes(),bytes(bad_kernel),init,'M45')
    bad_init=bytearray(init);bad_init[12]^=1;package(work/'bad-init-media',signed.read_bytes(),kernel,bytes(bad_init),'M45')
    bad_loader=bytearray(signed.read_bytes());pe=int.from_bytes(bad_loader[60:64],'little');optional=pe+24;table=optional+int.from_bytes(bad_loader[pe+20:pe+22],'little');code=int.from_bytes(bad_loader[table+20:table+24],'little');bad_loader[code]^=1;package(work/'bad-loader-media',bytes(bad_loader),kernel,init,'M45')
    record={'build_id':manifest['build_id'],'cryptography_version':cryptography.__version__,'signtool_sha256':hashlib.sha256(signer.read_bytes()).hexdigest(),'public_certificate_sha256':hashlib.sha256(certificate).hexdigest(),'signed_loader_sha256':hashlib.sha256(signed.read_bytes()).hexdigest(),'kernel_sha256':hashlib.sha256(kernel).hexdigest(),'init_sha256':hashlib.sha256(init).hexdigest(),'test_source_sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in ('tools/build/secure.py','tests/m45/firmware/enroll.c','tests/m45/secure_boot.py')},'identity_reused':owner_pfx is not None,'policy':'pinned persistent owner identity' if owner_pfx is not None else 'ephemeral VM test identity; no factory trust'}
    (work/'provenance.json').write_bytes((json.dumps(record,indent=2)+'\n').encode());return record
if __name__=='__main__':
    import argparse,os,getpass
    parser=argparse.ArgumentParser();parser.add_argument('--owner-pfx',type=Path);parser.add_argument('--owner-sha256');parser.add_argument('--work-name',default='secure');args=parser.parse_args()
    if (args.owner_pfx is None)!=(args.owner_sha256 is None):parser.error('--owner-pfx and --owner-sha256 must be supplied together')
    password=None
    if args.owner_pfx:
        secret=os.environ.pop('ENGINE_OS_PFX_PASSWORD',None)
        if secret is None:secret=getpass.getpass('Owner PFX password: ')
        password=secret.encode() or None
    print(json.dumps(prepare(args.owner_pfx,password,args.owner_sha256,args.work_name),indent=2))
