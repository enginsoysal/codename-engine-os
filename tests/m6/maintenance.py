"""Native installer customization, cancellation, distinct-version update and recovery."""
import importlib.util,json,struct,subprocess,time,traceback,zlib
from pathlib import Path
spec=importlib.util.spec_from_file_location('m6guided',Path(__file__).with_name('guided.py'));h=importlib.util.module_from_spec(spec);spec.loader.exec_module(h)
q=h.q;ROOT=q.ROOT

def activation(path):
    candidates=[]
    with path.open('rb') as f:
        for sector in [41,42]:
            f.seek(sector*512);b=f.read(512)
            if b[:8]==b'ENGINEI6' and zlib.crc32(b[:508])==struct.unpack_from('<I',b,508)[0] and struct.unpack_from('<I',b,72)[0]==2:candidates.append((struct.unpack_from('<Q',b,80)[0],sector,b))
    assert candidates;return max(candidates)

def inspect(path,user,guide):
    _,_,record=activation(path)
    with path.open('rb') as f:
        sectors=path.stat().st_size//512;tables=[]
        for where in [1,sectors-1]:
            f.seek(where*512);header=bytearray(f.read(512));assert header[:8]==b'EFI PART';sum=struct.unpack_from('<I',header,16)[0];struct.pack_into('<I',header,16,0);assert zlib.crc32(header[:92])==sum
            f.seek(struct.unpack_from('<Q',header,72)[0]*512);table=f.read(16384);assert zlib.crc32(table)==struct.unpack_from('<I',header,88)[0];tables.append(table)
        assert tables[0]==tables[1];parts=[struct.unpack_from('<QQ',tables[0],i*128+32) for i in range(5)]
        assert parts[4][1]<sectors-33 and parts[4][0]>parts[3][1]
        # Independently follow the installed FAT chain and compare the original ELF and BIOS bytes.
        import hashlib
        from bios import flatten64
        esp=parts[1][0];f.seek(esp*512);boot=f.read(512);reserved=struct.unpack_from('<H',boot,14)[0];fat_sectors=struct.unpack_from('<I',boot,36)[0];cluster_bytes=boot[13]*512;data_lba=esp+reserved+boot[16]*fat_sectors
        f.seek(data_lba*512);root=f.read(cluster_bytes);entry=next(root[o:o+32] for o in range(0,cluster_bytes,32) if root[o:o+11]==b'KERNEL  ELF');cluster=(struct.unpack_from('<H',entry,20)[0]<<16)|struct.unpack_from('<H',entry,26)[0];size=struct.unpack_from('<I',entry,28)[0];kernel=bytearray();seen=set()
        assert 0<size<=4*1024*1024
        while len(kernel)<size:
            assert 2<=cluster<0x0ffffff0 and cluster not in seen;seen.add(cluster);f.seek((data_lba+(cluster-2)*boot[13])*512);kernel+=f.read(cluster_bytes);f.seek((esp+reserved)*512+cluster*4);cluster=struct.unpack('<I',f.read(4))[0]&0x0fffffff
        kernel=bytes(kernel[:size]);versions=[]
        for name in ['m6','m6u1','m6minimal']:
            manifest=ROOT/'dist'/(name+'-build-manifest.json')
            if manifest.exists():versions.append(json.loads(manifest.read_text(encoding='utf-8')))
        expected=next(v for v in versions if v['build_id']==record[24:56].decode());assert hashlib.sha256(kernel).hexdigest()==expected['installed_kernel_sha256'];raw,_=flatten64(kernel);f.seek(2112*512);assert f.read(len(raw))==raw
        start=parts[4][0]*512;banks=[]
        for bank in [0,1]:
            f.seek(start+bank*71*4096);header=f.read(4096)
            if header[:8]!=b'ENGINEV6' or zlib.crc32(header[:4092])!=struct.unpack_from('<I',header,4092)[0]:continue
            f.seek(start+(bank*71+1)*4096);raw=f.read(1024*280)
            if zlib.crc32(raw)!=struct.unpack_from('<I',header,36)[0]:continue
            banks.append((struct.unpack_from('<Q',header,24)[0],raw))
        assert banks;raw=max(banks)[1];entries={}
        for i in range(1024):
            b=raw[i*280:(i+1)*280];first,size,crc,kind=struct.unpack_from('<QQII',b,256)
            if kind:entries[b[:256].split(b'\0')[0].decode()]=(first,size,crc,kind)
        first,size,crc,_=entries['/System/Settings/setup.bin'];f.seek(start+first*4096);settings=f.read(size);assert zlib.crc32(settings)==crc
        assert settings[:32].split(b'\0')[0].decode()==user and struct.unpack_from('<I',settings,36)[0]==guide
        assert ('/Users/'+user+'/Documents/Welcome.txt' in entries)==bool(guide)
    return start,(parts[4][1]-parts[4][0]+1)*512,entries,record

def flow(g,tabs,expected):
    h.next(g,tabs);h.next(g);h.next(g,2);start=len(g.log());h.next(g,1);g.wait(expected,start,240)

