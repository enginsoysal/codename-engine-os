"""Restricted build-host FAT32/GPT/EFI ISO authoring. Never opens host block devices."""
from pathlib import Path
import re
import struct
import uuid
import zlib
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/local/python'))
import pycdlib

def fat32(files):
    # Fixed 64 MiB profile, 512-byte clusters; three directories plus short-name files.
    sectors,reserved,fat_sectors=131072,32,1024
    data_start=reserved+2*fat_sectors
    clusters=sectors-data_start
    if clusters<65525 or (clusters+2)*4>fat_sectors*512:raise ValueError('FAT32 geometry')
    image=bytearray(sectors*512)
    image[:3],image[3:11]=b'\xeb\x58\x90',b'ENGINEOS'
    struct.pack_into('<HBHBHHBHHHII',image,11,512,1,reserved,2,0,0,0xf8,0,63,255,0,sectors)
    struct.pack_into('<IHHIHH',image,36,fat_sectors,0,0,2,1,6)
    image[64],image[66]=0x80,0x29
    struct.pack_into('<I',image,67,0x454e474e)
    image[71:82],image[82:90],image[510:512]=b'ENGINE OS  ',b'FAT32   ',b'\x55\xaa'
    struct.pack_into('<I',image,512,0x41615252);struct.pack_into('<I',image,996,0x61417272)
    struct.pack_into('<II',image,1000,0xffffffff,0xffffffff);struct.pack_into('<I',image,1020,0xaa550000)
    image[3072:3584]=image[:512];image[3584:4096]=image[512:1024]
    table=bytearray(fat_sectors*512)
    for c,v in [(0,0x0ffffff8),(1,0xffffffff),(2,0x0fffffff),(3,0x0fffffff),(4,0x0fffffff)]:struct.pack_into('<I',table,c*4,v)
    def entry(name,cluster,directory=False,length=0):
        if len(name)!=11 or not name.isascii():raise ValueError('short-name encoding')
        v=bytearray(32);v[:11],v[11]=name,0x10 if directory else 0x20
        struct.pack_into('<H',v,20,cluster>>16);struct.pack_into('<HI',v,26,cluster&65535,length);return v
    dirs={2:[entry(b'EFI        ',3,True)],
          3:[entry(b'.          ',3,True),entry(b'..         ',0,True),entry(b'BOOT       ',4,True)],
          4:[entry(b'.          ',4,True),entry(b'..         ',3,True)]}
    next_cluster=5
    seen=set()
    for parent,name,payload in files:
        if parent not in (2,4) or (parent,name) in seen or not payload or len(payload)>4*1024*1024:raise ValueError('fixture file constraints')
        seen.add((parent,name));count=(len(payload)+511)//512
        if next_cluster+count>clusters+2:raise ValueError('FAT capacity')
        dirs[parent].append(entry(name,next_cluster,False,len(payload)))
        for c in range(next_cluster,next_cluster+count):struct.pack_into('<I',table,c*4,c+1 if c<next_cluster+count-1 else 0x0fffffff)
        start=(data_start+next_cluster-2)*512;image[start:start+len(payload)]=payload;next_cluster+=count
    for cluster,entries in dirs.items():
        if len(entries)>16:raise ValueError('directory capacity')
        start=(data_start+cluster-2)*512
        for i,record in enumerate(entries):image[start+i*32:start+(i+1)*32]=record
    image[reserved*512:(reserved+fat_sectors)*512]=table
    image[(reserved+fat_sectors)*512:data_start*512]=table
    return image

def gpt(volume,label='Engine OS M1',first=2048,bios=None,bios_limit=0x200000):
    if first<2048 or len(volume)%512:raise ValueError('GPT volume alignment')
    total=first+len(volume)//512+2048
    image=bytearray(total*512);image[450]=0xee;struct.pack_into('<II',image,454,1,total-1);image[510:512]=b'\x55\xaa'
    entries=bytearray(16384)
    entries[:16]=uuid.UUID('c12a7328-f81f-11d2-ba4b-00a0c93ec93b').bytes_le
    entries[16:32]=uuid.UUID('1432896c-811c-4a61-bc1f-a12000000011').bytes_le
    struct.pack_into('<QQQ',entries,32,first,first+len(volume)//512-1,0)
    name=label.encode('utf-16le');entries[56:56+len(name)]=name
    if bios is not None:
        mbr,stage2,kernel=bios
        if first<8192 or len(mbr)!=512 or len(stage2)!=32768 or not kernel or bios_limit>0x400000 or len(kernel)>bios_limit or 128+len(kernel)//512>first or len(kernel)%512:raise ValueError('BIOS boot partition bounds')
        entries[128:144]=uuid.UUID('21686148-6449-6e6f-744e-656564454649').bytes_le;entries[144:160]=uuid.UUID('1432896c-811c-4a61-bc1f-a12000000013').bytes_le;struct.pack_into('<QQQ',entries,160,64,128+len(kernel)//512-1,0)
        label_bytes='Engine BIOS'.encode('utf-16le');entries[184:184+len(label_bytes)]=label_bytes;image[:446]=mbr[:446];image[64*512:128*512]=stage2;image[128*512:128*512+len(kernel)]=kernel
    def header(current,backup,table):
        value=bytearray(512)
        struct.pack_into('<8sIIIIQQQQ16sQIII',value,0,b'EFI PART',0x10000,92,0,0,current,backup,34,total-34,
                         uuid.UUID('1432896c-811c-4a61-bc1f-a12000000012').bytes_le,table,128,128,zlib.crc32(entries))
        struct.pack_into('<I',value,16,zlib.crc32(value[:92]));return value
    image[512:1024]=header(1,total-1,2);image[1024:1024+len(entries)]=entries
    image[(total-33)*512:(total-1)*512]=entries;image[(total-1)*512:]=header(total-1,1,total-33)
    image[first*512:first*512+len(volume)]=volume;return image

def package(output,efi,kernel,init,milestone='M1'):
    if milestone not in ('M1','M2','M3','M4','M45','M5','M6','M7') and not re.fullmatch(r'M[67][A-Z0-9]{1,16}',milestone):raise ValueError('milestone')
    output.mkdir(parents=True,exist_ok=True)
    fat=fat32([(4,b'BOOTX64 EFI',efi),(2,b'KERNEL  ELF',kernel),(2,b'INIT    BIN',init)])
    (output/'efi-fat.img').write_bytes(fat);(output/f'EngineOS-{milestone}-usb.img').write_bytes(gpt(fat,'Engine OS '+milestone))
    iso=pycdlib.PyCdlib();iso.new(interchange_level=3,vol_ident='ENGINE_OS_'+milestone)
    iso.add_file(str(output/'efi-fat.img'),iso_path='/EFI.IMG;1')
    iso.add_eltorito('/EFI.IMG;1',platform_id=0xef,efi=True,boot_load_size=1)
    iso.write(str(output/f'EngineOS-{milestone}.iso'));iso.close()
