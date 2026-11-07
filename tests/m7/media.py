"""Check larger M7 BIOS authoring bounds without mistaking metadata tests for boot proof."""
from pathlib import Path
import hashlib
import json
import struct
import sys
import zlib

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/build'))
from media import gpt


def main():
    mbr=bytes(510)+b'\x55\xaa'
    stage=b'S'*32768
    kernel=b'K'*(0x200000+32768)
    fat=b'F'*4096
    image=gpt(fat,'Engine OS M7',first=16384,bios=(mbr,stage,kernel),bios_limit=0x3f8000)
    header=bytearray(image[512:1024]);crc=struct.unpack_from('<I',header,16)[0]
    struct.pack_into('<I',header,16,0)
    assert zlib.crc32(header[:92])==crc
    table=image[1024:1024+16384]
    assert zlib.crc32(table)==struct.unpack_from('<I',header,88)[0]
    esp=struct.unpack_from('<QQ',table,32)
    boot=struct.unpack_from('<QQ',table,160)
    assert boot==(64,128+len(kernel)//512-1) and boot[1]<esp[0]==16384
    assert image[64*512:128*512]==stage
    assert image[128*512:128*512+len(kernel)]==kernel
    assert image[esp[0]*512:esp[0]*512+len(fat)]==fat
    cases=[(8192,0x200000,kernel),(8192,0x400000,b'K'*0x400000),
           (16384,0x400001,kernel),(16384,0x400000,b'')]
    for first,limit,payload in cases:
        try:gpt(fat,first=first,bios=(mbr,stage,payload),bios_limit=limit)
        except ValueError:pass
        else:raise AssertionError('Invalid BIOS extent accepted')
    result={'passed':True,'scope':'actual media authoring and independent byte/GPT inspection, not firmware execution',
            'm7_complete':False,'checks':[{'name':'larger bounded BIOS payload remains disjoint from EFI, with exact bytes and valid GPT CRCs','passed':True},
                                        {'name':'legacy-size, overlap, excessive-policy and empty-payload refusals','passed':True,'cases':len(cases)}],
            'source_sha256':{name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in ['tools/build/media.py','tools/build/bios.py']}}
    out=ROOT/'evidence/M7/media-authoring-results.json'
    out.write_bytes((json.dumps(result,indent=2)+'\n').encode())
    print('M7 media authoring PASS: bounds, refusal and actual placed bytes')


if __name__=='__main__':main()
