"""Publish only the exact accepted M92 artifacts; anonymously verify primary downloads."""
import argparse,datetime,hashlib,json,re,subprocess,urllib.request,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];TAG='m92-v0.9.2';REPO='enginsoysal/codename-engine-os'
def gh(*args):return subprocess.check_output(['gh',*args],cwd=ROOT,text=True,encoding='utf-8')
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def load(path):return json.loads(Path(path).read_text())
def main():
    p=argparse.ArgumentParser();p.add_argument('action',choices=['stage','publish','verify']);a=p.parse_args()
    accepted=load(ROOT/'evidence/M92/acceptance-results.json');assert accepted['passed'] and accepted['scope_id']=='M92-STORAGE-VM-v1'
    files={name:ROOT/'dist'/name for name in accepted['artifacts']}
    for name,value in accepted['artifacts'].items():assert sha(files[name])==value['sha256'],name
    shutil.copyfile(ROOT/'evidence/M92/acceptance-results.json',ROOT/'dist/m92-acceptance-results.json')
    files.update({'7z2606-src.tar.xz':ROOT/'third_party/7zip/7z2606-src.tar.xz',
                  'm92-acceptance-results.json':ROOT/'dist/m92-acceptance-results.json',
                  'm92-build-manifest.json':ROOT/'dist/m92-build-manifest.json',
                  'm92-starter-manifest.json':ROOT/'dist/m92-starter-manifest.json'})
    sums=ROOT/'dist/M92-SHA256SUMS.txt'
    sums.write_bytes(''.join(sha(path)+'  '+name+'\n' for name,path in sorted(files.items())).encode())
    files[sums.name]=sums
    assert all(name==path.name for name,path in files.items()),'Upload filenames must match checksum entries'
    if a.action=='stage':
        assert not subprocess.check_output(['git','status','--porcelain'],cwd=ROOT,text=True,encoding='utf-8').strip(),'Commit the final release first'
        commit=gh('api','repos/'+REPO+'/commits/main','--jq','.sha').strip()
        local=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True,encoding='utf-8').strip();assert local==commit,'Push the release commit first'
        # Repository-relative manual links need an absolute canonical target in
        # the release body, whose URL is outside docs/releases/.
        source=ROOT/'docs/releases/m92-v0.9.2.md'
        def canonical(match):
            label,link=match.group(1),match.group(2)
            if re.match(r'^[a-z]+://|^#',link):return match.group(0)
            target,mark,fragment=link.partition('#');path=(source.parent/target).resolve();assert path.is_relative_to(ROOT) and path.exists()
            return '['+label+'](https://github.com/'+REPO+'/blob/'+commit+'/'+path.relative_to(ROOT).as_posix()+('#'+fragment if mark else '')+')'
        notes=ROOT/'build/m92/release-notes.md';notes.write_text(re.sub(r'\[([^\]]+)\]\(([^)]+)\)',canonical,source.read_text(encoding='utf-8')),encoding='utf-8')
        gh('release','create',TAG,'--repo',REPO,'--draft','--target',commit,'--title','Engine OS 0.9.2 - Automatic writable storage and reachable desktop windows','--notes-file',str(notes))
        gh('release','upload',TAG,'--repo',REPO,*[str(x) for x in files.values()])
    # GitHub's tag endpoint excludes unpublished drafts. Authenticated listing
    # exposes the owned draft so its complete asset set can be checked first.
    candidates=json.loads(gh('api','repos/'+REPO+'/releases?per_page=100'))
    matches=[value for value in candidates if value['tag_name']==TAG]
    assert len(matches)==1,'Expected one owned release for the tag'
    remote=matches[0]
    expected={name:{'sha256':sha(path),'size':path.stat().st_size} for name,path in files.items()}
    assert {x['name'] for x in remote['assets']}==set(expected),'Unexpected remote asset set'
    for asset in remote['assets']:
        value=expected[asset['name']];assert asset['size']==value['size'] and asset['digest']=='sha256:'+value['sha256'],asset['name']
    if a.action=='publish':gh('release','edit',TAG,'--repo',REPO,'--draft=false')
    if a.action=='stage':print(f'M92 draft verified: {len(expected)} assets');return
    request=urllib.request.Request(f'https://api.github.com/repos/{REPO}/releases/tags/{TAG}',headers={'User-Agent':'EngineOS-release-verification'})
    public=json.load(urllib.request.urlopen(request,timeout=60));assert not public['draft']
    downloaded={}
    for name in ['EngineOS-M92.iso','EngineOS-M92-VMware.zip']:
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
    path=ROOT/'evidence/documentation/m92-release-publication.json';path.write_bytes((json.dumps(result,indent=2)+'\n').encode())
    print(f'M92 public release verified: {len(expected)} assets and complete anonymous ISO/ZIP downloads')
if __name__=='__main__':main()