def main():
    work=ROOT/'build/m6/maintenance'/str(time.time_ns());work.mkdir(parents=True);out=ROOT/'evidence/M6/maintenance';out.mkdir(parents=True,exist_ok=True);checks=[];result={'passed':False,'checks':checks}
    try:
        base=json.loads((ROOT/'dist/m6-build-manifest.json').read_text(encoding='utf-8'));new=json.loads((ROOT/'dist/m6u1-build-manifest.json').read_text(encoding='utf-8'));assert base['build_id']!=new['build_id'] and base['source_sha256']==new['source_sha256']
        disk=work/'owned.img';disk.touch();subprocess.run(['fsutil','sparse','setflag',str(disk)],check=True,capture_output=True)
        with disk.open('r+b') as f:f.truncate(2*1024**3)
        empty=q.digest(disk)
        with q.Guest(work/'install',disk) as g:
            h.next(g);h.next(g);h.next(g,2);h.next(g,2)
            h.key(g,'backspace',5)
            for letter in 'tester':h.key(g,letter)
            h.next(g,2);h.next(g,3);h.shot(g,out,'custom-review');assert q.digest(disk)==empty
            h.next(g);h.shot(g,out,'retained-choices');assert q.digest(disk)==empty
            h.next(g,5);start=len(g.log());h.next(g,1);g.wait('M6_INSTALL_PASS',start,240);(out/'install.log').write_bytes(g.log().encode())
        start,length,entries,record=inspect(disk,'tester',1)
        checks.append({'name':'editable account/accent, retained Back/Next choices and full disk unchanged before final Install','passed':True})
        with q.Guest(work/'baseline',disk,media='installed') as g:
            h.next(g,2);g.wait('M6_GUIDE_REMOVED');h.next(g);pos=len(g.log());h.next(g);g.wait('M6_LARGE_FILE_PASS',pos,120);(out/'baseline.log').write_bytes(g.log().encode())
        inspect(disk,'tester',0);saved=q.digest(disk,start,length)
        checks.append({'name':'optional guide removed independently; settings remain usable and a real large document can be written','passed':True})
        with q.Guest(work/'update',disk,image_name='EngineOS-M6U1') as g:
            flow(g,3,'M6_MAINTENANCE_PASS');(out/'update.log').write_bytes(g.log().encode())
        assert q.digest(disk,start,length)==saved;assert inspect(disk,'tester',0)[3][24:56].decode()==new['build_id']
        with q.Guest(work/'updated-boot',disk,media='installed') as g:
            assert 'ENGINE_OS_KERNEL_ENTRY build='+new['build_id'] in g.log();h.next(g,2);pos=len(g.log());h.next(g,1);g.wait('M6_LARGE_FILE_PASS',pos,120);(out/'updated-boot.log').write_bytes(g.log().encode())
        checks.append({'name':'different original kernel build stages into inactive slot and boots after activation; complete data partition unchanged','passed':True})
        with q.Guest(work/'rollback',disk) as g:
            flow(g,4,'M6_MAINTENANCE_PASS');(out/'rollback.log').write_bytes(g.log().encode())
        assert q.digest(disk,start,length)==saved and inspect(disk,'tester',0)[3][24:56].decode()==base['build_id']
        with disk.open('r+b') as f:f.seek(2112*512+511);v=f.read(1);f.seek(-1,1);f.write(bytes([v[0]^255]))
        rejected=False
        try:
            with q.Guest(work/'corrupted-bios',disk,'bios',media='installed'):pass
        except RuntimeError:
            rejected='BIOS_REJECT: kernel raw CRC' in (work/'corrupted-bios/serial.log').read_text(encoding='utf-8',errors='replace')
        assert rejected
        with q.Guest(work/'repair',disk) as g:
            flow(g,2,'M6_MAINTENANCE_PASS');(out/'repair.log').write_bytes(g.log().encode())
        assert q.digest(disk,start,length)==saved
        with q.Guest(work/'repaired-bios',disk,'bios',media='installed') as g:
            assert 'ENGINE_OS_KERNEL_ENTRY build='+base['build_id'] in g.log();h.shot(g,out,'repaired-installed');(out/'repaired.log').write_bytes(g.log().encode())
        checks.append({'name':'rollback restores original kernel; deliberately corrupted BIOS payload refuses boot; ISO repair restores boot with entire data partition unchanged','passed':True})
        _,sector,_=activation(disk)
        with disk.open('r+b') as f:f.seek(sector*512);f.write(b'TORN-ACTIVATION')
        with q.Guest(work/'activation-fallback',disk,media='installed') as g:
            assert 'M6_INSTALLED_VOLUME' in g.log();(out/'activation-fallback.log').write_bytes(g.log().encode())
        assert q.digest(disk,start,length)==saved
        checks.append({'name':'one torn activation record falls back to previous valid generation without modifying user data','passed':True})
        result.update(passed=True,build_id=base['build_id'],update_build_id=new['build_id'],iso_sha256=base['artifacts']['EngineOS-M6.iso']['sha256'],m6_acceptance_complete=False)
    except Exception:result['error']=traceback.format_exc().replace(str(ROOT),'<project-root>');print(result['error'])
    (out/'results.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print('M6 maintenance '+('PASS' if result['passed'] else 'FAIL'),flush=True)
    if not result['passed']:raise SystemExit(1)
if __name__=='__main__':main()
