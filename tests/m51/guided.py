"""Task-driven guided setup: only displayed choices, real installs and durable files."""
import importlib.util,json,time,traceback,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def module(name,path):
    s=importlib.util.spec_from_file_location(name,ROOT/path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
q=module('m51_guest','tests/m5/vm.py')

def main():
    import argparse
    p=argparse.ArgumentParser();p.add_argument('--firmware',choices=['uefi','bios'],default='uefi');p.add_argument('--custom',action='store_true');p.add_argument('--usb',action='store_true');p.add_argument('--fault',action='store_true');a=p.parse_args()
    name=('forge' if a.custom else a.firmware+('-usb' if a.usb else '-iso'))+('-fault' if a.fault else '');work=ROOT/'build/m51/tests'/name/str(time.time_ns());work.mkdir(parents=True);out=ROOT/'evidence/M5.1'/name;out.mkdir(parents=True,exist_ok=True);disk=work/'target.img'
    with disk.open('xb') as f:f.truncate(256*1024*1024)
    before=q.digest(disk);checks=[];errors=[];manifest=json.loads((ROOT/'dist/m5-build-manifest.json').read_text(encoding='utf-8'));profile=json.loads((ROOT/'build/m5/distro.json').read_text(encoding='utf-8'));fault={'event':'write_aio','iotype':'write','errno':5,'sector':8192,'once':False} if a.fault else None
    def choice(g,v,expected='Choice> '):return g.command(v,expected,180)
    def shot(g,name):
        from PIL import Image
        p=work/(name+'.ppm');g.qmp('screendump',{'filename':str(p)})
        with Image.open(p) as image:image.save(out/(name+'.png'))
    try:
        with q.Guest(work/'live',disk,a.firmware,'usb' if a.usb else 'iso',ram=64 if a.usb else 128,fault=fault) as g:
            g.wait('M51_HOME_READY');shot(g,'welcome');assert 'No terminal commands are needed.' in g.log() if not a.custom else 'community edition' in g.log()
            choice(g,'n');assert 'Choose one of the displayed numbers' in g.log();choice(g,'1');choice(g,'1');assert 'Sample note saved' in g.log();choice(g,'2');assert 'My first Engine OS note.' in g.log();choice(g,'0');checks.append({'name':'welcome, invalid input and usable Try file preview without commands','passed':True})
            choice(g,'2');choice(g,'1');assert 'Review installation' in g.log();shot(g,'review');choice(g,'0');assert before==q.digest(disk);checks.append({'name':'review/back cancel leaves complete disk unchanged','passed':True})
            if a.custom:
                choice(g,'6');assert profile['name'] in g.log();choice(g,'0');choice(g,'4','engine> ');text=choice(g,'cat /Users/test/Documents/Developer.txt','engine> ');assert 'developer notes selected' in text
                checks.append({'name':'custom branding and selected data package are present in the actual kernel filesystem','passed':True});choice(g,'menu');choice(g,'2');choice(g,'1');choice(g,'1','M51_GUIDED_COMPLETE')
            else:
                choice(g,'2');choice(g,'1');choice(g,'1','M51_ACTION_REFUSED_OR_FAILED' if a.fault else 'M51_GUIDED_COMPLETE')
                if a.fault:
                    assert 'NOT completed successfully' in g.log();assert 'M5_INSTALL_PASS' not in g.log();choice(g,'D');assert 'Last operation code: 3' in g.log();checks.append({'name':'real copy EIO shows actionable error and optional technical details','passed':True})
                else:
                    assert 'Disconnect the ISO/CD' in g.log();checks.append(q.inspect_disk(disk,manifest));regs=g.qmp('human-monitor-command',{'command-line':'info registers -a'});assert regs.count('HLT=1')==2;checks.append({'name':'menu-driven install, independent disk inspection and idle CPUs halted','passed':True})
            ppm=work/'screen.ppm';g.qmp('screendump',{'filename':str(ppm)})
            from PIL import Image
            with Image.open(ppm) as frame:
                frame.save(out/'screen.png');assert frame.getpixel((0,0))==tuple(bytes.fromhex(profile['accent'][1:]))
            (out/'live.log').write_bytes(g.log().encode())
        if a.custom:
            with q.Guest(work/'custom-installed',disk,media='installed') as g:
                g.wait('M51_HOME_READY');choice(g,'4','engine> ');assert 'developer notes selected' in choice(g,'cat /Users/test/Documents/Developer.txt','engine> ');(out/'installed.log').write_bytes(g.log().encode())
            checks.append({'name':'custom ISO installs and selected package survives media-detached boot','passed':True})
        if not a.custom and not a.fault:
            for i in range(2):
                with q.Guest(work/f'installed-{i}',disk,a.firmware,'installed') as g:
                    g.wait('M51_HOME_READY');choice(g,'1');assert 'installed files are persistent' in g.log();choice(g,'1' if i==0 else '2');assert 'Sample note saved' in g.log() if i==0 else 'My first Engine OS note.' in g.log();(out/f'installed-{i}.log').write_bytes(g.log().encode())
            saved=q.digest(disk,139264*512,16*1024*1024)
            with disk.open('r+b') as f:f.seek(64*512);f.write(b'BROKEN-BOOT'+bytes(502))
            with q.Guest(work/'repair',disk,a.firmware) as g:
                g.wait('M51_HOME_READY');choice(g,'3');choice(g,'1');assert 'Review boot repair' in g.log();choice(g,'1','M51_GUIDED_COMPLETE');(out/'repair.log').write_bytes(g.log().encode())
            assert saved==q.digest(disk,139264*512,16*1024*1024);checks.append({'name':'two detached boots, persistent sample and menu repair preserving whole data partition','passed':True})
    except Exception:errors.append(traceback.format_exc().replace(str(ROOT),'<project-root>'));print(errors[-1],flush=True)
    result={'passed':not errors,'build_id':manifest['build_id'],'profile':profile['id'],'iso_sha256':manifest['artifacts']['EngineOS-M5.iso']['sha256'],'checks':checks,'errors':errors};(out/'results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print(name,'PASS' if not errors else 'FAIL',flush=True)
    if errors:raise SystemExit(1)
if __name__=='__main__':main()
