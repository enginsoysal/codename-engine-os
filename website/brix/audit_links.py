"""Validate observed repository, release and external reference destinations."""
import argparse,json,re,subprocess,time
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime,timezone
from pathlib import Path
from urllib.parse import urlsplit,unquote
from urllib.request import Request,build_opener
from audit import Redirects,SITE

ROOT=Path(__file__).resolve().parents[2]

def remote(url):
    try:
        with build_opener(Redirects).open(Request(url,method='HEAD',headers={'User-Agent':'EngacerOS-link-audit/1.0'}),timeout=35) as r:
            return {'url':url,'status':r.status,'final_url':r.url,'passed':r.status<400}
    except Exception as e:
        code=getattr(e,'code',0)
        return {'url':url,'status':code,'passed':False,'error':str(e),'classification':'blocked_or_temporarily_unverified' if code in (0,401,403,405,429,500,502,503,504) else 'broken'}

def main():
    a=argparse.ArgumentParser();a.add_argument('--input',type=Path,required=True);a.add_argument('--output',type=Path,required=True);args=a.parse_args()
    crawl=json.loads(args.input.read_text(encoding='utf-8'));links=sorted({u for p in crawl['pages'] for u in p.get('links',[])})
    releases=json.loads(subprocess.check_output(['gh','api','repos/enginsoysal/engacer-os/releases?per_page=100'],cwd=ROOT))
    release_assets={unquote(a['browser_download_url']):a for r in releases for a in r['assets']};tags={r['tag_name'] for r in releases}
    source_links=[];assets=[];others=[];actions=[];repo_links=[]
    for url in links:
        u=urlsplit(url)
        if u.netloc=='engaceros.brixcanvas.com':continue
        if u.netloc in ('x.com','www.facebook.com','www.linkedin.com','wa.me'):
            actions.append(url);continue # Do not submit share, message or other communication actions.
        match=re.match(r'^/enginsoysal/(?:engacer-os|codename-engine-os)/(blob|tree)/([^/]+)(?:/(.*))?$',u.path)
        if u.netloc=='github.com' and match:
            ref=unquote(match[2]);path=unquote(match[3] or '');source_links.append((url,ref,path));continue
        if u.netloc=='github.com' and re.match(r'^/enginsoysal/(?:engacer-os|codename-engine-os)/releases/download/',u.path):
            canonical=url.replace('/codename-engine-os/','/engacer-os/');asset=release_assets.get(unquote(canonical));assets.append({'url':url,'passed':bool(asset),'size':asset['size'] if asset else None,'digest':asset.get('digest') if asset else None});continue
        if u.netloc=='github.com' and re.match(r'^/enginsoysal/(?:engacer-os|codename-engine-os)(?:/releases(?:/tag/[^/]+)?)?/?$',u.path):
            tag=re.search(r'/releases/tag/([^/]+)',u.path);repo_links.append({'url':url,'passed':not tag or unquote(tag[1]) in tags});continue
        others.append(url)
    expressions=sorted({ref+(':'+path if path else '') for _,ref,path in source_links})
    batch=subprocess.run(['git','cat-file','--batch-check'],cwd=ROOT,input='\n'.join(expressions)+'\n',text=True,capture_output=True,check=True)
    valid={expression:not line.endswith(' missing') for expression,line in zip(expressions,batch.stdout.splitlines())}
    assert len(valid)==len(expressions)
    sources=[{'url':url,'passed':valid[ref+(':'+path if path else '')],'ref':ref,'path':path} for url,ref,path in source_links]
    with ThreadPoolExecutor(max_workers=4) as pool:external=list(pool.map(remote,others))
    broken=[x for x in sources+assets+repo_links+external if not x['passed'] and x.get('classification')!='blocked_or_temporarily_unverified']
    unverified=[x for x in external if x.get('classification')=='blocked_or_temporarily_unverified']
    result={'passed':not broken,'checked_at':datetime.now(timezone.utc).isoformat(),'source_commit':crawl['source_commit'],'repository_targets':sources,'release_assets':assets,'repository_pages':repo_links,'external_references':external,'unsubmitted_share_actions':actions,'broken':broken,'unverified':unverified,'methods':'Repository refs/paths checked against Git objects; download targets/digests checked against live GitHub release assets; external read-only references use HTTP HEAD. Share/message actions are never submitted.'}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('Repository targets',len(sources),'release assets',len(assets),'external references',len(external),'broken',len(broken),'unverified',len(unverified),flush=True)

if __name__=='__main__':main()
