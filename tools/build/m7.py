"""Build separately named M7 media; never overwrite accepted M6 assets."""
import argparse
from pathlib import Path
from m6 import build

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--revision', default='standard')
    parser.add_argument('--profile', type=Path)
    args = parser.parse_args()
    build(args.revision, args.profile, desktop=True)
