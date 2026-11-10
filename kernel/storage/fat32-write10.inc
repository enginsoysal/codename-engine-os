/* Serialized desktop broker only. Create-only FAT32 transfers publish the
 * directory entry last. An interrupted write leaves the volume dirty, never
 * advertises safe removal, and requires offline checking before more writes. */
static unsigned exact10(const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
static rem10_entry *listing10;static unsigned listed10,skip10,capacity10;
static unsigned emit10(const char*name,unsigned kind,unsigned size){
 if(!listing10)return 0;if(equal(name,".")||equal(name,".."))return 1;
 if(listed10>=skip10&&listed10-skip10<capacity10){rem10_entry*e=&listing10[listed10-skip10];memset(e,0,sizeof(*e));unsigned n=0;while(name[n]&&n<255){e->name[n]=name[n];++n;}e->kind=kind;e->size=size;}++listed10;return 1;
}
static unsigned lease10(unsigned disk,unsigned token){const storage_disk*d=storage_get(disk);return d&&d->readable&&d->transport==STORAGE_USB&&d->context==token;}
static unsigned resolve10(volume*v,const char*path,unsigned*cluster,unsigned*size,unsigned*attr){
 if(!path||path[0]!='/')return B_DENIED;unsigned total=0;while(path[total])if(++total>255)return B_DENIED;*cluster=v->root;*size=0;*attr=16;
 if(total==1)return B_OK;for(unsigned start=1,i=1;i<=total;++i)if(!path[i]||path[i]=='/'){unsigned n=i-start;if(!n||n>255||!(*attr&16))return B_DENIED;char name[256];memcpy(name,path+start,n);name[n]=0;if(equal(name,".")||equal(name,".."))return B_DENIED;unsigned status=directory(v,*cluster,name,cluster,size,attr);if(status)return status;start=i+1;}return B_OK;
}
unsigned rem10_volume(unsigned disk,unsigned*part,unsigned*token){
 const storage_disk*d=storage_get(disk);if(!d||!d->readable||d->transport!=STORAGE_USB)return B_ABSENT;volume v;*token=d->context;
 if(!mount(disk,UINT32_MAX,&v)){*part=UINT32_MAX;return B_OK;}
 for(unsigned i=0;i<16;++i)if(!mount(disk,i,&v)){*part=i;return B_OK;}return B_UNSUPPORTED;
}
unsigned rem10_list(unsigned disk,unsigned token,unsigned part,const char*path,unsigned offset,rem10_entry*out,unsigned cap,unsigned*total){
 if(!lease10(disk,token))return B_ABSENT;if(!out||!cap||cap>16||!total)return B_DENIED;volume v;unsigned status=mount(disk,part,&v),cluster,size,attr;if(status)return status;if((status=resolve10(&v,path,&cluster,&size,&attr)))return status;if(!(attr&16))return B_DENIED;
 listing10=out;listed10=0;skip10=offset;capacity10=cap;status=directory(&v,cluster,0,0,0,0);*total=listed10;listing10=0;return status;
}
static unsigned write_sector10(volume*v,uint64_t lba,const void*data){if(lba<v->start||lba>=v->start+v->count)return B_DENIED;return storage_device_io(v->disk,lba,(void*)data,v->bytes,1);}
static unsigned fat_value10(volume*v,unsigned cluster,unsigned*value){
 if(cluster>=v->clusters+2)return B_DENIED;uint64_t at=v->fat_start+(uint64_t)cluster*4/v->bytes;unsigned off=cluster*4%v->bytes;
 if(storage_read(v->disk,at,sector))return B_IO;*value=r32(sector+off)&0x0fffffff;if(storage_read(v->disk,at+v->fat_sectors,fat_sector))return B_IO;return (r32(fat_sector+off)&0x0fffffff)==*value?B_OK:B_DENIED;
}
static void w32_10(uint8_t*p,unsigned value){memcpy(p,&value,4);}
static unsigned fat_set10(volume*v,unsigned cluster,unsigned value){
 uint64_t at=v->fat_start+(uint64_t)cluster*4/v->bytes;unsigned off=cluster*4%v->bytes;
 if(storage_read(v->disk,at,sector)||storage_read(v->disk,at+v->fat_sectors,fat_sector)||memcmp(sector,fat_sector,v->bytes))return B_IO;
 unsigned before=r32(sector+off);w32_10(sector+off,(before&0xf0000000)|(value&0x0fffffff));return write_sector10(v,at,sector)||write_sector10(v,at+v->fat_sectors,sector)?B_IO:B_OK;
}
typedef struct {uint64_t sector;unsigned offset;} dir_slot10;
static unsigned slots10(volume*v,unsigned cluster,unsigned needed,dir_slot10 slots[7],unsigned*end_marker){
 unsigned used=0,ended=0;
 for(unsigned steps=0;cluster&&steps<128;++steps){for(unsigned j=0;j<v->spc;++j){uint64_t at=v->data_start+(uint64_t)(cluster-2)*v->spc+j;if(storage_read(v->disk,at,sector))return B_IO;for(unsigned off=0;off<v->bytes;off+=32){if(!sector[off])ended=1;if(ended||sector[off]==0xe5){slots[used++]=(dir_slot10){at,off};if(used==needed){*end_marker=ended;return B_OK;}}else used=0;}}unsigned status=next_cluster(v,cluster,&cluster);if(status)return status;}return B_UNSUPPORTED;
}
static unsigned entry_write10(volume*v,dir_slot10 slot,const uint8_t data[32]){if(storage_read(v->disk,slot.sector,sector))return B_IO;memcpy(sector+slot.offset,data,32);return write_sector10(v,slot.sector,sector);}
static unsigned writable10(volume*v){
 const storage_disk*d=storage_get(v->disk);unsigned flags;if(v->bytes!=512||v->fats!=2||!v->mirrored||!storage_writable(d)||fat_value10(v,1,&flags))return B_UNSUPPORTED;if((flags&0x0c000000)!=0x0c000000)return B_DENIED;
 /* Unknown free-space hints are valid and avoid stale advisory accounting. */
 if(storage_read(v->disk,v->start,sector))return B_IO;unsigned info=r16(sector+48),backup=r16(sector+50),reserved=r16(sector+14);if(!info||info>=reserved||backup>=reserved)return B_UNSUPPORTED;
 for(unsigned copy=0;copy<2;++copy){unsigned index=info+(copy?backup:0);if(index>=reserved)return B_DENIED;uint64_t at=v->start+index;if(storage_read(v->disk,at,sector))return B_IO;if(r32(sector)!=0x41615252||r32(sector+484)!=0x61417272||r32(sector+508)!=0xaa550000)return B_DENIED;w32_10(sector+488,UINT32_MAX);w32_10(sector+492,UINT32_MAX);if(write_sector10(v,at,sector))return B_IO;}
 return fat_set10(v,1,flags&~0x08000000u)||storage_device_flush(v->disk)?B_IO:B_OK;
}
static unsigned clean10(volume*v){unsigned flags;if(fat_value10(v,1,&flags))return B_IO;return storage_device_flush(v->disk)||fat_set10(v,1,flags|0x08000000u)||storage_device_flush(v->disk)?B_IO:B_OK;}
static unsigned free_run10(volume*v,unsigned needed,unsigned*first){
 if(!needed){*first=0;return B_OK;}unsigned run=0;uint64_t limit=v->clusters+2,cached=UINT64_MAX;if(limit>1048578)limit=1048578;
 for(unsigned cluster=2;cluster<limit;++cluster){uint64_t index=(uint64_t)cluster*4/v->bytes;if(index!=cached){if(storage_read(v->disk,v->fat_start+index,sector)||storage_read(v->disk,v->fat_start+v->fat_sectors+index,fat_sector))return B_IO;if(memcmp(sector,fat_sector,v->bytes))return B_DENIED;cached=index;}unsigned value=r32(sector+cluster*4%v->bytes)&0x0fffffff;if(!value){if(!run)*first=cluster;if(++run==needed)return B_OK;}else run=0;}return B_UNSUPPORTED;
}
static unsigned chain10(volume*v,unsigned first,unsigned count,unsigned allocated){
 for(unsigned n=0;n<count;){unsigned cluster=first+n,off=cluster*4%v->bytes;uint64_t at=v->fat_start+(uint64_t)cluster*4/v->bytes;if(storage_read(v->disk,at,sector)||storage_read(v->disk,at+v->fat_sectors,fat_sector)||memcmp(sector,fat_sector,v->bytes))return B_IO;
  while(n<count&&off<v->bytes){unsigned value=allocated?(n+1<count?first+n+1:0x0fffffff):0;w32_10(sector+off,(r32(sector+off)&0xf0000000)|value);++n;off+=4;}if(write_sector10(v,at,sector)||write_sector10(v,at+v->fat_sectors,sector))return B_IO;
 }return B_OK;
}
static unsigned alias_used10(volume*v,unsigned cluster,const uint8_t alias[11]){
 for(unsigned steps=0;cluster&&steps<128;++steps){for(unsigned j=0;j<v->spc;++j){if(storage_read(v->disk,v->data_start+(uint64_t)(cluster-2)*v->spc+j,sector))return B_IO;for(unsigned off=0;off<v->bytes;off+=32){if(!sector[off])return B_ABSENT;if(sector[off]!=0xe5&&sector[off+11]!=15&&!memcmp(sector+off,alias,11))return B_OK;}}unsigned status=next_cluster(v,cluster,&cluster);if(status)return status;}return cluster?B_DENIED:B_ABSENT;
}
static unsigned source_native10(void*context,uint64_t offset,void*data,unsigned bytes){unsigned got=0;return v6_read(m6_volume(),context,offset,data,bytes,&got)||got!=bytes;}
static unsigned name10(const char*name,unsigned*size){unsigned n=0;if(!name||!name[0]||equal(name,".")||equal(name,".."))return B_DENIED;while(name[n]){unsigned c=(uint8_t)name[n];if(n==63||c<32||c>126||c=='/'||c=='\\'||c=='"'||c=='*'||c==':'||c=='<'||c=='>'||c=='?'||c=='|')return B_UNSUPPORTED;++n;}if(name[n-1]==' '||name[n-1]=='.')return B_DENIED;*size=n;return B_OK;}
unsigned rem10_export(unsigned disk,unsigned token,unsigned part,const char*folder,const char*native){
 if(!lease10(disk,token))return B_ABSENT;const char*name=native;for(unsigned i=0;native[i];++i)if(native[i]=='/')name=native+i+1;unsigned length;if(name10(name,&length))return B_UNSUPPORTED;
 v6_volume*root=m6_volume();const v6_entry*file=0;for(unsigned i=0;i<V6_ENTRIES;++i)if(root->entries[i].kind==1&&exact10(root->entries[i].path,native)){file=&root->entries[i];break;}if(!file||file->size>64*1024*1024||v6_verify(root,native))return B_DENIED;
 volume v;unsigned status=mount(disk,part,&v),dir,ignored,attr;if(status)return status;if((status=resolve10(&v,folder,&dir,&ignored,&attr))||!(attr&16))return status?status:B_DENIED;
 if(directory(&v,dir,name,&ignored,&ignored,&attr)!=B_ABSENT)return B_DENIED;
 unsigned lfn=(length+12)/13,entries=lfn+1,end_marker=0;dir_slot10 slots[7];if((status=slots10(&v,dir,entries+1,slots,&end_marker)))return status;
 uint8_t alias[11];unsigned chosen=0;for(unsigned serial=1;serial<129&&!chosen;++serial){memset(alias,' ',11);memcpy(alias,"ENG",3);unsigned value=serial;for(int i=7;i>=3;--i){alias[i]=(uint8_t)('0'+value%10);value/=10;}memcpy(alias+8,"DAT",3);unsigned exists=alias_used10(&v,dir,alias);if(exists==B_ABSENT)chosen=1;else if(exists)return exists;}if(!chosen)return B_UNSUPPORTED;
 unsigned bytes_cluster=v.bytes*v.spc,clusters=(unsigned)((file->size+bytes_cluster-1)/bytes_cluster),first=0;if((status=free_run10(&v,clusters,&first)))return status;
 if((status=writable10(&v)))return status;if((status=chain10(&v,first,clusters,1)))return status;
 uint8_t data[4096];uint64_t offset=0;
 for(unsigned cluster=0;cluster<clusters&&!status;++cluster)for(unsigned sec=0;sec<v.spc&&!status;){unsigned n=v.spc-sec;if(n>8)n=8;unsigned bytes=n*v.bytes;memset(data,0,bytes);unsigned used=file->size>offset?(unsigned)(file->size-offset):0;if(used>bytes)used=bytes;if(used&&source_native10((void*)native,offset,data,used)){status=B_IO;break;}status=storage_device_io(disk,v.data_start+(uint64_t)(first+cluster-2)*v.spc+sec,data,bytes,1);offset+=used;sec+=n;}
 if(!status)status=storage_device_flush(disk);
 uint8_t e[32];memset(e,0,32);if(!status&&end_marker)status=entry_write10(&v,slots[entries],e);
 static const uint8_t offsets[13]={1,3,5,7,9,14,16,18,20,22,24,28,30};
 for(unsigned index=0;index<lfn&&!status;++index){unsigned number=lfn-index;memset(e,255,32);e[0]=(uint8_t)(number|(index?0:64));e[11]=15;e[12]=0;e[13]=(uint8_t)checksum(alias);e[26]=e[27]=0;for(unsigned j=0;j<13;++j){unsigned at=(number-1)*13+j;uint16_t c=at<length?(uint8_t)name[at]:at==length?0:65535;memcpy(e+offsets[j],&c,2);}status=entry_write10(&v,slots[index],e);}
 if(!status)status=storage_device_flush(disk);
 if(!status){memset(e,0,32);memcpy(e,alias,11);e[11]=32;uint16_t high=(uint16_t)(first>>16),low=(uint16_t)first;memcpy(e+20,&high,2);memcpy(e+26,&low,2);w32_10(e+28,(unsigned)file->size);status=entry_write10(&v,slots[lfn],e);}
 if(status)return status;status=clean10(&v);if(!status)serial_text("M10_USB_FILE_EXPORTED\n");return status;
}
typedef struct {volume v;unsigned cluster;uint64_t position,size;unsigned sector_index,slow,fast;} stream10;
static unsigned source_usb10(void*context,uint64_t offset,void*out,unsigned bytes){
 stream10*s=context;if(offset!=s->position||bytes>s->size-offset)return 1;uint8_t*data=out;while(bytes){if(s->cluster<2||s->cluster>=s->v.clusters+2)return 1;if(storage_read(s->v.disk,s->v.data_start+(uint64_t)(s->cluster-2)*s->v.spc+s->sector_index,sector))return 1;unsigned within=(unsigned)(s->position%s->v.bytes),n=s->v.bytes-within;if(n>bytes)n=bytes;memcpy(data,sector+within,n);s->position+=n;data+=n;bytes-=n;if(!(s->position%s->v.bytes)&&++s->sector_index==s->v.spc){s->sector_index=0;if(next_cluster(&s->v,s->cluster,&s->cluster))return 1;if(s->slow&&next_cluster(&s->v,s->slow,&s->slow))return 1;if(s->fast&&next_cluster(&s->v,s->fast,&s->fast))return 1;if(s->fast&&next_cluster(&s->v,s->fast,&s->fast))return 1;if(s->slow&&s->fast&&s->slow==s->fast)return 1;}}return 0;
}
unsigned rem10_import(unsigned disk,unsigned token,unsigned part,const char*path,const char*native){
 if(!lease10(disk,token))return B_ABSENT;v6_volume*root=m6_volume();for(unsigned i=0;i<V6_ENTRIES;++i)if(root->entries[i].kind&&exact10(root->entries[i].path,native))return B_DENIED;
 stream10 stream={0};unsigned status=mount(disk,part,&stream.v),size,attr;if(status)return status;if((status=resolve10(&stream.v,path,&stream.cluster,&size,&attr)))return status;if(attr&16||size>64*1024*1024)return B_UNSUPPORTED;stream.size=size;stream.slow=stream.fast=stream.cluster;status=v6_put(root,native,size,source_usb10,&stream);if(!status)serial_text("M10_USB_FILE_IMPORTED\n");return status;
}
