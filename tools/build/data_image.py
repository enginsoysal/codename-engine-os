"""Create only a new marked disposable M4 data image; never overwrite existing files."""
from pathlib import Path
import argparse
import struct
import zlib
SECTORS=32768
def create(path):
    path=Path(path);path.parent.mkdir(parents=True,exist_ok=True)
    header=bytearray(512);header[:16]=b'ENGINE-M4-DATA01';struct.pack_into('<I',header,16,1);struct.pack_into('<Q',header,24,SECTORS);struct.pack_into('<I',header,508,zlib.crc32(header[:508]))
    record=bytearray(512);record[:8]=b'ENGM4REC';struct.pack_into('<I',record,508,zlib.crc32(record[:508]))
    with path.open('xb') as f:f.truncate(SECTORS*512);f.seek(0);f.write(header);f.write(record)
    return path
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('path',type=Path);a=p.parse_args();create(a.path);print('Created new marked 16 MiB data image')
