"""Validate data-only distribution profiles before invoking a compiler or creating media."""
import hashlib,json,re
from pathlib import Path,PurePosixPath
from m1 import ROOT

def load(path):
    base=(ROOT/'distros').resolve();path=Path(path).resolve()
    def local(p):
        p=Path(p).resolve()
        if not p.is_relative_to(base) or not p.is_file() or p.is_symlink():raise ValueError('Distribution input must be a regular file inside distros/')
        return p
    path=local(path);p=json.loads(path.read_text(encoding='utf-8'))
    if set(p)!={'schema','id','name','version','welcome','accent','packages','files'} or p['schema']!=1:raise ValueError('Unsupported profile schema/fields')
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
        if str(dst)!=r['target'] or any(ord(c)<32 for c in r['target']) or not dst.is_absolute() or '..' in dst.parts or not r['target'].startswith(('/Users/test/Documents/','/Data/Packages/')) or '\\' in r['target'] or len(r['target'].encode())>255 or any(len(s.encode())>63 for s in dst.parts):raise ValueError('File target outside distribution data roots')
        if r['target'] in seen:raise ValueError('Duplicate file target')
        seen.add(r['target']);f=local(base/str(src));raw=f.read_bytes()
        if len(raw)>49152:raise ValueError('Distribution file exceeds the native 48 KiB file limit')
        inputs.append(f);files.append({'source':f.relative_to(ROOT).as_posix(),'target':r['target'],'sha256':hashlib.sha256(raw).hexdigest()})
    return {**p,'resolved_files':files,'input_sha256':{f.relative_to(ROOT).as_posix():hashlib.sha256(f.read_bytes()).hexdigest() for f in inputs}}

def prepare(work,p):
    (work/'distro.json').write_bytes((json.dumps(p,indent=2)+'\n').encode())
    text=''.join('#define DISTRO_'+k.upper()+' '+json.dumps(p[k])+'\n' for k in ['name','version','welcome'])
    text+='#define DISTRO_ACCENT 0x'+p['accent'][1:]+'u\n'
    (work/'distro.h').write_bytes(text.encode())
