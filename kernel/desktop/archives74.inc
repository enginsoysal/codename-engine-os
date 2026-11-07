/* ZIP Stored/Deflate, bounded to 32 members and 64 MiB expanded data. */
enum { ZIP_CHOOSE74=14000,ZIP_EXTRACT74,ZIP_CREATE74,ZIP_PREV74,ZIP_NEXT74,ZIP_RESULT74,ZIP_FOLDER74 };
typedef struct {char name[208];uint64_t data;uint32_t size,compressed,crc;uint16_t method,folder;} zip_entry74;
_Static_assert(sizeof(zip_entry74)*32<=APP7_TEXT,"bounded ZIP catalogue");
static uint32_t crc_next74(uint32_t crc,const void *bytes,unsigned n){
    const uint8_t *p=bytes;for(unsigned i=0;i<n;++i){crc^=p[i];for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&-(crc&1));}return crc;
}
static unsigned zip_name74(char *name){
    unsigned component=0,n=0;if(!name[0]||name[0]=='/')return 0;
    for(;name[n];++n){unsigned c=(unsigned char)name[n];if(c<32||c>126||c=='\\'||c==':')return 0;
        if(c=='/'){if(!component||component>63)return 0;unsigned start=n-component;if((component==1&&name[start]=='.')||(component==2&&name[start]=='.'&&name[start+1]=='.'))return 0;component=0;}
        else ++component;
    }
    unsigned start=n-component;return component&&component<=63&&!(component==1&&name[start]=='.')&&!(component==2&&name[start]=='.'&&name[start+1]=='.');
}
static unsigned zip_eocd74(const char *path,uint64_t size,uint64_t *central,unsigned *count,uint64_t *central_bytes){
    uint8_t buffer[4117];uint64_t end=size,floor=size>65557?size-65557:0;
    while(end>floor){uint64_t start=end-floor>4096?end-4096:floor;unsigned base=(unsigned)(end-start),extra=(unsigned)(size-end);if(extra>21)extra=21;
        if(read74(path,start,buffer,base+extra))return V6_IO;
        for(unsigned i=base;i;){--i;if(i+22>base+extra||le32_74(buffer+i)!=0x06054b50)continue;
            const uint8_t *e=buffer+i;if(start+i+22+le16_74(e+20)!=size)continue;
            if(le16_74(e+4)||le16_74(e+6)||le16_74(e+8)!=le16_74(e+10)||le16_74(e+10)>32)return V6_INVALID;
            *count=le16_74(e+10);*central=le32_74(e+16);*central_bytes=le32_74(e+12);
            return *central>start+i||*central_bytes>start+i-*central?V6_INVALID:0;
        }end=start;
    }return V6_INVALID;
}
static unsigned zip_load74(application *app){
    const v6_entry *file=entry(app->path);if(!file||file->kind!=1||file->size<22||file->size>64u*1024*1024||v6_verify(m6_volume(),app->path))return V6_INVALID;
    uint64_t central,central_bytes;unsigned count,status=zip_eocd74(app->path,file->size,&central,&count,&central_bytes);if(status)return status;
    memset(app->text,0,APP7_TEXT);zip_entry74 *members=(zip_entry74 *)app->text;uint64_t at=central,total=0;
    for(unsigned i=0;i<count;++i){uint8_t h[46],local[30];if(at>central+central_bytes||central+central_bytes-at<46||read74(app->path,at,h,46)||le32_74(h)!=0x02014b50)return V6_INVALID;
        unsigned flags=le16_74(h+8),method=le16_74(h+10),name_bytes=le16_74(h+28),extra=le16_74(h+30),comment=le16_74(h+32);
        uint64_t next=at+46+name_bytes+extra+comment;uint32_t attributes=le32_74(h+38),mode=(attributes>>16)&0xf000;
        if(next>central+central_bytes||!name_bytes||name_bytes>=sizeof(members[i].name)||(flags&~0x080eu)||(method!=0&&method!=8)||le16_74(h+34)||
           (mode&&mode!=0x8000&&mode!=0x4000))return V6_INVALID;
        zip_entry74 *e=&members[i];if(read74(app->path,at+46,e->name,name_bytes))return V6_IO;
        for(unsigned n=0;n<name_bytes;++n)if(!e->name[n])return V6_INVALID;
        e->folder=e->name[name_bytes-1]=='/'||(attributes&16)||mode==0x4000;
        unsigned slash=e->name[name_bytes-1]=='/';if(slash)e->name[name_bytes-1]=0;
        if(!zip_name74(e->name))return V6_INVALID;
        e->method=(uint16_t)method;e->crc=le32_74(h+16);e->compressed=le32_74(h+20);e->size=le32_74(h+24);uint64_t local_at=le32_74(h+42);
        if((e->folder&&e->size)||e->size>64u*1024*1024||e->compressed>64u*1024*1024||local_at>central||central-local_at<30||
           read74(app->path,local_at,local,30)||le32_74(local)!=0x04034b50||le16_74(local+6)!=flags||le16_74(local+8)!=method||le16_74(local+26)!=name_bytes)return V6_INVALID;
        e->data=local_at+30+name_bytes+le16_74(local+28);
        if(e->data>central||e->compressed>central-e->data)return V6_INVALID;
        char local_name[208];if(read74(app->path,local_at+30,local_name,name_bytes)||memcmp(local_name,e->name,name_bytes-slash)||(slash&&local_name[name_bytes-1]!='/'))return V6_INVALID;
        if(!method&&e->compressed!=e->size)return V6_INVALID;
        total+=e->size;if(total>64u*1024*1024)return V6_LIMIT;
        for(unsigned j=0;j<i;++j){unsigned a=length(e->name),b=length(members[j].name),n=a<b?a:b;
            if(!memcmp(e->name,members[j].name,n)&&((a==b)||(a<b&&members[j].name[a]=='/'&&!e->folder)||(b<a&&e->name[b]=='/'&&!members[j].folder)))return V6_INVALID;}
        at=next;
    }
    if(at!=central+central_bytes)return V6_INVALID;
    app->count74=count;app->offset74=central;app->total74=total;app->entered=1;app->offset=0;app->length=app->cursor=app->dirty=0;return 0;
}
static unsigned zip_parents74(char *path){
    for(unsigned i=1;path[i];++i)if(path[i]=='/'){path[i]=0;unsigned status=v6_mkdir(m6_volume(),path);path[i]='/';if(status&&status!=V6_EXISTS)return status;}return 0;
}
static unsigned zip_file74(application *app,const zip_entry74 *e,const char *target,uint8_t *input){
    v6_volume *volume=m6_volume();unsigned status=0,active=0;uint64_t consumed=0,produced=0;uint32_t crc=~0u;
    if(e->size){status=v6_stream_begin(volume,target,e->size);if(status)return status;active=1;}
    if(e->method==8){status=send_app(app,APP74_INFLATE_BEGIN,0,0,0);if(status)goto done;}
    for(;;){unsigned n=0;
        if((e->method!=8||!app->number)&&consumed<e->compressed){n=(unsigned)(e->compressed-consumed);if(n>4096)n=4096;
            status=read74(app->path,e->data+consumed,input,n);if(status)break;}
        const void *out=input;unsigned bytes=n;
        if(e->method==8){status=send_app(app,APP74_INFLATE_STEP,consumed+n<e->compressed,input,n);if(status)break;
            if(app->calc_error||app->operand<0||(uint64_t)app->operand>n||app->number<0||app->number>32768||app->length>4096||app->operation>1){status=V6_INVALID;break;}
            consumed+=(uint64_t)app->operand;out=app->text;bytes=app->length;
        }else consumed+=n;
        if(bytes>e->size-produced){status=V6_INVALID;break;}produced+=bytes;crc=crc_next74(crc,out,bytes);
        if(bytes&&active){status=v6_stream_append(volume,out,bytes);if(status)break;}
        if(e->method==8){if(!app->operation)break;if(!app->operand&&!bytes){status=V6_INVALID;break;}}
        else if(consumed==e->compressed)break;
    }
    if(!status&&(consumed!=e->compressed||produced!=e->size||~crc!=e->crc))status=V6_INVALID;
done:
    if(e->method==8&&!app->stopped){unsigned ended=send_app(app,APP74_INFLATE_END,0,0,0);if(!status)status=ended;}
    if(status){if(active)v6_stream_abort(volume);return status;}
    return active?v6_stream_finish(volume):v6_put(volume,target,0,0,0);
}
static void zip_extract74(application *app){
    download7_info download=download7_current();if(download.state>=DOWNLOAD7_RESOLVING&&download.state<=DOWNLOAD7_RECEIVING){notice(app,"Wait for the current download to finish, or cancel it in Downloads.");return;}
    if(zip_load74(app)){notice(app,"This ZIP could not be verified. No files were extracted.");return;}
    uint64_t pages[3]={0};unsigned allocated=0,status=0,count=app->count74;
    for(;allocated<3;++allocated){pages[allocated]=page_alloc();if(!pages[allocated])break;}
    if(allocated!=3){status=V6_FULL;goto cleanup;}
    for(unsigned i=0;i<count;++i)memcpy((zip_entry74 *)(uintptr_t)pages[i/16]+i%16,(zip_entry74 *)app->text+i,sizeof(zip_entry74));
    char folder[256];root_path(folder,"Downloads");unique_in(app->pending,folder,"Extracted","");if(!app->pending[0]){status=V6_FULL;goto cleanup;}
    for(unsigned i=0;i<count;++i){const zip_entry74 *e=(zip_entry74 *)(uintptr_t)pages[i/16]+i%16;if(length(app->pending)+1+length(e->name)>=256){status=V6_INVALID;goto cleanup;}}
    status=v6_mkdir(m6_volume(),app->pending);if(status)goto cleanup;
    for(unsigned i=0;i<count;++i){zip_entry74 e=*((zip_entry74 *)(uintptr_t)pages[i/16]+i%16);char target[256];copy(target,app->pending,256);append(target,"/",256);append(target,e.name,256);
        status=zip_parents74(target);if(status)break;
        if(e.folder){status=v6_mkdir(m6_volume(),target);if(status==V6_EXISTS)status=0;}
        else status=zip_file74(app,&e,target,(uint8_t *)(uintptr_t)pages[2]);
        if(status)break;
    }
cleanup:
    while(allocated)page_free(pages[--allocated]);
    if(!app->stopped)(void)zip_load74(app);
    notice(app,status?"Extraction stopped. The original ZIP is unchanged; check the partial output folder.":"Extracted into a new Downloads folder. Open it below.");
}
static void archives_init74(unsigned kind,application *app){
    if(kind!=ARCHIVES74)return;
    if(!app->path[0])root_path(app->path,"Downloads");const v6_entry *e=entry(app->path);
    if(e&&e->kind==1&&zip_load74(app)){root_path(app->path,"Downloads");notice(app,"Choose a supported ZIP: Stored or Deflate, up to 32 entries and 64 MiB expanded.");}
}
static void archives_draw74(desktop_window *w,application *app,uint64_t token){
    if(w->app!=ARCHIVES74)return;
    if(!app->entered){draw_picker74(w,app,token,"Choose a ZIP archive - Stored / Deflate");return;}
    int x=w->bounds.x+24,y=w->bounds.y+60;button(token,x,y,160,"Choose ZIP",ZIP_CHOOSE74,0);button(token,x+175,y,150,"Extract all",ZIP_EXTRACT74,1);
    if(app->pending[0])button(token,x+340,y,190,"Open output folder",ZIP_RESULT74,0);
    ui73_fit(x,y+55,w->bounds.width-48,app->path,1,ink());
    const zip_entry74 *entries=(const zip_entry74 *)app->text;unsigned rows=(unsigned)(w->bounds.height-240)/40;
    for(unsigned i=0;i<rows&&app->offset+i<app->count74;++i){const zip_entry74 *e=entries+app->offset+i;int row=y+100+(int)i*40;
        ui73_fit(x,row,w->bounds.width-190,e->name,1,ink());char size[40];decimal(size,e->size);append(size,e->folder?" (folder)":" bytes",40);text(w->bounds.x+w->bounds.width-150,row,size,1,ink());}
    if(!app->count74)text(x,y+110,"This archive is empty.",1,ink());
    button(token,x,w->bounds.y+w->bounds.height-100,110,"Previous",ZIP_PREV74,0);button(token,x+125,w->bounds.y+w->bounds.height-100,100,"Next",ZIP_NEXT74,0);
    ui73_paragraph(x,w->bounds.y+w->bounds.height-55,w->bounds.width-48,2,app->notice[0]?app->notice:"Extracts to a new folder. Encrypted ZIP, ZIP64 and links are not supported.",ink());
}
static unsigned archives_action74(desktop_window *w,application *app,unsigned command){
    if(w->app!=ARCHIVES74)return 0;
    if(command==ZIP_CHOOSE74){root_path(app->path,"Downloads");app->entered=app->offset=0;}
    else if(command==ZIP_EXTRACT74&&app->entered)zip_extract74(app);
    else if(command==ZIP_RESULT74&&app->pending[0]){uint64_t t=launch(FILES,0);desktop_window *f=desktop_lookup(&model,t);if(f)visit72(&apps[f-model.windows],app->pending,1);whole();return 1;}
    else if(command==ZIP_PREV74)app->offset=app->offset>=5?app->offset-5:0;
    else if(command==ZIP_NEXT74){unsigned rows=(unsigned)(w->bounds.height-240)/40;if(app->offset+rows<app->count74)app->offset+=rows;}
    else if(!app->entered&&command==PICK_UP74){char home[256];home72(home);if(!equal(home,app->path)){unsigned n=length(app->path);while(n&&app->path[n]!='/')--n;if(n)app->path[n]=0;}app->offset=0;}
    else if(!app->entered&&(command==IMAGE_PREV74||command==IMAGE_NEXT74)){unsigned rows=(unsigned)(w->bounds.height-210)/44;
        if(command==IMAGE_PREV74)app->offset=app->offset>=rows?app->offset-rows:0;else if(pick_child74(app,app->offset+rows))app->offset+=rows;}
    else if(!app->entered&&command>=PICK_ENTRY74&&command<PICK_ENTRY74+V6_ENTRIES){const v6_entry *e=pick_child74(app,command-PICK_ENTRY74);if(e){char previous[256];copy(previous,app->path,256);unsigned kind=e->kind;copy(app->path,e->path,256);app->offset=0;
        if(kind==1&&zip_load74(app)){copy(app->path,previous,256);notice(app,"Cannot open this ZIP. Check the supported formats and limits.");}}}
    else return 0;
    invalidate(w->bounds);return 1;
}
