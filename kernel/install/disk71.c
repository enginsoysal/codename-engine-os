/* Bounded original GPT/primary-MBR editor. No device writes occur during scan/prepare. */
#include <engine/base.h>
#include <engine/install/disk71.h>
static uint8_t page[4096],verify_page[4096],table[16384],head[512];
static d71_disk fresh;
static const uint8_t basic[16]={0xa2,0xa0,0xd0,0xeb,0xe5,0xb9,0x33,0x44,0x87,0xc0,0x68,0xb6,0xb7,0x26,0x99,0xc7};
static const uint8_t native[16]={0x71,0x4e,0x47,0x45,0x2d,0x44,0x41,0x54,0x81,0x71,0x45,0x4e,0x47,0x49,0x4e,0x45};
static const uint8_t system_type[16]={0x2b,0xd4,0xc3,0xe3,0x47,0x1c,0x0e,0x5e,0x99,0xcb,0xea,0x67,0x1c,0x38,0x72,0xb3};
static uint32_t r32(const void*p){uint32_t v;memcpy(&v,p,4);return v;}
static uint64_t r64(const void*p){uint64_t v;memcpy(&v,p,8);return v;}
static void w16(void*p,uint16_t v){memcpy(p,&v,2);}static void w32(void*p,uint32_t v){memcpy(p,&v,4);}static void w64(void*p,uint64_t v){memcpy(p,&v,8);}
static unsigned zero(const void*p,unsigned n){const uint8_t*b=p;while(n--)if(*b++)return 0;return 1;}
static uint32_t crc_more(uint32_t c,const void*p,unsigned n){const uint8_t*b=p;while(n--){c^=*b++;for(unsigned j=0;j<8;++j)c=(c>>1)^(0xedb88320u&-(c&1));}return c;}
static uint32_t crc(const void*p,unsigned n){return ~crc_more(~0u,p,n);}
static unsigned eligible(const d71_device*d){return d&&d->read&&d->write&&d->flush&&d->sector_bytes==512&&d->sectors>=4096&&d->sectors<=UINT64_C(4294967296);}
static unsigned read_at(const d71_device*d,uint64_t lba,void*p,unsigned n){return !n||n>4096||n%512||lba>=d->sectors||n/512>d->sectors-lba?D71_INVALID:d->read(d->context,lba,p,n)?D71_IO:0;}
static unsigned write_at(const d71_device*d,uint64_t lba,void*p,unsigned n){if(!n||n>4096||n%512||lba>=d->sectors||n/512>d->sectors-lba)return D71_INVALID;return d->write(d->context,lba,p,n)||read_at(d,lba,verify_page,n)||memcmp(p,verify_page,n)?D71_IO:0;}
static unsigned fingerprint(const d71_device*d,unsigned*out){uint32_t c=~0u;for(unsigned i=0;i<97;++i){uint64_t lba=i<64?i:d->sectors-(i-63);if(read_at(d,lba,page,512))return D71_IO;c=crc_more(c,page,512);}*out=~c;return 0;}
static unsigned header_ok(const d71_device*d,uint8_t*h,unsigned copy){
 if(memcmp(h,"EFI PART",8)||r32(h+8)!=0x10000||r32(h+12)!=92||r32(h+20)||r64(h+24)!=(copy?d->sectors-1:1)||r64(h+32)!=(copy?1:d->sectors-1)||r64(h+72)!=(copy?d->sectors-33:2)||r32(h+80)!=128||r32(h+84)!=128||zero(h+56,16))return 0;
 uint64_t first=r64(h+40),last=r64(h+48);if(first<34||last>d->sectors-34||first>last)return 0;
 unsigned sum=r32(h+16);w32(h+16,0);unsigned valid=crc(h,92)==sum;w32(h+16,sum);return valid;
}
static unsigned entries_ok(const uint8_t*t,uint64_t first,uint64_t last){
 for(unsigned i=0;i<128;++i){const uint8_t*p=t+i*128;if(zero(p,16))continue;uint64_t a=r64(p+32),b=r64(p+40);if(zero(p+16,16)||a<first||b>last||a>b)return 0;
  for(unsigned j=0;j<i;++j){const uint8_t*q=t+j*128;if(zero(q,16))continue;if(!memcmp(p+16,q+16,16)||(a<=r64(q+40)&&r64(q+32)<=b))return 0;}}
 return 1;
}
static void describe(const d71_device*d,d71_partition*p){
 memcpy(p->format,"Unknown",8);if(!read_at(d,p->first,page,512)){
  if(!memcmp(page,"ENGINEV6",8))memcpy(p->format,"Engine",7);
  else if(page[510]==0x55&&page[511]==0xaa&&!memcmp(page+82,"FAT32   ",8))memcpy(p->format,"FAT32",6);
  else if(!memcmp(page+3,"NTFS    ",8))memcpy(p->format,"NTFS",5);
 }
}
unsigned d71_scan(const d71_device*d,d71_disk*out){
 if(!out)return D71_INVALID;memset(out,0,sizeof(*out));out->scheme=D71_UNKNOWN;if(!eligible(d))return D71_UNSUPPORTED;out->sectors=d->sectors;
 if(fingerprint(d,&out->fingerprint)||read_at(d,0,out->mbr,512))return D71_IO;
 unsigned protective=0;for(unsigned i=0;i<4;++i)if(out->mbr[446+i*16+4]==0xee)protective=1;
 unsigned signatures=0;for(unsigned copy=0;copy<2;++copy){uint8_t*h=out->headers[copy];if(read_at(d,copy?d->sectors-1:1,h,512))return D71_IO;if(!memcmp(h,"EFI PART",8))signatures=1;
  if(!header_ok(d,h,copy))continue;uint64_t at=r64(h+72);for(unsigned j=0;j<4;++j)if(read_at(d,at+j*8,out->tables[copy]+j*4096,4096))return D71_IO;
  if(crc(out->tables[copy],16384)==r32(h+88)&&entries_ok(out->tables[copy],r64(h+40),r64(h+48)))out->valid_copies|=1u<<copy;
 }
 if(out->valid_copies){out->scheme=D71_GPT;unsigned source=(out->valid_copies&1)?0:1;uint8_t*h=out->headers[source];out->first_usable=r64(h+40);out->last_usable=r64(h+48);
  out->editable=out->valid_copies==3&&!memcmp(out->tables[0],out->tables[1],16384)&&!memcmp(out->headers[0]+40,out->headers[1]+40,32);
  /* A hybrid MBR or a non-protective boot record is inspectable, not silently replaced. */
  unsigned mbr_entries=0;for(unsigned i=0;i<4;++i)if(out->mbr[446+i*16+4])++mbr_entries;
  if(!protective||mbr_entries!=1||out->mbr[510]!=0x55||out->mbr[511]!=0xaa)out->editable=0;
  for(unsigned i=0;i<128;++i){uint8_t*p=out->tables[source]+i*128;if(zero(p,16))continue;d71_partition*v=out->parts+out->count++;v->index=i;v->first=r64(p+32);v->last=r64(p+40);v->protected=(r64(p+48)&(UINT64_C(1)|(UINT64_C(1)<<60)))!=0;
   for(unsigned j=0;j<36;++j){unsigned c=p[56+j*2]|((unsigned)p[57+j*2]<<8);if(!c)break;v->name[j]=c>=32&&c<127?(char)c:'?';}
   if(!memcmp(p,system_type,16))out->owned=1;describe(d,v);
  }
 }else if(signatures||protective)out->scheme=D71_DAMAGED;
 else if(out->mbr[510]==0x55&&out->mbr[511]==0xaa){out->scheme=D71_MBR;out->editable=1;out->first_usable=2048;out->last_usable=d->sectors-1;
  for(unsigned i=0;i<4;++i){uint8_t*p=out->mbr+446+i*16;if(!p[4]&&!r32(p+12))continue;uint64_t a=r32(p+8),n=r32(p+12);if(!p[4]||!n||a<1||a>=d->sectors||n>d->sectors-a){out->editable=0;continue;}d71_partition*v=out->parts+out->count++;v->index=i;v->first=a;v->last=a+n-1;v->protected=p[4]==5||p[4]==15||p[4]==0x85;memcpy(v->name,"Primary partition",18);describe(d,v);
  }for(unsigned i=0;i<out->count;++i)for(unsigned j=0;j<i;++j)if(out->parts[i].first<=out->parts[j].last&&out->parts[j].first<=out->parts[i].last)out->editable=0;
 }else {unsigned allzero=1;for(unsigned i=0;i<64;++i){if(read_at(d,i,page,512))return D71_IO;if(!zero(page,512))allzero=0;}for(unsigned i=1;i<=33;++i){if(read_at(d,d->sectors-i,page,512))return D71_IO;if(!zero(page,512))allzero=0;}if(allzero)out->scheme=D71_BLANK;}
 for(unsigned i=0;i<out->count;++i){if(out->owned)out->parts[i].protected=1;for(unsigned j=i+1;j<out->count;++j)if(out->parts[j].first<out->parts[i].first){d71_partition temp=out->parts[i];out->parts[i]=out->parts[j];out->parts[j]=temp;}}
 if(out->scheme==D71_GPT||out->scheme==D71_MBR){uint64_t cursor=(out->first_usable+2047)&~UINT64_C(2047);for(unsigned i=0;i<=out->count;++i){uint64_t end=i<out->count?out->parts[i].first-1:out->last_usable;
   uint64_t aligned=((end+1)/2048)*2048;if(aligned>cursor){out->free[out->gaps++]=(d71_gap){cursor,aligned-1};}if(i<out->count)cursor=(out->parts[i].last+2048)&~UINT64_C(2047);}}
 return 0;
}
static const d71_partition*find(const d71_disk*s,unsigned index){for(unsigned i=0;i<s->count;++i)if(s->parts[i].index==index)return s->parts+i;return 0;}
static unsigned fat_geometry(uint64_t n,unsigned*spc,unsigned*fat,uint64_t*clusters){
 if(n<131072||n>UINT32_MAX)return D71_SMALL;unsigned c=1;while((n/c)>UINT64_C(0x0ffffff0)&&c<128)c*=2;
 uint64_t f=((n/c+2)*4+511)/512;for(unsigned i=0;i<32;++i){if(n<=32+2*f)return D71_SMALL;uint64_t k=(n-32-2*f)/c,needed=((k+2)*4+511)/512;if(needed<=f){if(k<65525||k>=0x0ffffff5)return D71_SMALL;*spc=c;*fat=(unsigned)f;*clusters=k;return 0;}f=needed;}return D71_INVALID;
}
unsigned d71_prepare(const d71_device*d,const d71_disk*s,unsigned op,unsigned index,uint64_t mib,const char*name,const uint8_t*nonce,d71_plan*p){
 if(!eligible(d)||!s||!p||s->sectors!=d->sectors)return D71_INVALID;memset(p,0,sizeof(*p));p->operation=op;p->index=index;if(nonce)memcpy(p->nonce,nonce,16);p->nonce[7]=(p->nonce[7]&15)|0x40;p->nonce[8]=(p->nonce[8]&63)|0x80;
 if(op==D71_ERASE)return 0;
 if(op==D71_INITIALIZE)return s->scheme==D71_BLANK?0:D71_PROTECTED;
 if(op==D71_REPAIR_PRIMARY||op==D71_REPAIR_BACKUP)return s->scheme==D71_GPT&&(s->valid_copies&(op==D71_REPAIR_PRIMARY?1:2))?0:D71_INVALID;
 if(!s->editable)return D71_PROTECTED;
 if(op==D71_CREATE){if(index>=s->gaps||!mib||mib>UINT64_MAX/2048)return D71_INVALID;d71_gap gap=s->free[index];uint64_t n=mib*2048;if(n>gap.last-gap.first+1)return D71_FULL;
  unsigned available=0;if(s->scheme==D71_MBR){for(unsigned i=0;i<4;++i)if(!s->mbr[446+i*16+4]&&!r32(s->mbr+446+i*16+12))available=1;}else {unsigned c=(s->valid_copies&1)?0:1;for(unsigned i=0;i<128;++i)if(zero(s->tables[c]+i*128,16))available=1;}
  if(!available)return D71_FULL;p->first=gap.first;p->last=gap.first+n-1;unsigned j=0;if(s->scheme==D71_MBR)return 0;if(name)for(;name[j]&&j<36;++j){if((unsigned char)name[j]<32||(unsigned char)name[j]>126)return D71_INVALID;p->name[j]=name[j];}if(name&&name[j])return D71_INVALID;return 0;
 }
 const d71_partition*v=find(s,index);if(!v)return D71_INVALID;if(v->protected)return D71_PROTECTED;p->first=v->first;p->last=v->last;
 if(op==D71_DELETE)return 0;
 if(op!=D71_FAT32&&op!=D71_NATIVE)return D71_INVALID;
 if(op==D71_NATIVE&&((p->first&7)||(p->last-p->first+1)/8<=V6_DATA_START))return D71_SMALL;
 if(op==D71_FAT32){unsigned c,f;uint64_t k;unsigned r=fat_geometry(p->last-p->first+1,&c,&f,&k);if(r)return r;}
 if(read_at(d,p->first,page,512))return D71_IO;p->boot_crc=crc(page,512);return 0;
}
static void make_header(const d71_device*d,const d71_disk*s,const uint8_t*guid,unsigned copy){memset(head,0,512);memcpy(head,"EFI PART",8);w32(head+8,0x10000);w32(head+12,92);w64(head+24,copy?d->sectors-1:1);w64(head+32,copy?1:d->sectors-1);w64(head+40,s?s->first_usable:2048);w64(head+48,s?s->last_usable:d->sectors-34);memcpy(head+56,guid,16);w64(head+72,copy?d->sectors-33:2);w32(head+80,128);w32(head+84,128);w32(head+88,crc(table,16384));w32(head+16,crc(head,92));}
static unsigned save_gpt(const d71_device*d,const d71_disk*s,const uint8_t*guid){
 for(unsigned order=0;order<2;++order){unsigned copy=!order;uint64_t at=copy?d->sectors-33:2;for(unsigned j=0;j<4;++j)if(write_at(d,at+j*8,table+j*4096,4096))return D71_IO;make_header(d,s,guid,copy);if(write_at(d,copy?d->sectors-1:1,head,512)||d->flush(d->context))return D71_IO;}
 return 0;
}
static unsigned format_fat(const d71_device*d,const d71_plan*p){unsigned spc,fat;uint64_t clusters,n=p->last-p->first+1;unsigned status=fat_geometry(n,&spc,&fat,&clusters);if(status)return status;
 memset(page,0,sizeof(page));if(write_at(d,p->first,page,512)||d->flush(d->context))return D71_IO;
 /* Clear only reserved sectors, allocation tables and the empty root directory. */
 uint64_t metadata=32+(uint64_t)fat*2+spc;for(uint64_t off=0;off<metadata;){unsigned count=metadata-off>8?8:(unsigned)(metadata-off);memset(page,0,sizeof(page));if(write_at(d,p->first+off,page,count*512))return D71_IO;off+=count;}
 memset(page,0,512);w32(page,0x0ffffff8);w32(page+4,0x0fffffff);w32(page+8,0x0fffffff);if(write_at(d,p->first+32,page,512)||write_at(d,p->first+32+fat,page,512))return D71_IO;
 memset(page,0,512);w32(page,0x41615252);w32(page+484,0x61417272);w32(page+488,(uint32_t)clusters-1);w32(page+492,3);w32(page+508,0xaa550000);if(write_at(d,p->first+1,page,512)||write_at(d,p->first+7,page,512))return D71_IO;
 memset(page,0,512);page[0]=0xeb;page[1]=0x58;page[2]=0x90;memcpy(page+3,"ENGINE71",8);w16(page+11,512);page[13]=(uint8_t)spc;w16(page+14,32);page[16]=2;page[21]=0xf8;w16(page+24,63);w16(page+26,255);w32(page+28,(uint32_t)p->first);w32(page+32,(uint32_t)n);w32(page+36,fat);w32(page+44,2);w16(page+48,1);w16(page+50,6);page[64]=0x80;page[66]=0x29;w32(page+67,crc(p->nonce,16));memcpy(page+71,"ENGINE DATA",11);memcpy(page+82,"FAT32   ",8);page[510]=0x55;page[511]=0xaa;
 if(write_at(d,p->first+6,page,512)||d->flush(d->context)||write_at(d,p->first,page,512)||d->flush(d->context))return D71_IO;return 0;
}
typedef struct {const d71_device*d;uint64_t first,blocks;} slice;
static unsigned slice_read(void*c,uint64_t b,void*p,unsigned n){slice*s=c;return n!=4096||b>=s->blocks?D71_INVALID:read_at(s->d,s->first+b*8,p,n);}
static unsigned slice_write(void*c,uint64_t b,void*p,unsigned n){slice*s=c;return n!=4096||b>=s->blocks?D71_INVALID:write_at(s->d,s->first+b*8,p,n);}
static unsigned slice_flush(void*c){slice*s=c;return s->d->flush(s->d->context);}
unsigned d71_apply(const d71_device*d,const d71_disk*s,const d71_plan*p,v6_volume*workspace){
 if(!eligible(d)||!s||!p||s->sectors!=d->sectors)return D71_INVALID;unsigned sum;if(fingerprint(d,&sum))return D71_IO;if(sum!=s->fingerprint)return D71_CHANGED;
 /* Re-run semantic validation; callers cannot forge an out-of-bounds plan. */
 d71_plan checked;uint64_t mib=p->last>=p->first?(p->last-p->first+1)/2048:0;unsigned status=d71_prepare(d,s,p->operation,p->index,mib,p->name,p->nonce,&checked);if(status)return status;
 if(memcmp(&checked,p,sizeof(checked)))return D71_CHANGED;
 if(p->operation==D71_ERASE){memset(page,0,sizeof(page));for(unsigned i=0;i<64;i+=8)if(write_at(d,i,page,4096))return D71_IO;for(unsigned i=1;i<=33;++i)if(write_at(d,d->sectors-i,page,512))return D71_IO;return d->flush(d->context)?D71_IO:0;}
 if(p->operation==D71_INITIALIZE){memset(table,0,sizeof(table));if(save_gpt(d,0,p->nonce))return D71_IO;memset(page,0,512);page[450]=0xee;w32(page+454,1);w32(page+458,d->sectors-1>UINT32_MAX?UINT32_MAX:(uint32_t)d->sectors-1);page[510]=0x55;page[511]=0xaa;if(write_at(d,0,page,512)||d->flush(d->context))return D71_IO;return 0;}
 if(p->operation==D71_REPAIR_PRIMARY||p->operation==D71_REPAIR_BACKUP){unsigned source=p->operation==D71_REPAIR_PRIMARY?0:1;memcpy(table,s->tables[source],sizeof(table));fresh.first_usable=r64(s->headers[source]+40);fresh.last_usable=r64(s->headers[source]+48);if(!entries_ok(table,fresh.first_usable,fresh.last_usable))return D71_INVALID;if(save_gpt(d,&fresh,s->headers[source]+56)||d71_scan(d,&fresh)||fresh.valid_copies!=3)return D71_IO;return 0;}
 if(p->operation==D71_FAT32||p->operation==D71_NATIVE){if(p->operation==D71_FAT32)status=format_fat(d,p);else {if(!workspace)return D71_INVALID;slice part={d,p->first,(p->last-p->first+1)/8};memset(workspace,0,sizeof(*workspace));workspace->context=&part;workspace->read=slice_read;workspace->write=slice_write;workspace->flush=slice_flush;workspace->blocks=part.blocks;status=v6_format(workspace);memset(workspace,0,sizeof(*workspace));}if(status)return D71_IO;}
 if(s->scheme==D71_MBR){memcpy(page,s->mbr,512);unsigned index=p->index;if(p->operation==D71_CREATE){for(index=0;index<4;++index)if(!page[446+index*16+4]&&!r32(page+446+index*16+12))break;if(index==4)return D71_FULL;}
  uint8_t*entry=page+446+index*16;if(p->operation==D71_DELETE)memset(entry,0,16);else if(p->operation==D71_CREATE){memset(entry,0,16);entry[1]=entry[5]=0xfe;entry[2]=entry[3]=entry[6]=entry[7]=0xff;entry[4]=0xda;w32(entry+8,(uint32_t)p->first);w32(entry+12,(uint32_t)(p->last-p->first+1));}else entry[4]=p->operation==D71_FAT32?0x0c:0xda;
  if(write_at(d,0,page,512)||d->flush(d->context))return D71_IO;return 0;
 }
 memcpy(table,s->tables[0],sizeof(table));unsigned index=p->index;if(p->operation==D71_CREATE){for(index=0;index<128;++index)if(zero(table+index*128,16))break;if(index==128)return D71_FULL;}
 uint8_t*entry=table+index*128;if(p->operation==D71_DELETE)memset(entry,0,128);else if(p->operation==D71_CREATE){memset(entry,0,128);memcpy(entry,native,16);memcpy(entry+16,p->nonce,16);w64(entry+32,p->first);w64(entry+40,p->last);for(unsigned i=0;p->name[i]&&i<36;++i)entry[56+i*2]=(uint8_t)p->name[i];}else memcpy(entry,p->operation==D71_FAT32?basic:native,16);
 if(!entries_ok(table,s->first_usable,s->last_usable))return D71_INVALID;status=save_gpt(d,s,s->headers[0]+56);if(status)return status;
 if(d71_scan(d,&fresh)||!fresh.editable)return D71_IO;return 0;
}
const char*d71_error(unsigned s){switch(s){case 0:return "Operation completed and verified.";case D71_IO:return "Disk I/O failed. Changes may be incomplete. Refresh before continuing.";case D71_INVALID:return "The partition choice or size is invalid.";case D71_UNSUPPORTED:return "Use a writable supported disk with 512-byte sectors (2 MiB to 2 TiB).";case D71_CHANGED:return "Disk contents changed. Nothing more was written. Refresh and review again.";case D71_PROTECTED:return "Protected system/extended partition or damaged table. Use whole-disk replacement only if intended.";case D71_FULL:return "Not enough contiguous free space or partition slots.";case D71_SMALL:return "Too small or unaligned. FAT32 needs 64 MiB; Engine needs 1 MiB aligned.";default:return "Unknown disk operation error.";}}
