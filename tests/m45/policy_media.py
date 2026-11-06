"""Build signed, TCG-only policy test media; all private material remains ignored."""
from datetime import datetime,timezone
import hashlib,json,struct,subprocess,uuid
from cryptography.hazmat.primitives import hashes,serialization
from cryptography.hazmat.primitives.serialization import pkcs7
from m1 import ROOT,pinned_tools
from owner import load_identity,sign_image
from media import package

GLOBAL=uuid.UUID('8be4df61-93ca-11d2-aa0d-00e098032b8c').bytes_le
DATABASE=uuid.UUID('d719b2cb-3d3a-4596-a3bc-dad00e67656f').bytes_le
def esl(cert):
    raw=cert.public_bytes(serialization.Encoding.DER)
    return uuid.UUID('a5c059a1-94e4-4aa7-87b5-ab155c2bf072').bytes_le+struct.pack('<III',44+len(raw),0,16+len(raw))+GLOBAL+raw

def authenticated_db(identity,data,when):
    key,cert=identity;stamp=struct.pack('<HBBBBBBIhBB',when.year,when.month,when.day,when.hour,when.minute,when.second,0,0,0,0,0)
    message='db'.encode('utf-16le')+DATABASE+struct.pack('<I',0x27)+stamp+data
    cms=pkcs7.PKCS7SignatureBuilder().set_data(message).add_signer(cert,key,hashes.SHA256()).sign(serialization.Encoding.DER,[pkcs7.PKCS7Options.DetachedSignature,pkcs7.PKCS7Options.Binary])
    return stamp+struct.pack('<IHH',24+len(cms),0x200,0xef1)+uuid.UUID('4aafd29d-68df-49ee-8aa9-347d375665a7').bytes_le+cms+data

def build_policy(name,image_identity,authority,before,after,auth,denied=False):
    work=ROOT/'build/m45'/name;work.mkdir(parents=True,exist_ok=True);tools=pinned_tools()
    header=''
    for label,data in [('expected_authority',esl(authority[1])),('expected_before',esl(before[1])),('expected_after',esl(after[1])),('auth_update',auth)]:
        header+='static const unsigned char '+label+'[]={'+','.join(map(str,data))+'};\n'
    (work/'policy-payload.h').write_bytes(header.encode());objects=[]
    for source,stem in [('tests/m45/firmware/policy.c','policy'),('runtime/memory.c','memory')]:
        obj=work/(stem+'.obj');subprocess.run([tools['clang'],'--target=x86_64-pc-windows-msvc','-std=c17','-O2','-ffreestanding','-fno-builtin','-fno-stack-protector','-mno-red-zone','-mgeneral-regs-only','-Wall','-Wextra','-Werror','-Iinclude','-Iboot/uefi','-I'+str(work),*(['-DEXPECT_DENIED'] if denied else []),'-c',source,'-o',str(obj)],cwd=ROOT,check=True,capture_output=True);objects.append(obj)
    image=work/'policy.efi';subprocess.run([tools['lld-link'],'/subsystem:efi_application','/entry:efi_main','/nodefaultlib','/timestamp:0','/machine:x64','/out:'+str(image),*map(str,objects)],cwd=ROOT,check=True,capture_output=True)
    from pathlib import Path
    signer=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64/signtool.exe');lock=json.loads((ROOT/'tools/security-toolchain.json').read_text(encoding='utf-8'))
    if hashlib.sha256(signer.read_bytes()).hexdigest()!=lock['files']['signtool']['sha256']:raise RuntimeError('SignTool differs from lock')
    sign_image(signer,image,*image_identity,work)
    package(work/'media',image.read_bytes(),(ROOT/'build/m45/KERNEL.ELF').read_bytes(),(ROOT/'build/m45/INIT.BIN').read_bytes(),'M45')
    return work/'media'
