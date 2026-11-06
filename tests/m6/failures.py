"""Real native refusal/restart and activation-write EIO recovery on owned images."""
import importlib.util,json,subprocess,time,traceback
from pathlib import Path
spec=importlib.util.spec_from_file_location('m6maintenance',Path(__file__).with_name('maintenance.py'));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
h=m.h;q=m.q;ROOT=q.ROOT

def install(g):
    h.next(g);h.next(g);h.next(g,2);h.next(g,2);h.next(g,5);start=len(g.log());h.next(g,1);return start

def main():
    work=ROOT/'build/m6/failures'/str(time.time_ns());work.mkdir(parents=True);out=ROOT/'evidence/M6/failures';out.mkdir(parents=True,exist_ok=True);checks=[];result={'passed':False,'checks':checks}
    def fresh(name,mib=2048,seed=b''):
        p=work/(name+'.img');p.touch();subprocess.run(['fsutil','sparse','setflag',str(p)],check=True,capture_output=True)
        with p.open('r+b') as f:f.truncate(mib*1024**2);f.write(seed)
        return p
    try:
        for name,size,seed in [('existing-owner',2048,b'EXISTING-OWNER-DATA'),('undersize',512,b'')]:
            disk=fresh(name,size,seed);before=q.digest(disk)
            with q.Guest(work/name,disk) as g:
                h.next(g);h.next(g);h.next(g,2);h.shot(g,out,name)
            assert q.digest(disk)==before;checks.append({'name':name+' selection refused; entire image unchanged','passed':True})
        for name,sector in [('boot-copy',2048),('volume-format',2369536),('initial-activation',41)]:
            disk=fresh(name);fault={'event':'write_aio','iotype':'write','errno':5,'sector':sector,'once':False}
            with q.Guest(work/(name+'-fault'),disk,fault=fault) as g:
                pos=install(g);g.wait('M6_INSTALL_FAILED',pos,240);assert 'M6_INSTALL_PASS' not in g.log();h.shot(g,out,name+'-error');(out/(name+'-error.log')).write_bytes(g.log().encode())
            with q.Guest(work/(name+'-restart'),disk) as g:
                pos=install(g);g.wait('M6_INSTALL_PASS',pos,240);(out/(name+'-restart.log')).write_bytes(g.log().encode())
            with q.Guest(work/(name+'-installed'),disk,media='installed') as g:assert 'M6_INSTALLED_VOLUME' in g.log()
            checks.append({'name':'real SATA '+name+' EIO refuses success; owned incomplete installation restarts and boots without media','passed':True})
        start,length,entries,record=m.inspect(disk,'Owner',1);saved=q.digest(disk,start,length)
        fault={'event':'write_aio','iotype':'write','errno':5,'sector':42,'once':False}
        with q.Guest(work/'update-activation-fault',disk,fault=fault,image_name='EngineOS-M6U1') as g:
            h.next(g,3);h.next(g);h.next(g,2);pos=len(g.log());h.next(g,1);g.wait('M6_INSTALL_FAILED',pos,240);assert 'M6_MAINTENANCE_PASS' not in g.log();(out/'update-error.log').write_bytes(g.log().encode())
        assert q.digest(disk,start,length)==saved
        with q.Guest(work/'uncertain-version',disk,media='installed') as g:
            assert 'M6_BOOT_VERSION_MISMATCH' in g.log();h.shot(g,out,'uncertain-version');(out/'uncertain-version.log').write_bytes(g.log().encode())
        with q.Guest(work/'recover-activation',disk) as g:
            m.flow(g,2,'M6_MAINTENANCE_PASS');(out/'activation-recovery.log').write_bytes(g.log().encode())
        assert q.digest(disk,start,length)==saved
        with q.Guest(work/'recovered-installed',disk,media='installed') as g:assert 'M6_INSTALLED_VOLUME' in g.log()
        checks.append({'name':'interrupted cross-version activation preserves data, refuses uncertain boot identity and restores the active version through ISO repair','passed':True})
        meta=json.loads((ROOT/'dist/m6-build-manifest.json').read_text(encoding='utf-8'));result.update(passed=True,build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M6.iso']['sha256'],scope='named SATA EIO/write refusal model; not arbitrary physical power-loss qualification')
    except Exception:result['error']=traceback.format_exc().replace(str(ROOT),'<project-root>');print(result['error'])
    (out/'results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print('M6 failures '+('PASS' if result['passed'] else 'FAIL'),flush=True)
    if not result['passed']:raise SystemExit(1)
if __name__=='__main__':main()
