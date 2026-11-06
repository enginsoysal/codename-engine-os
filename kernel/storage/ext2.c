#include <engine/fs.h>
#ifdef ENGINE_M5
#include <engine/m5_volume.h>
#endif
#define BS 4096u
static uint8_t super[1024],group[512],bmap[4096],imap[4096],scratch[4096],seen_blocks[512],seen_inodes[64];
static unsigned mounted,readonly=1;
static uint16_t check_queue[512],check_parent[513];static uint64_t name_hashes[512];static uint32_t name_locations[512];static uint8_t compare_block[4096];
typedef struct {uint8_t raw[128];} inode;
typedef struct {unsigned ino,rights,share,generation,live;} file_handle;
static file_handle handles[8];
static uint16_t r16(const void *p){uint16_t v;memcpy(&v,p,2);return v;}
static uint32_t r32(const void *p){uint32_t v;memcpy(&v,p,4);return v;}
static void w16(void *p,unsigned v){uint16_t x=(uint16_t)v;memcpy(p,&x,2);}
static void w32(void *p,unsigned v){uint32_t x=v;memcpy(p,&x,4);}
static unsigned fail(void){readonly=1;return F_IO;}
static unsigned read_block(unsigned b,void *data){if(b>=4096)return F_INVALID;return block_read_page((uint64_t)b*8,data)?fail():0;}
static unsigned write_block(unsigned b,const void *data){if(readonly||b<2||b>=4096)return F_RO;return block_write_page((uint64_t)b*8,data)?fail():0;}
static unsigned inode_read(unsigned n,inode *i){if(n<2||n>512)return F_INVALID;uint8_t s[512];if(block_read(32+(n-1)/4,s))return fail();memcpy(i->raw,s+((n-1)%4)*128,128);return 0;}
static unsigned inode_write(unsigned n,const inode *i){uint8_t s[512];if(n<2||n>512||readonly)return F_RO;if(block_read(32+(n-1)/4,s))return fail();memcpy(s+((n-1)%4)*128,i->raw,128);return block_write(32+(n-1)/4,s)?fail():0;}
static unsigned bit(const uint8_t *m,unsigned n){return (m[n/8]>>(n%8))&1;}
static void set(uint8_t *m,unsigned n,unsigned v){if(v)m[n/8]|=(uint8_t)(1u<<(n%8));else m[n/8]&=(uint8_t)~(1u<<(n%8));}
static unsigned free_count(const uint8_t *m,unsigned count){unsigned f=0;for(unsigned n=0;n<count;++n)f+=!bit(m,n);return f;}
static unsigned allocate(uint8_t *m,unsigned first,unsigned end){for(unsigned n=first;n<end;++n)if(!bit(m,n)){set(m,n,1);return n;}return 0;}
static unsigned sb_write(void){return block_write(2,super)||block_write(3,super+512)?fail():0;}
static unsigned begin(void){if(!mounted||readonly)return F_RO;w16(super+58,2);if(sb_write()||block_flush())return fail();return 0;}
static unsigned end(void){
    unsigned fb=free_count(bmap,4096),fi=free_count(imap,512);w32(super+12,fb);w32(super+16,fi);w16(group+12,fb);w16(group+14,fi);
    if(write_block(2,bmap)||write_block(3,imap)||block_write(8,group)||sb_write()||block_flush())return fail();
    w16(super+58,1);if(sb_write()||block_flush())return fail();return 0;
}
/* UTF-8 is preserved byte-for-byte: no normalization or case folding. */
static int name_valid(const char *s,unsigned n){
    if(!n||n>63)return 0;for(unsigned i=0;i<n;){unsigned c=(uint8_t)s[i++];if(c<32||c==127||c=='/'||c=='\\')return 0;if(c<128)continue;
        unsigned more,min,value;if(c>=0xc2&&c<=0xdf){more=1;min=128;value=c&31;}else if(c>=0xe0&&c<=0xef){more=2;min=2048;value=c&15;}else if(c>=0xf0&&c<=0xf4){more=3;min=65536;value=c&7;}else return 0;
        if(more>n-i)return 0;while(more--){unsigned x=(uint8_t)s[i++];if((x&0xc0)!=0x80)return 0;value=(value<<6)|(x&63);}if(value<min||value>0x10ffff||(value>=0xd800&&value<=0xdfff))return 0;
    }return 1;
}
static unsigned dir_find(unsigned parent,const char *name,unsigned length,unsigned *found,unsigned *block,unsigned *offset){
    inode d;unsigned s=inode_read(parent,&d);if(s)return s;if((r16(d.raw)&0xf000)!=0x4000)return F_INVALID;
    for(unsigned j=0;j<12;++j){unsigned b=r32(d.raw+40+4*j);if(!b)continue;if(read_block(b,scratch))return F_IO;
        for(unsigned off=0;off<BS;){unsigned n=r32(scratch+off),rec=r16(scratch+off+4),len=scratch[off+6];if(rec<8||(rec&3)||rec>BS-off||len>rec-8)return F_INVALID;
            if(n&&len==length&&!memcmp(scratch+off+8,name,length)){*found=n;if(block)*block=b;if(offset)*offset=off;return 0;}off+=rec;}
    }return F_NOTFOUND;
}
static unsigned resolve(const char *path,unsigned parent,char name[64],unsigned *out){
    if(!mounted||!path||path[0]!='/')return F_INVALID;unsigned current=2,start=1,total=0;
    while(path[total])if(++total>255)return F_LIMIT;if(total<2){if(parent)return F_INVALID;*out=2;return 0;}
    for(unsigned i=1;;++i)if(path[i]=='/'||!path[i]){unsigned len=i-start;if(!name_valid(path+start,len)||(len==1&&path[start]=='.')||(len==2&&path[start]=='.'&&path[start+1]=='.'))return F_INVALID;
        if(parent&&!path[i]){memcpy(name,path+start,len);name[len]=0;*out=current;return 0;}
        unsigned next,s=dir_find(current,path+start,len,&next,0,0);if(s)return s;current=next;if(!path[i]){*out=current;return 0;}start=i+1;
    }
}
static unsigned length(const char *s){unsigned n=0;while(s[n])++n;return n;}
static unsigned dir_insert(unsigned parent,const char *name,unsigned number,unsigned type){
    inode d;if(inode_read(parent,&d))return F_IO;unsigned len=length(name),need=(8+len+3)&~3u;
    for(unsigned j=0;j<12;++j){unsigned b=r32(d.raw+40+j*4);if(!b){b=allocate(bmap,20,4096);if(!b)return F_NOSPC;memset(scratch,0,BS);w16(scratch+4,BS);w32(d.raw+40+j*4,b);w32(d.raw+4,(j+1)*BS);w32(d.raw+28,(j+1)*8);if(inode_write(parent,&d))return F_IO;}else if(read_block(b,scratch))return F_IO;
        for(unsigned off=0;off<BS;){unsigned old=r32(scratch+off),rec=r16(scratch+off+4),used=old?((8+scratch[off+6]+3)&~3u):0;
            if(rec<8||(rec&3)||rec>BS-off||used>rec)return F_INVALID;
            if(rec-used>=need){if(old){w16(scratch+off+4,used);off+=used;rec-=used;}w32(scratch+off,number);w16(scratch+off+4,rec);scratch[off+6]=(uint8_t)len;scratch[off+7]=(uint8_t)type;memcpy(scratch+off+8,name,len);return write_block(b,scratch);}off+=rec;}
    }return F_LIMIT;
}
static unsigned busy(unsigned n,unsigned write){for(unsigned j=0;j<8;++j)if(handles[j].live&&handles[j].ino==n&&(!write||!(handles[j].share&2)))return 1;return 0;}
static unsigned remove_entry(unsigned p,const char *name){unsigned n,b,off,s=dir_find(p,name,length(name),&n,&b,&off);if(s)return s;w32(scratch+off,0);return write_block(b,scratch);}
unsigned fs_mkdir(const char *path){
    char name[64];unsigned p,n,s=resolve(path,1,name,&p);if(s)return s;s=dir_find(p,name,length(name),&n,0,0);if(!s)return F_EXISTS;if(s!=F_NOTFOUND)return s;
    if(!free_count(imap,512)||free_count(bmap,4096)<2)return F_NOSPC;if((s=begin()))return s;
    n=allocate(imap,10,512)+1;inode i;memset(&i,0,sizeof(i));w16(i.raw,0x41ed);w16(i.raw+26,2);if(inode_write(n,&i)||dir_insert(n,".",n,2)||dir_insert(n,"..",p,2)||dir_insert(p,name,n,2))return fail();
    if(inode_read(p,&i))return fail();w16(i.raw+26,r16(i.raw+26)+1);if(inode_write(p,&i))return fail();w16(group+16,r16(group+16)+1);return end();
}
unsigned fs_write(const char *path,const void *data,unsigned size){
    if(size>FS_MAX_FILE)return F_LIMIT;char name[64];unsigned p,n=0,s=resolve(path,1,name,&p);if(s)return s;s=dir_find(p,name,length(name),&n,0,0);if(s!=0&&s!=F_NOTFOUND)return s;
    inode old;memset(&old,0,sizeof(old));if(n){if(inode_read(n,&old))return F_IO;if((r16(old.raw)&0xf000)!=0x8000)return F_INVALID;if(busy(n,1))return F_BUSY;}
    unsigned count=(size+BS-1)/BS;if(free_count(bmap,4096)<count+(!n)||(!n&&!free_count(imap,512)))return F_NOSPC;
    if((s=begin()))return s;inode next;memset(&next,0,sizeof(next));w16(next.raw,0x81a4);w16(next.raw+26,1);w32(next.raw+4,size);w32(next.raw+28,count*8);
    for(unsigned j=0;j<count;++j){unsigned b=allocate(bmap,20,4096);if(!b)return fail();w32(next.raw+40+4*j,b);memset(scratch,0,BS);unsigned bytes=size-j*BS;if(bytes>BS)bytes=BS;memcpy(scratch,(const uint8_t*)data+j*BS,bytes);if(write_block(b,scratch))return fail();}
    if(block_flush())return fail();unsigned fresh=!n;if(fresh)n=allocate(imap,10,512)+1;if(inode_write(n,&next))return fail();if(fresh&&dir_insert(p,name,n,1))return fail();
    if(block_flush())return fail();for(unsigned j=0;j<12;++j){unsigned b=r32(old.raw+40+4*j);if(b)set(bmap,b,0);}return end();
}
static unsigned read_inode_data(unsigned n,void *data,unsigned capacity,unsigned *size){inode i;unsigned s=inode_read(n,&i);if(s)return s;if((r16(i.raw)&0xf000)!=0x8000)return F_INVALID;*size=r32(i.raw+4);if(*size>capacity)return F_LIMIT;
    for(unsigned off=0;off<*size;off+=BS){if(read_block(r32(i.raw+40+4*(off/BS)),scratch))return F_IO;unsigned bytes=*size-off;if(bytes>BS)bytes=BS;memcpy((uint8_t*)data+off,scratch,bytes);}return 0;}
