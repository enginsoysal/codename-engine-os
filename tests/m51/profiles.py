"""Reject unsafe/unsupported recipes before any compiler, shell hook or media write."""
import importlib.util,json,tempfile,copy
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
import sys
sys.path.insert(0,str(ROOT/'tools/build'))
from distro import load

def main():
    original=json.loads((ROOT/'distros/engineos.json').read_text(encoding='utf-8'));checks=[]
    with tempfile.TemporaryDirectory(dir=ROOT/'distros',prefix='validation-') as tmp:
        path=Path(tmp)/'profile.json'
        (Path(tmp)/'large.bin').write_bytes(b'x'*49153)
        cases=[('oversized file',{'files':[{'source':Path(tmp).name+'/large.bin','target':'/Users/test/Documents/Large.txt'}]}),('outside source',{'files':[{'source':'../../README.md','target':'/Users/test/Documents/x'}]}),('outside target',{'files':[{'source':'packages/welcome-guide/content.txt','target':'/System/x'}]}),('traversal',{'files':[{'source':'packages/welcome-guide/content.txt','target':'/Users/test/Documents/../x'}]}),('shell hook',{'post_install':'run anything'}),('unsupported package',{'packages':['desktop-not-implemented']}),('invalid colour',{'accent':'red'}),('duplicate package',{'packages':['welcome-guide','welcome-guide']}),('duplicate target',{'files':[{'source':'packages/welcome-guide/content.txt','target':'/Users/test/Documents/Welcome.txt'}]})]
        for label,change in cases:
            path.write_bytes(json.dumps({**original,**change}).encode());refused=False
            try:load(path)
            except (ValueError,TypeError,OSError):refused=True
            assert refused,label;checks.append({'name':label+' rejected','passed':True})
    for name in ['engineos','forge']:assert load(ROOT/'distros'/f'{name}.json')['id']==name
    out=ROOT/'evidence/M5.1';out.mkdir(parents=True,exist_ok=True);(out/'profile-results.json').write_bytes((json.dumps({'passed':True,'checks':checks},indent=2)+'\n').encode());print('Profile validation PASS',flush=True)
if __name__=='__main__':main()
