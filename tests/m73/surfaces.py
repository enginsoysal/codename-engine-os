"""Review native recovery, partition actions and destructive-operation guards."""
import argparse
import json
import traceback
import ui
q,h=ui.q,ui.h
n=ui.module('disk_fixture71',ui.ROOT/'tests/m71/native.py')
def main():
 p=argparse.ArgumentParser();p.add_argument('--resolution',default='1280x800');a=p.parse_args();resolution=tuple(map(int,a.resolution.split('x')))
 work=q.new_work('m73-surfaces');out=ui.ROOT/'evidence/M7.3'/('surfaces-'+a.resolution);out.mkdir(parents=True,exist_ok=True);report={'passed':False}
 try:
  disk=q.sparse_disk(work/'owned.img',2*1024**3);parts=n.fixture(disk);before=q.digest(disk);keep=q.digest(disk,parts[0][0]*512,(parts[0][1]-parts[0][0]+1)*512)
  with ui.guest(work/'utility',disk,resolution) as g:
   point=ui.Pointer(g);point.wizard(50);ui.shot(g,out,'01-recovery');point.wizard(62);point.wizard(83);ui.shot(g,out,'02-no-compatible-install');point.wizard(2);point.wizard(100);point.wizard(2);ui.shot(g,out,'03-nonblank-guidance')
   point.wizard(90);ui.shot(g,out,'04-disk-manager');point.wizard(301);point.wizard(212);ui.shot(g,out,'05-format-choice');point.wizard(224);ui.shot(g,out,'06-format-review');point.wizard(228);ui.shot(g,out,'07-unchecked-refused');assert q.digest(disk)==before
   point.wizard(226);start=point.wizard(228);g.wait('M71_DISK_APPLIED op=0x0000000000000005',start);ui.shot(g,out,'08-formatted');assert q.digest(disk,parts[0][0]*512,(parts[0][1]-parts[0][0]+1)*512)==keep
   with disk.open('rb') as f:f.seek(parts[1][0]*512);assert f.read(512)[82:90]==b'FAT32   '
   point.wizard(302);point.wizard(210);ui.shot(g,out,'09-create-partition');point.wizard(223);ui.shot(g,out,'10-create-review');point.wizard(227)
   point.wizard(215);ui.shot(g,out,'11-gpt-repair');point.wizard(200);point.wizard(200);point.wizard(91);erase_before=q.digest(disk);point.wizard(2);point.wizard(2);point.wizard(2);ui.shot(g,out,'12-erase-review');point.wizard(2);ui.shot(g,out,'13-whole-disk-consent-refused')
   assert q.digest(disk)==erase_before
   assert 'M6_INSTALL_PASS' not in g.log() and 'M73_TEXT_OVERFLOW' not in g.log();(out/'native.log').write_bytes(g.log().encode())
  meta=json.loads((ui.ROOT/'dist/m73-build-manifest.json').read_text());report.update(passed=True,build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M73.iso']['sha256'],checks=['recovery, missing installation and nonblank guidance captured','native format refuses unchecked consent without changing a single disk byte','checked FAT32 format writes real volume and preserves unrelated partition SHA256','create and repair screens; creation cancel','whole-disk replacement refuses missing consent','no measured paragraph overflow'])
 except Exception:
  report['error']=q.clean(traceback.format_exc());print(report['error'])
  if 'g' in locals():(out/'last-run.log').write_bytes(g.log().encode())
 (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M7.3 surfaces '+a.resolution+' '+str(report['passed']),flush=True)
 if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