unsigned fs_read(const char *path,void *data,unsigned cap,unsigned *size){unsigned n,s=resolve(path,0,0,&n);if(s)return s;for(unsigned j=0;j<8;++j)if(handles[j].live&&handles[j].ino==n&&!(handles[j].share&1))return F_BUSY;return read_inode_data(n,data,cap,size);}
unsigned fs_rename(const char *from,const char *to){
    unsigned p,q,n,dest,s;char a[64],b[64];if((s=resolve(from,1,a,&p))||(s=resolve(to,1,b,&q))||(s=dir_find(p,a,length(a),&n,0,0)))return s;
    if(busy(n,0))return F_BUSY;s=dir_find(q,b,length(b),&dest,0,0);if(!s)return F_EXISTS;if(s!=F_NOTFOUND)return s;inode i;if(inode_read(n,&i))return F_IO;if((r16(i.raw)&0xf000)!=0x8000)return F_UNSUPPORTED;
    if(!free_count(bmap,4096))return F_NOSPC;if((s=begin()))return s;if(dir_insert(q,b,n,1)||remove_entry(p,a))return fail();return end();
}
unsigned fs_remove(const char *path){
    unsigned p,n,s;char name[64];if((s=resolve(path,1,name,&p))||(s=dir_find(p,name,length(name),&n,0,0)))return s;if(busy(n,0))return F_BUSY;
    inode i;if(inode_read(n,&i))return F_IO;if((r16(i.raw)&0xf000)!=0x8000)return F_UNSUPPORTED;if((s=begin()))return s;if(remove_entry(p,name)||block_flush())return fail();
    for(unsigned j=0;j<12;++j){unsigned b=r32(i.raw+40+4*j);if(b)set(bmap,b,0);}memset(&i,0,sizeof(i));if(inode_write(n,&i))return fail();set(imap,n-1,0);return end();
}
unsigned fs_open(const char *path,unsigned rights,unsigned share,uint32_t *handle){unsigned n,s=resolve(path,0,0,&n);if(s)return s;if(!rights||(rights&~3u)||(share&~7u))return F_INVALID;
    if((rights&2)&&readonly)return F_RO;
    inode i;if(inode_read(n,&i))return F_IO;if((r16(i.raw)&0xf000)!=0x8000)return F_INVALID;
    for(unsigned j=0;j<8;++j)if(handles[j].live&&handles[j].ino==n&&((rights&~handles[j].share)||(handles[j].rights&~share)))return F_BUSY;
    for(unsigned j=0;j<8;++j)if(!handles[j].live&&handles[j].generation<0xffffff){file_handle *h=&handles[j];++h->generation;h->ino=n;h->rights=rights;h->share=share;h->live=1;*handle=(h->generation<<3)|j;return 0;}return F_LIMIT;
}
static file_handle *get(uint32_t value){file_handle *h=&handles[value&7];return h->live&&h->generation==(value>>3)?h:0;}
unsigned fs_close(uint32_t h){file_handle *p=get(h);if(!p)return F_INVALID;p->live=0;return 0;}
unsigned fs_read_handle(uint32_t h,void *data,unsigned cap,unsigned *size){file_handle *p=get(h);return !p||!(p->rights&1)?F_INVALID:read_inode_data(p->ino,data,cap,size);}
unsigned fs_list(const char *path){unsigned n,s=resolve(path,0,0,&n);if(s)return s;inode i;if(inode_read(n,&i))return F_IO;if((r16(i.raw)&0xf000)!=0x4000)return F_INVALID;
    for(unsigned j=0;j<12;++j){unsigned b=r32(i.raw+40+4*j);if(!b)continue;if(read_block(b,scratch))return F_IO;for(unsigned off=0;off<BS;){unsigned rec=r16(scratch+off+4);if(rec<8||rec>BS-off)return F_INVALID;if(r32(scratch+off)){char name[64];unsigned len=scratch[off+6];if(len>63)return F_INVALID;memcpy(name,scratch+off+8,len);name[len]=0;console_text(name);console_text(scratch[off+7]==2?"/\n":"\n");}off+=rec;}}return 0;}
