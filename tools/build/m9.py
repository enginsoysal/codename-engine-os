"""Build the M9 graphics-first follow-up without rewriting accepted M8 artifacts."""
import argparse
from pathlib import Path
from m6 import build

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--profile',type=Path)
    parser.add_argument('--revision',default='standard')
    args=parser.parse_args()
    build(args.revision,args.profile,graphics=True)
