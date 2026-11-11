"""Build isolated, explicitly partial M11 desktop candidates."""
import argparse
from pathlib import Path
from m6 import build
if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--revision',default='stage2')
    p.add_argument('--profile',type=Path,default=Path(__file__).resolve().parents[2]/'distros/engacer-desktop.json')
    a=p.parse_args()
    build(a.revision,a.profile,m11=True)
