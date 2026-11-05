"""Original bounded ext2 author and independent offline checker for disposable M4 images."""
import struct
U16=lambda b,o:struct.unpack_from('<H',b,o)[0]
U32=lambda b,o:struct.unpack_from('<I',b,o)[0]
def put16(b,o,v):struct.pack_into('<H',b,o,v)
def put32(b,o,v):struct.pack_into('<I',b,o,v)
BS=4096;BLOCKS=4096;INODES=512

class Image:
    def __init__(self,data):self.data=data
    def inode(self,n):return memoryview(self.data)[4*BS+(n-1)*128:4*BS+n*128]
    def block(self,n):return memoryview(self.data)[n*BS:(n+1)*BS]
    def allocate(self,kind):
        start,end,bm=(21,BLOCKS,2) if kind=='block' else (10,INODES,3)
        for n in range(start,end):
            if not self.data[bm*BS+n//8]&(1<<(n%8)):
                self.data[bm*BS+n//8]|=1<<(n%8);return n if kind=='block' else n+1
        raise ValueError('fixture full')
    def entries(self,n):
        inode=self.inode(n)
        for j in range(12):
            block=U32(inode,40+j*4)
            if not block:continue
            data=self.block(block);off=0
            while off<BS:
                number,rec,name,typ=struct.unpack_from('<IHBB',data,off)
                if number:yield bytes(data[off+8:off+8+name]).decode('utf-8'),number,typ
                off+=rec
    def insert(self,parent,name,n,typ):
        encoded=name.encode('utf-8');need=(8+len(encoded)+3)&~3;inode=self.inode(parent)
        for j in range(12):
            bn=U32(inode,40+j*4)
            if not bn:
                bn=self.allocate('block');put32(inode,40+j*4,bn);put32(inode,4,(j+1)*BS);put32(inode,28,(j+1)*8)
                put16(self.block(bn),4,BS)
            data=self.block(bn);off=0
            while off<BS:
                old=U32(data,off);rec=U16(data,off+4);used=((8+data[off+6]+3)&~3) if old else 0
                if rec-used>=need:
                    if old:put16(data,off+4,used);off+=used;rec-=used
                    struct.pack_into('<IHBB',data,off,n,rec,len(encoded),typ);data[off+8:off+8+len(encoded)]=encoded;return
                off+=rec
        raise ValueError('directory full')
    def mkdir(self,parent,name):
        n=self.allocate('inode');ino=self.inode(n);put16(ino,0,0x41ed);put16(ino,26,2)
        self.insert(n,'.',n,2);self.insert(n,'..',parent,2);self.insert(parent,name,n,2)
        put16(self.inode(parent),26,U16(self.inode(parent),26)+1);return n
    def file(self,parent,name,payload):
        n=self.allocate('inode');ino=self.inode(n);put16(ino,0,0x81a4);put16(ino,26,1);put32(ino,4,len(payload))
        for i in range((len(payload)+BS-1)//BS):
            bn=self.allocate('block');put32(ino,40+i*4,bn);part=payload[i*BS:(i+1)*BS];self.block(bn)[:len(part)]=part
        put32(ino,28,((len(payload)+BS-1)//BS)*8);self.insert(parent,name,n,1);return n
    def counts(self):
        freeb=sum(not self.data[2*BS+n//8]&(1<<(n%8)) for n in range(BLOCKS))
        freei=sum(not self.data[3*BS+n//8]&(1<<(n%8)) for n in range(INODES))
        dirs=sum(U16(self.inode(n),0)&0xf000==0x4000 for n in range(1,INODES+1))
        put32(self.data,1024+12,freeb);put32(self.data,1024+16,freei);put16(self.data,BS+12,freeb);put16(self.data,BS+14,freei);put16(self.data,BS+16,dirs)

def format_into(data):
    sb=memoryview(data)[1024:2048]
    for off,v in ((0,INODES),(4,BLOCKS),(12,4075),(16,502),(24,2),(28,2),(32,32768),(36,32768),(40,INODES),(76,1),(84,11),(96,2)) :put32(sb,off,v)
    for off,v in ((54,65535),(56,0xef53),(58,1),(60,1),(88,128)):put16(sb,off,v)
    sb[104:120]=b'ENGINE-EXT2-M4-01'[:16];sb[120:129]=b'EngineM4\0'
    struct.pack_into('<IIIHHH',data,BS,2,3,4,4075,502,1)
    data[2*BS:2*BS+BS]=b'\xff'*BS;data[3*BS:3*BS+BS]=b'\xff'*BS
    for n in range(21,BLOCKS):data[2*BS+n//8]&=~(1<<(n%8))
    for n in range(10,INODES):data[3*BS+n//8]&=~(1<<(n%8))
    im=Image(data);ino=im.inode(2);put16(ino,0,0x41ed);put16(ino,26,2);put32(ino,4,BS);put32(ino,28,8);put32(ino,40,20);put16(im.block(20),4,BS)
    im.insert(2,'.',2,2);im.insert(2,'..',2,2)
    for name in ('System','Applications','Users','Data','Cache','Logs','Temporary','Volumes'):
        n=im.mkdir(2,name)
        if name=='Users':
            user=im.mkdir(n,'test')
            for sub in ('Desktop','Documents','Downloads','Pictures','AppData'):im.mkdir(user,sub)
        if name=='Data':
            for sub in ('Services','Environments','Packages'):im.mkdir(n,sub)
    im.counts();return data

def check(data,repair=False):
    if len(data)!=BLOCKS*BS or U16(data,1080)!=0xef53 or U32(data,1048)!=2 or U16(data,1112)!=128 or U32(data,1120)!=2:raise ValueError('unsupported ext2 geometry/features')
    im=Image(data);seen=set();owned=set(range(20));parents={2:2};pending=[2];useddirs=0
    while pending:
        n=pending.pop()
        if n in seen:raise ValueError('hard link or directory cycle unsupported')
        if not 2<=n<=INODES:raise ValueError('inode bounds')
        seen.add(n);ino=im.inode(n);kind=U16(ino,0)&0xf000;size=U32(ino,4)
        if kind not in (0x4000,0x8000) or size>12*BS or any(U32(ino,o) for o in (88,92,96,104,108)):raise ValueError('unsupported inode')
        count=(size+BS-1)//BS
        if U32(ino,28)!=count*8:raise ValueError('inode block count')
        for j in range(12):
            block=U32(ino,40+j*4)
            if j>=count:
                if block:raise ValueError('block beyond file size')
                continue
            if not 20<=block<BLOCKS or block in owned:raise ValueError('duplicate or invalid block')
            owned.add(block)
        if kind==0x4000:
            useddirs+=1;names=set();subdirs=0;dot=False;dotdot=False
            for j in range(count):
                b=im.block(U32(ino,40+j*4));off=0
                while off<BS:
                    child,rec,length,typ=struct.unpack_from('<IHBB',b,off)
                    if rec<8 or rec%4 or off+rec>BS or length>rec-8:raise ValueError('directory record')
                    if child:
                        name=bytes(b[off+8:off+8+length]).decode('utf-8')
                        if not name or '/' in name or '\0' in name or name in names:raise ValueError('directory name collision')
                        names.add(name)
                        if name=='.':dot=child==n
                        elif name=='..':dotdot=child==parents[n]
                        else:
                            if not 11<=child<=INODES or child in parents:raise ValueError('duplicate inode reference')
                            childkind=U16(im.inode(child),0)&0xf000
                            if typ!={0x4000:2,0x8000:1}.get(childkind):raise ValueError('directory type')
                            parents[child]=n;pending.append(child);subdirs+=typ==2
                    off+=rec
            if not dot or not dotdot or U16(ino,26)!=2+subdirs:raise ValueError('directory links')
        elif U16(ino,26)!=1:raise ValueError('file links')
    allocated=set(range(1,11))|seen
    expectedb=bytearray(b'\xff'*BS);expectedi=bytearray(b'\xff'*BS)
    for n in range(BLOCKS):
        if n not in owned:expectedb[n//8]&=~(1<<(n%8))
    for n in range(1,INODES+1):
        if n not in allocated:expectedi[(n-1)//8]&=~(1<<((n-1)%8))
    freeb=BLOCKS-len(owned);freei=INODES-len(allocated)
    consistent=bytes(im.block(2))==expectedb and bytes(im.block(3))==expectedi and U32(data,1036)==freeb and U32(data,1040)==freei and U16(data,BS+12)==freeb and U16(data,BS+14)==freei and U16(data,BS+16)==useddirs
    clean=U16(data,1082)==1
    if repair:
        im.block(2)[:]=expectedb;im.block(3)[:]=expectedi
        for n in range(11,INODES+1):
            if n not in allocated:im.inode(n)[:]=bytes(128)
        im.counts();put16(data,1082,1)
    elif not consistent or not clean:raise ValueError('dirty/inconsistent filesystem; recover to a new copy')
    return {'reachable_inodes':len(seen),'allocated_blocks':len(owned),'free_blocks':freeb,'free_inodes':freei,'previously_clean':clean,'metadata_consistent':consistent,'repaired':repair}
