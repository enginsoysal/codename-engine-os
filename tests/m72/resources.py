"""Measure Files navigation and input-idle on low-memory original-kernel guests."""
import importlib.util
import json
from pathlib import Path
import re
import struct
import time
import traceback
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('session72',Path(__file__).with_name('session.py'));s=importlib.util.module_from_spec(spec);spec.loader.exec_module(s)
q,h=s.q,s.h

def address(name):
    text=(ROOT/'dist/EngineOS-M72-installed-kernel.map').read_text(encoding='utf-8')
    m=re.search(r'^\s*([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)\s+\d+\s+'+name+r'$',text,re.M);assert m,name
    return int(m[1],16)
def memory(g,at,size):
    raw=g.qmp('human-monitor-command',{'command-line':f'xp /{(size+7)//8}gx 0x{at:x}'})
    values=[]
    for line in raw.splitlines():
        if ':' in line:values.extend(int(v,16) for v in re.findall('0x[0-9a-fA-F]+',line.split(':',1)[1]))
    return b''.join(struct.pack('<Q',v) for v in values)[:size]
def stats(g):return struct.unpack('<QQQQ',memory(g,address('stats'),32))

def main():
    work=q.new_work('m72-resources');out=ROOT/'evidence/M7.2/resources';out.mkdir(parents=True,exist_ok=True);report={'passed':False,'profiles':[]}
    try:
        disk=q.sparse_disk(work/'owned.img',2*1024**3)
        with q.Guest(work/'install',disk,image_name='EngineOS-M72') as g:
            h.next(g);h.next(g);h.next(g,4);h.next(g,2);h.next(g,5);h.next(g,1);g.wait('M6_INSTALL_PASS',timeout=240)
        for ram,cpus in [(64,1),(128,2),(128,4)]:
            with q.Guest(work/(str(ram)+'-'+str(cpus)),disk,media='installed',ram=ram,cpus=cpus,image_name='EngineOS-M72',ready='M7_DESKTOP_READY') as g:
                baseline=stats(g);pointer=s.s.Pointer(g)
                for _ in range(4):
                    pointer.click(260,325);pointer.click(260,360);pointer.click(260,398);pointer.click(260,250)
                after=stats(g);assert after[0]==baseline[0] and after[3]==baseline[3]
                time.sleep(.25);paints=memory(g,address('paints'),8);time.sleep(.6)
                assert paints==memory(g,address('paints'),8)
                assert g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')==cpus
                h.shot(g,out,str(ram)+'mib-'+str(cpus)+'cpu');(out/(str(ram)+'-'+str(cpus)+'.log')).write_bytes(g.log().encode())
                report['profiles'].append({'ram_mib':ram,'cpus':cpus,'managed_current_bytes':after[0]*4096,'managed_peak_including_boot_bytes':after[1]*4096,'folder_navigation_retained_page_delta':after[0]-baseline[0],'folder_navigation_allocation_failures':after[3]-baseline[3],'idle_all_cpus_halted':True,'idle_repaint_count':0})
        data=(ROOT/'dist/EngineOS-M72-installed-kernel.elf').read_bytes();offset=struct.unpack_from('<Q',data,32)[0];count=struct.unpack_from('<H',data,56)[0]
        loads=[struct.unpack_from('<IIQQQQQQ',data,offset+n*56) for n in range(count)];loads=[v for v in loads if v[0]==1]
        meta=json.loads((ROOT/'dist/m72-build-manifest.json').read_text());report.update(passed=True,build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M72.iso']['sha256'],kernel_physical_span_bytes=max(v[4]+v[6] for v in loads)-min(v[4] for v in loads),framebuffer_visible_bytes=1280*800*4,scope='allocator excludes static kernel, firmware/boot reservations and framebuffer; no physical-hardware or human usability claim')
    except Exception:report['error']=q.clean(traceback.format_exc());print(report['error'])
    (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M7.2 resources '+('PASS' if report['passed'] else 'FAIL'))
    if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
