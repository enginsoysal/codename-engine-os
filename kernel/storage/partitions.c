#include <engine/storage.h>
typedef struct {uint64_t start,end;unsigned type;} partition;
static uint8_t sector[4096],header_copy[4096];static partition found[128];static unsigned used;
static uint32_t r32(const void *p){uint32_t v;memcpy(&v,p,4);return v;}
static uint64_t r64(const void *p){uint64_t v;memcpy(&v,p,8);return v;}
static uint32_t crc_update(uint32_t crc,const uint8_t *p,unsigned count){for(unsigned i=0;i<count;++i){crc^=p[i];for(unsigned b=0;b<8;++b)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1)));}return crc;}
static unsigned append(uint64_t start,uint64_t end,unsigned type,uint64_t sectors){if(used==128||!start||start>end||end>=sectors)return B_DENIED;for(unsigned i=0;i<used;++i)if(start<=found[i].end&&found[i].start<=end)return B_DENIED;found[used++]=(partition){start,end,type};return B_OK;}
static unsigned gpt_header(unsigned disk,uint64_t where,uint64_t mirror,uint64_t *array,unsigned *entries,unsigned *size,uint32_t *crc,uint64_t *first,uint64_t *last,uint8_t guid[16]){
    const storage_disk *d=storage_get(disk);if(storage_read(disk,where,sector))return B_IO;if(memcmp(sector,"EFI PART",8)||r32(sector+8)!=0x10000||r32(sector+12)<92||r32(sector+12)>d->sector_bytes||r32(sector+20)||r64(sector+24)!=where||r64(sector+32)!=mirror)return B_DENIED;
    unsigned bytes=r32(sector+12);memcpy(header_copy,sector,bytes);memset(header_copy+16,0,4);if(m4_crc(header_copy,bytes)!=r32(sector+16))return B_DENIED;
    *array=r64(sector+72);*entries=r32(sector+80);*size=r32(sector+84);*crc=r32(sector+88);*first=r64(sector+40);*last=r64(sector+48);memcpy(guid,sector+56,16);
    if(!*entries||*entries>128||*size<128||*size>d->sector_bytes||(*size&(*size-1))||*first<2||*first>*last||*last>=d->sectors-1||!*array)return B_UNSUPPORTED;
    uint64_t blocks=((uint64_t)*entries**size+d->sector_bytes-1)/d->sector_bytes;if(*array>=d->sectors||blocks>d->sectors-*array)return B_DENIED;
    uint64_t end=*array+blocks-1;if((*array<=where&&where<=end)||(*array<=mirror&&mirror<=end)||(*array<=*last&&*first<=end))return B_DENIED;return B_OK;
}
static unsigned gpt_array(unsigned disk,uint64_t array,unsigned entries,unsigned size,uint32_t expected,uint64_t first,uint64_t last,unsigned collect){
    const storage_disk *d=storage_get(disk);unsigned remaining=entries*size,crc=UINT32_MAX,index=0;uint64_t lba=array;
    while(remaining){if(storage_read(disk,lba++,sector))return B_IO;unsigned bytes=remaining<d->sector_bytes?remaining:d->sector_bytes;crc=crc_update(crc,sector,bytes);
        for(unsigned off=0;off<bytes;off+=size,++index){unsigned nonzero=0;for(unsigned j=0;j<16;++j)nonzero|=sector[off+j];if(nonzero&&collect){uint64_t a=r64(sector+off+32),b=r64(sector+off+40);if(a<first||b>last||append(a,b,index+1,d->sectors))return B_DENIED;}}
        remaining-=bytes;
    }return ~crc==expected?B_OK:B_DENIED;
}
static void filesystem(unsigned disk,uint64_t first,uint64_t count){
    const storage_disk *d=storage_get(disk);if(storage_read(disk,first,sector)){console_text("read error");return;}
    if(!memcmp(sector+3,"NTFS    ",8)){console_text("NTFS - recognized, not mounted");return;}
    if(!memcmp(sector+3,"EXFAT   ",8)){console_text("exFAT - recognized, not mounted");return;}
    if(!memcmp(sector+82,"FAT32   ",8)){console_text("FAT32 candidate");return;}
    uint64_t offset=1024/d->sector_bytes;unsigned within=1024%d->sector_bytes;
    if(offset<count&&storage_read(disk,first+offset,sector)==B_OK&&sector[within+56]==0x53&&sector[within+57]==0xef){console_text("ext2-family candidate");return;}
    console_text("unknown or empty filesystem");
}
unsigned storage_partitions(unsigned disk){
    const storage_disk *d=storage_get(disk);if(!d)return B_DENIED;used=0;if(storage_read(disk,0,sector))return B_IO;
    if(sector[510]!=0x55||sector[511]!=0xaa){console_text("No MBR/GPT partition table - ");filesystem(disk,0,d->sectors);console_text("\n");return B_OK;}
    unsigned protective=0,other=0;for(unsigned i=0;i<4;++i){const uint8_t *p=sector+446+i*16;unsigned type=p[4];if(!type)continue;if(type==0xee){++protective;continue;}++other;if(type==5||type==15||type==0x85)return B_UNSUPPORTED;uint64_t start=r32(p+8),count=r32(p+12);if(!count||start>d->sectors||count>d->sectors-start||append(start,start+count-1,type,d->sectors))return B_DENIED;}
    if(protective){if(protective!=1||other||d->sectors<4)return B_DENIED;uint64_t array,backup_array,first,last,bfirst,blast;unsigned entries,size,bentries,bsize;uint32_t crc,bcrc;uint8_t guid[16],bguid[16];unsigned s=gpt_header(disk,1,d->sectors-1,&array,&entries,&size,&crc,&first,&last,guid);if(s)return s;
        if((s=gpt_header(disk,d->sectors-1,1,&backup_array,&bentries,&bsize,&bcrc,&bfirst,&blast,bguid)))return s;
        if(entries!=bentries||size!=bsize||crc!=bcrc||first!=bfirst||last!=blast||memcmp(guid,bguid,16)||array>=first||backup_array<=last)return B_DENIED;
        if((s=gpt_array(disk,array,entries,size,crc,first,last,1))||(s=gpt_array(disk,backup_array,entries,size,crc,first,last,0)))return s;console_text("GPT: ");
    }else console_text("MBR: ");
    console_decimal(used);console_text(" partitions\n");for(unsigned i=0;i<used;++i){console_text("  ");console_decimal(i);console_text(" LBA ");console_decimal(found[i].start);console_text("..");console_decimal(found[i].end);console_text(" - ");filesystem(disk,found[i].start,found[i].end-found[i].start+1);console_text("\n");}return B_OK;
}
unsigned storage_partition(unsigned disk,unsigned index,uint64_t *start,uint64_t *count){unsigned s=storage_partitions(disk);if(s)return s;if(index>=used)return B_DENIED;*start=found[index].start;*count=found[index].end-found[index].start+1;return B_OK;}
