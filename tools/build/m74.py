"""Build isolated M7.4 standard-application development media."""
import argparse
import re
from pathlib import Path

def catalogue():
    """Read fixed package identities, execution classes and readiness once."""
    from m1 import ROOT
    text=(ROOT/'include/engine/desktop/catalog74.h').read_text(encoding='utf-8')
    return [dict(name=name,kind=int(kind),folder=folder,title=title,worker=bool(int(worker)),ready=bool(int(ready)))
            for name,kind,folder,title,worker,ready in re.findall(r'X\((\w+),(\d+),(\w+),"([^"]+)",([01]),([01])\)',text)]

def applications():
    """Only actual ready CPL3 programs become executable build artifacts."""
    return [(p['name'],p['kind']) for p in catalogue() if p['ready'] and p['worker']]

def descriptor(package):
    import json
    return (json.dumps({'schema':1,'id':package['name'],'version':'1.0','frontend':'builtin.'+package['name']+'.v1',
                        'requires':['engine.desktop','engine.broker'],'removal':'optional-keep-data'},sort_keys=True,separators=(',',':'))+'\n').encode('ascii')

if __name__=='__main__':
    from m6 import build
    parser=argparse.ArgumentParser()
    parser.add_argument('--profile',type=Path)
    parser.add_argument('--revision',default='standard')
    args=parser.parse_args()
    build(args.revision,args.profile,standard_apps=True)
