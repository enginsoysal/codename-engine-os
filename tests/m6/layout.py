"""Execute the actual freestanding install planner; never open block devices."""
import ctypes as c
import hashlib,json,random,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/build'))
from m1 import pinned_tools
class Partition(c.Structure):
    _fields_=[('first',c.c_uint64),('last',c.c_uint64)]
class Layout(c.Structure):
    _fields_=[('version',c.c_uint32),('sector_bytes',c.c_uint32),('sectors',c.c_uint64),('backup_table',c.c_uint64),('backup_header',c.c_uint64),*[(n,Partition) for n in ('bios','esp','system_a','system_b','data')],('reserved_sectors',c.c_uint64),('partition_sectors',c.c_uint64)]
def main():
    work=ROOT/'build/m6/layout';work.mkdir(parents=True,exist_ok=True)
    tools=pinned_tools();source=ROOT/'kernel/install/layout.c'
    def run(args):
        p=subprocess.run([str(a) for a in args],cwd=ROOT,capture_output=True,text=True)
        if p.returncode:raise RuntimeError(p.stderr.replace(str(ROOT),'<project-root>'))
    flags=['-std=c17','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin','-fno-stack-protector','-I',ROOT/'include']
    run([tools['clang'],'--target=x86_64-unknown-none-elf',*flags,'-mno-red-zone','-c',source,'-o',work/'layout.o'])
    run([tools['clang'],'--target=x86_64-pc-windows-msvc',*flags,'-c',source,'-o',work/'layout.obj'])
    run([tools['lld-link'],'/dll','/noentry','/nodefaultlib','/export:engine_install_plan',work/'layout.obj','/out:'+str(work/'layout.dll')])
    dll=c.CDLL(str(work/'layout.dll'));plan=dll.engine_install_plan
    plan.argtypes=[c.c_uint64,c.c_uint32,c.POINTER(Layout)];plan.restype=c.c_uint
    checks=[];rng=random.Random(6001);minimum=2*1024**3//512;maximum=2*1024**4//512
    sizes=[minimum,minimum+1,20*1024**3//512,maximum-1,maximum]+[rng.randrange(minimum,maximum+1) for _ in range(10000)]
    for sectors in sizes:
        p=Layout();assert plan(sectors,512,c.byref(p))==0
        assert (p.version,p.sector_bytes,p.sectors)==(1,512,sectors)
        assert p.backup_table==sectors-33 and p.backup_header==sectors-1
        end=33;total=0
        for n in ('bios','esp','system_a','system_b','data'):
            part=getattr(p,n);assert part.first>end and part.first%2048==0
            assert part.last>=part.first and (part.last+1)%2048==0
            total+=part.last-part.first+1;end=part.last
        assert end<p.backup_table and p.backup_table-end<=2048
        assert p.partition_sectors==total and total+p.reserved_sectors==sectors
        assert p.data.last-p.data.first+1>=512*2048
        assert p.reserved_sectors<=2048+2047+33
    checks.append({'name':'actual C geometry: exact accounting, alignment, no overlap, GPT bounds','passed':True,'disk_cases':len(sizes)})
    for sectors,sector_bytes,status in [(0,512,3),(minimum-1,512,3),(maximum+1,512,2),(2**64-1,512,2),(minimum,4096,2),(minimum,0,2)]:
        p=Layout();c.memset(c.byref(p),0xa5,c.sizeof(p));before=bytes(p)
        assert plan(sectors,sector_bytes,c.byref(p))==status and bytes(p)==before
    assert plan(minimum,512,None)==1
    checks.append({'name':'undersize, overflow/unsupported capacity, sector size and null output refusals','passed':True,'output_unchanged_on_refusal':True})
    result={'passed':True,'scope':'M6 initial planner host execution and freestanding compilation only','runtime_installation_qualified':False,'iso_released':False,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'checks':checks}
    out=ROOT/'evidence/M6';out.mkdir(parents=True,exist_ok=True)
    (out/'layout-results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode())
    print('M6 planner PASS: '+str(len(sizes))+' disk geometries; refusal/output checks; kernel target compiles')
if __name__=='__main__':main()
