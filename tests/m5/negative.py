"""Real installer refusals and failed-copy recovery on disposable block backends."""
import importlib.util,json,struct,time
from pathlib import Path
spec=importlib.util.spec_from_file_location('m5_guest',Path(__file__).with_name('vm.py'));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
ROOT=m.ROOT

def main():
    work=ROOT/'build/m5/negative'/str(time.time_ns());work.mkdir(parents=True);out=ROOT/'evidence/M5/negative';out.mkdir(parents=True,exist_ok=True);checks=[];errors=[]
    def fresh(name,mib=256,seed=None):
        p=work/(name+'.img')
        with p.open('xb') as f:
            f.truncate(mib*1024*1024)
            if seed:f.write(seed)
        return p
    try:
        for name,size,seed in [('nonblank',256,b'EXISTING-OWNER-DATA'),('undersize',64,None)]:
            disk=fresh(name,size,seed);before=m.digest(disk)
            with m.Guest(work/name,disk) as g:
                assert 'Installer status: 2' in g.command('plan 0')
                assert 'Installer status: 2' in g.command('repairplan 0')
                assert 'Installer status: 2' in g.command('install 0 ERASE-'+str(size*2048))
                (out/(name+'.log')).write_text(g.log(),encoding='utf-8')
            assert before==m.digest(disk);checks.append({'name':name+' refused; entire disk hash unchanged','passed':True})
        disk=fresh('copy-failure');fault={'event':'write_aio','iotype':'write','errno':5,'sector':8192,'once':False}
        with m.Guest(work/'failed-copy',disk,fault=fault) as g:
            g.command('plan 0');assert 'Installer status: 3' in g.command('install 0 ERASE-524288')
            assert 'M5_INSTALL_PASS' not in g.log();assert 'help' in g.command('help');(out/'copy-failure.log').write_text(g.log(),encoding='utf-8')
        with disk.open('rb') as f:f.seek(40*512);pending=f.read(512);complete=f.read(512)
        assert pending[:8]==b'ENGM5V1\0' and struct.unpack_from('<I',pending,12)[0]==1 and not any(complete)
        checks.append({'name':'injected real SATA write EIO: no completion, live console survives','passed':True})
        with m.Guest(work/'resume',disk) as g:
            assert 'REPAIR PLAN' in g.command('repairplan 0');g.command('repair 0 REPAIR-524288','M5_REPAIR_PASS',180);(out/'resumed.log').write_text(g.log(),encoding='utf-8')
        with m.Guest(work/'resumed-installed',disk,media='installed') as g:
            assert 'M5_INSTALLED_VOLUME' in g.log();assert 'Write status: 0x0000000000000000' in g.command('write /Users/test/Documents/recovered.txt yes');(out/'resumed-installed.log').write_text(g.log(),encoding='utf-8')
        checks.append({'name':'incomplete first install resumes from live ISO and boots usable data volume','passed':True})
        guard=fresh('guard');guard_before=m.digest(guard);disk_before=m.digest(disk)
        extra=['-drive',f'if=none,id=guard,format=raw,file={guard}','-device','ide-hd,bus=ide.1,drive=guard']
        with m.Guest(work/'ambiguous',disk,media='installed',extra=extra) as g:
            assert 'M5_AMBIGUOUS_INSTALLS_REFUSED' in g.log()
            assert 'Write status: 0x0000000000000000' not in g.command('write /Users/test/Documents/unsafe.txt no')
            (out/'ambiguous.log').write_text(g.log(),encoding='utf-8')
        assert disk_before==m.digest(disk) and guard_before==m.digest(guard)
        checks.append({'name':'multiple eligible SATA disks refuse persistent mount; both disk hashes unchanged','passed':True})
        blank=fresh('plan-target');blank_before=m.digest(blank)
        with m.Guest(work/'target-binding',blank,extra=extra) as g:
            assert 'No changes yet' in g.command('plan 0')
            assert 'Installer status: 2' in g.command('install 1 ERASE-524288');g.command('cancel')
            (out/'target-binding.log').write_text(g.log(),encoding='utf-8')
        assert blank_before==m.digest(blank) and guard_before==m.digest(guard)
        checks.append({'name':'confirmation cannot redirect an outstanding plan to another disk','passed':True})
    except Exception:
        import traceback
        errors.append(traceback.format_exc().replace(str(ROOT),'<project-root>'));print(errors[-1],flush=True)
    manifest=json.loads((ROOT/'dist/m5-build-manifest.json').read_text(encoding='utf-8'));result={'passed':not errors,'build_id':manifest['build_id'],'checks':checks,'errors':errors};(out/'results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');print('Installer negative tests', 'PASS' if not errors else 'FAIL',flush=True)
    if errors:raise SystemExit(1)
if __name__=='__main__':main()
