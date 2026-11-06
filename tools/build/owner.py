"""Pinned owner identity loading and isolated signing; no global certificate store."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,os,subprocess,tempfile
from cryptography import x509
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives.serialization import pkcs12
from cryptography.x509.oid import ExtendedKeyUsageOID

def load_identity(path,password,expected_sha256):
    if len(expected_sha256)!=64 or any(c not in '0123456789abcdef' for c in expected_sha256):
        raise ValueError('A lowercase SHA-256 owner certificate pin is required')
    key,cert,chain=pkcs12.load_key_and_certificates(Path(path).read_bytes(),password)
    if not isinstance(key,rsa.RSAPrivateKey) or key.key_size<2048 or cert is None:
        raise ValueError('Owner PFX requires an RSA signing key of at least 2048 bits and its certificate')
    if key.public_key().public_numbers()!=cert.public_key().public_numbers():raise ValueError('Owner key/certificate mismatch')
    now=datetime.now(timezone.utc)
    if not cert.not_valid_before_utc<=now<cert.not_valid_after_utc:raise ValueError('Owner certificate is not currently valid')
    if ExtendedKeyUsageOID.CODE_SIGNING not in cert.extensions.get_extension_for_class(x509.ExtendedKeyUsage).value:
        raise ValueError('Owner certificate lacks code-signing usage')
    if hashlib.sha256(cert.public_bytes(serialization.Encoding.DER)).hexdigest()!=expected_sha256:
        raise ValueError('Owner certificate differs from the approved pin')
    return key,cert

def sign_image(signer,image,key,cert,work):
    # SignTool cannot receive a PFX password through stdin. Keep passwords out of
    # argv by creating a short-lived PFX in an owner-only directory instead.
    if os.name!='nt':raise RuntimeError('This pinned signing adapter requires Windows')
    sid=subprocess.run(['whoami','/user','/fo','csv','/nh'],check=True,capture_output=True,text=True).stdout.strip().split(',')[-1].strip('"')
    if not sid.startswith('S-1-') or any(c not in 'S-0123456789' for c in sid):raise RuntimeError('Cannot identify signing account SID')
    with tempfile.TemporaryDirectory(prefix='private-sign-',dir=work) as directory:
        subprocess.run(['icacls',directory,'/inheritance:r','/grant:r','*'+sid+':(OI)(CI)F'],check=True,capture_output=True)
        pfx=Path(directory)/'one-use.pfx'
        try:
            pfx.write_bytes(pkcs12.serialize_key_and_certificates(b'engine-owner',key,cert,None,serialization.NoEncryption()))
            subprocess.run([str(signer),'sign','/fd','SHA256','/f',str(pfx),str(image)],check=True,capture_output=True)
        finally:
            if pfx.exists():pfx.unlink()
