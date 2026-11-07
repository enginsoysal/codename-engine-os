"""Build native M6 development media without modifying historical release outputs."""
import argparse,hashlib,json,re,shutil,struct,zlib
from m1 import ROOT
from m2 import main
from m5_layout import compact_elf,trim
from media import fat32
from bios import stages,dual_media
from distro import load
ESP=10240

def embed(work,live):
    data=(work/'install.pack').read_bytes() if live else b''
    text='#include <engine/install/m6.h>\nconst uint8_t m6_bundle[]={'+','.join(map(str,data or b'\0'))+'};\n'
    text+=f'const unsigned m6_bundle_size={len(data)};\nconst unsigned m6_bundle_crc={zlib.crc32(data)}u;\n'
    profile=json.loads((work/'distro.json').read_text(encoding='utf-8'));records=profile['resolved_files']
    for i,r in enumerate(records):
        if len(r['target'].encode())>228:raise ValueError('M6 profile target leaves insufficient room for the selected account name')
        data=(ROOT/r['source']).read_bytes();text+='static const uint8_t file_'+str(i)+'[]={'+','.join(map(str,data or b'\0'))+'};\n'
    text+='const m6_distribution_file m6_files[]={'+(','.join('{'+json.dumps(r['target'])+',file_'+str(i)+','+str((ROOT/r['source']).stat().st_size)+'}' for i,r in enumerate(records)) or '{0,0,0}')+'};\nconst unsigned m6_file_count='+str(len(records))+';\n'
    if work.name=='m7':
        for app in ['editor','calculator']:
            data=(work/('APP7-'+app+'.ELF')).read_bytes() if live else b''
            if len(data)>65536:raise ValueError('Native desktop app exceeds the bounded loader size')
            text+='const uint8_t app7_'+app+'_image[]={'+','.join(map(str,data or b'\0'))+'};\n'
            text+='const unsigned app7_'+app+'_size='+str(len(data))+';\n'
    (work/'installer-image.c').write_bytes(text.encode())

def pack(parts):
    pages={}
    for lba,content in parts:
        assert lba%8==0
        for offset in range(0,len(content),4096):
            page=content[offset:offset+4096].ljust(4096,b'\0')
            where=lba+offset//512;assert where not in pages;pages[where]=page
    return struct.pack('<I',len(pages))+b''.join(struct.pack('<I',k|(0x80000000 if not any(v) else 0))+(v if any(v) else b'') for k,v in sorted(pages.items()))

def build(revision="standard",profile_path=None,desktop=False):
    if not re.fullmatch("[a-z][a-z0-9]{0,15}",revision):raise ValueError("Invalid build revision")
    prefix="M7" if desktop else "M6"
    name=prefix+("" if revision=="standard" else revision.upper())
    work=ROOT/'build'/prefix.lower();work.mkdir(parents=True,exist_ok=True);profile=load(profile_path or ROOT/('distros/engineos-desktop.json' if desktop else 'distros/engineos.json'),desktop=desktop)
    if profile['id']=='engineos':profile={**profile,'name':'Engine OS'}
    base=main('M5',distro=profile,m6=True,m6_revision=revision,m7=desktop)
    kernel=compact_elf((work/'KERNEL.ELF').read_bytes());efi=(work/'BOOTX64.EFI').read_bytes();init=(work/'INIT.BIN').read_bytes()
    installed=work/'installed';installed.mkdir(exist_ok=True)
    (installed/'KERNEL.ELF').write_bytes(kernel)
    if desktop:shutil.copyfile(work/'kernel.map',installed/'kernel.map')
    mbr,stage,raw,config=stages(installed/'bios',kernel,init,stage_lba=2048,kernel_lba=2112,kernel_limit=0x5f8000 if desktop else 0x400000)
    low=bytearray(4096);low[:446]=mbr[:446];low[510:512]=b'\x55\xaa'
    fat=fat32([(4,b'BOOTX64 EFI',efi),(2,b'KERNEL  ELF',kernel),(2,b'INIT    BIN',init)])
    payload=pack([(0,bytes(low)),(2048,stage),(2112,raw),(ESP,trim(fat))])
    (work/'install.pack').write_bytes(payload)
    live=main('M5',m5_live=True,distro=profile,m6=True,m6_revision=revision,m7=desktop)
    if live['build_id']!=base['build_id']:raise RuntimeError('Build inputs changed between installed and live images; rebuild with frozen inputs')
    if any(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()!=value for p,value in profile['input_sha256'].items()):raise RuntimeError('Distribution inputs changed during build')
    media=dual_media(work,name)
    live.update(milestone=prefix,build_revision=revision,distribution=profile,qualification_scope='M7-VM-v1' if desktop else 'M6-VM-v1',qualification='Requires the exact artifact acceptance report; building alone does not qualify runtime',release_version=('0.7.0' if desktop else '0.6.0')+('' if revision=='standard' else '+'+revision),installed_kernel_sha256=hashlib.sha256(kernel).hexdigest(),installer_payload_bytes=len(payload),bios_media_layout=media,limits=['Qualify the exact artifact digests against evidence/M6/acceptance-results.json','SATA/AHCI 512-byte logical-sector blank targets only, 2 GiB to 2 TiB','Native extent volume: 1024 entries, 63-byte path components, contiguous copy-on-write files','ISO-assisted verified-slot update/rollback/repair; interrupted boot replay can require ISO recovery','Single local profile, no authentication/ACLs or cross-layout migration; no desktop or Windows/DOS application runtime'])
    if desktop:
        live['limits']=['M7 development only: no completed desktop acceptance or stable release',
                        'Original native Editor/Calculator CPL3 apps; trusted compositor, Files, Settings and network broker',
                        'One bounded TLS 1.2 connection, IPv4 Intel 8254x and hardware RDRAND; correct virtual UTC clock required',
                        'Editor 8191 bytes; download 64 MiB; 8 windows; 1024 volume entries; 4-KiB streaming buffer',
                        'M6 installed-volume bounds remain; no Windows or DOS application runtime']
    live['artifacts']={p.name:{'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'size':p.stat().st_size} for p in [ROOT/'dist'/('EngineOS-'+name+suffix) for suffix in ('.iso','-usb.img','-kernel.elf','-kernel.map','-user.elf')] if p.is_file()}
    if desktop:
        for suffix,source in [('.elf',installed/'KERNEL.ELF'),('.map',installed/'kernel.map')]:
            artifact=ROOT/'dist'/('EngineOS-'+name+'-installed-kernel'+suffix)
            shutil.copyfile(source,artifact)
            live['artifacts'][artifact.name]={'sha256':hashlib.sha256(artifact.read_bytes()).hexdigest(),'size':artifact.stat().st_size}
        for app in ['editor','calculator']:
            artifact=ROOT/'dist'/('EngineOS-'+name+'-'+app+'.elf')
            live['artifacts'][artifact.name]={'sha256':hashlib.sha256(artifact.read_bytes()).hexdigest(),'size':artifact.stat().st_size}
    (ROOT/'dist'/(name.lower()+'-build-manifest.json')).write_bytes((json.dumps(live,indent=2)+'\n').encode())
    (ROOT/'dist'/(name+'-SHA256SUMS.txt')).write_bytes(''.join(f'{v["sha256"]}  {k}\n' for k,v in live['artifacts'].items()).encode())
    print(json.dumps({'milestone':prefix+' development','build_id':live['build_id'],'iso':'dist/EngineOS-'+name+'.iso'}))
    return live
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--revision',default='standard');parser.add_argument('--profile',type=__import__('pathlib').Path);args=parser.parse_args();build(args.revision,args.profile)
