#include <engine/storage.h>
#ifdef ENGINE_M10
#include <engine/hardware10.h>
#include <engine/install/m6.h>
static unsigned emit10(const char*,unsigned,unsigned);
#endif
typedef struct {unsigned disk,bytes,spc,fats,fat_index,mirrored;uint64_t start,count,fat_start,data_start,fat_sectors,clusters;unsigned root;} volume;
static uint8_t sector[4096],fat_sector[4096];
static uint16_t r16(const void *p){uint16_t x;memcpy(&x,p,2);return x;}
static uint32_t r32(const void *p){uint32_t x;memcpy(&x,p,4);return x;}
static unsigned mount(unsigned disk,unsigned part,volume *v){
    const storage_disk *d=storage_get(disk);if(!d||!d->readable)return B_DENIED;uint64_t start=0,count=d->sectors;unsigned s=0;
#ifdef ENGINE_M10
 if(part!=UINT32_MAX){s=storage_partition(disk,part,&start,&count);}
#else
 s=storage_partition(disk,part,&start,&count);
#endif
 if(s)return s;if(storage_read(disk,start,sector))return B_IO;
    unsigned bytes=r16(sector+11),spc=sector[13],reserved=r16(sector+14),fats=sector[16],fat_size=r32(sector+36),total=r32(sector+32),flags=r16(sector+40),root=r32(sector+44);
    if(sector[510]!=0x55||sector[511]!=0xaa||bytes!=d->sector_bytes||!spc||(spc&(spc-1))||spc>128||!reserved||(fats!=1&&fats!=2)||r16(sector+17)||r16(sector+19)||r16(sector+22)||r16(sector+42)||!fat_size||!total||total>count||(flags&0xff70))return B_UNSUPPORTED;
    uint64_t metadata=(uint64_t)reserved+(uint64_t)fats*fat_size;if(metadata>=total)return B_DENIED;uint64_t clusters=(total-metadata)/spc;if(clusters<65525||clusters>=0x0ffffff5||((uint64_t)fat_size*bytes)/4<clusters+2||root<2||root>=clusters+2)return B_DENIED;
    unsigned active=(flags&128)?flags&15:0;if(active>=fats)return B_DENIED;*v=(volume){disk,bytes,spc,fats,active,!(flags&128),start,total,start+reserved,start+metadata,fat_size,clusters,root};return B_OK;
}
static unsigned next_cluster(volume *v,unsigned cluster,unsigned *next){
    if(cluster<2||cluster>=v->clusters+2)return B_DENIED;uint64_t sector_index=(uint64_t)cluster*4/v->bytes;unsigned offset=cluster*4%v->bytes;if(storage_read(v->disk,v->fat_start+(uint64_t)v->fat_index*v->fat_sectors+sector_index,fat_sector))return B_IO;unsigned value=r32(fat_sector+offset)&0x0fffffff;
    if(v->mirrored&&v->fats==2){if(storage_read(v->disk,v->fat_start+v->fat_sectors+sector_index,fat_sector))return B_IO;if((r32(fat_sector+offset)&0x0fffffff)!=value)return B_DENIED;}
    if(value>=0x0ffffff8){*next=0;return B_OK;}if(value<2||value>=v->clusters+2)return B_DENIED;*next=value;return B_OK;
}
static unsigned checksum(const uint8_t *p){unsigned sum=0;for(unsigned i=0;i<11;++i)sum=(((sum&1)<<7)|(sum>>1))+p[i],sum&=255;return sum;}
static int equal(const char *a,const char *b){while(*a&&*b){unsigned x=(uint8_t)*a++,y=(uint8_t)*b++;if(x>='a'&&x<='z')x-=32;if(y>='a'&&y<='z')y-=32;if(x!=y)return 0;}return *a==*b;}
static unsigned long_name(const uint16_t units[260],char name[256]){
    unsigned out=0,ended=0;for(unsigned i=0;i<260;++i){unsigned c=units[i];if(!c){ended=1;break;}if(c==65535){if(i&&i%13==0){ended=1;break;}return B_DENIED;}if(c>=0xd800&&c<=0xdbff){if(i==259||units[i+1]<0xdc00||units[i+1]>0xdfff)return B_DENIED;c=0x10000+((c-0xd800)<<10)+(units[++i]-0xdc00);}else if(c>=0xdc00&&c<=0xdfff)return B_DENIED;
        if(c<32||c=='/'||c=='\\')return B_DENIED;unsigned bytes=c<128?1:c<2048?2:c<65536?3:4;if(out+bytes>255)return B_UNSUPPORTED;if(bytes==1)name[out++]=(char)c;else{unsigned shift=(bytes-1)*6;name[out++]=(char)((bytes==2?0xc0:bytes==3?0xe0:0xf0)|(c>>shift));while(shift){shift-=6;name[out++]=(char)(0x80|((c>>shift)&63));}}}
    if(!out||!ended)return B_DENIED;name[out]=0;return B_OK;
}
static unsigned directory(volume *v,unsigned cluster,const char *wanted,uint32_t *file_cluster,uint32_t *file_size,unsigned *attribute){
    uint16_t units[260];unsigned expected=0,lfn_sum=0,have_lfn=0;memset(units,255,sizeof(units));
    unsigned slow=cluster,fast=cluster;
    for(unsigned steps=0;cluster&&steps<128;++steps){if(cluster<2||cluster>=v->clusters+2)return B_DENIED;
        for(unsigned j=0;j<v->spc;++j){if(storage_read(v->disk,v->data_start+(uint64_t)(cluster-2)*v->spc+j,sector))return B_IO;
            for(unsigned off=0;off<v->bytes;off+=32){const uint8_t *entry=sector+off;unsigned first=entry[0],attr=entry[11];if(!first)return wanted?B_ABSENT:B_OK;if(first==0xe5){expected=have_lfn=0;continue;}
                if(attr==15){unsigned number=first&31;if(!number||number>20||(first&0xa0)||entry[12]||r16(entry+26))return B_DENIED;if(first&64){memset(units,255,sizeof(units));expected=number;lfn_sum=entry[13];have_lfn=1;}if(!have_lfn||number!=expected||entry[13]!=lfn_sum)return B_DENIED;
                    static const uint8_t offsets[13]={1,3,5,7,9,14,16,18,20,22,24,28,30};for(unsigned i=0;i<13;++i)units[(number-1)*13+i]=r16(entry+offsets[i]);--expected;continue;}
                if(attr&8){expected=have_lfn=0;continue;}char name[256];if(have_lfn){if(expected||checksum(entry)!=lfn_sum)return B_DENIED;unsigned s=long_name(units,name);if(s)return s;}
                else{unsigned n=0;for(unsigned i=0;i<8&&entry[i]!=' ';++i){if(entry[i]>=128)return B_UNSUPPORTED;name[n++]=(char)entry[i];}if(entry[8]!=' '){name[n++]='.';for(unsigned i=8;i<11&&entry[i]!=' ';++i){if(entry[i]>=128)return B_UNSUPPORTED;name[n++]=(char)entry[i];}}name[n]=0;}
                expected=have_lfn=0;if(wanted&&equal(name,wanted)){*file_cluster=((uint32_t)r16(entry+20)<<16)|r16(entry+26);*file_size=r32(entry+28);*attribute=attr;return B_OK;}if(!wanted){
#ifdef ENGINE_M10
 if(!emit10(name,attr&16?2:1,r32(entry+28)))
#endif
 {console_text(name);console_text(attr&16?"/\n":"\n");}}
            }
        }
        unsigned s=next_cluster(v,cluster,&cluster);if(s)return s;if(slow&&(s=next_cluster(v,slow,&slow)))return s;if(fast&&(s=next_cluster(v,fast,&fast)))return s;if(fast&&(s=next_cluster(v,fast,&fast)))return s;if(slow&&fast&&slow==fast)return B_DENIED;
    }return cluster?B_UNSUPPORTED:wanted?B_ABSENT:B_OK;
}
unsigned fat_list(unsigned disk,unsigned part){volume v;unsigned s=mount(disk,part,&v);if(s)return s;return directory(&v,v.root,0,0,0,0);}
unsigned fat_read(unsigned disk,unsigned part,const char *path,void *buffer,unsigned capacity,unsigned *size){
    if(!path||path[0]!='/')return B_DENIED;volume v;unsigned s=mount(disk,part,&v);if(s)return s;unsigned total=0;while(path[total])if(++total>255)return B_DENIED;unsigned start=1,cluster=v.root,file_size=0,attribute=16;
    for(unsigned i=1;i<=total;++i)if(path[i]=='/'||!path[i]){unsigned length=i-start;char component[256];if(!length||length>255)return B_DENIED;memcpy(component,path+start,length);component[length]=0;if(equal(component,".")||equal(component,".."))return B_DENIED;unsigned next;if((s=directory(&v,cluster,component,&next,&file_size,&attribute)))return s;cluster=next;if(path[i]&&!(attribute&16))return B_DENIED;start=i+1;}
    if(attribute&16)return B_DENIED;if(file_size>capacity)return B_DENIED;*size=file_size;unsigned copied=0,slow=cluster,fast=cluster;
    while(copied<file_size){if(cluster<2||cluster>=v.clusters+2)return B_DENIED;for(unsigned j=0;j<v.spc&&copied<file_size;++j){if(storage_read(disk,v.data_start+(uint64_t)(cluster-2)*v.spc+j,sector))return B_IO;unsigned bytes=file_size-copied;if(bytes>v.bytes)bytes=v.bytes;memcpy((uint8_t*)buffer+copied,sector,bytes);copied+=bytes;}
        if((s=next_cluster(&v,cluster,&cluster)))return s;if(copied<file_size){if(slow&&(s=next_cluster(&v,slow,&slow)))return s;if(fast&&(s=next_cluster(&v,fast,&fast)))return s;if(fast&&(s=next_cluster(&v,fast,&fast)))return s;if(slow&&fast&&slow==fast)return B_DENIED;}
    }return cluster?B_DENIED:B_OK;
}

#ifdef ENGINE_M10
#include "fat32-write10.inc"
#endif
