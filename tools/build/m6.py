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
    profile=json.loads((work/'distro.json').read_text(encoding='utf-8'))
    records=profile['resolved_files'] if live or work.name!='m74' else []
    for i,r in enumerate(records):
        if len(r['target'].encode())>228:raise ValueError('M6 profile target leaves insufficient room for the selected account name')
        data=(ROOT/r['source']).read_bytes();text+='static const uint8_t file_'+str(i)+'[]={'+','.join(map(str,data or b'\0'))+'};\n'
    text+='const m6_distribution_file m6_files[]={'+(','.join('{'+json.dumps(r['target'])+',file_'+str(i)+','+str((ROOT/r['source']).stat().st_size)+'}' for i,r in enumerate(records)) or '{0,0,0}')+'};\nconst unsigned m6_file_count='+str(len(records))+';\n'
    if work.name in ['m7','m71','m72','m73','m74']:
        from m74 import catalogue,descriptor
        packages=catalogue() if work.name=='m74' else [dict(name=app,worker=True,ready=True) for app in ['editor','calculator']]
        for package in packages:
            app=package['name'];data=b''
            if package['ready']:
                data=((work/('APP7-'+app+'.ELF')).read_bytes() if live else b'') if package['worker'] else descriptor(package)
            if len(data)>65536:raise ValueError('Native desktop app exceeds the bounded loader size')
            text+='const uint8_t app7_'+app+'_image[]={'+','.join(map(str,data or b'\0'))+'};\n'
            text+='const unsigned app7_'+app+'_size='+str(len(data))+';\n'
    if work.name=='m74':
        license=(ROOT/'third_party/miniz/LICENSE').read_bytes()
        text+='const uint8_t miniz74_license[]={'+','.join(map(str,license))+'};\nconst unsigned miniz74_license_size='+str(len(license))+';\n'
    (work/'installer-image.c').write_bytes(text.encode())

def pack(parts):
    pages={}
    for lba,content in parts:
        assert lba%8==0
        for offset in range(0,len(content),4096):
            page=content[offset:offset+4096].ljust(4096,b'\0')
            where=lba+offset//512;assert where not in pages;pages[where]=page
    return struct.pack('<I',len(pages))+b''.join(struct.pack('<I',k|(0x80000000 if not any(v) else 0))+(v if any(v) else b'') for k,v in sorted(pages.items()))

