"""Compile unchanged MIT BearSSL sources with explicit freestanding, portable backends."""
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import subprocess
from m1 import ROOT,pinned_tools,sha

DEFINES={
    'BR_64':1,'BR_i386':0,'BR_amd64':0,'BR_INT128':0,'BR_UMUL128':0,
    'BR_AES_X86NI':0,'BR_SSE2':0,'BR_POWER8':0,'BR_RDRAND':0,
    'BR_USE_URANDOM':0,'BR_USE_WIN32_RAND':0,'BR_USE_UNIX_TIME':0,'BR_USE_WIN32_TIME':0,
    'BR_MSC':0,
}


def objects(work,tools,flags,target='x86_64-unknown-none-elf'):
    work=Path(work);work.mkdir(parents=True,exist_ok=True)
    upstream=ROOT/'third_party/bearssl'
    includes=[ROOT/'third_party/bearssl-port',upstream/'inc',upstream/'src']
    sources=sorted((upstream/'src').rglob('*.c'))
    headers=sorted((upstream/'inc').rglob('*.h'))+sorted((upstream/'src').glob('*.h'))+sorted(includes[0].glob('*.h'))
    policy={name:sha(name) for name in headers}
    arguments=[*map(str,flags),'-ffunction-sections','-fdata-sections',
               *['-D'+name+'='+str(value) for name,value in DEFINES.items()],
               *[arg for path in includes for arg in ['-I',str(path)]]]
    # A content-bound object cache avoids compiling this unchanged dependency twice
    # for live/installed kernels. Project guards still bind the final native inputs.
    common=json.dumps({'compiler':sha(tools['clang']),'target':target,'flags':arguments,
                       'headers':{str(k.relative_to(ROOT)):v for k,v in policy.items()}},sort_keys=True)

    def compile_source(source):
        name=source.relative_to(upstream/'src').with_suffix('').as_posix().replace('/','_')
        obj=work/(name+'.obj' if 'windows' in target else name+'.o')
        digest=hashlib.sha256((common+sha(source)).encode()).hexdigest()
        marker=obj.with_suffix(obj.suffix+'.sha256')
        if not obj.exists() or not marker.exists() or marker.read_text(encoding='ascii')!=digest:
            result=subprocess.run([str(tools['clang']),'--target='+target,*arguments,'-c',str(source),'-o',str(obj)],
                                  cwd=ROOT,capture_output=True,text=True,timeout=60,
                                  creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
            if result.returncode:raise RuntimeError(result.stderr.replace(str(ROOT),'<project-root>'))
            marker.write_bytes(digest.encode('ascii'))
        return obj

    with ThreadPoolExecutor(max_workers=4) as workers:result=list(workers.map(compile_source,sources))
    return result


if __name__=='__main__':
    tools=pinned_tools()
    flags=['-std=c17','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin',
           '-fno-stack-protector','-fno-pic','-fno-pie','-mno-red-zone','-mgeneral-regs-only']
    result=objects(ROOT/'build/m7/bearssl-native',tools,flags)
    out=ROOT/'evidence/M7/bearssl-build-results.json'
    out.write_bytes((json.dumps({'passed':True,'objects':len(result),'upstream':'BearSSL 0.6',
                                'archive_sha256':json.loads((ROOT/'third_party/bearssl/provenance.json').read_text(encoding='utf-8'))['archive_sha256'],
                                'defines':DEFINES,'scope':'freestanding native object compilation only',
                                'native_https_qualified':False,'m7_complete':False},indent=2)+'\n').encode())
    print('Freestanding BearSSL compile PASS: '+str(len(result))+' portable objects; no HTTPS runtime claim')
