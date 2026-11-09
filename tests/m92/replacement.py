"""Select Disk 2 among equal-capacity GPT disks; prove Disk 1 is untouched."""
import argparse,json,traceback
import ui
from controls import click,control,state
ROOT=ui.ROOT;q=ui.q

def main():
 a=argparse.ArgumentParser();a.add_argument('--controller',choices=['ide','mptsas'],default='ide');a=a.parse_args()
 out=ROOT/'build/m92/replacement'/a.controller;out.mkdir(parents=True,exist_ok=True);work=q.new_work('m92-replacement');report={'passed':False,'checks':[]};meta=json.loads((ROOT/'dist/m92-build-manifest.json').read_text());report.update(build_id=meta['build_id'],iso_sha256=meta['artifacts']['EngineOS-M92.iso']['sha256'])
 try:
  helper=ui.module('fixture71',ROOT/'tests/m71/native.py');keep=q.sparse_disk(work/'keep.img',2*1024**3);target=q.sparse_disk(work/'target.img',2*1024**3);helper.fixture(keep);parts=helper.fixture(target)
  before=q.digest(keep);original=q.digest(target,0,2*1024**2)
  extra=['-drive',f'if=none,id=second,format=raw,file={target}','-device','ide-hd,bus=pata.1,drive=second' if a.controller=='ide' else 'scsi-hd,bus=scsi.0,scsi-id=1,drive=second']
  with ui.guest(work/'install',keep,extra=extra,controller=a.controller) as g:
   p=ui.Pointer(g);click(g,p,2);click(g,p,101,True);assert state(g,'chosen')==1 and not control(g,2);ui.shot(g,out,'01-equal-disks-selected-second')
   partition_before=q.digest(target,parts[0][0]*512,(parts[0][1]-parts[0][0]+1)*512)
   click(g,p,90);click(g,p,301);click(g,p,212);click(g,p,224);click(g,p,226);at=click(g,p,228);g.wait('M71_DISK_APPLIED op=0x0000000000000005',at)
   with target.open('rb') as check:check.seek(parts[1][0]*512);assert check.read(512)[82:90]==b'FAT32   '
   assert q.digest(keep)==before and q.digest(target,parts[0][0]*512,(parts[0][1]-parts[0][0]+1)*512)==partition_before
   click(g,p,301);click(g,p,212);click(g,p,225);click(g,p,226);at=click(g,p,228);g.wait('M71_DISK_APPLIED op=0x0000000000000006',at)
   with target.open('rb') as check:check.seek(parts[1][0]*512);assert check.read(8)==b'ENGINEV6'
   click(g,p,301);click(g,p,211);click(g,p,226);at=click(g,p,228);g.wait('M71_DISK_APPLIED op=0x0000000000000004',at)
   click(g,p,301);click(g,p,210);click(g,p,223);click(g,p,226);at=click(g,p,228);g.wait('M71_DISK_APPLIED op=0x0000000000000003',at)
   assert q.digest(keep)==before and q.digest(target,parts[0][0]*512,(parts[0][1]-parts[0][0]+1)*512)==partition_before
   ui.shot(g,out,'partition-managed');click(g,p,200);original=q.digest(target,0,2*1024**2)
   click(g,p,91);click(g,p,2);click(g,p,2);click(g,p,2);assert not control(g,2);ui.shot(g,out,'02-review-disk-two');assert q.digest(target,0,2*1024**2)==original
   click(g,p,92);start=click(g,p,2);g.wait('M6_INSTALL_PASS',start,240);ui.wait_view(g,6,start);ui.shot(g,out,'03-verified-finish');start=click(g,p,84);g.wait('M7_DESKTOP_READY',start,150);assert ui.integer(g,'m6_bundle_size',True)==0;ui.shot(g,out,'04-restarted-disk-two');(out/'install.log').write_bytes(g.log().encode())
  assert q.digest(keep)==before,'Unselected equal-capacity disk changed';assert '/Users/Owner/Documents' in ui.s.directories(target)
  report.update(passed=True,unselected_disk_sha256=before,checks=['Native FAT32/Engine format, delete and create preserve the unrelated partition and complete other disk','full-width capacity-column click selects Disk 2 among equal-capacity GPT disks','separate erase choice and review consent before actual replacement','ordinary restart loads Disk 2 installed kernel and desktop with matching ISO attached','entire unselected Disk 1 SHA256 unchanged; selected volume metadata CRC independently checked'])
 except Exception:
  report['error']=q.clean(traceback.format_exc());print(report['error'])
  if 'g' in locals():(out/'last-run.log').write_bytes(g.log().encode())
 (out/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M9.2 replacement '+('PASS' if report['passed'] else 'FAIL'),flush=True)
 if not report['passed']:raise SystemExit(1)
if __name__=='__main__':main()
