"""Build separately named M7.1 disk-manager media, preserving all accepted releases."""
import argparse
from pathlib import Path
from m6 import build
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--revision',default='standard');p.add_argument('--profile',type=Path);a=p.parse_args();build(a.revision,a.profile,disk_manager=True)