unsigned fs_readonly(void){return readonly;}
unsigned fs_check(void){
    memset(seen_blocks,0,sizeof(seen_blocks));memset(seen_inodes,0,sizeof(seen_inodes));for(unsigned j=0;j<20;++j)set(seen_blocks,j,1);for(unsigned j=0;j<10;++j)set(seen_inodes,j,1);
    unsigned dirs=0,head=0,tail=1;check_queue[0]=2;check_parent[2]=2;while(head<tail){unsigned n=check_queue[head++];inode i;if(inode_read(n,&i))return F_IO;unsigned kind=r16(i.raw)&0xf000,size=r32(i.raw+4),count=(size+BS-1)/BS;
        if((kind!=0x4000&&kind!=0x8000)||size>FS_MAX_FILE||r32(i.raw+28)!=count*8||r32(i.raw+88)||r32(i.raw+92)||r32(i.raw+96)||r32(i.raw+104)||r32(i.raw+108))return F_INVALID;
        for(unsigned j=0;j<12;++j){unsigned b=r32(i.raw+40+4*j);if(j>=count){if(b)return F_INVALID;continue;}if(b<20||b>=4096||bit(seen_blocks,b)||!bit(bmap,b))return F_INVALID;set(seen_blocks,b,1);}
        if(kind==0x4000){if(!count||size%BS)return F_INVALID;++dirs;unsigned names=0,subdirs=0,dot=0,dotdot=0;
            for(unsigned j=0;j<count;++j){if(read_block(r32(i.raw+40+4*j),scratch))return F_IO;for(unsigned off=0;off<BS;){unsigned child=r32(scratch+off),rec=r16(scratch+off+4),len=scratch[off+6];if(rec<8||(rec&3)||rec>BS-off||len>rec-8)return F_INVALID;
                if(child){if(child<2||child>512||!bit(imap,child-1)||!name_valid((char*)scratch+off+8,len)||names==512)return F_INVALID;
                    uint64_t hash=UINT64_C(14695981039346656037);for(unsigned k=0;k<len;++k)hash=(hash^scratch[off+8+k])*UINT64_C(1099511628211);
                    for(unsigned k=0;k<names;++k)if(name_hashes[k]==hash){unsigned oldj=name_locations[k]/BS,oldo=name_locations[k]%BS;const uint8_t *old=scratch;if(oldj!=j){if(read_block(r32(i.raw+40+4*oldj),compare_block))return F_IO;old=compare_block;}if(old[oldo+6]==len&&!memcmp(old+oldo+8,scratch+off+8,len))return F_INVALID;}
                    name_hashes[names]=hash;name_locations[names++]=j*BS+off;unsigned type=scratch[off+7];
                    if(len==1&&scratch[off+8]=='.'){if(child!=n||type!=2)return F_INVALID;dot=1;}
                    else if(len==2&&scratch[off+8]=='.'&&scratch[off+9]=='.'){if(child!=check_parent[n]||type!=2)return F_INVALID;dotdot=1;}
                    else {if(child<11||bit(seen_inodes,child-1)||tail==512)return F_INVALID;inode child_inode;if(inode_read(child,&child_inode))return F_IO;unsigned child_type=r16(child_inode.raw)&0xf000;if((type!=1||child_type!=0x8000)&&(type!=2||child_type!=0x4000))return F_INVALID;subdirs+=type==2;set(seen_inodes,child-1,1);check_parent[child]=(uint16_t)n;check_queue[tail++]=(uint16_t)child;}
                }off+=rec;}}
            if(!dot||!dotdot||r16(i.raw+26)!=2+subdirs)return F_INVALID;
        }else if(r16(i.raw+26)!=1)return F_INVALID;
    }
    if(memcmp(seen_blocks,bmap,512)||memcmp(seen_inodes,imap,64)||r32(super+12)!=free_count(bmap,4096)||r32(super+16)!=free_count(imap,512)||r16(group+12)!=r32(super+12)||r16(group+14)!=r32(super+16)||r16(group+16)!=dirs)return F_INVALID;return 0;
}
unsigned fs_mount(void){
    if(block_read(2,super)||block_read(3,super+512)||block_read(8,group)||read_block(2,bmap)||read_block(3,imap))return F_IO;
    if(r16(super+56)!=0xef53||r32(super)!=512||r32(super+4)!=4096||r32(super+20)||r32(super+24)!=2||r32(super+32)!=32768||r32(super+40)!=512||r32(super+76)!=1||r32(super+84)!=11||r16(super+88)!=128||r32(super+92)||r32(super+96)!=2||r32(super+100)||r32(group)!=2||r32(group+4)!=3||r32(group+8)!=4)return F_UNSUPPORTED;
    for(unsigned i=512;i<4096;++i)if(bmap[i]!=255)return F_INVALID;for(unsigned i=64;i<4096;++i)if(imap[i]!=255)return F_INVALID;
    mounted=1;if(r16(super+58)!=1)return F_RO;unsigned s=fs_check();if(s)return s;if(block_filesystem_enable())return F_IO;readonly=0;return 0;
}
