"""Real native M6 wizard installation and detached boot on owned QEMU disks."""
import argparse,importlib.util,json,time,traceback,uuid
from pathlib import Path
spec=importlib.util.spec_from_file_location('m6guest',Path(__file__).with_name('vm.py'));q=importlib.util.module_from_spec(spec);spec.loader.exec_module(q)
ROOT=q.ROOT
def key(g,name,n=1):
    for _ in range(n):g.qmp('send-key',{'keys':[{'type':'qcode','data':name}],'hold-time':30});time.sleep(.12)
def next(g,tabs=0):key(g,'tab',tabs);key(g,'ret')
def shot(g,out,name):
    from PIL import Image
    time.sleep(.2);p=g.folder/(name+'.ppm');g.qmp('screendump',{'filename':str(p)})
    with Image.open(p) as image:image.save(out/(name+'.png'))
def main():
    a=argparse.ArgumentParser();a.add_argument('--firmware',choices=['uefi','bios'],default='uefi');a.add_argument('--media',choices=['iso','usb'],default='iso');args=a.parse_args();work=ROOT/'build/m6/guest'/uuid.uuid4().hex;work.mkdir(parents=True);out=ROOT/'evidence/M6'/('qemu-'+args.firmware+('-usb' if args.media=='usb' else ''));out.mkdir(parents=True,exist_ok=True);checks=[];result={'passed':False,'checks':checks,'firmware':args.firmware,'media':args.media}
    try:
        disk=work/'owned.img'
        with disk.open('xb') as f:f.truncate(2*1024**3)
        with q.Guest(work/'live',disk,args.firmware,args.media) as g:
            shot(g,out,'welcome');next(g);next(g);next(g,2);shot(g,out,'components');next(g,2);shot(g,out,'personalize');next(g,5);shot(g,out,'review');start=len(g.log());next(g,1);g.wait('M6_INSTALL_PASS',start,240);shot(g,out,'complete');(out/'live.log').write_bytes(g.log().encode())
        checks.append({'name':'native graphical wizard, actual disk install and verified Finish','passed':True})
        for reboot in range(2):
            with q.Guest(work/('installed-'+str(reboot)),disk,args.firmware,media='installed') as g:
                assert 'M6_INSTALLED_VOLUME' in g.log();shot(g,out,'installed-'+str(reboot));next(g,3);start=len(g.log());next(g,0 if reboot==0 else 1);g.wait('M6_LARGE_FILE_PASS',start,120);(out/('installed-'+str(reboot)+'.log')).write_bytes(g.log().encode())
        checks.append({'name':'two media-detached boots, persisted user setup and real >2 MiB file checksum after reboot','passed':True})
        result['passed']=True
        manifest=json.loads((ROOT/'dist/m6-build-manifest.json').read_text(encoding='utf-8'));result.update(build_id=manifest['build_id'],iso_sha256=manifest['artifacts']['EngineOS-M6.iso']['sha256'],m6_acceptance_complete=False)
    except Exception:
        result['error']=traceback.format_exc().replace(str(ROOT),'<project-root>');print(result['error'])
    (out/'results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print('M6 native guest '+('PASS' if result['passed'] else 'FAIL'),flush=True)
    if not result['passed']:raise SystemExit(1)
if __name__=='__main__':main()
