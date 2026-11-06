"""Original BIOS EDD -> E820/VBE -> protected/long-mode loader and dual-firmware media.

Only creates generated images. No host block device or existing VM is opened.
"""
from pathlib import Path
import hashlib,json,struct,subprocess,zlib
from m1 import ROOT,pinned_tools
from media import gpt,pycdlib

def flatten64(data):
    if len(data)<64 or data[:6]!=b'\x7fELF\x02\x01' or struct.unpack_from('<H',data,18)[0]!=62:raise ValueError('BIOS kernel must be ELF64 x86-64')
    phoff=struct.unpack_from('<Q',data,32)[0];size,count=struct.unpack_from('<HH',data,54)
    if size!=56 or not 1<=count<=32 or phoff>len(data) or count*size>len(data)-phoff:raise ValueError('ELF program header bounds')
    segments=[struct.unpack_from('<IIQQQQQQ',data,phoff+i*56) for i in range(count)];loads=sorted((s for s in segments if s[0]==1),key=lambda s:s[4]);entry=struct.unpack_from('<Q',data,24)[0]
    if not loads or loads[0][4]!=0x200000 or not any(s[1]&1 and s[4]<=entry<s[4]+s[5] for s in loads):raise ValueError('ELF load entry')
    end=0x200000
    for s in loads:
        if s[4]<end or s[5]>s[6] or s[2]>len(data) or s[5]>len(data)-s[2] or s[4]>0x400000 or s[6]>0x400000-s[4]:raise ValueError('ELF physical/file bounds or overlap')
        end=s[4]+s[6]
    length=(end-0x200000+32767)&~32767;out=bytearray(length)
    for s in loads:out[s[4]-0x200000:s[4]-0x200000+s[5]]=data[s[2]:s[2]+s[5]]
    return bytes(out),entry

def flatten32(path):
    data=path.read_bytes();off=struct.unpack_from('<I',data,28)[0];count=struct.unpack_from('<H',data,44)[0];loads=[struct.unpack_from('<IIIIIIII',data,off+i*32) for i in range(count)];loads=[s for s in loads if s[0]==1]
    low=min(s[3] for s in loads);high=max(s[3]+s[5] for s in loads);out=bytearray(high-low)
    for s in loads:out[s[3]-low:s[3]-low+s[4]]=data[s[1]:s[1]+s[4]]
    return bytes(out)

def stages(work,kernel,init,sector_bytes=512,stage_lba=64,kernel_lba=128,video_mode=0x144,bad_boot=False):
    if sector_bytes not in (512,2048) or len(init)!=48:raise ValueError('BIOS profile')
    work=Path(work);work.mkdir(parents=True,exist_ok=True);raw,entry=flatten64(kernel);tools=pinned_tools();blocks=32768//sector_bytes
    defines={'BIOS_SECTOR_BYTES':sector_bytes,'BIOS_STAGE_BLOCKS':blocks,'BIOS_STAGE_LBA':stage_lba,'BIOS_KERNEL_LBA':kernel_lba,'BIOS_KERNEL_CHUNKS':len(raw)//32768,'BIOS_KERNEL_BYTES':len(raw),'BIOS_KERNEL_ENTRY':entry,'BIOS_KERNEL_CRC':zlib.crc32(raw),'BIOS_VIDEO_MODE':video_mode,'BIOS_BAD_BOOT':int(bad_boot)}
    header=''.join(f'#define {name} {value}\n' for name,value in defines.items())+'#ifndef __ASSEMBLER__\nstatic const unsigned char bios_init[]={'+','.join(map(str,init))+'};\n#endif\n';(work/'bios-config.h').write_bytes(header.encode())
    def command(args):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True)
        if p.returncode:raise RuntimeError(p.stderr.replace(str(ROOT),'<project-root>'))
    objects=[]
    for path in ('boot/bios/mbr.S','boot/bios/stage2.S','boot/bios/prepare.c','runtime/memory.c'):
        obj=work/(Path(path).name+'.o');args=[tools['clang'],'--target=i386-unknown-none-elf','-I'+str(work),'-Iinclude','-c',path,'-o',obj]
        if path.endswith('.c'):args[2:2]=['-std=c17','-O2','-ffreestanding','-fno-builtin','-fno-stack-protector','-fno-pic','-fno-pie','-mno-sse','-mno-mmx','-msoft-float','-fno-asynchronous-unwind-tables','-Wall','-Wextra','-Werror']
        command(args);objects.append(obj)
    command([tools['ld.lld'],'-m','elf_i386','-T','boot/bios/mbr.ld','-o',work/'mbr.elf',objects[0]])
    command([tools['ld.lld'],'-m','elf_i386','-T','boot/bios/stage2.ld','-o',work/'stage2.elf',*objects[1:]])
    mbr=flatten32(work/'mbr.elf');stage2=flatten32(work/'stage2.elf')
    if len(mbr)!=512 or mbr[510:]!=b'\x55\xaa' or len(stage2)>32768:raise RuntimeError('BIOS stage bounds')
    return mbr,stage2+bytes(32768-len(stage2)),raw,defines

def dual_media(build,milestone='M45'):
    build=Path(build);work=build/'bios';kernel=(build/'KERNEL.ELF').read_bytes();init=(build/'INIT.BIN').read_bytes();fat=(build/'normal/efi-fat.img').read_bytes();dist=ROOT/'dist';work.mkdir(parents=True,exist_ok=True)
    mbr,stage2,raw,usb_config=stages(work/'usb',kernel,init)
    usb=gpt(fat,'Engine OS '+milestone,first=8192,bios=(mbr,stage2,raw));(dist/('EngineOS-'+milestone+'-usb.img')).write_bytes(usb)
    placeholder,cd_stage,cd_raw,ignored=stages(work/'cd-layout',kernel,init,2048,1,17);blob=placeholder+bytes(1536)+cd_stage+cd_raw;boot_file=work/'bios-cd.img';boot_file.write_bytes(blob)
    iso=pycdlib.PyCdlib();iso.new(interchange_level=3,vol_ident='ENGINE_OS_'+milestone);iso.add_file(str(boot_file),iso_path='/BIOS.IMG;1');iso.add_file(str(build/'normal/efi-fat.img'),iso_path='/EFI.IMG;1');iso.add_eltorito('/BIOS.IMG;1',platform_id=0,boot_load_size=4);iso.add_eltorito('/EFI.IMG;1',platform_id=0xef,efi=True,boot_load_size=1)
    target=dist/('EngineOS-'+milestone+'.iso');iso.write(str(target));iso.close();reader=pycdlib.PyCdlib();reader.open(str(target));extent=reader.get_record(iso_path='/BIOS.IMG;1').extent_location();reader.close()
    mbr,cd_stage,cd_raw,cd_config=stages(work/'cd',kernel,init,2048,extent+1,extent+17);final=mbr+bytes(1536)+cd_stage+cd_raw
    if len(final)!=len(blob):raise RuntimeError('BIOS CD layout changed')
    with target.open('r+b') as f:f.seek(extent*2048);f.write(final)
    record={'kernel_sha256':hashlib.sha256(kernel).hexdigest(),'raw_kernel_sha256':hashlib.sha256(raw).hexdigest(),'usb':usb_config,'iso':cd_config,'iso_bios_extent':extent,'scope':'original EDD/E820/VBE BIOS path; unsigned UEFI path shares the same kernel; Secure Boot applies only to separately signed UEFI media'}
    (work/'media-layout.json').write_bytes((json.dumps(record,indent=2)+'\n').encode());return record
