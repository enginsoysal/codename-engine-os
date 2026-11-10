"""Crawl the public documentation site; record branding, metadata and link targets."""
import argparse,json,re,time
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime,timezone
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import urljoin,urlsplit,urldefrag,parse_qs
from urllib.request import Request,build_opener,HTTPRedirectHandler
from urllib.error import HTTPError

ROOT=Path(__file__).resolve().parents[2]
SITE='https://engaceros.brixcanvas.com'
OLD_HOSTS={'engineos.brixcanvas.com','engacer.brixcanvas.com','codenameenginos.brixcanvas.com'}

class Redirects(HTTPRedirectHandler):
    # Python 3.10's standard handler predates permanent HTTP 308 support.
    http_error_308=HTTPRedirectHandler.http_error_302
    def redirect_request(self,req,fp,code,msg,headers,newurl):
        redirected=super().redirect_request(req,fp,307 if code==308 else code,msg,headers,newurl)
        if redirected and req.get_method()=='HEAD':redirected.method='HEAD'
        return redirected

class Page(HTMLParser):
    def __init__(self):
        super().__init__();self.skip=0;self.parts=[];self.links=[];self.images=[];self.meta={};self.title=[];self.in_title=False;self.lang='';self.ids=set()
    def handle_starttag(self,tag,attrs):
        a=dict(attrs)
        if tag in ('script','style'):self.skip+=1
        if tag=='title':self.in_title=True
        if tag=='html':self.lang=a.get('lang','')
        if tag=='a':self.links.append(a.get('href',''))
        if tag=='img':self.images.append(a.get('src',''))
        if a.get('id'):self.ids.add(a['id'])
        if tag=='meta':self.meta[a.get('name',a.get('property',''))]=a.get('content','')
        if tag=='link' and a.get('rel')=='canonical':self.meta['canonical']=a.get('href','')
    def handle_endtag(self,tag):
        if tag in ('script','style'):self.skip-=1
        if tag=='title':self.in_title=False
    def handle_data(self,value):
        if not self.skip:self.parts.append(value)
        if self.in_title:self.title.append(value)
    @property
    def text(self):return ' '.join(' '.join(self.parts).split())

def fetch(url):
    try:
        with build_opener(Redirects).open(Request(url,headers={'User-Agent':'EngacerOS-site-audit/1.0'}),timeout=45) as response:
            raw=response.read();p=Page();p.feed(raw.decode('utf-8'));text=p.text
            legacy=[text[max(0,m.start()-75):m.end()+100] for m in re.finditer(r'(?i)(?<![A-Za-z])(?:Codename:\s*)?Engine OS\b|engineos\.brixcanvas\.com|engacer\.brixcanvas\.com|enginsoysal/codename-engine-os',text)]
            return {'url':url,'status':response.status,'final_url':response.url,'title':' '.join(p.title),'lang':p.lang,'meta':p.meta,'links':sorted(set(urljoin(url,h) for h in p.links if h and not h.startswith(('mailto:','tel:','javascript:')))),'images':sorted(set(urljoin(url,h) for h in p.images if h)),'legacy_text':legacy,'text_length':len(text),'soft_404':bool(re.search(r'(?i)^(?:404|page not found|site not found)',text))}
    except Exception as e:return {'url':url,'status':getattr(e,'code',0),'error':str(e)}

def main():
    p=argparse.ArgumentParser();p.add_argument('--payload',type=Path,default=ROOT/'build/brix/site-content.json');p.add_argument('--output',type=Path,default=ROOT/'build/brix/audit.json');a=p.parse_args()
    payload=json.loads(a.payload.read_text(encoding='utf-8'));posts=payload['page']['blog']['posts'];source_by_url={SITE+'/blog/'+p['slug']:next(x['value'] for x in p['meta'] if x['key']=='engine_source_path') for p in posts}
    pending={SITE+'/',*source_by_url};results={}
    with ThreadPoolExecutor(max_workers=4) as pool:
        while pending:
            assert len(results)+len(pending)<=600,'Unexpectedly large public crawl'
            batch=sorted(pending-set(results));pending=set()
            for r in pool.map(fetch,batch):
                results[r['url']]=r;r['source_path']=source_by_url.get(r['url'])
                if r.get('status')==200:
                    for href in r['links']:
                        u=urlsplit(href)
                        query=parse_qs(u.query)
                        paging=set(query)=={'page'} and len(query['page'])==1 and query['page'][0].isdigit() and 1<=int(query['page'][0])<=100
                        if u.netloc=='engaceros.brixcanvas.com' and (not u.query or paging) and not u.path.startswith(('/api/','/_next/')):
                            normalized=urldefrag(href)[0]
                            if normalized not in results:pending.add(normalized)
            pending.difference_update(results)
            print('Public pages checked:',len(results),'additional discovered:',len(pending),flush=True)
    links=sorted({u for r in results.values() for u in r.get('links',[])})
    stale=[u for u in links if urlsplit(u).netloc in OLD_HOSTS or re.search(r'github(?:usercontent)?\.com/enginsoysal/codename-engine-os(?:/|$)',u)]
    errors=[r for r in results.values() if r['status']!=200 or r.get('soft_404')]
    branding=[]
    for r in results.values():
        if r['status']!=200:continue
        values={'title':r['title'],**r['meta']}
        for key,value in values.items():
            if key in ('title','og:title','twitter:title','og:site_name','canonical','og:url') and re.search(r'(?i)\bEngine OS\b|(?:engineos|engacer)\.brixcanvas\.com|github\.com/enginsoysal/codename-engine-os',value):branding.append({'url':r['url'],'field':key,'value':value})
        if not r['lang'].startswith('en'):branding.append({'url':r['url'],'field':'lang','value':r['lang']})
    report={'passed':not errors and not stale and not branding,'checked_at':datetime.now(timezone.utc).isoformat(),'website':SITE,'source_commit':payload['source_commit'],'expected_articles':len(posts),'pages_checked':len(results),'unique_links':len(links),'stale_links':stale,'page_errors':errors,'branding_errors':branding,'pages':list(results.values())}
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:report[k] for k in ['pages_checked','unique_links','expected_articles']}),'stale links',len(stale),'page errors',len(errors),'legacy-containing pages',sum(bool(r.get('legacy_text')) for r in results.values()),flush=True)

if __name__=='__main__':main()
