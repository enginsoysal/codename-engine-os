"""Publish only the exact accepted M9 artifacts; anonymously verify primary downloads."""
import argparse,datetime,hashlib,json,subprocess,urllib.request
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];TAG='m9-v0.9.0';REPO='enginsoysal/codename-engine-os'
def gh(*args):return subprocess.check_output(['gh',*args],cwd=ROOT,text=True)
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def load(path):return json.loads(Path(path).read_text())
def main():
    p=argparse.ArgumentParser();p.add_argument('action',choices=['stage','publish','verify']);a=p.parse_args()
    accepted=load(ROOT/'evidence/M9/acceptance-results.json');assert accepted['passed'] and accepted['scope_id']=='M9-GFX-VM-v1'
    files={name:ROOT/'dist'/name for name in accepted['artifacts']}
    for name,value in accepted['artifacts'].items():assert sha(files[name])==value['sha256'],name
    files.update({'7z2606-src.tar.xz':ROOT/'third_party/7zip/7z2606-src.tar.xz',
                  'm9-acceptance-results.json':ROOT/'evidence/M9/acceptance-results.json',
                  'm9-build-manifest.json':ROOT/'dist/m9-build-manifest.json',
                  'm9-starter-manifest.json':ROOT/'dist/m9-starter-manifest.json'})
    sums=ROOT/'dist/M9-SHA256SUMS.txt'
    sums.write_bytes(''.join(sha(path)+'  '+name+'\n' for name,path in sorted(files.items())).encode())
    files[sums.name]=sums
    if a.action=='stage':
        assert not subprocess.check_output(['git','status','--porcelain'],cwd=ROOT,text=True).strip(),'Commit the final release first'
        commit=gh('api','repos/'+REPO+'/commits/main','--jq','.sha').strip()
        local=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip();assert local==commit,'Push the release commit first'
        gh('release','create',TAG,'--repo',REPO,'--draft','--target',commit,'--title','Engine OS 0.9.0 — VMware graphics and automatic pointer setup','--notes-file','docs/releases/m9-v0.9.0.md')
        gh('release','upload',TAG,'--repo',REPO,*[str(x) for x in files.values()])
    remote=json.loads(gh('api','repos/'+REPO+'/releases/tags/'+TAG))
    expected={name:{'sha256':sha(path),'size':path.stat().st_size} for name,path in files.items()}
    assert {x['name'] for x in remote['assets']}==set(expected),'Unexpected remote asset set'
    for asset in remote['assets']:
        value=expected[asset['name']];assert asset['size']==value['size'] and asset['digest']=='sha256:'+value['sha256'],asset['name']
    if a.action=='publish':gh('release','edit',TAG,'--repo',REPO,'--draft=false')
    if a.action=='stage':print(f'M9 draft verified: {len(expected)} assets');return
    request=urllib.request.Request(f'https://api.github.com/repos/{REPO}/releases/tags/{TAG}',headers={'User-Agent':'EngineOS-release-verification'})
    public=json.load(urllib.request.urlopen(request,timeout=60));assert not public['draft']
    downloaded={}
    for name in ['EngineOS-M9.iso','EngineOS-M9-VMware.zip']:
        url=f'https://github.com/{REPO}/releases/download/{TAG}/{name}'
        digest=hashlib.sha256();size=0
        with urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':'EngineOS-release-verification'}),timeout=120) as response:
            while True:
                block=response.read(1024**2)
                if not block:break
                digest.update(block);size+=len(block)
        assert digest.hexdigest()==expected[name]['sha256'] and size==expected[name]['size'],name
        downloaded[name]={'sha256':digest.hexdigest(),'size':size,'anonymous':True}
    result={'passed':True,'tag':TAG,'release':public['html_url'],'release_id':public['id'],
            'checked_at':datetime.datetime.now(datetime.timezone.utc).isoformat(),'all_remote_asset_digests_and_sizes_match':True,
            'asset_count':len(expected),'assets':expected,'anonymous_complete_primary_downloads':downloaded}
    path=ROOT/'evidence/documentation/m9-release-publication.json';path.write_bytes((json.dumps(result,indent=2)+'\n').encode())
    print(f'M9 public release verified: {len(expected)} assets and complete anonymous ISO/ZIP downloads')
if __name__=='__main__':main()
