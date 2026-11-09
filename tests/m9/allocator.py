"""Actual C contiguous-allocation ownership/admission tests in a private host process."""
import ctypes as c,hashlib,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools/build'))
from m1 import pinned_tools,command

def main():
    work=ROOT/'build/m9/allocator';work.mkdir(parents=True,exist_ok=True);tools=pinned_tools()
    objects=[]
    for source in ['tests/m9/allocator_host.c','runtime/memory.c']:
        obj=work/(Path(source).stem+'.obj')
        command([tools['clang'],'--target=x86_64-pc-windows-msvc','-std=c17','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin','-fno-stack-protector','-I',ROOT/'include','-c',ROOT/source,'-o',obj]);objects.append(obj)
    command([tools['lld-link'],'/dll','/noentry','/nodefaultlib','/timestamp:0','/export:page_alloc','/export:pages_alloc_run','/export:page_free','/export:pages_limit','/export:pages_stats','/out:'+str(work/'allocator.dll'),*objects])
    lib=c.CDLL(str(work/'allocator.dll'))
    class Stats(c.Structure):_fields_=[('current',c.c_uint64),('peak',c.c_uint64),('total',c.c_uint64),('failures',c.c_uint64)]
    lib.reset9.argtypes=[c.c_uint64,c.c_uint];lib.pages_alloc_run.argtypes=[c.c_uint];lib.pages_alloc_run.restype=c.c_uint64
    lib.page_alloc.restype=c.c_uint64;lib.page_free.argtypes=[c.c_uint64];lib.pages_limit.argtypes=[c.c_uint64];lib.pages_stats.restype=Stats
    api=c.WinDLL('kernel32',use_last_error=True);api.VirtualAlloc.argtypes=[c.c_void_p,c.c_size_t,c.c_uint,c.c_uint];api.VirtualAlloc.restype=c.c_void_p
    api.VirtualFree.argtypes=[c.c_void_p,c.c_size_t,c.c_uint]
    base=next((value for value in range(0x18000000,0x30000000,0x1000000) if api.VirtualAlloc(value,1024**2,0x3000,4)),None)
    assert base is not None,'Could not reserve a private lower-address test region'
    checks=[]
    try:
        lib.reset9(base,256);p=lib.pages_alloc_run(3);assert p==base
        data=(c.c_ubyte*(3*4096)).from_address(p);assert bytes(data)==bytes(len(data));data[0]=165
        q=lib.pages_alloc_run(2);assert q==base+3*4096
        lib.pages_limit(5);assert lib.pages_alloc_run(1)==0 and lib.pages_stats().current==5 and data[0]==165
        lib.pages_limit(0)
        for n in range(3):lib.page_free(p+n*4096)
        assert lib.pages_alloc_run(2)==base and bytes((c.c_ubyte*8192).from_address(base))==bytes(8192)
        for n in range(2):lib.page_free(base+n*4096);lib.page_free(q+n*4096)
        assert lib.pages_stats().current==0
        checks.append('Contiguous ownership, adjacent allocations, admission refusal, reclamation and zeroed reuse')
        lib.reset9(base,32);owned=[lib.page_alloc() for _ in range(32)];assert len(set(owned))==32
        for page in owned[::2]:lib.page_free(page)
        assert lib.pages_alloc_run(2)==0 and lib.pages_stats().current==16
        for page in owned[1::2]:lib.page_free(page)
        assert lib.pages_stats().current==0
        checks.append('Fragmented free pages never become a false contiguous run')
        lib.reset9(base,16);lib.pages_limit(5)
        assert not lib.pages_alloc_run(6) and not lib.pages_stats().current
        assert not lib.pages_alloc_run(0) and not lib.pages_alloc_run(8193)
        for iteration in range(32):
            p=lib.pages_alloc_run(4);assert p==base
            for n in range(4):lib.page_free(p+n*4096)
            assert lib.pages_stats().current==0
        checks.append('Bounds, whole-request admission and repeated no-leak release')
    finally:assert api.VirtualFree(base,0,0x8000)
    sources=['kernel/memory/pages.c','runtime/memory.c','tests/m9/allocator_host.c','tests/m9/allocator.py']
    report={'passed':True,'scope':'actual C allocator host execution; separate from native graphics/device acceptance','checks':checks,'source_sha256':{name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in sources}}
    (work/'results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M9 allocator PASS: ownership, fragmentation, admission and 32 reuse cycles')

if __name__=='__main__':main()
