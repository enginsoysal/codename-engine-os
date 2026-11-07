/* Streaming native formats: bounded buffers, validated dimensions and offsets. */
enum { CAPTURE74=10500,CAPTURE_OPEN74,IMAGE_CHOOSE74,IMAGE_PREV74,IMAGE_NEXT74,
       PICK_UP74=10900,PICK_ENTRY74=11000 };
static uint16_t le16_74(const uint8_t *p){return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t le32_74(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put32_74(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(v>>(i*8));}
static unsigned read74(const char *path,uint64_t off,void *out,unsigned n){
    unsigned bytes=0,status=v6_read(m6_volume(),path,off,out,n,&bytes);return status?status:bytes==n?0:V6_IO;
}
static const v6_entry *pick_child74(application *app,unsigned index){
    unsigned n=length(app->path),seen=0;v6_volume *v=m6_volume();
    for(unsigned i=0;i<V6_ENTRIES;++i){const v6_entry *e=&v->entries[i];if(!e->kind||memcmp(e->path,app->path,n)||e->path[n]!='/')continue;
        const char *tail=e->path+n+1;unsigned k=0;while(tail[k]&&tail[k]!='/')++k;if(tail[k])continue;
        if(seen++==index)return e;
    }return 0;
}
static void draw_picker74(desktop_window *w,application *app,uint64_t token,const char *heading){
    int x=w->bounds.x+24,y=w->bounds.y+65;unsigned width=(unsigned)w->bounds.width-48;
    text(x,y,heading,2,ink());button(token,x,y+42,90,"Up",PICK_UP74,0);
    ui73_fit(x+105,y+52,width-110,app->path,1,ink());
    unsigned rows=(unsigned)(w->bounds.height-210)/44;
    for(unsigned i=0;i<rows;++i){const v6_entry *e=pick_child74(app,app->offset+i);if(!e)break;
        const char *name=e->path;for(unsigned k=0;e->path[k];++k)if(e->path[k]=='/')name=e->path+k+1;
        int row=y+100+(int)i*44;button(token,x,row,(int)width,"",PICK_ENTRY74+app->offset+i,0);
        ui73_fit(x+10,row+10,width-100,name,1,ink());
        if(e->kind==2)text(x+(int)width-65,row+10,"Folder",1,ink());
    }
    if(!pick_child74(app,0))text(x,y+115,"This folder is empty.",1,ink());
    int bottom=w->bounds.y+w->bounds.height-75;
    button(token,x,bottom,110,"Previous",IMAGE_PREV74,0);button(token,x+120,bottom,100,"Next",IMAGE_NEXT74,0);
    ui73_fit(x,bottom+45,width,app->notice[0]?app->notice:"Choose a folder or file. Your documents are never changed by viewing them.",1,ink());
}
static unsigned image_load74(application *app){
    const v6_entry *e=entry(app->path);uint8_t h[54];
    if(!e||e->kind!=1||e->size<54||v6_verify(m6_volume(),app->path)||read74(app->path,0,h,sizeof(h)))return V6_IO;
    uint32_t width=le32_74(h+18),raw_height=le32_74(h+22),offset=le32_74(h+10),dib=le32_74(h+14);
    unsigned top=(raw_height>>31),height=top?(~raw_height+1):raw_height,bits=le16_74(h+28);
    if(h[0]!='B'||h[1]!='M'||dib<40||dib>124||offset<14+dib||!width||width>8192||!height||height>8192||
       le16_74(h+26)!=1||(bits!=24&&bits!=32)||le32_74(h+30))return V6_INVALID;
    uint64_t stride=((uint64_t)width*(bits/8)+3)&~UINT64_C(3);
    if(offset>e->size||stride*height>e->size-offset)return V6_INVALID;
    app->year74=width;app->month74=height;app->day74=bits/8;app->number=offset;app->operand=(int64_t)stride;app->operation=top;app->entered=1;app->source_crc=e->crc;return 0;
}
static void image_draw74(desktop_window *w,application *app,uint64_t token){
    if(!app->entered){draw_picker74(w,app,token,"Choose an image - BMP 24/32-bit");return;}
    int x=w->bounds.x+24,y=w->bounds.y+112;unsigned available_width=(unsigned)w->bounds.width-48,available_height=(unsigned)w->bounds.height-165;
    button(token,x,w->bounds.y+56,165,"Choose image",IMAGE_CHOOSE74,0);
    ui73_fit(x+180,w->bounds.y+66,available_width-180,app->path,1,ink());
    unsigned width=app->year74,height=app->month74,dw=available_width,dh=(unsigned)((uint64_t)height*dw/width);
    if(dh>available_height){dh=available_height;dw=(unsigned)((uint64_t)width*dh/height);}if(!dw||!dh)return;
    x+=(int)(available_width-dw)/2;y+=(int)(available_height-dh)/2;
    if(!ui74_intersects((unsigned)x,(unsigned)y,dw,dh))return;
    for(unsigned dy=0;dy<dh;++dy){unsigned sy=(unsigned)((uint64_t)dy*height/dh),cached=UINT32_MAX;
        if(!(dy&15))audio74_pump();
        if(!app->operation)sy=height-sy-1;
        for(unsigned dx=0;dx<dw;++dx){unsigned sx=(unsigned)((uint64_t)dx*width/dw),block=sx/2048;
            if(block!=cached){unsigned pixels=width-block*2048;if(pixels>2048)pixels=2048;
                if(read74(app->path,(uint64_t)app->number+(uint64_t)sy*(uint64_t)app->operand+(uint64_t)block*2048*app->day74,app->text,pixels*app->day74)){
                    notice(app,"Image read failed. The original file is unchanged.");return;
                }cached=block;
            }
            const uint8_t *p=(const uint8_t *)app->text+(sx%2048)*app->day74;
            rectangle((desktop_rect){x+(int)dx,y+(int)dy,1,1},(uint32_t)p[2]<<16|(uint32_t)p[1]<<8|p[0]);
        }
    }
    char info[80],part[24];decimal(info,width);append(info," x ",80);decimal(part,height);append(info,part,80);append(info," pixels - fit to window",80);
    text(w->bounds.x+24,w->bounds.y+w->bounds.height-29,info,1,ink());
}
typedef struct {uint32_t width,height,stride,total;} screenshot74;
static unsigned screenshot_source74(void *context,uint64_t off,void *out,unsigned bytes){
    audio74_pump();
    screenshot74 *s=context;uint8_t *p=out,header[54]={0};header[0]='B';header[1]='M';put32_74(header+2,s->total);put32_74(header+10,54);
    put32_74(header+14,40);put32_74(header+18,s->width);put32_74(header+22,s->height);header[26]=1;header[28]=24;put32_74(header+34,s->stride*s->height);
    unsigned i=0;
    while(i<bytes&&off+i<54){p[i]=header[off+i];++i;}
    while(i<bytes){uint64_t pos=off+i-54;unsigned within=(unsigned)(pos%s->stride),y=(unsigned)(pos/s->stride);
        unsigned row=s->stride-within;if(row>bytes-i)row=bytes-i;unsigned end=i+row;
        while(i<end){if(within>=s->width*3){p[i++]=0;++within;continue;}
            unsigned channel=within%3;uint32_t rgb=ui74_rgb(within/3,s->height-y-1);
            do{p[i++]=(uint8_t)(rgb>>(channel*8));++within;++channel;}while(channel<3&&i<end);
        }
    }return 0;
}
static void screenshot_take74(application *app){
    char folder[256];root_path(folder,"Pictures");(void)v6_mkdir(m6_volume(),folder);append(folder,"/Screenshots",256);
    unsigned status=v6_mkdir(m6_volume(),folder);if(status&&status!=V6_EXISTS){notice(app,"Cannot create the Screenshots folder.");return;}
    unique_in(app->path,folder,"Screenshot",".bmp");if(!app->path[0]){notice(app,"No free screenshot name is available.");return;}
    screenshot74 s={ui73_width(),ui73_height(),(ui73_width()*3+3)&~3u,0};s.total=54+s.stride*s.height;
    status=v6_put(m6_volume(),app->path,s.total,screenshot_source74,&s);
    notice(app,status?"Screenshot could not be saved. Existing files were kept.":"Screenshot saved in Pictures / Screenshots. Open it below.");
    if(status)app->path[0]=0;
}
static void formats_init74(unsigned kind,application *app){
    if(kind==IMAGES74){
        if(!app->path[0])root_path(app->path,"Pictures");
        const v6_entry *e=entry(app->path);
        if(e&&e->kind==1){if(image_load74(app)){root_path(app->path,"Pictures");notice(app,"Cannot open this image. Choose an uncompressed 24/32-bit BMP.");}}
    }
}
static void formats_draw74(desktop_window *w,application *app,uint64_t token){
    if(w->app==IMAGES74)image_draw74(w,app,token);
    if(w->app==SCREENSHOT74){int x=w->bounds.x+24,y=w->bounds.y+70;
        text(x,y,"Keep a picture of your desktop",2,ink());
        ui73_paragraph(x,y+48,w->bounds.width-48,3,"Capture the real screen at its native resolution. Pictures are saved as BMP files in your Pictures / Screenshots folder.",ink());
        button(token,x,y+160,190,"Take screenshot",CAPTURE74,1);
        if(app->path[0])button(token,x+205,y+160,150,"Open picture",CAPTURE_OPEN74,0);
        ui73_paragraph(x,y+230,w->bounds.width-48,4,app->notice,ink());
    }
}
static unsigned formats_action74(desktop_window *w,application *app,uint64_t token,unsigned command){
    (void)token;
    if(w->app==SCREENSHOT74&&command==CAPTURE74){screenshot_take74(app);invalidate(w->bounds);return 1;}
    if(w->app==SCREENSHOT74&&command==CAPTURE_OPEN74){if(app->path[0])launch(IMAGES74,app->path);whole();return 1;}
    if(w->app!=IMAGES74)return 0;
    if(command==IMAGE_CHOOSE74){root_path(app->path,"Pictures");app->entered=app->offset=0;notice(app,"");}
    else if(!app->entered&&command==PICK_UP74){char home[256];root_path(home,"");unsigned n=length(home);if(n)home[n-1]=0;
        if(!equal(app->path,home)){n=length(app->path);while(n&&app->path[n]!='/')--n;if(n)app->path[n]=0;}app->offset=0;
    }else if(!app->entered&&(command==IMAGE_PREV74||command==IMAGE_NEXT74)){
        unsigned rows=(unsigned)(w->bounds.height-210)/44;
        if(command==IMAGE_PREV74)app->offset=app->offset>=rows?app->offset-rows:0;
        else if(pick_child74(app,app->offset+rows))app->offset+=rows;
    }else if(!app->entered&&command>=PICK_ENTRY74&&command<PICK_ENTRY74+V6_ENTRIES){
        const v6_entry *e=pick_child74(app,command-PICK_ENTRY74);if(e){char previous[256];copy(previous,app->path,256);unsigned kind=e->kind;copy(app->path,e->path,256);app->offset=0;
            if(kind==1&&image_load74(app)){copy(app->path,previous,256);notice(app,"This format is not supported. Choose an uncompressed 24/32-bit BMP.");}}
    }else return 0;
    invalidate(w->bounds);return 1;
}
