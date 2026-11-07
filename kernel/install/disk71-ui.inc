/* Included by the trusted wizard; all destructive work is behind a review screen. */
static unsigned u71_mode,u71_disk,u71_row=UINT32_MAX,u71_page,u71_op,u71_consent;
static unsigned replace_selected,replace_consent,disk_page;
static storage_disk u71_target;static d71_device u71_device;static d71_disk u71_state;static d71_plan u71_plan;
static char u71_name[37]="Data",u71_size[16]="64";static const char*u71_notice;
static unsigned u71_read(void*c,uint64_t l,void*p,unsigned n){(void)c;const storage_disk*d=storage_get(u71_disk);if(!d||memcmp(d,&u71_target,sizeof(*d))||d->transport!=STORAGE_AHCI||!d->readable)return 1;uint8_t*b=p;while(n){unsigned bytes=n>=4096?4096:512;if(m5_sata_transfer(d->context,l,b,bytes,0))return 1;l+=bytes/512;b+=bytes;n-=bytes;}return 0;}
static unsigned u71_write(void*c,uint64_t l,void*p,unsigned n){(void)c;const storage_disk*d=storage_get(u71_disk);if(!d||memcmp(d,&u71_target,sizeof(*d))||d->transport!=STORAGE_AHCI||!d->readable)return 1;uint8_t*b=p;while(n){unsigned bytes=n>=4096?4096:512;if(m5_sata_transfer(d->context,l,b,bytes,1))return 1;l+=bytes/512;b+=bytes;n-=bytes;}return 0;}
static unsigned u71_flush(void*c){(void)c;const storage_disk*d=storage_get(u71_disk);return !d||memcmp(d,&u71_target,sizeof(*d))?1:m5_sata_flush(d->context);}
static void u71_refresh(void){u71_row=UINT32_MAX;u71_page=0;const storage_disk*d=storage_get(u71_disk);memset(&u71_state,0,sizeof(u71_state));if(!d){u71_notice="No disk selected.";return;}u71_target=*d;u71_device=(d71_device){0,u71_read,u71_write,u71_flush,d->sectors,d->sector_bytes};unsigned s=d71_scan(&u71_device,&u71_state);if(s)u71_notice=d71_error(s);}
static void u71_open(unsigned disk){if(screen->framebuffer_width<1024||screen->framebuffer_height<768){validation="Disk manager needs a 1024 x 768 or larger display.";return;}m6_cancel();u71_disk=disk;u71_mode=1;u71_notice=0;u71_refresh();focus=0;}
static const char*u71_scheme(void){switch(u71_state.scheme){case D71_BLANK:return "Blank boot metadata";case D71_GPT:return u71_state.editable?"GPT - verified mirrors":"GPT - needs repair / read-only";case D71_MBR:return u71_state.editable?"MBR - primary partitions":"MBR - damaged / read-only";case D71_DAMAGED:return "Damaged or unsupported GPT";default:return "Unknown disk contents";}}
static const char*u71_title(unsigned op){switch(op){case D71_ERASE:return "Clear ALL partition metadata";case D71_INITIALIZE:return "Create an empty GPT table";case D71_CREATE:return "Create a data partition";case D71_DELETE:return "Delete this partition";case D71_FAT32:return "Quick format as FAT32";case D71_NATIVE:return "Quick format as Engine data";case D71_REPAIR_PRIMARY:return "Rebuild GPT from primary copy";default:return "Rebuild GPT from backup copy";}}
static void u71_identity(unsigned x,unsigned y){char b[32]="Disk ";decimal((uint64_t)u71_disk+1,b+5);label(x,y,b,2,0xffffff);textnum(x+125,y,u71_target.sectors/2048," MiB");textnum(x,y+28,u71_target.bdf," controller BDF");textnum(x+250,y+28,u71_target.unit," port/unit");}
static void u71_draw(unsigned x,unsigned y){
 label(x,y,u71_mode==1?"Manage disks.":u71_mode==2?"New partition.":u71_mode==3?"Choose a format.":"Review disk changes.",3,0xffffff);u71_identity(x,y+48);
 if(u71_mode==1){label(x,y+104,u71_scheme(),1,0x9fb2ba);button(x+430,y+45,60,"<",201,0);button(x+500,y+45,60,">",202,0);
  unsigned total=u71_state.count+u71_state.gaps;
  for(unsigned row=0;row<4;++row){unsigned item=u71_page*4+row;if(item>=total)break;unsigned pos=y+128+row*43;char name[64];uint64_t first,last;
   if(item<u71_state.count){const d71_partition*p=u71_state.parts+item;memcpy(name,"Partition ",10);decimal(p->index+1,name+10);first=p->first;last=p->last;button(x,pos,200,name,300+row,u71_row==item);label(x+210,pos+5,p->name,1,0xa5bac2);label(x+210,pos+22,p->protected?"Protected system/extended":p->format,1,0xa5bac2);}
   else {d71_gap g=u71_state.free[item-u71_state.count];first=g.first;last=g.last;button(x,pos,200,"Free space",300+row,u71_row==item);}
   textnum(x+440,pos+10,(last-first+1)/2048," MiB");
  }
  button(x,y+306,80,"<",203,0);button(x+90,y+306,80,">",204,0);button(x+185,y+306,180,"Refresh",205,0);
  button(x,y+357,175,"New partition",210,0);button(x+185,y+357,175,"Delete",211,0);button(x+370,y+357,175,"Format",212,0);
  button(x,y+408,175,"New GPT",213,0);button(x+185,y+408,175,"Clear disk",214,0);button(x+370,y+408,175,"Repair GPT",215,0);
  if(u71_notice)label(x,y+460,u71_notice,1,0xf4b29d);
 }else if(u71_mode==2){label(x,y+120,"Size in MiB (digits; use Backspace to replace)",1,0x9fb2ba);button(x,y+150,210,u71_size,220,0);button(x+225,y+150,160,"Use all free",221,0);label(x,y+215,"Partition name (up to 36 ASCII characters)",1,0x9fb2ba);button(x,y+245,380,u71_name,222,0);label(x,y+310,"Creates an unformatted data partition; other partitions stay unchanged.",1,0xa5bac2);button(x,y+360,220,"Review creation",223,1);if(u71_notice)label(x,y+425,u71_notice,1,0xf4b29d);
 }else if(u71_mode==3){label(x,y+120,"Formatting removes files from the selected partition only.",1,0xf4b29d);button(x,y+170,300,"FAT32 (64 MiB minimum)",224,0);button(x,y+230,300,"Engine data (1 MiB minimum)",225,0);label(x,y+305,"Quick format is NOT secure erasure. No resize or conversion.",1,0xa5bac2);if(u71_notice)label(x,y+355,u71_notice,1,0xf4b29d);
 }else if(u71_mode==5){label(x,y+120,"Choose the verified table copy to keep. Filesystem data is not written.",1,0x9fb2ba);label(x,y+150,"If both copies disagree, choosing the wrong one can hide partitions.",1,0xf4b29d);button(x,y+210,240,"Use primary GPT",229,0);button(x,y+270,240,"Use backup GPT",230,0);if(u71_notice)label(x,y+350,u71_notice,1,0xf4b29d);
 }else {label(x,y+110,u71_title(u71_op),2,0xf4b29d);if(u71_op==D71_ERASE)label(x,y+155,"ALL partitions on this disk become inaccessible.",1,0xf4b29d);else if(u71_op==D71_DELETE||u71_op==D71_FAT32||u71_op==D71_NATIVE){textnum(x,y+155,u71_plan.index+1," - Partition number");textnum(x,y+190,u71_plan.first," first LBA");textnum(x,y+225,u71_plan.last," last LBA");label(x,y+264,"Only this partition is changed. Existing files will be lost.",1,0xf4b29d);}else label(x,y+160,"Review the selected disk and operation before writing.",1,0xa5bac2);
  label(x,y+300,"Quick operations do not securely erase old file contents.",1,0xa5bac2);button(x,y+395,160,"Keep / Cancel",227,1);button(x,y+340,410,u71_consent?"[X] I understand these disk changes":"[ ] I understand these disk changes",226,0);button(x+180,y+395,200,"Apply changes",228,0);if(u71_notice)label(x,y+460,u71_notice,1,0xf4b29d);
 }
 button(x,bottom+14,185,u71_mode==1?"Back to setup":"Back",200,0);
}
static void u71_review(unsigned op){uint8_t nonce[16];uint32_t low,high;__asm__ volatile("rdtsc":"=a"(low),"=d"(high));memcpy(nonce,&low,4);memcpy(nonce+4,&high,4);memcpy(nonce+8,&u71_state.fingerprint,4);unsigned serial=u71_state.count+u71_disk+1;memcpy(nonce+12,&serial,4);
 unsigned index=u71_row<u71_state.count?u71_state.parts[u71_row].index:u71_row-u71_state.count;uint64_t size=0;for(unsigned i=0;u71_size[i];++i){if(size>UINT64_C(2000000000)){u71_notice="Size exceeds disk limits.";return;}size=size*10+(unsigned)(u71_size[i]-'0');}
 unsigned s=d71_prepare(&u71_device,&u71_state,op,index,size,u71_name,nonce,&u71_plan);if(s){u71_notice=d71_error(s);return;}u71_op=op;u71_mode=4;u71_consent=0;u71_notice=0;focus=0;serial_text("M71_DISK_REVIEW op=");serial_hex(op);serial_text("\n");
}
static unsigned u71_action(unsigned a){if(!u71_mode)return 0;
 if(a==200){if(u71_mode==1){u71_mode=0;chosen=u71_disk;replace_selected=replace_consent=0;}else u71_mode=1;u71_notice=0;focus=0;}
 else if(a==201||a==202){unsigned n=storage_count();if(n){u71_disk=(u71_disk+n+(a==201?-1:1))%n;u71_notice=0;u71_refresh();}}
 else if(a==203){if(u71_page)--u71_page;}
 else if(a==204){if((u71_page+1)*4<u71_state.count+u71_state.gaps)++u71_page;}
 else if(a==205){u71_notice=0;u71_refresh();}
 else if(a>=300&&a<304){u71_row=u71_page*4+a-300;u71_notice=0;}
 else if(a==210){if(u71_row<u71_state.count||u71_row>=u71_state.count+u71_state.gaps)u71_notice="Select a free-space row first.";else {u71_mode=2;memcpy(u71_size,"64",3);focus=0;}}
 else if(a==211||a==212){if(u71_row>=u71_state.count)u71_notice="Select a partition row first.";else if(a==211)u71_review(D71_DELETE);else u71_mode=3;}
 else if(a==213)u71_review(D71_INITIALIZE);else if(a==214)u71_review(D71_ERASE);
 else if(a==215){u71_notice="Use Primary or Backup below only if that copy is valid.";u71_mode=5;}
 else if(a==229)u71_review(D71_REPAIR_PRIMARY);else if(a==230)u71_review(D71_REPAIR_BACKUP);
 else if(a==221){if(u71_row>=u71_state.count&&u71_row<u71_state.count+u71_state.gaps){d71_gap g=u71_state.free[u71_row-u71_state.count];decimal((g.last-g.first+1)/2048,u71_size);}}
 else if(a==223)u71_review(D71_CREATE);else if(a==224)u71_review(D71_FAT32);else if(a==225)u71_review(D71_NATIVE);
 else if(a==226)u71_consent^=1;else if(a==227){u71_mode=1;u71_consent=0;u71_notice="Cancelled. Nothing was written.";}
 else if(a==228){if(!u71_consent)u71_notice="Confirm the checkbox first, or Keep / Cancel.";else {label(body,top+110,"Working. Keep the disk connected.",2,0xffffff);unsigned s=d71_apply(&u71_device,&u71_state,&u71_plan,m71_workspace());u71_mode=1;u71_consent=0;u71_refresh();u71_notice=d71_error(s);serial_text(s?"M71_DISK_FAILED code=":"M71_DISK_APPLIED op=");serial_hex(s?s:u71_op);serial_text("\n");}}
 else if(a!=220&&a!=222)return 1;
 render();return 1;
}
static unsigned u71_key(int key){if(!u71_mode)return 0;if(key==27){u71_mode=1;u71_consent=0;u71_notice="Cancelled. Nothing was written.";render();return 1;}if(u71_mode==2&&focus<controls){unsigned a=buttons[focus].action;char*p=a==220?u71_size:a==222?u71_name:0;if(p){unsigned n=0;while(p[n])++n;if(key==8&&n)p[n-1]=0;else if(n<(a==220?10u:36u)&&((a==220&&key>='0'&&key<='9')||(a==222&&key>=32&&key<=126))){p[n]=(char)key;p[n+1]=0;}render();return 1;}}return 0;}
