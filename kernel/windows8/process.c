#include <engine/windows8.h>
#include <engine/install/m6.h>
#include <engine/elf.h>
#include <engine/m3.h>
#include <engine/sha256.h>
extern void *memmove(void*,const void*,size_t);
#define NX (UINT64_C(1)<<63)
#define OWN8 10000u
#define WRITE8_PAGES 2048u
#define TRAP8 (KERNEL_STACK+0x80000)
typedef struct {char path[256];uint64_t position,size;unsigned live,write,existed;uint32_t crc;} file8;
typedef struct {char path[256];uint64_t times[3];uint32_t attributes,reserved;} metadata8;
static struct {uint64_t magic;metadata8 records[128];} metadata8_db;
static unsigned metadata8_dirty,created_root8;
static uint64_t owned8[OWN8],cr3_8,pd8,entry8,write_pages8[WRITE8_PAGES];
static unsigned count8,writer8=UINT32_MAX,write_count8,ready8,quantum8,cancel8;
static file8 files8[32];static trap_frame context8;static _Alignas(16) uint8_t fx8[512];static uint8_t pic8;
static uint64_t old_gs8;static w8_mailbox input8;static w8_info info8;
static uint8_t io8[4096];
volatile unsigned w8_active;
uint64_t w8_saved_rsp,w8_kernel_cr3;
extern void w8_enter(trap_frame*,uint64_t);
extern NORETURN void w8_escape(void);
extern uint64_t kernel_entry_pt(void);
extern const char windows8_licenses[];
extern const uint8_t windows8_runtime_image[];extern const unsigned windows8_runtime_size;
static unsigned length8(const char*s){unsigned n=0;while(s[n])++n;return n;}
static int equal8(const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
static void copy8(char*out,const char*in,unsigned cap){unsigned n=0;while(n+1<cap&&in[n]){out[n]=in[n];++n;}out[n]=0;}
static int prefix8(const char*p,const char*root){unsigned n=length8(root);return n&&(!memcmp(p,root,n))&&(p[n]=='/'||!p[n]);}
static void peak8(void){if((uint64_t)count8+write_count8>info8.peak_pages)info8.peak_pages=(uint64_t)count8+write_count8;}
static uint64_t allocate8(void){if(count8==OWN8)return 0;uint64_t p=page_alloc();if(p)owned8[count8++]=p;peak8();return p;}
static uint64_t *pte8(uint64_t va,unsigned create){
 if(va<W8_BASE||va>=W8_LIMIT)return 0;uint64_t*pd=(void*)(uintptr_t)pd8;unsigned at=(unsigned)((va>>21)&511);uint64_t page=pd[at]&UINT64_C(0x000ffffffffff000);
 if(!page&&create){page=allocate8();if(!page)return 0;pd[at]=page|7;}return page?(uint64_t*)(uintptr_t)page+((va>>12)&511):0;
}
static unsigned map8(uint64_t va,uint64_t bytes,unsigned permissions){
 if(!bytes||(va&4095)||(bytes&4095)||va<W8_BASE||va>=W8_LIMIT||bytes>W8_LIMIT-va||bytes>32*1024*1024||(permissions!=1&&permissions!=3&&permissions!=5))return 1;
 for(uint64_t at=va;at<va+bytes;at+=4096){uint64_t*p=pte8(at,1);if(!p||*p)return 1;}
 for(uint64_t at=va;at<va+bytes;at+=4096){uint64_t*p=pte8(at,0),physical=allocate8();if(!physical)return 1;*p=physical|5|((permissions&2)?2:0)|((permissions&4)?0:NX);}
 return 0;
}
static unsigned user8(uint64_t va,void*buffer,unsigned bytes,unsigned to_user){
 if(va<W8_BASE||va>=W8_LIMIT||bytes>W8_LIMIT-va)return 1;uint8_t*b=buffer;
 while(bytes){uint64_t*p=pte8(va,0);if(!p||!(*p&1)||(to_user&&!(*p&2)))return 1;unsigned n=4096-(unsigned)(va&4095);if(n>bytes)n=bytes;void*physical=(void*)(uintptr_t)((*p&UINT64_C(0x000ffffffffff000))+(va&4095));
  if(to_user)memcpy(physical,b,n);else memcpy(b,physical,n);b+=n;va+=n;bytes-=n;}return 0;
}
static unsigned string8(uint64_t va,char*out,unsigned cap){for(unsigned i=0;i<cap;++i){if(user8(va+i,out+i,1,0))return 1;if(!out[i])return 0;}return 1;}
static unsigned path8(const char*p,unsigned write){
 if(p[0]!='/'||!p[1]||length8(p)>=255)return 0;unsigned component=0;
 for(unsigned i=1;p[i];++i){if((uint8_t)p[i]<32||p[i]=='\\')return 0;if(p[i]=='/'){if(!component)return 0;component=0;}else{if(++component>63)return 0;if(p[i]=='.'&&(p[i+1]=='/'||!p[i+1])&&(i==1||p[i-1]=='/'||(p[i-1]=='.'&&(i==2||p[i-2]=='/'))))return 0;}}
 if(!component)return 0;
 if(!write)return prefix8(p,input8.user_root)||prefix8(p,input8.product_root)||equal8(p,input8.source);
 if(input8.command==W8_INSTALL||input8.command==W8_REMOVE)return prefix8(p,input8.product_root)&&(!equal8(p,input8.product_root)||input8.command==W8_INSTALL||input8.command==W8_REMOVE);
 return input8.command==W8_RUN&&prefix8(p,input8.user_root)&&!equal8(p,input8.user_root);
}
static const v6_entry *find8(const char*p){v6_volume*v=m6_volume();if(!v)return 0;for(unsigned i=0;i<V6_ENTRIES;++i)if(v->entries[i].kind&&equal8(v->entries[i].path,p))return v->entries+i;return 0;}
static unsigned exact8(const char*p,uint64_t off,void*out,unsigned n){unsigned got=0;return v6_read(m6_volume(),p,off,out,n,&got)||got!=n;}
static unsigned memory_source8(void*c,uint64_t off,void*out,unsigned n){memcpy(out,(uint8_t*)c+off,n);return 0;}
static const uint8_t *boot_package8;static unsigned boot_package_size8;
unsigned w8_boot_package(const void*p,unsigned bytes){static const uint8_t expected[]={0x6d,0xc8,0x22,0x56,0x9e,0x77,0x7d,0x9a,0x3e,0xcd,0x5f,0xfc,0xbc,0x22,0xb5,0x56,0x00,0xaf,0x87,0xe6,0xc5,0xd9,0x81,0x0a,0x64,0xe8,0x56,0x6c,0xf5,0xa4,0xaf,0x9f};uint8_t hash[32];if(bytes!=2003456)return 1;engine_sha256(p,bytes,hash);if(memcmp(hash,expected,32))return 1;boot_package8=p;boot_package_size8=bytes;serial_text("M8_BOOT_MSI_DIGEST_OK\n");return 0;}
unsigned w8_distribute(void*opaque,const char*user){if(!boot_package8||!boot_package_size8)return V6_INVALID;char path[256];copy8(path,user,256);unsigned n=length8(path);if(n+27>=256)return V6_INVALID;memcpy(path+n,"/Downloads/7z2606-x64.msi",26);return v6_put(opaque,path,boot_package_size8,memory_source8,(void*)boot_package8);}
unsigned w8_install_runtime(void*opaque){v6_volume*v=opaque;unsigned parent=v6_mkdir(v,"/Applications/Windows");if(parent&&parent!=V6_EXISTS)return parent;unsigned s=v6_mkdir(v,"/System/Windows");if(s&&s!=V6_EXISTS)return s;if(!windows8_runtime_size)return V6_INVALID;unsigned licenses=length8(windows8_licenses);if(v6_put(v,"/System/Licenses/Windows-runtime-LGPL.txt",licenses,memory_source8,(void*)windows8_licenses))return V6_IO;return v6_put(v,"/System/Windows/runtime8.elf",windows8_runtime_size,memory_source8,(void*)windows8_runtime_image);}
static void output8(const char*s,unsigned n){unsigned old=length8(info8.output);if(n>W8_TEXT-1){s+=n-(W8_TEXT-1);n=W8_TEXT-1;}if(n>W8_TEXT-1-old){unsigned drop=n-(W8_TEXT-1-old);memmove(info8.output,info8.output+drop,old-drop);old-=drop;}memcpy(info8.output+old,s,n);info8.output[old+n]=0;++info8.revision;}
static void write_clear8(void){while(write_count8)page_free(write_pages8[--write_count8]);memset(write_pages8,0,sizeof(write_pages8));writer8=UINT32_MAX;}
static unsigned writer_source8(void*c,uint64_t off,void*out,unsigned bytes){(void)c;uint8_t*p=out;while(bytes){unsigned page=(unsigned)(off/4096),n=4096-(unsigned)(off&4095);if(n>bytes)n=bytes;if(page>=write_count8)return 1;memcpy(p,(void*)(uintptr_t)(write_pages8[page]+(off&4095)),n);off+=n;p+=n;bytes-=n;}return 0;}
static unsigned close8(unsigned id,unsigned commit){
 if(id>=32||!files8[id].live)return 1;file8*f=files8+id;unsigned status=0;
 if(f->write){const v6_entry*now=find8(f->path);if(commit){if((now&&!f->existed)||(!now&&f->existed)||(now&&(now->crc!=f->crc||now->kind!=1)))status=V6_INVALID;
  else status=v6_put(m6_volume(),f->path,f->size,writer_source8,0);}write_clear8();}
 memset(f,0,sizeof(*f));return status;
}
static void snapshot8(void){w8_mailbox value;if(user8(W8_MAILBOX,&value,sizeof(value),0))return;
 info8.percent=value.percent<=100?value.percent:0;copy8(info8.product,value.product,sizeof(info8.product));copy8(info8.version,value.version,sizeof(info8.version));copy8(info8.publisher,value.publisher,sizeof(info8.publisher));copy8(info8.code,value.code,sizeof(info8.code));copy8(info8.message,value.message,sizeof(info8.message));info8.files=value.files;info8.total_bytes=value.total_bytes;++info8.revision;serial_text("M8_PROGRESS percent=");serial_hex(info8.percent);serial_text(" ");serial_text(info8.message);serial_text("\n");}
static void metadata_path8(char*out){copy8(out,input8.product_root,256);unsigned n=length8(out);if(n+13<256)memcpy(out+n,"/metadata.bin",14);}
static void finish8(unsigned status){snapshot8();info8.status=status;info8.state=status?3:2;if(metadata8_dirty&&input8.command==W8_RUN){char path[256];metadata_path8(path);if(v6_put(m6_volume(),path,sizeof(metadata8_db),memory_source8,&metadata8_db)){info8.status=1;info8.state=3;copy8(info8.message,"Windows file metadata could not be saved.",sizeof(info8.message));}}
 for(unsigned i=0;i<32;++i)if(files8[i].live)(void)close8(i,0);w8_active=0;w8_escape();}
static void release8(void){write_clear8();while(count8)page_free(owned8[--count8]);cr3_8=pd8=entry8=0;memset(files8,0,sizeof(files8));}
static unsigned rollback8(void){
 if(!created_root8)return 0;v6_volume*v=m6_volume();
 for(unsigned pass=0;pass<V6_ENTRIES;++pass){const char*chosen=0;unsigned longest=0;
  for(unsigned i=0;i<V6_ENTRIES;++i){const v6_entry*e=v->entries+i;if(e->kind&&prefix8(e->path,input8.product_root)){unsigned n=length8(e->path);if(n>longest){chosen=e->path;longest=n;}}}
  if(!chosen){created_root8=0;return 0;}char path[256];copy8(path,chosen,256);if(v6_remove(v,path))return 1;
 }return 1;
}
void w8_cancel(void){if(info8.state==1){cancel8=1;info8.state=3;info8.status=1;copy8(info8.message,"Operation cancelled. Uncommitted file writes were discarded.",sizeof(info8.message));release8();if(rollback8())copy8(info8.message,"Cancelled. Disk cleanup was interrupted; use Recover / remove.",sizeof(info8.message));++info8.revision;}}
static unsigned prepare_transition8(void){if(ready8)return 0;uint64_t pages[4];unsigned count=0;for(;count<4;++count)if(!(pages[count]=page_alloc()))break;if(count<4){while(count)page_free(pages[--count]);return 1;}uint64_t*pt=(void*)(uintptr_t)kernel_entry_pt();for(unsigned i=0;i<4;++i)pt[((TRAP8>>12)&511)+i]=pages[i]|3|NX;ready8=1;return 0;}
unsigned w8_start(unsigned command,const char*source,const char*root,const char*user,const char*arguments){
 if(info8.state==1||command<1||command>4||!source||!user||length8(source)>255||length8(user)>223||!prefix8(user,"/Users"))return V6_INVALID;
 release8();created_root8=0;memset(&info8,0,sizeof(info8));memset(&input8,0,sizeof(input8));info8.state=1;info8.command=command;input8.magic=W8_MAGIC;input8.command=command;
 copy8(input8.source,source,256);copy8(input8.user_root,user,256);if(root)copy8(input8.product_root,root,256);if(arguments)copy8(input8.arguments,arguments,sizeof(input8.arguments));copy8(input8.windows_root,"C:\\Program Files\\7-Zip",256);
 if(!root||!equal8(root,"/Applications/Windows/{23170F69-40C1-2702-2606-000001000000}"))goto failure;
 if(prepare_transition8())goto failure;cr3_8=allocate8();uint64_t pdpt=allocate8();pd8=allocate8();if(!cr3_8||!pdpt||!pd8)goto failure;
 ((uint64_t*)(uintptr_t)cr3_8)[0]=pdpt|7;((uint64_t*)(uintptr_t)cr3_8)[256]=kernel_stack_branch()|3;for(unsigned i=0;i<4;++i)((uint64_t*)(uintptr_t)pdpt)[i]=kernel_pd(i)|3;((uint64_t*)(uintptr_t)pdpt)[4]=pd8|7;
 elf_header header;if(v6_verify(m6_volume(),"/System/Windows/runtime8.elf")||exact8("/System/Windows/runtime8.elf",0,&header,sizeof(header))||memcmp(header.ident,"\177ELF\2\1\1",7)||header.machine!=62||header.type!=2||header.phnum>8||header.phentsize!=56)goto failure;
 unsigned executable=0;for(unsigned i=0;i<header.phnum;++i){elf_segment s;if(exact8("/System/Windows/runtime8.elf",header.phoff+i*56,&s,sizeof(s)))goto failure;if(s.type!=1||!s.memsz)continue;
  if(s.vaddr<W8_BASE||s.vaddr>=W8_BASE+0x100000||s.memsz>W8_BASE+0x100000-s.vaddr||s.filesz>s.memsz||s.filesz>0x100000||(s.vaddr&4095)||s.flags>6||(s.flags&3)==3)goto failure;
  unsigned pages=(unsigned)((s.memsz+4095)/4096);if(map8(s.vaddr,(uint64_t)pages*4096,3))goto failure;
  for(unsigned p=0;p<pages;++p){unsigned n=s.filesz>(uint64_t)p*4096?(unsigned)(s.filesz-(uint64_t)p*4096):0;if(n>4096)n=4096;uint64_t*leaf=pte8(s.vaddr+(uint64_t)p*4096,0);if(n&&exact8("/System/Windows/runtime8.elf",s.offset+(uint64_t)p*4096,(void*)(uintptr_t)(*leaf&UINT64_C(0x000ffffffffff000)),n))goto failure;*leaf=(*leaf&UINT64_C(0x000ffffffffff000))|5|((s.flags&2)?2:0)|((s.flags&1)?0:NX);}
  if((s.flags&1)&&header.entry>=s.vaddr&&header.entry<s.vaddr+s.filesz)executable=1;
 }
 if(!executable||map8(W8_MAILBOX,4096,3)||map8(W8_STACK_TOP-16384,16384,3)||map8(W8_TEB,8192,3))goto failure;
 if(user8(W8_MAILBOX,&input8,sizeof(input8),1))goto failure;
 uint64_t self=W8_TEB,stack=W8_STACK_TOP,limit=W8_STACK_LOW;if(user8(W8_TEB+0x30,&self,8,1)||user8(W8_TEB+8,&stack,8,1)||user8(W8_TEB+16,&limit,8,1))goto failure;
 memset(&context8,0,sizeof(context8));context8.cs=0x1b;context8.ss=0x23;context8.rflags=0x202;context8.rip=header.entry;context8.rsp=W8_STACK_TOP-8;entry8=header.entry;
 memset(fx8,0,sizeof(fx8));*(uint16_t*)fx8=0x37f;*(uint32_t*)(fx8+24)=0x1f80;cancel8=0;metadata8_dirty=0;memset(&metadata8_db,0,sizeof(metadata8_db));metadata8_db.magic=UINT64_C(0x314154454d3857);
 if(root&&*root&&command==W8_RUN){char path[256];metadata_path8(path);const v6_entry*f=find8(path);if(f&&f->size==sizeof(metadata8_db)&&!v6_verify(m6_volume(),path)){unsigned got=0;(void)v6_read(m6_volume(),path,0,&metadata8_db,sizeof(metadata8_db),&got);}}
 serial_text("M8_CPL3_CREATED command=");serial_hex(command);serial_text("\n");return 0;
 failure:release8();info8.state=3;info8.status=1;copy8(info8.message,"The protected Windows runtime could not be loaded.",sizeof(info8.message));++info8.revision;return V6_INVALID;
}
unsigned w8_pending(void){return info8.state==1&&cr3_8&&!cancel8;}
const w8_info *w8_current(void){return &info8;}
void w8_pump(void){
 if(!w8_pending())return;uint64_t flags;__asm__ volatile("pushfq; popq %0; cli":"=r"(flags)::"memory");__asm__ volatile("movq %%cr3,%0":"=r"(w8_kernel_cr3));
 uint32_t lo,hi;__asm__ volatile("rdmsr":"=a"(lo),"=d"(hi):"c"(0xc0000101u));old_gs8=((uint64_t)hi<<32)|lo;__asm__ volatile("wrmsr"::"a"((uint32_t)W8_TEB),"d"((uint32_t)(W8_TEB>>32)),"c"(0xc0000101u));
 pic8=in8(0x21);out8(0x21,pic8&~1u);interrupts_set_kernel_stack(TRAP8+16384);quantum8=0;w8_active=1;
 __asm__ volatile("fxrstor64 %0"::"m"(fx8):"memory");w8_enter(&context8,cr3_8);__asm__ volatile("fxsave64 %0":"=m"(fx8)::"memory");
 __asm__ volatile("wrmsr"::"a"((uint32_t)old_gs8),"d"((uint32_t)(old_gs8>>32)),"c"(0xc0000101u));out8(0x21,pic8);interrupts_set_kernel_stack(KERNEL_STACK_TOP);if(flags&512)__asm__ volatile("sti":::"memory");
 ++info8.quanta;if(count8>info8.peak_pages)info8.peak_pages=count8;
 if(info8.state!=1){serial_text(info8.status?"M8_JOB_FAILED code=":"M8_JOB_DONE code=");serial_hex(info8.status);serial_text("\n");release8();if(info8.status&&rollback8())copy8(info8.message,"Installation failed and cleanup was interrupted; use Recover / remove.",sizeof(info8.message));}
}

#include <engine/network/clock.h>
static uint64_t now8(void){clock7_time t;if(clock7_read(&t)||t.days<584754)return 0;return (((uint64_t)t.days-584754)*86400+t.seconds)*UINT64_C(10000000);}
static metadata8 *meta8(const char*path,unsigned create){
 metadata8*empty=0;for(unsigned i=0;i<128;++i){metadata8*m=metadata8_db.records+i;if(!m->path[0]){if(!empty)empty=m;}else if(equal8(m->path,path))return m;}
 if(!create||!empty)return 0;copy8(empty->path,path,256);uint64_t time=now8();for(unsigned i=0;i<3;++i)empty->times[i]=time;empty->attributes=32;return empty;
}
static void information8(const v6_entry*e,w8_file_info*out){
 memset(out,0,sizeof(*out));out->size=e->size;out->crc=e->crc;out->kind=e->kind;out->attributes=e->kind==2?16:32;
 metadata8*m=meta8(e->path,0);uint64_t time=now8();for(unsigned i=0;i<3;++i)out->time[i]=m?m->times[i]:time;if(m)out->attributes=m->attributes;
 const char*name=e->path;for(unsigned i=0;e->path[i];++i)if(e->path[i]=='/')name=e->path+i+1;copy8(out->name,name,256);
}
static void free_owned8(uint64_t page){for(unsigned i=0;i<count8;++i)if(owned8[i]==page){owned8[i]=owned8[--count8];page_free(page);return;}}
static int64_t dispatch8(trap_frame*f){
 uint64_t a=f->rdi,b=f->rsi,c=f->rdx,d=f->r10;char path[256];
 switch(f->rax){
 case W8_SYS_MAP:return map8(a,b,(unsigned)c);
 case W8_SYS_UNMAP:case W8_SYS_PROTECT:{
  if(!b||(a&4095)||(b&4095)||a<W8_BASE||a>=W8_LIMIT||b>W8_LIMIT-a||b>32*1024*1024)return -1;
  if(f->rax==W8_SYS_PROTECT&&c!=1&&c!=3&&c!=5)return -1;
  for(uint64_t at=a;at<a+b;at+=4096){uint64_t*p=pte8(at,0);if(!p||!(*p&1))return -1;}
  for(uint64_t at=a;at<a+b;at+=4096){uint64_t*p=pte8(at,0),physical=*p&UINT64_C(0x000ffffffffff000);if(f->rax==W8_SYS_UNMAP){*p=0;free_owned8(physical);}else *p=physical|5|((c&2)?2:0)|((c&4)?0:NX);__asm__ volatile("invlpg (%0)"::"r"(at):"memory");}return 0;
 }
 case W8_SYS_OPEN:{
  if(string8(a,path,256)||b>2||!path8(path,b!=0))return -1;const v6_entry*e=find8(path);if(e&&e->kind!=1)return -1;
  if(!b&&(!e||v6_verify(m6_volume(),path)))return -1;if(b&&(writer8!=UINT32_MAX||(e&&(b==2||input8.command==W8_INSTALL))))return -1;
  unsigned id=0;while(id<32&&files8[id].live)++id;if(id==32)return -1;file8*h=files8+id;memset(h,0,sizeof(*h));h->live=1;h->write=b!=0;h->existed=e!=0;h->size=b?0:e->size;h->crc=e?e->crc:0;copy8(h->path,path,256);if(b)writer8=id;return id;
 }
 case W8_SYS_READ:{
  if(a>=32||!files8[a].live||c>4096)return -1;file8*h=files8+a;unsigned n=h->position<h->size?(unsigned)(h->size-h->position):0;if(n>c)n=(unsigned)c;
  if(h->write){if(writer_source8(0,h->position,io8,n))return -1;}else{unsigned got=0;if(v6_read(m6_volume(),h->path,h->position,io8,n,&got)||got!=n)return -1;}
  if(user8(b,io8,n,1))return -1;h->position+=n;return n;
 }
 case W8_SYS_WRITE:{
  if(a>=32||!files8[a].live||!files8[a].write||c>4096)return -1;file8*h=files8+a;if(h->position>WRITE8_PAGES*4096||c>WRITE8_PAGES*4096-h->position||user8(b,io8,(unsigned)c,0))return -1;
  unsigned need=(unsigned)((h->position+c+4095)/4096);while(write_count8<need){uint64_t p=page_alloc();if(!p)return -1;write_pages8[write_count8++]=p;peak8();}
  unsigned left=(unsigned)c,off=0;while(left){unsigned page=(unsigned)(h->position/4096),n=4096-(unsigned)(h->position&4095);if(n>left)n=left;memcpy((void*)(uintptr_t)(write_pages8[page]+(h->position&4095)),io8+off,n);off+=n;left-=n;h->position+=n;}
  if(h->position>h->size)h->size=h->position;return (int64_t)c;
 }
 case W8_SYS_SEEK:{if(a>=32||!files8[a].live||c>2)return -1;file8*h=files8+a;int64_t base=c==0?0:c==1?(int64_t)h->position:(int64_t)h->size,offset=(int64_t)b,position;if(__builtin_add_overflow(base,offset,&position)||position<0||position>64*1024*1024)return -1;h->position=(uint64_t)position;return position;}
 case W8_SYS_CLOSE:return close8((unsigned)a,1)?-1:0;
 case W8_SYS_STAT:{if(string8(a,path,256)||!path8(path,0))return -1;const v6_entry*e=find8(path);if(!e)return -1;w8_file_info result;information8(e,&result);return user8(b,&result,sizeof(result),1)?-1:0;}
 case W8_SYS_ENUM:{
  if(string8(a,path,256)||!path8(path,0)||c>=V6_ENTRIES)return -1;unsigned len=length8(path);v6_volume*v=m6_volume();
  for(unsigned i=(unsigned)c;i<V6_ENTRIES;++i){const v6_entry*e=v->entries+i;if(!e->kind||memcmp(e->path,path,len)||e->path[len]!='/')continue;const char*p=e->path+len+1;unsigned slash=0;while(*p)if(*p++=='/')slash=1;if(slash)continue;w8_file_info result;information8(e,&result);if(user8(b,&result,sizeof(result),1))return -1;return i+1;}return 0;
 }
 case W8_SYS_MKDIR:case W8_SYS_DELETE:{if(string8(a,path,256)||!path8(path,1))return -1;unsigned s=f->rax==W8_SYS_MKDIR?v6_mkdir(m6_volume(),path):v6_remove(m6_volume(),path);if(!s&&f->rax==W8_SYS_MKDIR&&input8.command==W8_INSTALL&&equal8(path,input8.product_root))created_root8=1;return !s||s==V6_EXISTS?0:-1;}
 case W8_SYS_MOVE:{char destination[256];if(string8(a,path,256)||string8(b,destination,256)||!path8(path,1)||!path8(destination,1)||c>1)return -1;return v6_rename(m6_volume(),path,destination,(unsigned)c)?-1:0;}
 case W8_SYS_TRUNCATE:{if(a>=32||!files8[a].live||!files8[a].write||files8[a].position>WRITE8_PAGES*4096)return -1;file8*h=files8+a;unsigned need=(unsigned)((h->position+4095)/4096);while(write_count8<need){uint64_t page=page_alloc();if(!page)return -1;write_pages8[write_count8++]=page;peak8();}while(write_count8>need){page_free(write_pages8[--write_count8]);write_pages8[write_count8]=0;}h->size=h->position;return 0;}
 case W8_SYS_OUTPUT:{if(b>4096||user8(a,io8,(unsigned)b,0))return -1;output8((char*)io8,(unsigned)b);return (int64_t)b;}
 case W8_SYS_EXIT:finish8((unsigned)a);break;
 case W8_SYS_TIME:return a?((int64_t)now8()):(int64_t)(m3_clock_ticks()*10);
 case W8_SYS_YIELD:snapshot8();context8=*f;context8.rax=0;w8_active=0;w8_escape();break;
 case W8_SYS_METADATA:{
  if(a>=32||!files8[a].live||b>1)return -1;metadata8*m=meta8(files8[a].path,1);if(!m)return -1;w8_file_info data;memset(&data,0,sizeof(data));
  if(b){if(user8(c,&data,sizeof(data),0))return -1;memcpy(m->times,data.time,sizeof(m->times));m->attributes=data.attributes;metadata8_dirty=1;}else{memcpy(data.time,m->times,sizeof(m->times));data.attributes=m->attributes;if(user8(c,&data,sizeof(data),1))return -1;}return 0;
 }
 case W8_SYS_INFO:if(a==0)return 1;else if(a==1)return (int64_t)(pages_stats().total_pages*4096);else if(a==2)return (int64_t)((pages_stats().total_pages-pages_stats().current_pages)*4096);else if(a==3)return (int64_t)(v6_capacity(m6_volume())-v6_used(m6_volume()));else if(a==4)return (int64_t)v6_capacity(m6_volume());return -1;
 default:(void)d;return -1;
 }return -1;
}
trap_frame *w8_trap(trap_frame*f){
 if(!w8_active)return f;
 if(f->vector==14){uint64_t address;__asm__ volatile("movq %%cr2,%0":"=r"(address));
  if(!(f->error&1)&&address>=W8_STACK_LOW&&address<W8_STACK_TOP&&address+65536>=f->rsp&&!map8(address&~UINT64_C(4095),4096,3))return f;
 }
 if(f->vector==128){f->rax=(uint64_t)dispatch8(f);return f;}
 if(f->vector==32){out8(0x20,0x20);if(++quantum8<1)return f;context8=*f;w8_active=0;w8_escape();}
 serial_text("M8_PROCESS_FAULT vector=");serial_hex(f->vector);serial_text(" rip=");serial_hex(f->rip);serial_text(" address=");uint64_t addr;__asm__ volatile("movq %%cr2,%0":"=r"(addr));serial_hex(addr);serial_text("\n");
 copy8(info8.message,"The Windows process stopped after an unsupported operation or memory fault.",sizeof(info8.message));info8.state=3;info8.status=1;w8_active=0;w8_escape();
}
