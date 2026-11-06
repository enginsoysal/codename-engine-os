"""Bounded native installer payloads; only generated regular files are opened."""
import struct,zlib
from pathlib import Path
from m1 import ROOT
from data_image import create

ESP=8192;DATA=139264;END=172032;MIN=524288

def pack(parts):
    pages={}
    for lba,data in parts:
        assert lba%8==0 and len(data)%4096==0
        for off in range(0,len(data),4096):
            n=lba+off//512
            assert n not in pages
            pages[n]=data[off:off+4096]
    return struct.pack('<I',len(pages))+b''.join(struct.pack('<I',n | (0x80000000 if not any(pages[n]) else 0))+(pages[n] if any(pages[n]) else b'') for n in sorted(pages))

def trim(data):
    end=len(data.rstrip(b'\0'));return bytes(data[:(end+4095)&~4095])

def template(work):
    import zlib
    from ext2_image import format_into,check,Image
    data=bytearray(16*1024*1024);data[:16]=b'ENGINE-M4-DATA02';struct.pack_into('<I',data,16,2);struct.pack_into('<Q',data,24,32768);struct.pack_into('<I',data,508,zlib.crc32(data[:508]));data[512:520]=b'ENGM4REC';struct.pack_into('<I',data,1020,zlib.crc32(data[512:1020]))
    format_into(data);im=Image(data);system=next(n for name,n,t in im.entries(2) if name=='System');versions=im.mkdir(system,'Versions')
    build_id=(work/'build.h').read_text().split('"')[1];version=im.mkdir(versions,build_id);current=im.mkdir(system,'Current')
    for n in (version,current):im.file(n,'version.txt',('Engine OS M5 '+build_id+'\nBoot code: EFI partition and BIOS boot area.\n').encode())
    if (work/'distro.json').exists():
        import json
        profile=json.loads((work/'distro.json').read_text(encoding='utf-8'))
        def directory(parts):
            n=2
            for part in parts:
                found=next((i for label,i,t in im.entries(n) if label==part),None)
                n=found if found else im.mkdir(n,part)
            return n
        d=directory(['System','Distro']);im.file(d,'profile.json',(json.dumps(profile,indent=2)+'\n').encode())
        for record in profile['resolved_files']:
            parts=record['target'].strip('/').split('/');im.file(directory(parts[:-1]),parts[-1],(ROOT/record['source']).read_bytes())
    im.counts();check(data);(work/'data-template.img').write_bytes(data);return pack([(0,trim(data))])

def embed(work,live):
    records={'m5_template':template(work),'m5_install':(work/'install.pack').read_bytes() if live else b''}
    text='#include <stdint.h>\n'
    for name,data in records.items():
        text+='const uint8_t '+name+'[]={'+','.join(map(str,data or b'\0'))+'};\n'
        text+=f'const unsigned {name}_size={len(data)};\nconst unsigned {name}_crc={zlib.crc32(data)}u;\n'
    (work/'installer-image.c').write_text(text,encoding='utf-8')

def compact_elf(data):
    # Preserve the ELF header and exact PT_LOAD offsets, omit debug/symbol tails.
    ph=struct.unpack_from('<Q',data,32)[0];n=struct.unpack_from('<H',data,56)[0]
    end=max(struct.unpack_from('<Q',data,ph+i*56+8)[0]+struct.unpack_from('<Q',data,ph+i*56+32)[0] for i in range(n) if struct.unpack_from('<I',data,ph+i*56)[0]==1)
    out=bytearray(data[:end]);struct.pack_into('<Q',out,40,0);struct.pack_into('<HHH',out,58,0,0,0);return bytes(out)
