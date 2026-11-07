"""Convert the pinned public Mozilla PEM bundle into immutable BearSSL anchors."""
import json
from pathlib import Path
from m1 import ROOT,sha


def prepare(destination):
    import cryptography
    from cryptography import x509
    from cryptography.hazmat.primitives import serialization
    from cryptography.hazmat.primitives.asymmetric import rsa,ec
    lock=json.loads((ROOT/'tools/security-toolchain.json').read_text(encoding='utf-8'))
    if cryptography.__version__!=lock['cryptography_version']:raise RuntimeError('Trust conversion package differs from lock')
    source=ROOT/'third_party/mozilla-ca/cacert.pem'
    provenance=json.loads(source.with_name('provenance.json').read_text(encoding='utf-8'))
    if sha(source)!=provenance['sha256']:raise RuntimeError('Public trust bundle differs from pinned provenance')
    certificates=x509.load_pem_x509_certificates(source.read_bytes())
    lines=['/* Generated from the pinned MPL-2.0 Mozilla CA bundle. See third_party/mozilla-ca. */']
    anchors=[]
    def array(name,data):
        lines.append('static const unsigned char '+name+'[]={'+','.join(map(str,data))+'};')
        return '(unsigned char*)'+name+','+str(len(data))
    def integer(n):return n.to_bytes((n.bit_length()+7)//8,'big')
    for n,cert in enumerate(certificates):
        if not cert.extensions.get_extension_for_class(x509.BasicConstraints).value.ca:raise RuntimeError('Non-CA trust input')
        name='root7_'+str(n);dn=array(name+'_dn',cert.subject.public_bytes());key=cert.public_key()
        if isinstance(key,rsa.RSAPublicKey):
            numbers=key.public_numbers();modulus=array(name+'_n',integer(numbers.n));exponent=array(name+'_e',integer(numbers.e))
            public='{BR_KEYTYPE_RSA,{.rsa={'+modulus+','+exponent+'}}}'
        elif isinstance(key,ec.EllipticCurvePublicKey):
            curve={'secp256r1':23,'secp384r1':24,'secp521r1':25}.get(key.curve.name)
            if curve is None:raise RuntimeError('Unsupported root curve: '+key.curve.name)
            point=array(name+'_q',key.public_bytes(serialization.Encoding.X962,serialization.PublicFormat.UncompressedPoint))
            public='{BR_KEYTYPE_EC,{.ec={'+str(curve)+','+point+'}}}'
        else:raise RuntimeError('Unsupported root public key')
        anchors.append('{{'+dn+'},BR_X509_TA_CA,'+public+'}')
    lines+=['static const br_x509_trust_anchor root7_anchors[]={',',\n'.join(anchors),'};',
            '#define ROOT7_COUNT '+str(len(anchors))]
    Path(destination).write_bytes(('\n'.join(lines)+'\n').encode('ascii'))
    return {'bundle_sha256':sha(source),'anchors':len(anchors),'generated_sha256':sha(destination)}


if __name__=='__main__':
    path=ROOT/'build/m7/trust7.h';path.parent.mkdir(parents=True,exist_ok=True)
    print(json.dumps(prepare(path),indent=2))
