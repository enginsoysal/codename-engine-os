"""Anonymous public native-site and exact GitHub release download verification."""
import hashlib,json,subprocess
from datetime import datetime,timezone
from html.parser import HTMLParser
from pathlib import Path
from urllib.request import Request,urlopen
ROOT=Path(__file__).resolve().parents[2];SITE='https://engaceros.brixcanvas.com'
class Page(HTMLParser):
    def __init__(self):super().__init__();self.parts=[];self.links=[];self.skip=0;self.lang='';self.iframes=0
    def handle_starttag(self,tag,attrs):
        attrs=dict(attrs)
        if tag in ('script','style'):self.skip+=1
        if tag=='html':self.lang=attrs.get('lang','')
        if tag=='iframe':self.iframes+=1
        if tag=='a':self.links.append(attrs.get('href',''))
    def handle_endtag(self,tag):
        if tag in ('script','style'):self.skip-=1
    def handle_data(self,value):
        if not self.skip:self.parts.append(value)
    @property
    def text(self):return ' '.join(' '.join(self.parts).split())
def read(url):
    with urlopen(Request(url,headers={'User-Agent':'EngacerOS-M10-release-check'}),timeout=60) as response:
        assert response.status==200
        return response.read()
def main():
    ref=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT).decode().strip()
    meta=json.loads((ROOT/'dist/m10-build-manifest.json').read_text(encoding='utf-8'))
    starter=json.loads((ROOT/'dist/m10-starter-manifest.json').read_text(encoding='utf-8'))
    checks=[]
    for route,words in [('/', ['ENGACER OS','Before you install M10','USB']),('/blog/docs-handbook-install-m10',['Install Engacer OS M10','EngacerOS.vmx']),('/blog/docs-handbook-hardware-m10',['Devices and USB files','Safe remove']),('/blog/documentation',['English manuals']),('/blog/downloads',['EngacerOS-M10.iso','EngacerOS-M10-VMware.zip'])]:
        p=Page();p.feed(read(SITE+route).decode('utf-8'));assert p.lang.startswith('en') and not p.iframes,route
        for text in words:assert text in p.text,(route,text)
        if route!='/':assert ref[:7] in p.text or any('/blob/'+ref+'/' in link for link in p.links),route
        if route=='/':assert not any('engineos.brixcanvas.com' in link for link in p.links),'Stale homepage domain'
        checks.append({'route':route,'passed':True})
    release=json.loads(subprocess.check_output(['gh','api','repos/enginsoysal/engacer-os/releases/tags/m10-v0.10.0'],cwd=ROOT))
    assert not release['draft'];assets={a['name']:a for a in release['assets']}
    for name,digest in [('EngacerOS-M10.iso',meta['artifacts']['EngacerOS-M10.iso']['sha256']),('EngacerOS-M10-VMware.zip',starter['sha256'])]:
        asset=assets[name];assert asset['digest']=='sha256:'+digest
        assert hashlib.sha256(read(asset['browser_download_url'])).hexdigest()==digest,name
        checks.append({'asset':name,'passed':True,'sha256':digest})
    out=ROOT/'evidence/documentation/homepage-m10';out.mkdir(parents=True,exist_ok=True)
    report={'passed':True,'checked_at':datetime.now(timezone.utc).isoformat(),'website':SITE,'source_commit':ref,'release':release['html_url'],'build_id':meta['build_id'],'checks':checks,'native_editor_save_and_visual_review':'separate browser screenshots'}
    (out/'public-readback.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print('Engacer M10 public site and exact release downloads PASS')
if __name__=='__main__':main()
