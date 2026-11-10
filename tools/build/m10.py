"""Build isolated M10 device-foundation/removable-media candidates."""
import argparse
from pathlib import Path
from m6 import build
if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--profile',type=Path)
    p.add_argument('--revision',default='standard')
    a=p.parse_args()
    build(a.revision,a.profile,devices=True)