def build(revision="standard",profile_path=None,desktop=False,disk_manager=False,desktop_first=False,polished=False,standard_apps=False):
    if not re.fullmatch("[a-z][a-z0-9]{0,15}",revision):raise ValueError("Invalid build revision")
    polished=polished or standard_apps
    desktop_first=desktop_first or polished
    disk_manager=disk_manager or desktop_first
    desktop=desktop or disk_manager
    prefix="M74" if standard_apps else "M73" if polished else "M72" if desktop_first else "M71" if disk_manager else "M7" if desktop else "M6"
    name=prefix+("" if revision=="standard" else revision.upper())
    work=ROOT/'build'/prefix.lower();work.mkdir(parents=True,exist_ok=True);profile=load(profile_path or ROOT/('distros/engineos-apps.json' if standard_apps else 'distros/engineos-polished.json' if polished else 'distros/engineos-workspace.json' if desktop_first else 'distros/engineos-disk.json' if disk_manager else 'distros/engineos-desktop.json' if desktop else 'distros/engineos.json'),desktop=desktop,standard_apps=standard_apps)
    if profile['id']=='engineos':profile={**profile,'name':'Engine OS'}
    base=main('M5',distro=profile,m6=True,m6_revision=revision,m7=desktop,m71=disk_manager,m72=desktop_first,m73=polished,m74=standard_apps)
    kernel=compact_elf((work/'KERNEL.ELF').read_bytes());efi=(work/'BOOTX64.EFI').read_bytes();init=(work/'INIT.BIN').read_bytes()
    installed=work/'installed';installed.mkdir(exist_ok=True)
    (installed/'KERNEL.ELF').write_bytes(kernel)
    if desktop:shutil.copyfile(work/'kernel.map',installed/'kernel.map')
    mbr,stage,raw,config=stages(installed/'bios',kernel,init,stage_lba=2048,kernel_lba=2112,kernel_limit=0x5f8000 if desktop else 0x400000)
    low=bytearray(4096);low[:446]=mbr[:446];low[510:512]=b'\x55\xaa'
    fat=fat32([(4,b'BOOTX64 EFI',efi),(2,b'KERNEL  ELF',kernel),(2,b'INIT    BIN',init)])
    payload=pack([(0,bytes(low)),(2048,stage),(2112,raw),(ESP,trim(fat))])
    (work/'install.pack').write_bytes(payload)
    live=main('M5',m5_live=True,distro=profile,m6=True,m6_revision=revision,m7=desktop,m71=disk_manager,m72=desktop_first,m73=polished,m74=standard_apps)
    if live['build_id']!=base['build_id']:raise RuntimeError('Build inputs changed between installed and live images; rebuild with frozen inputs')
    if any(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()!=value for p,value in profile['input_sha256'].items()):raise RuntimeError('Distribution inputs changed during build')
    media=dual_media(work,name)
    live.update(milestone=prefix,build_revision=revision,distribution=profile,qualification_scope='M71-VM-v1' if disk_manager else 'M7-VM-v1' if desktop else 'M6-VM-v1',qualification='Requires the exact artifact acceptance report; building alone does not qualify runtime',release_version=('0.7.1' if disk_manager else '0.7.0' if desktop else '0.6.0')+('' if revision=='standard' else '+'+revision),installed_kernel_sha256=hashlib.sha256(kernel).hexdigest(),installer_payload_bytes=len(payload),bios_media_layout=media,limits=['Qualify the exact artifact digests against evidence/M6/acceptance-results.json','SATA/AHCI 512-byte logical-sector blank targets only, 2 GiB to 2 TiB','Native extent volume: 1024 entries, 63-byte path components, contiguous copy-on-write files','ISO-assisted verified-slot update/rollback/repair; interrupted boot replay can require ISO recovery','Single local profile, no authentication/ACLs or cross-layout migration; no desktop or Windows/DOS application runtime'])
    if desktop:
        live['limits']=['Qualification is valid only with the matching M7-VM-v1 acceptance report',
                        'Original native Editor/Calculator CPL3 event workers; trusted compositor, Files, Settings, Terminal and network broker',
                        'One bounded TLS 1.2 connection, IPv4 Intel 8254x/82574 and hardware RDRAND; correct virtual UTC clock required',
                        'Editor 8191 bytes; download 64 MiB; 8 windows; 1024 volume entries; 4-KiB streaming buffer',
                        'SATA/AHCI 512-byte sectors and M6 installed-volume bounds; one local profile without authentication/ACLs',
                        'Named x86-64 BIOS/UEFI virtual profiles only; Secure Boot disabled; no universal or physical qualification',
                        'Desktop work runs on the BSP; APs halt after original runtime qualification; no Windows or DOS application runtime']
    if disk_manager:live['limits'][0]='Qualification requires the matching M71-VM-v1 acceptance report';live['limits'] += ['GPT 128x128 canonical tables and primary MBR partitions; FAT32/Engine quick formats; no resize/move or filesystem conversion', 'Installation still replaces the whole selected SATA disk after explicit erase consent; no dual-boot installation into a selected partition']
    if desktop_first:
        live.update(qualification_scope='M72-VM-v1',release_version='0.7.2'+('' if revision=='standard' else '+'+revision))
        live['limits'][0]='Qualification requires the matching M72-VM-v1 exact-artifact acceptance report'
        live['limits'] += ['Finish opens the real persistent desktop immediately from the installation session; independent disk boot is qualified separately', 'Files browses the installed native volume; additional partitions are not automatically mounted; system folders are read-only in Files']
    if polished:
        live.update(qualification_scope='M73-VM-v1',release_version='0.7.3'+('' if revision=='standard' else '+'+revision))
        live['limits'][0]='Qualification requires the matching M73-VM-v1 acceptance report'
        live['limits'] += ['Inter OFL coverage masks at native physical font sizes, 100/150/200 percent UI scales; no runtime font interpreter', '4K refers to qualified framebuffer modes, not every physical monitor or GPU']
        live['font']=json.loads((work/'font73-manifest.json').read_text(encoding='utf-8'))
    if standard_apps:
        from m74 import catalogue
        live.update(qualification_scope='M74-VM-v1',release_version='0.7.4'+('' if revision=='standard' else '+'+revision))
        live['limits'][0]='M7.4 development: not qualified without matching native acceptance evidence'
        live['native_packages']=[{**p,'execution_class':'cpl3-event-worker' if p['worker'] else 'fixed-broker-frontend',
                                 'owned_file':'/Applications/'+p['folder']+('/program.elf' if p['worker'] else '/manifest.json')}
                                for p in catalogue() if p['ready']]
    live['artifacts']={p.name:{'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'size':p.stat().st_size} for p in [ROOT/'dist'/('EngineOS-'+name+suffix) for suffix in ('.iso','-usb.img','-kernel.elf','-kernel.map','-user.elf')] if p.is_file()}
    if desktop:
        for suffix,source in [('.elf',installed/'KERNEL.ELF'),('.map',installed/'kernel.map')]:
            artifact=ROOT/'dist'/('EngineOS-'+name+'-installed-kernel'+suffix)
            shutil.copyfile(source,artifact)
            live['artifacts'][artifact.name]={'sha256':hashlib.sha256(artifact.read_bytes()).hexdigest(),'size':artifact.stat().st_size}
        from m74 import applications
        for app,kind in (applications() if standard_apps else [('editor',2),('calculator',4)]):
            artifact=ROOT/'dist'/('EngineOS-'+name+'-'+app+'.elf')
            live['artifacts'][artifact.name]={'sha256':hashlib.sha256(artifact.read_bytes()).hexdigest(),'size':artifact.stat().st_size}
    (ROOT/'dist'/(name.lower()+'-build-manifest.json')).write_bytes((json.dumps(live,indent=2)+'\n').encode())
    (ROOT/'dist'/(name+'-SHA256SUMS.txt')).write_bytes(''.join(f'{v["sha256"]}  {k}\n' for k,v in live['artifacts'].items()).encode())
    print(json.dumps({'milestone':prefix+(' candidate' if desktop else ' development'),'build_id':live['build_id'],'iso':'dist/EngineOS-'+name+'.iso'}))
    return live
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--revision',default='standard');parser.add_argument('--profile',type=__import__('pathlib').Path);args=parser.parse_args();build(args.revision,args.profile)
