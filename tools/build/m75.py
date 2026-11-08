"""Build isolated M7.5 guided installation and independent-startup media."""
import argparse
from pathlib import Path
from m6 import build
if __name__=="__main__":
    p=argparse.ArgumentParser();p.add_argument("--revision",default="standard");p.add_argument("--profile",type=Path);a=p.parse_args()
    build(a.revision,a.profile,installer_ux=True)
