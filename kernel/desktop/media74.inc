/* The optional player frontend owns no shared audio driver or desktop service. */
enum { MEDIA_CHOOSE74=18000,MEDIA_PRIMARY74,MEDIA_STOP74,MEDIA_QUIETER74,MEDIA_LOUDER74,MEDIA_MUTE74 };
static unsigned previous_volume74=100;
static void media_duration74(char *out,uint64_t frames,unsigned rate){
    uint64_t ms=rate?frames*1000/rate:0;duration74(out,ms/1000);append(out,".",32);
    char digits[4]={(char)('0'+ms/100%10),(char)('0'+ms/10%10),(char)('0'+ms%10),0};append(out,digits,32);
}
static void media_init74(unsigned kind,application *app){
    if(kind!=MEDIA74)return;uint64_t token=desktop_token(&model,(unsigned)(app-apps));
    desktop_place(&model,token,(desktop_rect){((int)model.width-760)/2,65,760,580});
    if(!app->path[0])root_path(app->path,"Music");const v6_entry *e=entry(app->path);
    if(e&&e->kind==1){if(!audio74_inspect(app->path))app->entered=1;else{root_path(app->path,"Music");notice(app,audio74_current().message);}}
}
static void media_draw74(desktop_window *w,application *app,uint64_t token){
    if(w->app!=MEDIA74)return;
    if(!app->entered){draw_picker74(w,app,token,"Choose your audio - PCM WAV");
        if(audio74_current().state==AUDIO74_PLAYING||audio74_current().state==AUDIO74_PAUSED){
            button(token,w->bounds.x+264,w->bounds.y+w->bounds.height-75,140,"Stop audio",MEDIA_STOP74,0);}
        return;}
    int x=w->bounds.x+24,y=w->bounds.y+58,width=w->bounds.width-48;audio74_info audio=audio74_current();
    button(token,x,y,170,"Choose audio",MEDIA_CHOOSE74,0);ui73_fit(x+185,y+10,width-185,app->path,1,ink());
    ui73_round(x,y+66,width,128,11,preferences.dark?0x203c45:0xe6f2ee);
    ui73_round(x+25,y+92,76,76,18,preferences.accent);
    rectangle((desktop_rect){x+58,y+108,5,39},0x183c36);rectangle((desktop_rect){x+62,y+108,19,5},0x183c36);
    ui73_round(x+44,y+137,21,15,7,0x183c36);
    const char *name=app->path;for(unsigned i=0;app->path[i];++i)if(app->path[i]=='/')name=app->path+i+1;
    ui73_fit(x+126,y+93,width-148,name,2,ink());
    char value[80],part[32];decimal(value,audio.rate);append(value," Hz  -  ",80);append(value,audio.channels==1?"Mono source":"Stereo source",80);text(x+126,y+140,value,1,ink());
    media_duration74(value,audio.frames,audio.rate);append(value," / ",80);media_duration74(part,audio.total_frames,audio.rate);append(value,part,80);
    text(x,y+222,value,2,ink());ui73_round(x,y+264,width,7,3,preferences.dark?0x2c4551:0xd7e4e7);
    unsigned filled=audio.total_frames?(unsigned)(audio.frames*(unsigned)width/audio.total_frames):0;if(filled)ui73_round(x,y+264,filled,7,3,preferences.accent);
    button(token,x,y+297,165,audio.state==AUDIO74_PLAYING?"Pause":audio.state==AUDIO74_PAUSED?"Resume":"Play",MEDIA_PRIMARY74,1);
    button(token,x+180,y+297,120,"Stop",MEDIA_STOP74,0);
    decimal(value,audio.volume);append(value,"% volume",80);text(x,y+363,value,1,ink());
    button(token,x+150,y+352,100,"Quieter",MEDIA_QUIETER74,0);button(token,x+260,y+352,100,"Louder",MEDIA_LOUDER74,0);button(token,x+370,y+352,110,audio.volume?"Mute":"Unmute",MEDIA_MUTE74,0);
    ui73_paragraph(x,w->bounds.y+w->bounds.height-95,width,3,app->notice[0]?app->notice:audio.message,ink());
}
static void media_save_volume74(void){
    unsigned values[2]={0x41443734,audio74_current().volume};(void)v6_put(m6_volume(),"/System/Settings/audio74.bin",sizeof(values),memory_source,values);
}
static unsigned media_action74(desktop_window *w,application *app,unsigned command){
    if(w->app!=MEDIA74)return 0;
    if(command==MEDIA_CHOOSE74){root_path(app->path,"Music");app->entered=app->offset=0;notice(app,"");}
    else if(command==MEDIA_PRIMARY74&&app->entered){audio74_info audio=audio74_current();notice(app,"");
        if(audio.state==AUDIO74_PLAYING||audio.state==AUDIO74_PAUSED)audio74_pause();else (void)audio74_play(app->path);}
    else if(command==MEDIA_STOP74){audio74_stop();notice(app,"");}
    else if(command==MEDIA_QUIETER74||command==MEDIA_LOUDER74||command==MEDIA_MUTE74){unsigned volume=audio74_current().volume;
        if(command==MEDIA_QUIETER74)volume=volume>=10?volume-10:0;
        if(command==MEDIA_LOUDER74)volume=volume<=90?volume+10:100;
        if(command==MEDIA_MUTE74){if(volume){previous_volume74=volume;volume=0;}else volume=previous_volume74?previous_volume74:100;}
        audio74_volume(volume);media_save_volume74();notice(app,"");
    }else if(!app->entered&&command==PICK_UP74){char home[256];home72(home);if(!equal(home,app->path)){unsigned n=length(app->path);while(n&&app->path[n]!='/')--n;if(n)app->path[n]=0;}app->offset=0;}
    else if(!app->entered&&(command==IMAGE_PREV74||command==IMAGE_NEXT74)){unsigned rows=(unsigned)(w->bounds.height-210)/44;
        if(command==IMAGE_PREV74)app->offset=app->offset>=rows?app->offset-rows:0;else if(pick_child74(app,app->offset+rows))app->offset+=rows;}
    else if(!app->entered&&command>=PICK_ENTRY74&&command<PICK_ENTRY74+V6_ENTRIES){const v6_entry *e=pick_child74(app,command-PICK_ENTRY74);if(e){char previous[256];copy(previous,app->path,256);unsigned kind=e->kind;copy(app->path,e->path,256);app->offset=0;
        if(kind==1){if(!audio74_inspect(app->path)){app->entered=1;notice(app,"");}else{copy(app->path,previous,256);notice(app,audio74_current().message);}}}}
    else return 0;
    invalidate(w->bounds);return 1;
}
