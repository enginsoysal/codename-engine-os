"""Reject incomplete PVSCSI completions in the owned QEMU implementation."""
import hashlib,json,struct,traceback
import ui
from controls import click,control
ROOT=ui.ROOT

def main():
 out=ROOT/'build/m92/negative-pvscsi';out.mkdir(parents=True,exist_ok=True);work=ui.q.new_work('m92-short-scsi');disk=ui.q.sparse_disk(work/'owned.img',2*1024**3);before=ui.q.digest(disk);meta=json.loads((ROOT/'dist/m92-build-manifest.json').read_text(encoding='utf-8'));r={'passed':False,'build_id':meta['build_id'],'iso_sha256':meta['artifacts']['EngineOS-M92.iso']['sha256']}
 try:
  with ui.guest(work/'boot',disk,controller='pvscsi') as g:
   assert ui.integer(g,'count')==0
   adapter=ui.memory(g,ui.symbol('adapters92')[0],128);reply=struct.unpack_from('<Q',adapter,48)[0];path=g.folder/'completion.bin';g.qmp('human-monitor-command',{'command-line':f'pmemsave 0x{reply:x} 0x20 "{path.as_posix()}"'});completion=path.read_bytes();context,length=struct.unpack_from('<QQ',completion);status=struct.unpack_from('<I',completion,20)[0];assert context==1 and length==0 and status==0
   p=ui.Pointer(g);click(g,p,2);assert not control(g,2);ui.shot(g,out,'no-false-installation');log=g.log();assert 'M6_INSTALL_PASS' not in log and 'KERNEL_PANIC' not in log;(out/'serial.log').write_bytes(log.encode())
  assert ui.q.digest(disk)==before
  r.update(passed=True,first_inquiry_completion={'context':context,'reported_transferred_bytes':length,'status':status},checks=['QEMU PVSCSI reports zero transferred bytes for a successful 36-byte inquiry; strict transfer-length validation refuses admission','No installer success or disk modification; unsupported condition remains navigable'],qualified_for_installation=False,device_implementation='QEMU PVSCSI; distinct from qualified VMware PVSCSI')
 except Exception:r['error']=ui.q.clean(traceback.format_exc());raise
 finally:(out/'results.json').write_bytes((json.dumps(r,indent=2)+'\n').encode());print('M92 short-completion refusal '+str(r['passed']),flush=True)
if __name__=='__main__':main()
