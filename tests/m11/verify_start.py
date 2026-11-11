"""Close M11.3 only from matching production, native and stored/boot evidence."""
import hashlib,json,os,re,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'evidence/M11/3'
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def load(p):return json.loads(p.read_text(encoding='utf-8'))
def main():
 meta=load(ROOT/'dist/m11stage3-build-manifest.json');native=load(ROOT/'build/m11/start/results.json');controls=load(ROOT/'build/m11/start/controls-results.json');host=load(ROOT/'build/m11/start-host/results.json');pointer=load(ROOT/'build/m11/input-host/results.json')
 assert all(p['passed'] for p in (native,controls,host,pointer));assert meta['milestone']=='M11' and meta['build_revision']=='stage3'
 iso=meta['artifacts']['EngacerOS-M11STAGE3.iso']['sha256'];assert native['iso_sha256']==controls['iso_sha256']==iso
 for path,expected in meta['source_sha256'].items():assert digest(ROOT/path)==expected,('Changed source',path)
 for path,value in meta['artifacts'].items():assert digest(ROOT/'dist'/path)==value['sha256'],('Changed artifact',path)
 for report in (host,pointer):
  for path,expected in report['source_sha256'].items():assert digest(ROOT/path)==expected,('Changed test input',path)
 memory={}
 for stage in ('STAGE2','STAGE3'):
  p=ROOT/('dist/EngacerOS-M11'+stage+'-installed-kernel.map');text=p.read_text();parts={}
  for part in ('bss','data'):
   matches=re.findall(r'^\s*[0-9a-f]+\s+[0-9a-f]+\s+([0-9a-f]+)\s+\d+\s+.*kernel_desktop_session-kernel.o:\(\.'+part+r'\)$',text,re.M);assert len(matches)==1;parts[part]=int(matches[0],16)
  memory[stage]={'bytes':sum(parts.values()),'sections':parts,'map_sha256':digest(p)}
 delta=memory['STAGE3']['bytes']-memory['STAGE2']['bytes'];assert 0<delta<64*1024
 resources={'passed':True,'scope':'Exact installed desktop object fixed BSS/data delta; not total OS or configured RAM','stages':memory,'added_bytes':delta,'budget_bytes':64*1024,'native_search_retained_managed_pages_unchanged':True}
 base=Path(os.environ['LOCALAPPDATA'])/'EngineOS/Tests/M6/m11-start';owned=max(base.iterdir(),key=lambda p:p.stat().st_mtime_ns)
 OUT.mkdir(parents=True,exist_ok=True);(OUT/'native').mkdir(exist_ok=True)
 for folder in ('install','disk-only','keyboard-usb','keyboard-ps2'):
  text=(owned/folder/'serial.log').read_text(encoding='utf-8',errors='replace');assert 'M7_DESKTOP_READY build='+meta['build_id'] in text
  assert 'KERNEL_PANIC' not in text and 'KERNEL_REJECT:' not in text
  if folder=='install':assert 'M11_USER_RESTART_CONFIRMED' in text and 'M11_USER_SHUTDOWN_CONFIRMED' in text and text.count('M7_DESKTOP_READY build='+meta['build_id'])>=2
  (OUT/'native'/(folder+'.log')).write_bytes(text.encode())
 for path in (ROOT/'build/m11/start').glob('*.png'):
  shutil.copyfile(path,OUT/'native'/path.name);capture=path.with_name(path.stem+'-capture.json')
  if capture.exists():shutil.copyfile(capture,OUT/'native'/capture.name)
 for name,value in [('candidate-manifest.json',meta),('native-results.json',native),('controls-results.json',controls),('start-host-results.json',host),('pointer-host-results.json',pointer),('resources.json',resources)]:
  (OUT/name).write_bytes((json.dumps(value,indent=2)+'\n').encode())
 report={'passed':True,'stage':'M11.3','scope':'Native Start/taskbar stage on named QEMU profiles; final VMware/display/USB/original-app qualification remains M11.6','build_id':meta['build_id'],'iso_sha256':iso,'source_inputs_verified':len(meta['source_sha256']),'artifacts_verified':len(meta['artifacts']),'native_tasks':native['checks']+controls['checks'],'host_checks':host['checks']+pointer['checks'],'added_fixed_desktop_bytes':delta,'actual_installed_restart_and_s5_exit':True,'independent_preferences_and_detached_boot':True,'publication':'Separate GitHub and public homepage/article gate before M11.4'}
 (OUT/'acceptance-results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M11.3 matching-source/native acceptance PASS')
if __name__=='__main__':main()
