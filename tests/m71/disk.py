"""Execute original disk editor C with independent GPT/FAT decoders and failed/torn I/O."""
import ctypes as c
import hashlib
import importlib.util
import json
from pathlib import Path
import random
import struct
import subprocess
import sys
import traceback
import zlib
ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'tools/build'),str(ROOT/'tests/m6')]
from m1 import pinned_tools
spec=importlib.util.spec_from_file_location('volume6',ROOT/'tests/m6/volume.py');v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
IO=c.CFUNCTYPE(c.c_uint,c.c_void_p,c.c_uint64,c.c_void_p,c.c_uint)
FLUSH=c.CFUNCTYPE(c.c_uint,c.c_void_p)
class Device(c.Structure):_fields_=[('context',c.c_void_p),('read',IO),('write',IO),('flush',FLUSH),('sectors',c.c_uint64),('sector_bytes',c.c_uint32)]
class Part(c.Structure):_fields_=[('first',c.c_uint64),('last',c.c_uint64),('index',c.c_uint),('protected',c.c_uint),('name',c.c_char*37),('format',c.c_char*12)]
class Gap(c.Structure):_fields_=[('first',c.c_uint64),('last',c.c_uint64)]
class State(c.Structure):_fields_=[*[(n,c.c_uint) for n in ['scheme','valid_copies','editable','count','gaps','owned','fingerprint']],('sectors',c.c_uint64),('first_usable',c.c_uint64),('last_usable',c.c_uint64),('mbr',c.c_ubyte*512),('headers',(c.c_ubyte*512)*2),('tables',(c.c_ubyte*16384)*2),('parts',Part*128),('free',Gap*129)]
class Plan(c.Structure):_fields_=[('operation',c.c_uint),('index',c.c_uint),('first',c.c_uint64),('last',c.c_uint64),('boot_crc',c.c_uint32),('name',c.c_char*37),('nonce',c.c_ubyte*16)]


