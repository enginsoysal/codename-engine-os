"""Menu-level missing/unsupported/nonblank disk explanations and no-write hashes."""
import importlib.util,json,time,traceback
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];s=importlib.util.spec_from_file_location('m51_base',ROOT/'tests/m5/vm.py');q=importlib.util.module_from_spec(s);s.loader.exec_module(q)
def main():
    work=ROOT/'build/m51/refusals'/str(time.time_ns());work.mkdir(parents=True);out=ROOT/'evidence/M5.1/refusals';out.mkdir(parents=True,exist_ok=True);checks=[];errors=[]
    try:
        for name,mib,seed in [('small',64,b''),('nonblank',256,b'OWNER-DATA'),('missing',256,b'')]:
            disk=work/(name+'.img')
            with disk.open('xb') as f:f.truncate(mib*1024**2);f.seek(0);f.write(seed)
            before=q.digest(disk);guest=q.Guest(work/name,disk)
            if name=='missing':
                for prefix in ['if=none,id=target,','ide-hd,bus=ide.0,drive=target']:
                    i=next(i for i,a in enumerate(guest.args) if a.startswith(prefix));del guest.args[i-1:i+1]
            with guest as g:
                g.wait('M51_HOME_READY');text=g.command('2','Choice> ')
                if name=='missing':assert 'No disks were detected' in text and 'add a SATA hard disk' in text
                else:
                    text=g.command('1','Choice> ');assert 'cannot be used' in text;assert 'NOT completed' not in text;g.command('D','Choice> ');assert 'Last operation code: 2' in g.log()
                (out/(name+'.log')).write_bytes(g.log().encode())
            assert before==q.digest(disk);checks.append({'name':name+' explained through menu; full backing unchanged','passed':True})
    except Exception:errors.append(traceback.format_exc().replace(str(ROOT),'<project-root>'));print(errors[-1],flush=True)
    m=json.loads((ROOT/'dist/m51-build-manifest.json').read_text(encoding='utf-8'));(out/'results.json').write_bytes((json.dumps({'passed':not errors,'build_id':m['build_id'],'checks':checks,'errors':errors},indent=2)+'\n').encode());print('Guided refusals','PASS' if not errors else 'FAIL',flush=True)
    if errors:raise SystemExit(1)
if __name__=='__main__':main()
