"""The optional native terminal operates real persistent files and preserves core settings."""
import importlib.util,json,subprocess,time,traceback
from pathlib import Path
spec=importlib.util.spec_from_file_location('m6failures',Path(__file__).with_name('failures.py'));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
h=m.h;q=m.q;ROOT=q.ROOT

def open_terminal(g):
    h.next(g,3);h.next(g,2);g.wait('M6_TERMINAL_READY')

def main():
    work=q.new_work('terminal');out=ROOT/'evidence/M6/terminal';out.mkdir(parents=True,exist_ok=True);result={'passed':False}
    try:
        disk=work/'owned.img';q.sparse_disk(disk,2*1024**3)
        with q.Guest(work/'live',disk) as g:pos=m.install(g);g.wait('M6_INSTALL_PASS',pos,240)
        with q.Guest(work/'installed',disk,media='installed') as g:
            open_terminal(g);assert 'Write status: 0' in g.command('write /Users/Owner/Documents/terminal.txt persistent');assert 'persistent' in g.command('cat /Users/Owner/Documents/terminal.txt');assert 'Remove status: 7' in g.command('rm /System/Settings/setup.bin');assert 'Command too long. No command was executed.' in g.command('a'*260);[h.key(g,c) for c in 'menu'];h.key(g,'ret');h.next(g,1);h.shot(g,out,'returned-to-installed-screen')
            (out/'write.log').write_bytes(g.log().encode())
        with q.Guest(work/'reboot',disk,media='installed') as g:
            assert 'M6_INSTALLED_VOLUME' in g.log();open_terminal(g);assert 'persistent' in g.command('cat /Users/Owner/Documents/terminal.txt');assert 'Remove status: 0' in g.command('rm /Users/Owner/Documents/terminal.txt');(out/'reboot.log').write_bytes(g.log().encode())
        meta=json.loads((ROOT/'dist/m6-build-manifest.json').read_text(encoding='utf-8'));result.update(passed=True,build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M6.iso']['sha256'],checks=[{'name':'optional native terminal write/read survives reboot, protects core settings, rejects overlong commands and removes only the selected user file','passed':True}])
    except Exception:result['error']=q.clean(traceback.format_exc());print(result['error'])
    (out/'results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print('M6 terminal '+('PASS' if result['passed'] else 'FAIL'),flush=True)
    if not result['passed']:raise SystemExit(1)
if __name__=='__main__':main()
