"""Create only a new marked disposable M4 data image; never overwrite existing files."""
from pathlib import Path
import argparse
import struct
import zlib
from ext2_image import format_into,check
SECTORS=32768
def create(path):
    path=Path(path);path.parent.mkdir(parents=True,exist_ok=True)
    header=bytearray(512);header[:16]=b'ENGINE-M4-DATA02';struct.pack_into('<I',header,16,2);struct.pack_into('<Q',header,24,SECTORS);struct.pack_into('<I',header,508,zlib.crc32(header[:508]))
    record=bytearray(512);record[:8]=b'ENGM4REC';struct.pack_into('<I',record,508,zlib.crc32(record[:508]))
    data=bytearray(SECTORS*512);data[:512]=header;data[512:1024]=record;format_into(data);check(data)
    with path.open('xb') as f:f.write(data)
    return path
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('path',type=Path);a=p.parse_args();create(a.path);print('Created new marked 16 MiB data image')
