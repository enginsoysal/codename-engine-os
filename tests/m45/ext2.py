"""Original two-group ext2 fixtures: native geometry, indirect/hole reads and damaged metadata."""
from pathlib import Path
import hashlib,json,struct,subprocess,sys
sys.path[:0]=[str(Path(__file__).resolve().parents[2]/'tools'),str(Path(__file__).resolve().parents[2]/'tools/build')]
from vm import ROOT,arguments,qemu_tools
from media import gpt,package

def fixture(block,inode_bytes):
    first=int(block==1024);bpg=1024;ipg=32;blocks=first+2*bpg;data=bytearray(blocks*block);table_blocks=(ipg*inode_bytes+block-1)//block;owned=[set(),set()];tables=[]
    for group in range(2):
        start=first+group*bpg;tables.append(start+4);owned[group].update(range(start,start+4+table_blocks))
    def take(group):
        start=first+group*bpg;n=start+4+table_blocks
        while n in owned[group]:n+=1
        owned[group].add(n);return n
    def write_inode(number,mode,size,pointers,links=1):
        group,index=divmod(number-1,ipg);offset=tables[group]*block+index*inode_bytes;struct.pack_into('<H',data,offset,mode);struct.pack_into('<I',data,offset+4,size);struct.pack_into('<H',data,offset+26,links);struct.pack_into('<I',data,offset+28,sum(bool(p) for p in pointers)*block//512)
        for index,pointer in enumerate(pointers):struct.pack_into('<I',data,offset+40+index*4,pointer)
        return offset
    def directory(entries):
        result=bytearray(block);offset=0
        for index,(number,name,kind) in enumerate(entries):
            name=name.encode('utf-8');record=(8+len(name)+3)&~3
            if index==len(entries)-1:record=block-offset
            struct.pack_into('<IHBB',result,offset,number,record,len(name),kind);result[offset+8:offset+8+len(name)]=name;offset+=record
        return result
    root=take(0);subdir=take(1);payload=bytes(0 if 32768<=i<36864 else (i*37+11)&255 for i in range(65536));pointers=[]
    for offset in range(0,65536,block):
        if 32768<=offset<36864:pointers.append(0);continue
        pointer=take(1);pointers.append(pointer);data[pointer*block:(pointer+1)*block]=payload[offset:offset+block]
    indirect=take(1)
    for index,pointer in enumerate(pointers[12:]):struct.pack_into('<I',data,indirect*block+index*4,pointer)
    write_inode(2,0x41ed,block,[root],3);big_inode=write_inode(33,0x81a4,65536,pointers[:12]+[indirect],2);struct.pack_into('<I',data,big_inode+28,(sum(bool(p) for p in pointers)+1)*block//512);write_inode(34,0x41ed,block,[subdir],2);link=write_inode(35,0xa1ff,7,[]);data[link+40:link+47]=b'BIG.BIN'
    data[root*block:(root+1)*block]=directory([(2,'.',2),(2,'..',2),(33,'BIG.BIN',1),(34,'Docs',2),(35,'link',7)]);data[subdir*block:(subdir+1)*block]=directory([(34,'.',2),(2,'..',2),(33,'R\u00e9sum\u00e9.txt',1)])
    superblock=bytearray(1024)
    for offset,value in [(0,64),(4,blocks),(12,2*bpg-sum(map(len,owned))),(16,51),(20,first),(24,{1024:0,2048:1,4096:2}[block]),(28,{1024:0,2048:1,4096:2}[block]),(32,bpg),(36,bpg),(40,ipg),(76,1),(84,11),(96,2),(100,3)]:struct.pack_into('<I',superblock,offset,value)
    for offset,value in [(56,0xef53),(58,1),(60,1),(88,inode_bytes)]:struct.pack_into('<H',superblock,offset,value)
    superblock[104:120]=bytes(range(16));descriptors=bytearray(64)
    for group in range(2):
        start=first+group*bpg;bitmap=bytearray(b'\xff'*block)
        for index in range(bpg):
            if start+index not in owned[group]:bitmap[index//8]&=~(1<<(index%8))
        data[(start+2)*block:(start+3)*block]=bitmap;bitmap=bytearray(b'\xff'*block);used=set(range(10)) if group==0 else {0,1,2}
        for index in range(ipg):
            if index not in used:bitmap[index//8]&=~(1<<(index%8))
        data[(start+3)*block:(start+4)*block]=bitmap;struct.pack_into('<IIIHHH',descriptors,group*32,start+2,start+3,tables[group],bpg-len(owned[group]),ipg-len(used),1)
    data[1024:2048]=superblock;data[(first+1)*block:(first+1)*block+64]=descriptors;backup=first+bpg;struct.pack_into('<H',superblock,90,1);data[backup*block:backup*block+1024]=superblock;data[(backup+1)*block:(backup+1)*block+64]=descriptors
    assert data[pointers[-1]*block:pointers[-1]*block+block]==payload[-block:]
    return data,{'big_inode':big_inode,'root':root,'descriptor':(first+1)*block,'blocks':blocks}

def main():
    work=ROOT/'build/m45/ext2-tests';work.mkdir(parents=True,exist_ok=True);out=ROOT/'evidence/M4.5/ext2';out.mkdir(parents=True,exist_ok=True);build=ROOT/'build/m45';manifest=json.loads((ROOT/'dist/m45-build-manifest.json').read_text(encoding='utf-8'));package(work,(build/'BOOTX64.EFI').read_bytes(),(build/'KERNEL.ELF').read_bytes(),struct.pack('<8sII32s',b'ENGINIT1',48,6,manifest['build_id'].encode()),'M45');checks=[]
    for name,block,inode_bytes,transport,sector,damage in [('1k-128-ahci',1024,128,'ahci',512,None),('2k-256-nvme4k',2048,256,'nvme',4096,None),('4k-256-usb',4096,256,'usb',512,None),('4k-512-ahci',4096,512,'ahci',512,None),('bad-pointer',1024,128,'ahci',512,'pointer'),('bad-dir-record',2048,256,'nvme',512,'directory'),('bad-inode-table',4096,256,'usb',512,'table'),('unsupported-extents',4096,256,'ahci',512,'features')]:
        data,layout=fixture(block,inode_bytes)
        if damage=='pointer':struct.pack_into('<I',data,layout['big_inode']+40,layout['blocks'])
        if damage=='directory':struct.pack_into('<H',data,layout['root']*block+4,0)
        if damage=='table':struct.pack_into('<I',data,layout['descriptor']+32+8,layout['blocks'])
        if damage=='features':struct.pack_into('<I',data,1024+96,0x42)
        if sector==512:image=gpt(data,'EXT2 read test')
        else:
            # MBR offset/length are in the actual device's 4096-byte sectors.
            image=bytearray(256*sector+len(data));image[510:512]=b'\x55\xaa';image[450]=0x83;struct.pack_into('<II',image,454,256,len(data)//sector);image[256*sector:]=data
        disk=work/(name+'.img');disk.write_bytes(image);before=hashlib.sha256(image).hexdigest();args,serial=arguments(qemu_tools(),'iso',work/'EngineOS-M45.iso',work/name,ram=128);device={'ahci':'ide-hd,bus=ide.0','nvme':f'nvme,serial=ext2-test,logical_block_size={sector},physical_block_size={sector}','usb':'usb-storage,bus=xhci.0'}[transport];args+=['-drive',f'if=none,id=data,format=raw,file={disk}','-device',device+',drive=data'];p=subprocess.run(args,capture_output=True,timeout=90,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));log=serial.read_text(encoding='utf-8',errors='replace');(out/(name+'.log')).write_bytes(('\n'.join(line.rstrip() for line in log.splitlines())+'\n').encode());expected=101 if damage else 33;passed=p.returncode==expected and ('M45_EXT2_READ_FAIL' if damage else 'M45_EXT2_READ_PASS') in log and 'M45_EXT2_TEST_BUFFERS_RECLAIMED' in log and hashlib.sha256(disk.read_bytes()).hexdigest()==before;checks.append({'name':name,'passed':passed,'exit':p.returncode,'filesystem_block_bytes':block,'inode_bytes':inode_bytes,'logical_sector_bytes':sector,'transport':transport,'backing_unchanged':hashlib.sha256(disk.read_bytes()).hexdigest()==before});print(name,'PASS' if passed else 'FAIL',flush=True)
    (out/'results.json').write_bytes((json.dumps({'build_id':manifest['build_id'],'full_m45_complete':False,'checks':checks,'fixture_scope':'Original fixed two-group ext2 fixtures; native payload bytes checked independently against a known pattern. No external e2fsck qualification claimed.'},indent=2)+'\n').encode())
    if not all(c['passed'] for c in checks):raise SystemExit(1)
if __name__=='__main__':main()
