#include <engine/storage.h>

typedef struct {unsigned disk,sector,block,inode_bytes,blocks,inodes,bpg,ipg,groups,first,filetype;uint64_t start,count;} volume;
typedef struct {unsigned mode,flags;uint64_t size;uint32_t pointers[15];} inode;
static uint8_t sector_data[4096],block_data[4096],indirect_data[4096];
static uint16_t u16(const void *p){uint16_t n;memcpy(&n,p,2);return n;}
static uint32_t u32(const void *p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned bytes(volume *v,uint64_t offset,void *buffer,unsigned length){
    uint64_t total=v->count*v->sector;if(v->blocks&&(uint64_t)v->blocks*v->block<total)total=(uint64_t)v->blocks*v->block;if(offset>total||length>total-offset)return B_DENIED;uint8_t *out=buffer;
    while(length){unsigned within=(unsigned)(offset%v->sector),chunk=v->sector-within;if(chunk>length)chunk=length;unsigned status=storage_read(v->disk,v->start+offset/v->sector,sector_data);if(status)return status;memcpy(out,sector_data+within,chunk);out+=chunk;offset+=chunk;length-=chunk;}return B_OK;
}
static unsigned mount(unsigned disk,unsigned part,volume *v){
    const storage_disk *d=storage_get(disk);if(!d||!d->readable||!d->sector_bytes||d->sectors>UINT64_MAX/d->sector_bytes)return B_DENIED;uint64_t start,count;unsigned status=storage_partition(disk,part,&start,&count);if(status)return status;*v=(volume){.disk=disk,.sector=d->sector_bytes,.start=start,.count=count};if((status=bytes(v,1024,block_data,1024)))return status;const uint8_t *s=block_data;
    unsigned log=u32(s+24),revision=u32(s+76);if(u16(s+56)!=0xef53||log>2||revision>1||u32(s+28)!=log||(u16(s+58)&3)!=1)return B_UNSUPPORTED;v->block=1024u<<log;v->blocks=u32(s+4);v->inodes=u32(s);v->bpg=u32(s+32);v->ipg=u32(s+40);v->first=u32(s+20);v->inode_bytes=revision?u16(s+88):128;unsigned compat=revision?u32(s+92):0,incompat=revision?u32(s+96):0,ro=revision?u32(s+100):0;v->filetype=incompat&2;
    if((compat&~0x38u)||(incompat&~2u)||(ro&~3u))return B_UNSUPPORTED;if(!v->blocks||v->blocks<=v->first||v->first!=(v->block==1024)||!v->inodes||!v->bpg||v->bpg>v->block*8||!v->ipg||v->ipg>v->block*8||v->inode_bytes<128||v->inode_bytes>512||(v->inode_bytes&(v->inode_bytes-1))||v->inode_bytes>v->block||(uint64_t)v->blocks*v->block>count*v->sector)return B_DENIED;
    v->groups=(v->blocks-v->first-1)/v->bpg+1;if(!v->groups||v->groups>256||(v->inodes-1)/v->ipg+1!=v->groups)return B_UNSUPPORTED;if((uint64_t)(v->first+1)*v->block+(uint64_t)v->groups*32>(uint64_t)v->blocks*v->block)return B_DENIED;return B_OK;
}
static unsigned read_inode(volume *v,unsigned number,inode *node){
    if(!number||number>v->inodes)return B_DENIED;unsigned group=(number-1)/v->ipg,index=(number-1)%v->ipg;uint8_t descriptor[32],raw[512];unsigned status=bytes(v,(uint64_t)(v->first+1)*v->block+(uint64_t)group*32,descriptor,32);if(status)return status;
    unsigned table=u32(descriptor+8);uint64_t group_start=(uint64_t)v->first+(uint64_t)group*v->bpg,group_end=group_start+v->bpg;if(group_end>v->blocks)group_end=v->blocks;uint64_t table_blocks=((uint64_t)v->ipg*v->inode_bytes+v->block-1)/v->block;if(table<group_start||table>=group_end||table_blocks>group_end-table)return B_DENIED;
    if((status=bytes(v,(uint64_t)table*v->block+(uint64_t)index*v->inode_bytes,raw,v->inode_bytes)))return status;node->mode=u16(raw);node->flags=u32(raw+32);node->size=u32(raw+4);if((node->mode&0xf000)==0x8000)node->size|=(uint64_t)u32(raw+108)<<32;memcpy(node->pointers,raw+40,60);if(!u16(raw+26)||u32(raw+20)||(node->flags&(0x80000u|0x1000u|0x10000000u)))return B_UNSUPPORTED;return B_OK;
}
static unsigned file_block(volume *v,const inode *node,unsigned index,unsigned *physical){
    if(index<12)*physical=node->pointers[index];else{index-=12;if(index>=v->block/4)return B_UNSUPPORTED;unsigned block=node->pointers[12];if(!block){*physical=0;return B_OK;}if(block<v->first||block>=v->blocks)return B_DENIED;unsigned status=bytes(v,(uint64_t)block*v->block,indirect_data,v->block);if(status)return status;*physical=u32(indirect_data+index*4);}
    if(*physical&&(*physical<v->first||*physical>=v->blocks))return B_DENIED;return B_OK;
}
static int name_valid(const char *s,unsigned n){
    if(!n||n>255)return 0;for(unsigned i=0;i<n;){unsigned c=(uint8_t)s[i++];if(c<32||c==127||c=='/'||c=='\\')return 0;if(c<128)continue;
        unsigned more,min,value;if(c>=0xc2&&c<=0xdf){more=1;min=128;value=c&31;}else if(c>=0xe0&&c<=0xef){more=2;min=2048;value=c&15;}else if(c>=0xf0&&c<=0xf4){more=3;min=65536;value=c&7;}else return 0;
        if(more>n-i)return 0;while(more--){unsigned x=(uint8_t)s[i++];if((x&0xc0)!=0x80)return 0;value=(value<<6)|(x&63);}if(value<min||value>0x10ffff||(value>=0xd800&&value<=0xdfff))return 0;
    }return 1;
}
static unsigned directory(volume *v,const inode *node,const char *wanted,unsigned length,unsigned *found){
    if((node->mode&0xf000)!=0x4000)return B_DENIED;if(node->size%v->block||node->size/v->block>128)return B_UNSUPPORTED;
    for(unsigned i=0;i<node->size/v->block;++i){unsigned physical,status=file_block(v,node,i,&physical);if(status||!physical)return status?status:B_DENIED;if((status=bytes(v,(uint64_t)physical*v->block,block_data,v->block)))return status;
        for(unsigned offset=0;offset<v->block;){const uint8_t *entry=block_data+offset;if(v->block-offset<8)return B_DENIED;unsigned number=u32(entry),record=u16(entry+4),name=v->filetype?entry[6]:u16(entry+6);if(record<8||(record&3)||record>v->block-offset||name>255||name>record-8||number>v->inodes)return B_DENIED;
            if(number){if(!name_valid((const char*)entry+8,name))return B_DENIED;if(wanted&&length==name&&!memcmp(wanted,entry+8,name)){*found=number;return B_OK;}if(!wanted){char text[256];memcpy(text,entry+8,name);text[name]=0;console_text(text);console_text("\n");}}
            offset+=record;}
    }return wanted?B_ABSENT:B_OK;
}
static unsigned resolve(volume *v,const char *path,inode *node){
    if(!path||path[0]!='/')return B_DENIED;unsigned total=0;while(path[total])if(++total>255)return B_DENIED;unsigned status=read_inode(v,2,node),offset=1;if(status)return status;if(!path[offset])return B_OK;
    while(path[offset]){unsigned begin=offset;while(path[offset]&&path[offset]!='/')++offset;unsigned length=offset-begin;if(!name_valid(path+begin,length)||(length==1&&path[begin]=='.')||(length==2&&path[begin]=='.'&&path[begin+1]=='.'))return B_DENIED;unsigned found=0;if((status=directory(v,node,path+begin,length,&found))||(status=read_inode(v,found,node)))return status;if(path[offset]&&!path[++offset])return B_DENIED;}return B_OK;
}
unsigned ext_read(unsigned disk,unsigned part,const char *path,void *buffer,unsigned capacity,unsigned *size){
    if(!buffer||!size||capacity>65536)return B_DENIED;*size=0;volume v;inode node;unsigned status=mount(disk,part,&v);if(status||(status=resolve(&v,path,&node)))return status;if((node.mode&0xf000)!=0x8000)return B_UNSUPPORTED;if(node.size>capacity||node.size>(uint64_t)(12+v.block/4)*v.block)return B_UNSUPPORTED;
    unsigned offset=0;while(offset<node.size){unsigned physical,chunk=v.block;if(chunk>node.size-offset)chunk=(unsigned)node.size-offset;if((status=file_block(&v,&node,offset/v.block,&physical)))return status;if(physical){if((status=bytes(&v,(uint64_t)physical*v.block,(uint8_t*)buffer+offset,chunk)))return status;}else memset((uint8_t*)buffer+offset,0,chunk);offset+=chunk;}*size=(unsigned)node.size;return B_OK;
}
unsigned ext_list(unsigned disk,unsigned part,const char *path){volume v;inode node;unsigned status=mount(disk,part,&v);if(status||(status=resolve(&v,path,&node)))return status;return directory(&v,&node,0,0,0);}