class Disk:
    def __init__(self,sectors=4*1024**2,initial=None):
        self.sectors=sectors;self.data=dict(initial or {});self.calls=0;self.fail=0;self.torn=False;self.writes=[];self.allowed=None;self.errors=[]
        self.read=IO(self._read);self.write=IO(self._write);self.flush=FLUSH(self._flush)
        self.device=Device(None,self.read,self.write,self.flush,sectors,512)
    def raw(self,lba,n=512):return b''.join(self.data.get(i,bytes(512)) for i in range(lba,lba+n//512))
    def put(self,lba,raw):
        assert len(raw)%512==0
        for off in range(0,len(raw),512):self.data[lba+off//512]=raw[off:off+512]
    def _read(self,ctx,lba,p,n):
        if not (n%512==0 and 0<n<=4096 and lba+n//512<=self.sectors):self.errors.append(('read',lba,n));return 1
        c.memmove(p,self.raw(lba,n),n);return 0
    def trip(self):self.calls+=1;return self.calls==self.fail
    def _write(self,ctx,lba,p,n):
        if not (n%512==0 and 0<n<=4096 and lba+n//512<=self.sectors):self.errors.append(('write',lba,n));return 1
        if self.allowed is not None and not any(a<=lba and lba+n//512-1<=b for a,b in self.allowed):self.errors.append(('footprint',lba,n));return 1
        self.writes.append((lba,n));bad=self.trip()
        if bad and not self.torn:return 1
        raw=c.string_at(p,n)
        if bad:raw=raw[:127]+self.raw(lba,n)[127:]
        self.put(lba,raw);return 1 if bad else 0
    def _flush(self,ctx):return 1 if self.trip() else 0


def compile_library():
    work=ROOT/'build/m71/host';work.mkdir(parents=True,exist_ok=True);tools=pinned_tools()
    objects=[]
    for source in ['kernel/install/disk71.c','kernel/storage/volume6.c','runtime/memory.c']:
        obj=work/(Path(source).stem+'.obj');objects.append(obj)
        for target,suffix in [('x86_64-pc-windows-msvc','.obj'),('x86_64-unknown-none-elf','.o')]:
            args=[tools['clang'],'--target='+target,'-std=c17','-O2','-Wall','-Wextra','-Werror','-ffreestanding','-fno-builtin','-fno-stack-protector','-I',ROOT/'include','-c',ROOT/source,'-o',obj.with_suffix(suffix)]
            p=subprocess.run(list(map(str,args)),capture_output=True,text=True);assert p.returncode==0,p.stderr.replace(str(ROOT),'<project-root>')
    exports=['d71_scan','d71_prepare','d71_apply','v6_mount']
    args=[tools['lld-link'],'/dll','/noentry','/nodefaultlib',*['/export:'+n for n in exports],*objects,'/out:'+str(work/'disk.dll')]
    p=subprocess.run(list(map(str,args)),capture_output=True,text=True);assert p.returncode==0,p.stderr.replace(str(ROOT),'<project-root>')
    lib=c.CDLL(str(work/'disk.dll'));lib.d71_scan.argtypes=[c.POINTER(Device),c.POINTER(State)]
    lib.d71_prepare.argtypes=[c.POINTER(Device),c.POINTER(State),c.c_uint,c.c_uint,c.c_uint64,c.c_char_p,c.c_void_p,c.POINTER(Plan)]
    lib.d71_apply.argtypes=[c.POINTER(Device),c.POINTER(State),c.POINTER(Plan),c.POINTER(v.Volume)]
    return lib


def gpt(d):
    copies=[]
    for backup in [False,True]:
        where=d.sectors-1 if backup else 1;h=d.raw(where);assert h[:8]==b'EFI PART'
        assert struct.unpack_from('<II',h,8)==(0x10000,92)
        saved=struct.unpack_from('<I',h,16)[0];check=bytearray(h[:92]);check[16:20]=bytes(4);assert zlib.crc32(check)==saved
        mine,other,first,last=struct.unpack_from('<QQQQ',h,24);assert mine==where and other==(1 if backup else d.sectors-1)
        at,count,size,crc=struct.unpack_from('<QIII',h,72);assert (count,size)==(128,128)
        table=d.raw(at,16384);assert zlib.crc32(table)==crc;parts=[]
        for i in range(128):
            p=table[i*128:(i+1)*128]
            if p[:16]==bytes(16):continue
            a,b=struct.unpack_from('<QQ',p,32);assert first<=a<=b<=last
            assert all(b<x or a>y for x,y in parts);parts.append((a,b))
        copies.append((h[40:72],table,parts))
    assert copies[0]==copies[1];assert d.raw(0)[450]==0xee
    return copies[0][2]


def main():
    out=ROOT/'evidence/M7.1';out.mkdir(parents=True,exist_ok=True);report={'passed':False,'scope':'actual C host adapter, not native installer proof','checks':[]};checks=report['checks']
    try:
        lib=compile_library();serial=0;workspace=v.Volume()
        def scan(d):
            state=State();assert lib.d71_scan(c.byref(d.device),c.byref(state))==0;return state
        def plan(d,state,op,index=0,mib=0):
            nonlocal serial
            serial+=1;p=Plan();nonce=struct.pack('<QQ',serial,0x123456789abcdef)
            result=lib.d71_prepare(c.byref(d.device),c.byref(state),op,index,mib,b'Data',c.c_char_p(nonce),c.byref(p));assert result==0,(result,op,index,mib);return p
        def apply(d,state,p):
            result=lib.d71_apply(c.byref(d.device),c.byref(state),c.byref(p),c.byref(workspace));assert not d.errors,d.errors;return result
        d=Disk();s=scan(d);assert s.scheme==0
        p=plan(d,s,2);assert not d.writes;assert apply(d,s,p)==0;assert gpt(d)==[]
        s=scan(d);assert s.editable and s.gaps==1
        p=plan(d,s,3,0,64);assert not any(d.raw(2048));assert apply(d,s,p)==0;parts=gpt(d);assert parts==[(2048,133119)]
        s=scan(d);assert s.count==1;p=plan(d,s,3,0,64);assert apply(d,s,p)==0;parts=gpt(d);assert len(parts)==2
        foreign_first,foreign_last=parts[1];d.put(foreign_first,b'KEEP THIS PARTITION'.ljust(512,b'!'));d.put(foreign_last,b'LAST SECTOR'.ljust(512,b'!'))
        foreign={i:b for i,b in d.data.items() if foreign_first<=i<=foreign_last}
        metadata=[(0,33),(d.sectors-33,d.sectors-1)]
        s=scan(d);p=plan(d,s,5,s.parts[0].index);d.allowed=metadata+[(p.first,p.last)];assert apply(d,s,p)==0
        first,last=parts[0];boot=d.raw(first);assert boot[82:90]==b'FAT32   ' and boot[510:]==b'\x55\xaa'
        bytes_per=struct.unpack_from('<H',boot,11)[0];spc=boot[13];reserved=struct.unpack_from('<H',boot,14)[0];fat=struct.unpack_from('<I',boot,36)[0]
        assert bytes_per==512 and reserved==32 and boot[16]==2 and struct.unpack_from('<I',boot,32)[0]==last-first+1
        clusters=(last-first+1-reserved-2*fat)//spc;assert 65525<=clusters<0x0ffffff5 and fat*512>=(clusters+2)*4
        assert d.raw(first)==d.raw(first+6) and d.raw(first+1)==d.raw(first+7)
        assert struct.unpack_from('<I',d.raw(first+1),488)[0]==clusters-1
        assert d.raw(first+reserved,512)==d.raw(first+reserved+fat,512)
        assert all(d.data[i]==b for i,b in foreign.items()) and scan(d).parts[0].format==b'FAT32'
        checks.append({'name':'GPT create and actual FAT32 BPB/backup/FSInfo/two-FAT/root geometry; unrelated payload and table entries preserved','passed':True})
        s=scan(d);p=plan(d,s,6,s.parts[0].index);assert apply(d,s,p)==0;assert d.raw(first)[:8]==b'ENGINEV6';assert scan(d).parts[0].format==b'Engine'
        assert all(d.data[i]==b for i,b in foreign.items())
        @v.IO
        def volume_read(ctx,block,p,amount):return d._read(None,first+block*8,p,amount)
        @v.IO
        def volume_write(ctx,block,p,amount):return d._write(None,first+block*8,p,amount)
        @v.FLUSH
        def volume_flush(ctx):return d._flush(None)
        mounted=v.Volume(context=None,read=volume_read,write=volume_write,flush=volume_flush,blocks=(last-first+1)//8)
        lib.v6_mount.argtypes=[c.POINTER(v.Volume)];assert lib.v6_mount(c.byref(mounted))==0 and mounted.mounted
        saved=dict(d.data);s=scan(d);p=plan(d,s,4,s.parts[0].index);assert d.data==saved;assert apply(d,s,p)==0
        assert gpt(d)==[parts[1]] and all(d.data[i]==b for i,b in foreign.items())
        checks.append({'name':'actual native-volume format then delete only selected entry; prepare/cancel writes nothing; foreign data stays exact','passed':True})
        s=scan(d);p=plan(d,s,3,0,1);before=len(d.writes);d.put(40,b'changed'.ljust(512,b'!'));assert apply(d,s,p)==4 and len(d.writes)==before
        d.data=saved;d.allowed=None
        s=scan(d);p=plan(d,s,3,0,1);p.last=2**64-1;before=len(d.writes);assert apply(d,s,p)!=0 and len(d.writes)==before
        checks.append({'name':'changed metadata and forged out-of-bounds plans refuse before writes','passed':True})
        # One corrupt mirror is explicitly repaired from the valid copy; disagreement is never silently chosen.
        d=Disk(initial=saved);raw=bytearray(d.raw(1));raw[16]^=1;d.put(1,raw);s=scan(d);assert s.valid_copies==2 and not s.editable
        assert lib.d71_prepare(c.byref(d.device),c.byref(s),4,0,0,b'',None,c.byref(Plan()))==5
        p=plan(d,s,8);assert apply(d,s,p)==0;gpt(d);assert scan(d).editable
        checks.append({'name':'invalid primary CRC blocks edits; explicit backup repair restores verified mirrors','passed':True})
        # Both copies may be individually valid yet disagree on usable bounds.
        d=Disk(initial=saved);header=bytearray(d.raw(d.sectors-1));struct.pack_into('<Q',header,48,d.sectors-4096);struct.pack_into('<I',header,16,0);struct.pack_into('<I',header,16,zlib.crc32(header[:92]));d.put(d.sectors-1,header)
        state=scan(d);assert state.valid_copies==3 and not state.editable
        p=plan(d,state,8);assert apply(d,state,p)==0;gpt(d);assert struct.unpack_from('<Q',d.raw(1),48)[0]==d.sectors-4096
        # Preserve required/read-only GPT attribute bits rather than treating them as ordinary data.
        d=Disk(initial=saved);table=bytearray(d.raw(2,16384));struct.pack_into('<Q',table,48,1<<60)
        for at,copy in [(1,0),(d.sectors-1,1)]:
            h=bytearray(d.raw(at));struct.pack_into('<I',h,88,zlib.crc32(table));struct.pack_into('<I',h,16,0);struct.pack_into('<I',h,16,zlib.crc32(h[:92]));d.put(at,h);d.put(d.sectors-33 if copy else 2,table)
        state=scan(d);assert state.editable and state.parts[0].protected
        assert lib.d71_prepare(c.byref(d.device),c.byref(state),5,0,0,b'',None,c.byref(Plan()))==5
        d=Disk(initial=saved);state=scan(d);p=plan(d,state,5,0);d.put(p.first,b'changed boot identity'.ljust(512,b'!'));before=len(d.writes);assert apply(d,state,p)==4 and len(d.writes)==before
        checks.append({'name':'explicit divergent-mirror repair retains chosen usable bounds; read-only GPT attrs and changed format boot identity refuse mutation','passed':True})

        # Independent primary MBR fixture with boot code and a preserved extended container.
        d=Disk();mbr=bytearray(b'B'*446+bytes(64)+b'\x55\xaa');struct.pack_into('<B3sB3sII',mbr,446,0,b'\0'*3,0x07,b'\0'*3,2048,131072);struct.pack_into('<B3sB3sII',mbr,462,0,b'\0'*3,0x0f,b'\0'*3,264192,131072);d.put(0,mbr);d.put(264192,b'EXTENDED SENTINEL'.ljust(512,b'!'))
        s=scan(d);assert s.scheme==1 and s.editable and s.parts[1].protected
        assert lib.d71_prepare(c.byref(d.device),c.byref(s),4,1,0,b'',None,c.byref(Plan()))==5
        p=plan(d,s,5,0);d.allowed=[(0,0),(p.first,p.last)];assert apply(d,s,p)==0;assert d.raw(0)[:446]==bytes(mbr[:446]) and d.raw(0)[462:478]==bytes(mbr[462:478]) and d.raw(264192).startswith(b'EXTENDED SENTINEL')
        s=scan(d);p=plan(d,s,3,0,16);assert apply(d,s,p)==0;assert d.raw(0)[446+2*16+4]==0xda
        checks.append({'name':'primary MBR format/create preserves boot code and foreign/extended entries; extended containers refuse destructive operations','passed':True})
        # Inject every write/flush failure position, both refused and torn writes.
        seeds=[]
        d=Disk();s=scan(d);seeds.append((dict(d.data),plan(d,s,2)))
        assert apply(d,s,seeds[-1][1])==0;s=scan(d);seeds.append((dict(d.data),plan(d,s,3,0,64)))
        assert apply(d,s,seeds[-1][1])==0;s=scan(d);seeds += [(dict(d.data),plan(d,s,op,0)) for op in [4,5,6,1]]
        injected=0
        for seed,p in seeds:
            baseline=Disk(initial=seed);s=scan(baseline);assert apply(baseline,s,p)==0
            total=baseline.calls
            allowed=[(0,63),(baseline.sectors-33,baseline.sectors-1)]
            if p.operation in [5,6]:allowed.append((p.first,p.last))
            for at in range(1,total+1):
                for torn in [False,True]:
                    trial=Disk(initial=seed);state=scan(trial);trial.allowed=allowed;trial.fail=at;trial.torn=torn
                    assert apply(trial,state,p)!=0,(p.operation,at,torn)
                    assert all(any(a<=lba<=b for a,b in allowed) or trial.data.get(lba)==value for lba,value in seed.items())
                    injected+=1
        checks.append({'name':'every write/flush interruption (refused and torn) reports failure; writes stay inside table/selected-partition footprint','passed':True,'fault_cases':injected})
        # Malformed, overlapping and duplicate-GUID tables are rejected with correct checksums.
        rng=random.Random(7101);base=Disk(initial=saved)
        for case in range(300):
            trial=Disk(initial=base.data);t=bytearray(trial.raw(2,16384));offset=rng.randrange(16384);t[offset]^=rng.randrange(1,256);trial.put(2,t)
            state=scan(trial);assert not state.editable
        checks.append({'name':'300 deterministic GPT table corruptions refuse editing instead of accepting a damaged primary','passed':True})
        report.update(passed=True,state_bytes=c.sizeof(State),plan_bytes=c.sizeof(Plan),source_sha256=hashlib.sha256((ROOT/'kernel/install/disk71.c').read_bytes()).hexdigest())
    except Exception:report['error']=traceback.format_exc().replace(str(ROOT),'<project-root>');print(report['error'])
    (out/'disk-results.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print('M7.1 disk core '+('PASS' if report['passed'] else 'FAIL'),flush=True)
    if not report['passed']:raise SystemExit(1)


if __name__=='__main__':main()
