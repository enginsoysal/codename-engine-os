"""Inspect a disposable ext2 image; recovery always writes a new exclusive copy."""
from pathlib import Path
import argparse,json
from ext2_image import check
def main():
    p=argparse.ArgumentParser();p.add_argument('image',type=Path);p.add_argument('--repair-copy',type=Path);a=p.parse_args()
    data=bytearray(a.image.read_bytes());result=check(data,repair=bool(a.repair_copy))
    if a.repair_copy:
        with a.repair_copy.open('xb') as f:f.write(data)
    print(json.dumps(result,indent=2))
if __name__=='__main__':main()
