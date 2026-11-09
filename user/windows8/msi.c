/* First bounded MSI adapter. Only the exact reviewed upstream package is accepted. */
#include "cab.h"
#include <engine/sha256.h>
#define PRODUCT8 "{23170F69-40C1-2702-2606-000001000000}"
#define ROOT8 "/Applications/Windows/" PRODUCT8
typedef struct {char path[256];uint8_t sha256[32];uint64_t bytes;} installed_file8;
typedef struct {uint64_t magic;unsigned schema,count;char product[96],version[32],publisher[96],code[40];installed_file8 files[128];} manifest8;
static const uint8_t package_hash8[32]={0x6d,0xc8,0x22,0x56,0x9e,0x77,0x7d,0x9a,0x3e,0xcd,0x5f,0xfc,0xbc,0x22,0xb5,0x56,0x00,0xaf,0x87,0xe6,0xc5,0xd9,0x81,0x0a,0x64,0xe8,0x56,0x6c,0xf5,0xa4,0xaf,0x9f};
static void check8(int good,const char*message){if(!good)fail8(message);}
static void path_msi8(char*out,const char*relative){copy8(out,ROOT8,256);append8(out,"/",256);append8(out,relative,256);}
static int component8(const char*name){unsigned n=len8(name);if(!n||n>63||equal8(name,".")||equal8(name,".."))return 0;for(unsigned i=0;i<n;++i)if((uint8_t)name[i]<32||name[i]=='/'||name[i]=='\\'||name[i]==':')return 0;return 1;}
static void digest8(const void*data,unsigned size,uint8_t hash[32]){engine_sha256(data,size,hash);}
static void manifest_file8(manifest8*m,const char*path,const void*data,unsigned bytes){check8(m->count<128,"Product file manifest is full.");installed_file8*f=m->files+m->count++;copy8(f->path,path,256);f->bytes=bytes;digest8(data,bytes,f->sha256);}
static int file_matches8(const installed_file8*f){void*data=0;unsigned n=load8(f->path,&data,8*1024*1024);if(!n)return 0;uint8_t hash[32];digest8(data,n,hash);free8(data);return n==f->bytes&&!memcmp(hash,f->sha256,32);}
static void properties8(manifest8*m){
 msi8_table properties=msi8_table_open("Property");check8(properties.columns==2,"Unsupported MSI Property table.");
 for(unsigned row=0;row<properties.rows;++row){const char*key=msi8_text(&properties,row,0),*value=msi8_text(&properties,row,1);
  if(equal8(key,"ProductName"))copy8(m->product,value,96);else if(equal8(key,"ProductVersion"))copy8(m->version,value,32);else if(equal8(key,"Manufacturer"))copy8(m->publisher,value,96);else if(equal8(key,"ProductCode"))copy8(m->code,value,40);
 }check8(equal8(m->code,PRODUCT8)&&m->product[0]&&m->version[0]&&m->publisher[0],"This MSI product identity is unsupported.");
 copy8(job8->product,m->product,96);copy8(job8->version,m->version,32);copy8(job8->publisher,m->publisher,96);copy8(job8->code,m->code,40);free8(properties.data.data);
}
static char *records8(const msi8_table*t,unsigned*bytes){
 char*out=alloc8(65536);check8(out!=0,"Not enough memory for product registration.");unsigned used=0;
 for(unsigned r=0;r<t->rows;++r){for(unsigned c=0;c<t->columns;++c){const char*value="";char number[24];if(t->column[c].type&0x800)value=msi8_text(t,r,c);else{int32_t n=msi8_integer(t,r,c);if(n!=INT32_MIN){unsigned a=0;uint32_t u=n<0?(uint32_t)-(int64_t)n:(uint32_t)n;char reverse[16];unsigned count=0;do{reverse[count++]=(char)('0'+u%10);u/=10;}while(u);if(n<0)number[a++]='-';while(count)number[a++]=reverse[--count];number[a]=0;value=number;}}
   while(*value){check8(used+3<65536&&*value!='\n'&&*value!='\t',"MSI registration text exceeds its format.");out[used++]=*value++;}out[used++]=c+1<t->columns?'\t':'\n';}}
 out[used]=0;*bytes=used;return out;
}
static void remove_product8(void){
 char path[256];path_msi8(path,"product.bin");void*data=0;unsigned size=load8(path,&data,sizeof(manifest8));unsigned prepared=0;
 if(!size){path_msi8(path,"install.plan");size=load8(path,&data,sizeof(manifest8));prepared=1;}
 if(!size){w8_file_info root,child;if(!stat8(ROOT8,&root)&&root.kind==2&&sys8(W8_SYS_ENUM,(uint64_t)ROOT8,(uint64_t)&child,0,0,0)==0){check8(!remove8(ROOT8),"The empty incomplete product directory could not be cleared.");progress8(100,"Recovered the empty incomplete installation. You can install again.");return;}}
 check8(size==sizeof(manifest8),"No valid installed product or recovery plan was found. No files were removed.");manifest8*m=data;
 check8(m->magic==UINT64_C(0x31494d38574f53)&&m->schema==1&&m->count<=128&&equal8(m->code,PRODUCT8),"Product registration is invalid; no files were removed.");
 copy8(job8->product,m->product,96);copy8(job8->version,m->version,32);unsigned kept=0;
 /* Validate every owned path before modifying anything, even for a damaged manifest. */
 for(unsigned i=0;i<m->count;++i){const char*p=m->files[i].path;check8(p[255]==0&&len8(p)>len8(ROOT8)+1&&!memcmp(p,ROOT8,len8(ROOT8))&&p[len8(ROOT8)]=='/',"Product manifest contains an invalid owned path.");}
 for(unsigned i=0;i<m->count;++i){installed_file8*f=m->files+i;w8_file_info info;if(!stat8(f->path,&info)){if(file_matches8(f)){check8(!remove8(f->path),"Removal was interrupted; retry Remove to finish safely.");}else ++kept;}progress8((i+1)*90/(m->count?m->count:1),"Removing unchanged product files; keeping your documents.");}
 check8(!remove8(path),"Product registration could not be removed; retry Remove.");
 char plan[256];path_msi8(plan,"install.plan");w8_file_info info;if(!prepared&&!stat8(plan,&info))check8(!remove8(plan),"Recovery plan could not be cleared.");
 path_msi8(path,"Lang");(void)remove8(path);path_msi8(path,"metadata.bin");if(!stat8(path,&info))(void)remove8(path);(void)remove8(ROOT8);
 progress8(100,kept?"Removed. Modified product files were preserved; your documents are unchanged.":"Removed. Your documents are unchanged.");free8(m);
}
void msi8_execute(void){
 check8(equal8(job8->product_root,ROOT8),"Unsupported product directory.");
 if(job8->command==W8_REMOVE){remove_product8();return;}
 progress8(2,"Reading the original MSI package.");void*package=0;unsigned package_size=load8(job8->source,&package,8*1024*1024);check8(package_size!=0,"The MSI package could not be read.");uint8_t hash[32];digest8(package,package_size,hash);
 check8(!memcmp(hash,package_hash8,32),"This first Windows profile supports only the reviewed 7-Zip 26.06 x64 MSI. No files were changed.");
 msi8_database(package,package_size);manifest8*m=calloc8(1,sizeof(*m));check8(m!=0,"Not enough memory for the product manifest.");m->magic=UINT64_C(0x31494d38574f53);m->schema=1;properties8(m);
 msi8_table files=msi8_table_open("File"),components=msi8_table_open("Component"),custom=msi8_table_open("CustomAction");
 check8(!custom.rows&&files.columns==8&&components.columns==6&&files.rows<=120,"This MSI requires unsupported installation behavior.");
 cab8_archive*cab=cab8_decode(msi8_stream("product.cab",0));check8(cab->count==files.rows,"CAB and MSI file counts do not match.");
 unsigned indices[128];
 for(unsigned row=0;row<files.rows;++row){const char*id=msi8_text(&files,row,0),*component=msi8_text(&files,row,1),*name=msi8_text(&files,row,2),*directory=0;
  const char*long_name=name;for(const char*p=name;*p;++p)if(*p=='|')long_name=p+1;check8(component8(long_name),"MSI filename is unsupported.");
  for(unsigned i=0;i<components.rows;++i)if(equal8(component,msi8_text(&components,i,0)))directory=msi8_text(&components,i,2);
  check8(directory&&(equal8(directory,"INSTALLDIR")||equal8(directory,"MyLang")),"MSI destination directory is unsupported.");
  char path[256];path_msi8(path,equal8(directory,"MyLang")?"Lang/":"");append8(path,long_name,256);unsigned index=0;while(index<cab->count&&!equal8(id,cab->files[index].name))++index;check8(index<cab->count,"MSI file has no matching CAB member.");
  cab8_file*f=cab->files+index;check8(msi8_integer(&files,row,3)==(int32_t)f->size,"MSI file size differs from its CAB member.");indices[row]=index;for(unsigned i=0;i<m->count;++i)check8(!equal8(path,m->files[i].path),"MSI destinations collide.");manifest_file8(m,path,cab->expanded.data+f->offset,f->size);job8->total_bytes+=f->size;
 }
 job8->files=files.rows;msi8_table features=msi8_table_open("Feature"),registry=msi8_table_open("Registry"),shortcuts=msi8_table_open("Shortcut");job8->features=features.rows;job8->registry=registry.rows;job8->shortcuts=shortcuts.rows;
 /* Retain original metadata honestly; unsupported shell/GUI activation stays disabled. */
 const char*names[]={"features.tsv","components.tsv","registry-msi.tsv","shortcuts-disabled.tsv"};const msi8_table*tables[]={&features,&components,&registry,&shortcuts};char*metadata[4];unsigned metadata_size[4];
 for(unsigned i=0;i<4;++i){metadata[i]=records8(tables[i],metadata_size+i);char path[256];path_msi8(path,names[i]);manifest_file8(m,path,metadata[i],metadata_size[i]);}
 static const char registry_console[]="1|Software\\7-Zip|Path|C:\\Program Files\\7-Zip\\\n1|Software\\7-Zip|Path64|C:\\Program Files\\7-Zip\\\n";
 char path[256];path_msi8(path,"registry.txt");manifest_file8(m,path,registry_console,sizeof(registry_console)-1);
 progress8(15,"7-Zip console profile: original files; Windows GUI and shell extensions are unavailable.");
 if(job8->command==W8_PREVIEW){progress8(100,"Ready to install 7-Zip console support. Your documents are kept separate.");return;}
 w8_file_info existing;check8(stat8(ROOT8,&existing)!=0,"This product already has files. Use Remove or recovery before installing again.");
 check8(sys8(W8_SYS_INFO,3,0,0,0,0)>job8->total_bytes+package_size+sizeof(*m)*2+1024*1024,"Not enough free disk space. No files were changed.");
 check8(!mkdir8(ROOT8),"The product directory could not be created.");path_msi8(path,"install.plan");check8(!save8(path,m,sizeof(*m)),"The recoverable installation plan could not be written.");path_msi8(path,"Lang");check8(!mkdir8(path),"The language directory could not be created.");
 for(unsigned row=0;row<files.rows;++row){cab8_file*f=cab->files+indices[row];check8(!save8(m->files[row].path,cab->expanded.data+f->offset,f->size),"Installation was interrupted. Use recovery to remove the incomplete product.");progress8(15+(row+1)*70/files.rows,"Installing original Windows application files.");}
 for(unsigned i=0;i<4;++i){path_msi8(path,names[i]);check8(!save8(path,metadata[i],metadata_size[i]),"Product metadata could not be saved.");}path_msi8(path,"registry.txt");check8(!save8(path,registry_console,sizeof(registry_console)-1),"Console registry could not be saved.");
 progress8(88,"Verifying original installed files before publishing the product.");
 for(unsigned i=0;i<m->count;++i)check8(file_matches8(m->files+i),"Installed file verification failed. Use recovery before retrying.");
 path_msi8(path,"product.bin");check8(!save8(path,m,sizeof(*m)),"Product registration could not be published.");path_msi8(path,"install.plan");check8(!remove8(path),"Installed; the recovery plan could not be cleared.");
 progress8(100,"Installed 7-Zip console support. Windows GUI and shell extensions remain unavailable.");
}
