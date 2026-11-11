"""Close M11.2 only from matching candidate source, native tasks and fault evidence."""
import hashlib,json,os,re,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'evidence/M11/2'
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def load(path):return json.loads(path.read_text(encoding='utf-8'))
def main():
 meta=load(ROOT/'dist/m11stage2-build-manifest.json')
 native=load(ROOT/'build/m11/files/results.json')
 preferences=load(ROOT/'build/m11/files/preferences-results.json')
 pointer=load(ROOT/'build/m11/input-host/results.json')
 volume=load(ROOT/'build/m11/volume-host/results.json')
 assert native['passed'] and preferences['passed'] and pointer['passed'] and volume['passed']
 assert preferences['iso_sha256']==native['iso_sha256']
 assert meta['milestone']=='M11' and meta['build_revision']=='stage2'
 assert native['iso_sha256']==meta['artifacts']['EngacerOS-M11STAGE2.iso']['sha256']
 for name,expected in meta['source_sha256'].items():assert digest(ROOT/name)==expected,('Changed candidate source',name)
 for name,expected in meta['artifacts'].items():assert digest(ROOT/'dist'/name)==expected['sha256'],('Changed artifact',name)
 for report in (pointer,volume):
  for name,expected in report['source_sha256'].items():assert digest(ROOT/name)==expected,('Changed host input',name)
 sizes={};maps={}
 for stage in ('STAGE1','STAGE2'):
  path=ROOT/('dist/EngacerOS-M11'+stage+'-installed-kernel.map');text=path.read_text()
  match=re.search(r'^\s*[0-9a-f]+\s+[0-9a-f]+\s+([0-9a-f]+)\s+\d+\s+.*kernel_desktop_session-kernel.o:\(\.bss\)$',text,re.M);assert match
  sizes[stage]=int(match[1],16);maps[stage]=digest(path)
 delta=sizes['STAGE2']-sizes['STAGE1'];assert 0<delta<512*1024
 resources={'passed':True,'scope':'Exact installed desktop object BSS delta; separate from managed pages, framebuffer and configured RAM','baseline_bytes':sizes['STAGE1'],'candidate_bytes':sizes['STAGE2'],'added_bytes':delta,'budget_bytes':512*1024,'maps':maps}
 owned=Path(os.environ['LOCALAPPDATA'])/'EngineOS/Tests/M6/m11-files'
 latest=max(owned.iterdir(),key=lambda p:p.stat().st_mtime_ns)
 OUT.mkdir(parents=True,exist_ok=True);(OUT/'native').mkdir(exist_ok=True)
 for folder in ('install','detached','preferences-first','preferences-reboot'):
  text=(latest/folder/'serial.log').read_text(encoding='utf-8',errors='replace')
  assert 'M7_DESKTOP_READY build='+meta['build_id'] in text
  assert 'KERNEL_PANIC' not in text and 'KERNEL_REJECT:' not in text
  (OUT/'native'/(folder+'.log')).write_bytes(text.encode())
 for path in (ROOT/'build/m11/files').glob('*.png'):
  shutil.copyfile(path,OUT/'native'/path.name)
  capture=path.with_name(path.stem+'-capture.json')
  if capture.exists():shutil.copyfile(capture,OUT/'native'/capture.name)
 for name,value in [('candidate-manifest.json',meta),('native-results.json',native),('preferences-results.json',preferences),('pointer-host-results.json',pointer),('volume-host-results.json',volume),('resources.json',resources)]:
  (OUT/name).write_bytes((json.dumps(value,indent=2)+'\n').encode())
 report={'passed':True,'stage':'M11.2','scope':'Native Files stage; not final M11 media qualification','build_id':meta['build_id'],'iso_sha256':native['iso_sha256'],'source_inputs_verified':len(meta['source_sha256']),'artifacts_verified':len(meta['artifacts']),'native_tasks':native['checks']+preferences['checks'],'storage_fault_cases':volume['fault_cases'],'added_desktop_bss_bytes':delta,'independent_stored_bytes_and_detached_boot':True,'publication':'Separate public website gate before M11.3'}
 (OUT/'acceptance-results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M11.2 matching-source/native acceptance PASS')
if __name__=='__main__':main()
