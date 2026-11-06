"""Read actual guest allocator counters and CPU halt state without instrumenting success."""
import importlib.util,json,re,struct,time,traceback
from pathlib import Path
spec=importlib.util.spec_from_file_location('m6guided',Path(__file__).with_name('guided.py'));h=importlib.util.module_from_spec(spec);spec.loader.exec_module(h)
q=h.q;ROOT=q.ROOT

def main():
    work=q.new_work('resources');out=ROOT/'evidence/M6/resources';out.mkdir(parents=True,exist_ok=True);checks=[];result={'passed':False,'checks':checks}
    try:
        manifest=json.loads((ROOT/'dist/m6-build-manifest.json').read_text(encoding='utf-8'));mapping=(ROOT/'dist/EngineOS-M6-kernel.map').read_text(encoding='utf-8');match=re.search(r'^\s*([0-9a-f]+)\s+[0-9a-f]+\s+20\s+1\s+stats$',mapping,re.M);assert match;address=int(match[1],16)
        def stats(g):
            raw=g.qmp('human-monitor-command',{'command-line':f'xp /4gx 0x{address:x}'})
            values=[]
            for line in raw.splitlines():
                if ':' in line:values.extend(int(v,16) for v in re.findall(r'0x[0-9a-fA-F]+',line.split(':',1)[1]))
            assert len(values)==4;return values
        disk=work/'owned.img'
        q.sparse_disk(disk,2*1024**3)
        for ram,cpus in [(64,1),(128,2),(256,4)]:
            with q.Guest(work/(str(ram)+'-'+str(cpus)),disk,ram=ram,cpus=cpus) as g:
                before=stats(g)
                for _ in range(10):h.key(g,'tab');h.key(g,'tab');h.key(g,'tab');h.key(g,'tab');h.key(g,'tab')
                time.sleep(.2);after=stats(g);assert before[0]==after[0] and before[1]==after[1]
                registers=g.qmp('human-monitor-command',{'command-line':'info registers -a'});assert registers.count('HLT=1')==cpus
                (out/(str(ram)+'-'+str(cpus)+'.log')).write_bytes(g.log().encode())
                checks.append({'name':'actual menu navigation retains allocations and all CPUs halt at input idle','passed':True,'guest_ram_mib':ram,'cpus':cpus,'managed_current_bytes':after[0]*4096,'managed_peak_bytes':after[1]*4096,'managed_available_pages':after[2],'allocation_failures':after[3]})
        data=(ROOT/'dist/EngineOS-M6-kernel.elf').read_bytes();ph=struct.unpack_from('<Q',data,32)[0];n=struct.unpack_from('<H',data,56)[0];segments=[struct.unpack_from('<IIQQQQQQ',data,ph+i*56) for i in range(n)];loads=[s for s in segments if s[0]==1]
        result.update(passed=True,build_id=manifest['build_id'],iso_sha256=manifest['artifacts']['EngineOS-M6.iso']['sha256'],kernel_physical_span_bytes=max(s[4]+s[6] for s in loads)-min(s[4] for s in loads),scope='named QEMU configurations; managed counters exclude static kernel and boot/firmware resources')
    except Exception:result['error']=q.clean(traceback.format_exc());print(result['error'])
    (out/'results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print('M6 resources '+('PASS' if result['passed'] else 'FAIL'),flush=True)
    if not result['passed']:raise SystemExit(1)
if __name__=='__main__':main()
