"""Build a validated custom distribution using the original native guided installer."""
import argparse,hashlib,json,shutil
from pathlib import Path
from m1 import ROOT
from distro import load
from m5 import build

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--profile',type=Path,default=ROOT/'distros/engineos.json');parser.add_argument('--validate-only',action='store_true');a=parser.parse_args();p=load(a.profile)
    if a.validate_only:print(json.dumps({'valid':True,'id':p['id'],'files':len(p['resolved_files'])}));return
    m=build(p);out=ROOT/'dist/profiles'/p['id'];out.mkdir(parents=True,exist_ok=True);artifacts={}
    for name in m['artifacts']:
        target=name.replace('EngineOS-M5','EngineOS-'+p['id']+'-M51');shutil.copyfile(ROOT/'dist'/name,out/target);artifacts[target]={'sha256':hashlib.sha256((out/target).read_bytes()).hexdigest(),'size':(out/target).stat().st_size}
    m.update(milestone='M5.1',qualification_scope='M51-VM-v1',release_version='0.5.1',distribution=p,artifacts=artifacts)
    (out/'build-manifest.json').write_bytes((json.dumps(m,indent=2)+'\n').encode())
    (out/'SHA256SUMS.txt').write_bytes(''.join(v['sha256']+'  '+k+'\n' for k,v in artifacts.items()).encode())
    if p['id']=='engineos':
        public={}
        for n in artifacts:
            dest=n.replace('EngineOS-engineos-M51','EngineOS-M51');shutil.copyfile(out/n,ROOT/'dist'/dest);public[dest]=artifacts[n]
        m['artifacts']=public;(ROOT/'dist/m51-build-manifest.json').write_bytes((json.dumps(m,indent=2)+'\n').encode());(ROOT/'dist/M51-SHA256SUMS.txt').write_bytes(''.join(v['sha256']+'  '+k+'\n' for k,v in public.items()).encode())
    print(json.dumps({'profile':p['id'],'build_id':m['build_id'],'output':'dist/profiles/'+p['id']}))
if __name__=='__main__':main()
