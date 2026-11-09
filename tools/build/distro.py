"""Validate data-only distribution profiles before invoking a compiler or creating media."""
import hashlib,json,re
from pathlib import Path,PurePosixPath
from m1 import ROOT

def load(path,desktop=False,standard_apps=False,windows=False):
    base=(ROOT/'distros').resolve();path=Path(path).resolve()
    def local(p):
        p=Path(p).resolve()
        if not p.is_relative_to(base) or not p.is_file() or p.is_symlink():raise ValueError('Distribution input must be a regular file inside distros/')
        return p
    path=local(path);p=json.loads(path.read_text(encoding='utf-8'))
    if not isinstance(p,dict) or type(p.get('schema')) is not int:raise ValueError('Profile schema must be an integer in an object')
    fields={'schema','id','name','version','welcome','accent','packages','files'}
    if p.get('schema') in (2,3) and desktop:
        if set(p)!=fields|{'desktop'}:raise ValueError('Unsupported desktop profile fields')
        d=p['desktop']
        expected={'dark','calculator','wallpaper','tagline'}
        if p['schema']==3:
            if not standard_apps:raise ValueError('Native application selections require an M7.4 build')
            expected.add('applications')
        if not isinstance(d,dict) or set(d)!=expected:raise ValueError('Unsupported desktop options')
        if p['schema']==3:
            from m74 import catalogue
            available={p['name'] for p in catalogue(windows=windows) if p['ready']}
            if not isinstance(d['applications'],list) or any(not isinstance(v,str) or v not in available for v in d['applications']) or len(d['applications'])!=len(set(d['applications'])):raise ValueError('Invalid native application selection')
            # Deferred engines cannot be advertised as selectable working apps.
            if d['calculator']!=('calculator' in d['applications']):raise ValueError('Calculator toggle must match the native application selection')
        if type(d['dark']) is not bool or type(d['calculator']) is not bool:raise ValueError('Desktop toggles must be booleans')
        if not isinstance(d['wallpaper'],str) or not re.fullmatch('#[0-9a-fA-F]{6}',d['wallpaper']):raise ValueError('Invalid desktop wallpaper colour')
        if not isinstance(d['tagline'],str) or not 1<=len(d['tagline'])<=48 or any(ord(c)<32 or ord(c)>126 for c in d['tagline']):raise ValueError('Invalid desktop tagline')
    elif set(p)!=fields or p['schema']!=1:raise ValueError('Unsupported profile schema/fields')
    for k,limit in [('name',32),('version',16),('welcome',76)]:
        if not isinstance(p[k],str) or not 1<=len(p[k])<=limit or any(ord(c)<32 or ord(c)>126 for c in p[k]):raise ValueError('Profile '+k+' must be bounded printable ASCII')
    if not re.fullmatch('[a-z][a-z0-9-]{0,23}',p['id']) or not re.fullmatch('#[0-9a-fA-F]{6}',p['accent']):raise ValueError('Invalid distribution ID or colour')
    if not isinstance(p['packages'],list) or len(p['packages'])>8 or len(set(p['packages']))!=len(p['packages']):raise ValueError('Invalid package selection')
    inputs=[path];records=[]
    for package in p['packages']:
        if not isinstance(package,str) or not re.fullmatch('[a-z][a-z0-9-]{0,31}',package):raise ValueError('Invalid package ID')
        f=local(base/'packages'/package/'package.json');inputs.append(f);v=json.loads(f.read_text(encoding='utf-8'))
        if set(v)!={'schema','id','files'} or v['schema']!=1 or v['id']!=package:raise ValueError('Invalid data package manifest')
        records+=v['files']
    if not isinstance(p['files'],list):raise ValueError('files must be a list')
    records+=p['files'];seen=set();files=[]
    if len(records)>32:raise ValueError('At most 32 distribution files')
    for r in records:
        if set(r)!={'source','target'}:raise ValueError('Only source/target are supported; executable hooks are forbidden')
        src=PurePosixPath(r['source']);dst=PurePosixPath(r['target'])
        if src.is_absolute() or '..' in src.parts or '\\' in r['source'] or ':' in r['source']:raise ValueError('Invalid source path')
        roots=('/Users/test/Documents/','/Data/Packages/')
        if p['schema']==3:roots+=('/Users/test/Downloads/','/Users/test/Pictures/','/Users/test/Music/')
        if str(dst)!=r['target'] or any(ord(c)<32 for c in r['target']) or not dst.is_absolute() or '..' in dst.parts or not r['target'].startswith(roots) or '\\' in r['target'] or len(r['target'].encode())>255 or any(len(s.encode())>63 for s in dst.parts):raise ValueError('File target outside distribution data roots')
        if r['target'] in seen:raise ValueError('Duplicate file target')
        seen.add(r['target']);f=local(base/str(src));raw=f.read_bytes()
        if len(raw)>49152:raise ValueError('Distribution file exceeds the native 48 KiB file limit')
        inputs.append(f);files.append({'source':f.relative_to(ROOT).as_posix(),'target':r['target'],'sha256':hashlib.sha256(raw).hexdigest()})
    return {**p,**({'windows_runtime':True} if windows else {}),'resolved_files':files,'input_sha256':{f.relative_to(ROOT).as_posix():hashlib.sha256(f.read_bytes()).hexdigest() for f in inputs}}

def prepare(work,p):
    (work/'distro.json').write_bytes((json.dumps(p,indent=2)+'\n').encode())
    text=''.join('#define DISTRO_'+k.upper()+' '+json.dumps(p[k])+'\n' for k in ['name','version','welcome'])
    text+='#define DISTRO_ACCENT 0x'+p['accent'][1:]+'u\n'
    text+='#define DISTRO_M6_GUIDE '+str(int(any(r['target']=='/Users/test/Documents/Welcome.txt' for r in p['resolved_files'])))+'u\n'
    d=p.get('desktop',{'dark':False,'calculator':True,'wallpaper':'#10252f','tagline':'Your space. Your system.'})
    text+='#define DISTRO_DESKTOP_DARK '+str(int(d['dark']))+'u\n'
    text+='#define DISTRO_DESKTOP_CALCULATOR '+str(int(d['calculator']))+'u\n'
    text+='#define DISTRO_DESKTOP_WALLPAPER 0x'+d['wallpaper'][1:]+'u\n'
    text+='#define DISTRO_DESKTOP_TAGLINE '+json.dumps(d['tagline'])+'\n'
    from m74 import catalogue
    catalog=catalogue(windows=p.get('windows_runtime',False))
    selected=d.get('applications',[p['name'] for p in catalog if p['ready'] and (p['kind']!=4 or d['calculator'])])
    mask=sum(1<<p['kind'] for p in catalog if p['name'] in selected)
    text+='#define DISTRO_APPS74_MASK '+str(mask)+'u\n'
    (work/'distro.h').write_bytes(text.encode())
