"""A real large-framebuffer guest refuses the optional buffer and keeps its wizard."""
import hashlib,json,struct,time
from pathlib import Path
import ui

ROOT=Path(__file__).resolve().parents[2]

def main():
    work=ui.q.new_work('m9-fallback');disk=ui.q.sparse_disk(work/'owned.img',2*1024**3)
    out=ROOT/'build/m9/fallback';out.mkdir(parents=True,exist_ok=True)
    meta=json.loads((ROOT/'dist/m9-build-manifest.json').read_text())
    report={'passed':False,'build_id':meta['build_id'],'iso_sha256':meta['artifacts']['EngineOS-M9.iso']['sha256']}
    try:
        with ui.guest(work/'boot',disk,(4096,2160),ram=128) as g:
            assert g.qmp('query-memory-size-summary')['base-memory']==128*1024**2
            values=struct.unpack('<8Q4I',ui.memory(g,ui.symbol('graphics9',80)[0],80))
            assert values[0]==0 and values[10]==0,'Optional buffer was admitted despite the 32-MiB cap'
            assert ui.integer(g,'step')==0
            deadline=time.monotonic()+60
            while g.qmp('human-monitor-command',{'command-line':'info registers -a'}).count('HLT=1')!=2:
                if time.monotonic()>deadline:raise TimeoutError('Fallback wizard did not become idle')
                time.sleep(.05)
            g.qmp('stop')
            try:ui.raw_shot(g,out,'fallback-welcome')
            finally:g.qmp('cont')
            (out/'serial.log').write_bytes(g.log().encode())
            report.update(passed=True,ram_mib=128,actual_framebuffer=[4096,2160],shadow_bytes=0,backend=0,idle_processors_halted=True,
                          scope='Native wizard boot only; not application or installation qualification for this display profile')
    finally:
        assert hashlib.sha256((ROOT/'dist/EngineOS-M9.iso').read_bytes()).hexdigest()==report['iso_sha256']
        (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print('M9 optional-buffer fallback PASS')

if __name__=='__main__':main()
