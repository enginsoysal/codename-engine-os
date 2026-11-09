/* Native application surfaces. Privileged file/package operations use the broker. */
enum { MANAGE74=10000,PACKAGE_PREV74,PACKAGE_NEXT74,PACKAGE_CANCEL74,PACKAGE_APPLY74,
       SETTINGS_BACK74,
       CLOCK_WORLD74=10020,CLOCK_TIMER74,CLOCK_STOPWATCH74,CLOCK_ALARM74,
       ZONE74=10030,TIMER_LESS74,TIMER_MORE74,TIMER_START74,TIMER_CANCEL74,
       WATCH_START74,WATCH_RESET74,ALARM_HOUR74,ALARM_MINUTE74,ALARM_TOGGLE74,
       MONTH_PREV74=10050,MONTH_NEXT74,NOTE_SAVE74,NOTE_DELETE74,
       DAY74=10100,PACKAGE_OPEN74=10200,PACKAGE_CHANGE74=10300 };

typedef struct {
    uint64_t timer_deadline,watch_started,watch_accumulated;
    unsigned watch_running,alarm_minute,alarm_enabled,last_alarm_day;
} time_service74;
static time_service74 time74;
static uint64_t task_minute74=UINT64_MAX;

static unsigned line74(const char *s,unsigned at,unsigned width,char *out){
    unsigned start=at,n=0,space=0;out[0]=0;
    while(s[at]&&s[at]!='\n'&&n<252){out[n++]=s[at++];out[n]=0;if(out[n-1]==' ')space=n;
        if(ui73_measure(out,1)>width){if(space){at=start+space;n=space-1;}else if(n>1){--at;--n;}break;}}
    out[n]=0;if(s[at]=='\n')++at;while(s[at]==' ')++at;return at;
}
static unsigned page74(int x,int y,unsigned width,unsigned rows,const char *s,unsigned at){
    for(unsigned line=0;line<rows&&s[at];++line){char out[256];unsigned next=line74(s,at,width,out);text(x,y+(int)line*24,out,1,ink());if(next<=at)break;at=next;}return at;
}
static void note_view74(int x,int y,unsigned width,application *app){
    unsigned at=0,cursor_line=0;char out[256];
    while(app->text[at]){unsigned next=line74(app->text,at,width,out);if(next<=at||next>app->cursor)break;++cursor_line;at=next;}
    unsigned first=cursor_line>2?cursor_line-2:0,line=0;at=0;
    while(line<first){unsigned next=line74(app->text,at,width,out);if(next<=at)break;at=next;++line;}
    for(unsigned row=0;row<3;++row){unsigned next=line74(app->text,at,width,out);text(x,y+(int)row*24,out,1,ink());
        if(app->cursor>=at&&(app->cursor<next||!app->text[next])){unsigned prefix=app->cursor-at,n=length(out);if(prefix>n)prefix=n;out[prefix]=0;rectangle((desktop_rect){x+(int)ui73_measure(out,1),y+(int)row*24,2,20},preferences.accent);}
        if(next<=at||!app->text[next])break;at=next;
    }
}

static void padded74(char *out,unsigned value){out[0]=(char)('0'+value/10%10);out[1]=(char)('0'+value%10);out[2]=0;}
static void task_clock74(int bar){
    clock7_time now;if(clock7_read(&now))return;char value[40],part[24];
    padded74(value,now.hour);append(value,":",40);padded74(part,now.minute);append(value,part,40);
    unsigned width=ui73_measure(value,1);text((int)model.width-24-(int)width,bar+8,value,1,0xe5eef2);
    padded74(value,now.day);append(value," / ",40);padded74(part,now.month);append(value,part,40);append(value," / ",40);decimal(part,now.year);append(value,part,40);
    width=ui73_measure(value,1);text((int)model.width-24-(int)width,bar+33,value,1,0x95b1bd);
    if(control_count<DESKTOP_CONTROLS)controls[control_count++]=(control){{(int)model.width-180,bar,180,60},0,app7_available(CLOCK74)?CLOCK74:SETTINGS};
}
static void duration74(char *out,uint64_t seconds){
    char part[24];decimal(out,seconds/3600);append(out,":",32);padded74(part,(unsigned)(seconds/60%60));append(out,part,32);
    append(out,":",32);padded74(part,(unsigned)(seconds%60));append(out,part,32);
}
static unsigned days74(unsigned year,unsigned month){
    static const uint8_t days[]={31,28,31,30,31,30,31,31,30,31,30,31};
    if(month<1||month>12)return 0;
    return days[month-1]+(month==2&&year%4==0&&(year%100!=0||year%400==0));
}
static void calendar_path74(application *app){
    char part[24];root_path(app->path,"AppData");(void)v6_mkdir(m6_volume(),app->path);
    append(app->path,"/Calendar",256);(void)v6_mkdir(m6_volume(),app->path);append(app->path,"/",256);
    decimal(part,(uint64_t)app->year74);append(app->path,part,256);append(app->path,"-",256);
    padded74(part,(unsigned)app->month74);append(app->path,part,256);append(app->path,"-",256);
    padded74(part,app->day74);append(app->path,part,256);append(app->path,".txt",256);
}
static void calendar_load74(application *app){
    calendar_path74(app);unsigned bytes=0;const v6_entry *file=entry(app->path);
    app->text[0]=0;app->length=app->cursor=app->dirty=0;app->existed=0;app->source_crc=0;
    if(file){
        if(file->kind!=1||file->size>=APP7_TEXT||v6_verify(m6_volume(),app->path)||
           v6_read(m6_volume(),app->path,0,app->text,APP7_TEXT-1,&bytes)){
            notice(app,"This day's note could not be read. It has not been changed.");return;
        }
        app->existed=1;app->source_crc=file->crc;app->text[bytes]=0;
    }
    send_app(app,APP7_LOAD,0,app->text,bytes);
}
#include "formats74.inc"
#include "disks74.inc"
#include "calculator74-ui.inc"
#include "archives74.inc"
#include "browser74.inc"
#include "media74.inc"
#ifdef ENGINE_M8
#include "windows8.inc"
#endif
static void init74(unsigned kind,application *app){
    desktop_window *w=&model.windows[app-apps];uint64_t token=desktop_token(&model,(unsigned)(app-apps));
    #ifdef ENGINE_M8
    if(kind==WINDOWS8)init_windows8(app,token);
    #endif
    if(kind==CLOCK74){app->operand=5;desktop_place(&model,token,(desktop_rect){((int)model.width-700)/2,80,700,510});}
    if(kind==CALENDAR74){
        clock7_time now;unsigned valid=!clock7_read(&now);app->year74=valid?now.year:2026;app->month74=valid?now.month:1;app->day74=valid?now.day:1;
        desktop_place(&model,token,(desktop_rect){((int)model.width-760)/2,35,760,640});calendar_load74(app);
    }
    if(kind==SOFTWARE74)desktop_place(&model,token,(desktop_rect){((int)model.width-820)/2,50,820,620});
    if(kind==CALCULATOR)desktop_place(&model,token,(desktop_rect){((int)model.width-700)/2,30,700,650});
    if(kind==HELP74){
        static const char guide[]="Welcome to Engine OS\n\n"
            "Start opens your installed applications. Click a window title to bring it forward; drag its title to move it. "
            "The upper-right cross closes a window. Unsaved notes ask before closing.\n\n"
            "Files: Home contains your documents, downloads and pictures. Use the sidebar, Back, Forward and Up to browse. "
            "Create folder gives a real folder a name. System folders are protected.\n\n"
            "Settings: choose your accent, light or dark appearance and a suitable display size, then Save preferences. "
            "Manage applications remains available even when Software Manager is removed.\n\n"
            "Software Manager: Open launches an installed app. Remove asks for confirmation and keeps personal files. "
            "Close an app before removing it. Install restores its verified package from the offline cache. "
            "Files, Settings and shared system services are protected.\n\n"
            "Clock: fixed UTC offsets, a minute timer, a stopwatch and a daily UTC alarm. "
            "Timers continue while the app window is closed; restarting cancels timers. Alarms show a desktop message.\n\n"
            "Calendar: choose a month and day, type a private note and Save note. Notes stay local and survive restarting. "
            "Arrow keys move through text; Ctrl+S saves. Save or discard changes before choosing another day.\n\n"
            "Text Editor: open a text file from Files, edit, Save or Save as. Text is bounded to 8191 bytes. "
            "Calculator offers six decimal places, scientific functions and conversions. Angles use degrees; scientific results are approximate.\n\n"
            "Archive Manager extracts Stored/Deflate ZIP files into a fresh Downloads folder. Image Viewer opens BMP pictures; Screenshot saves a native BMP in Pictures/Screenshots.\n\n"
            "Browser is a bounded HTML/text browser with checked HTTPS and Links. JavaScript, CSS layout, forms and embedded pictures are not supported.\n\n"
            "Media Player accepts 16-bit PCM WAV at 44.1/48 kHz, mono or stereo, using a supported analog HDA output. Other codecs and video are not implemented.\n\n"
#ifdef ENGINE_M8
            "Windows Apps installs and runs the included original 7-Zip 26.06 x64 console program from MSI. "
            "Open Windows Apps, choose Install 7-Zip, confirm, then Open 7-Zip. Create and extract a test archive in Documents. "
            "Remove app keeps your documents. Licenses are available inside Windows Apps. "
            "Other Windows installers, Windows GUI programs and MS-DOS remain future work. "
#else
            "This is an independent operating system. Windows and MS-DOS application compatibility is not implemented yet. "
#endif
            "Virtual hardware support is limited to the documented profiles."
            ;send_app(app,APP7_LOAD,0,guide,sizeof(guide)-1);
    }
    formats_init74(kind,app);
    disks_init74(kind,app);
    archives_init74(kind,app);
    browser_init74(kind,app);
    media_init74(kind,app);
    (void)w;
}
static void draw_software74(desktop_window *w,application *app,uint64_t token){
    int x=w->bounds.x+24,y=w->bounds.y+64,width=w->bounds.width-48;
    text(x,y,"Your apps, your choice",2,ink());
    ui73_paragraph(x,y+36,width,2,"Remove an app without removing your documents or shared system services. Close its windows first.",ink());
    if(w->app==SETTINGS)button(token,x+width-105,y-6,105,"Appearance",SETTINGS_BACK74,0);
    unsigned first=w->app==SETTINGS?app->year74:app->offset,count=0,shown=0;
    unsigned rows=(unsigned)(w->bounds.height-220)/56;if(rows>6)rows=6;
    for(unsigned kind=1;kind<APP74_KINDS;++kind){if(!app74_ready(kind))continue;
        if(count++<first)continue;if(shown==rows)break;
        int row=y+100+(int)shown++*56;
        ui73_round(x,row,width,49,7,preferences.dark?0x203843:0xeaf0f3);
        ui73_fit(x+12,row+15,(unsigned)(width-310),titles[kind],1,ink());
        unsigned available=app7_available(kind);
        if(available)button(token,x+width-286,row+6,92,"Open",PACKAGE_OPEN74+kind,0);
        if(app74_required(kind))text(x+width-170,row+17,"Required",1,ink());
        else button(token,x+width-182,row+6,170,available?"Remove":"Install",PACKAGE_CHANGE74+kind,0);
    }
    int bottom=w->bounds.y+w->bounds.height-88;
    button(token,x,bottom,115,"Previous",PACKAGE_PREV74,0);button(token,x+125,bottom,100,"Next",PACKAGE_NEXT74,0);
    ui73_fit(x,bottom+48,(unsigned)width,app->notice[0]?app->notice:"Offline packages. Personal data is kept when removing an app.",1,ink());
}
static void draw_clock74(desktop_window *w,application *app,uint64_t token){
    int x=w->bounds.x+24,y=w->bounds.y+64;const char *tabs[]={"World clock","Timer","Stopwatch","Alarm"};
    for(unsigned i=0;i<4;++i)button(token,x+(int)i*155,y,145,tabs[i],CLOCK_WORLD74+i,app->offset==i);
    char value[64];uint64_t ticks=m3_clock_ticks();
    if(!app->offset){
        clock7_time now;if(clock7_read(&now)){text(x,y+90,"The hardware clock is unavailable.",2,ink());return;}
        static const int offsets[]={0,-5,1,8};static const char *zones[]={"UTC","UTC -05:00","UTC +01:00","UTC +08:00"};
        unsigned zone=app->entered%4;int seconds=(int)now.seconds+offsets[zone]*3600;
        seconds=(seconds+86400)%86400;padded74(value,(unsigned)seconds/3600);append(value,":",64);
        char part[8];padded74(part,(unsigned)seconds/60%60);append(value,part,64);append(value,":",64);padded74(part,(unsigned)seconds%60);append(value,part,64);
        text(x,y+85,value,6,ink());text(x,y+155,zones[zone],2,ink());
        button(token,x,y+210,210,"Change time zone",ZONE74,0);
        ui73_paragraph(x,y+270,w->bounds.width-48,3,"Fixed UTC offsets. Daylight-saving rules are not applied automatically.",ink());
    }else if(app->offset==1){
        uint64_t left=time74.timer_deadline>ticks?(time74.timer_deadline-ticks+99)/100:0;
        duration74(value,time74.timer_deadline?left:(uint64_t)app->operand*60);text(x,y+85,value,6,ink());
        button(token,x,y+180,100,"-1 min",TIMER_LESS74,0);button(token,x+110,y+180,100,"+1 min",TIMER_MORE74,0);
        button(token,x,y+240,160,"Start timer",TIMER_START74,1);button(token,x+175,y+240,140,"Cancel timer",TIMER_CANCEL74,0);
        text(x,y+305,"Continues with the window closed. Restart cancels it.",1,ink());
    }else if(app->offset==2){
        uint64_t elapsed=time74.watch_accumulated+(time74.watch_running?ticks-time74.watch_started:0);
        duration74(value,elapsed/100);text(x,y+85,value,6,ink());
        button(token,x,y+190,160,time74.watch_running?"Pause":"Start",WATCH_START74,1);
        button(token,x+175,y+190,130,"Reset",WATCH_RESET74,0);
        text(x,y+265,"One-second display. Measured with the monotonic clock.",1,ink());
    }else{
        padded74(value,time74.alarm_minute/60);append(value,":",64);char part[8];padded74(part,time74.alarm_minute%60);append(value,part,64);append(value," UTC",64);
        text(x,y+85,value,6,ink());button(token,x,y+180,130,"Hour +1",ALARM_HOUR74,0);button(token,x+145,y+180,130,"Minute +1",ALARM_MINUTE74,0);
        button(token,x,y+240,200,time74.alarm_enabled?"Turn alarm off":"Turn alarm on",ALARM_TOGGLE74,time74.alarm_enabled);
        text(x,y+305,"Daily desktop message; this session only. No sound yet.",1,ink());
    }
    ui73_fit(x,w->bounds.y+w->bounds.height-35,(unsigned)(w->bounds.width-48),app->notice,1,ink());
}
static void draw_calendar74(desktop_window *w,application *app,uint64_t token){
    int x=w->bounds.x+24,y=w->bounds.y+60;char title[48],part[24];
    static const char *months[]={"January","February","March","April","May","June","July","August","September","October","November","December"};
    copy(title,months[(unsigned)app->month74-1],48);append(title," ",48);decimal(part,(uint64_t)app->year74);append(title,part,48);
    button(token,x,y,100,"Previous",MONTH_PREV74,0);text(x+120,y+10,title,2,ink());button(token,x+540,y,100,"Next",MONTH_NEXT74,0);
    static const char *week[]={"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
    clock7_time first;unsigned leading=0;if(!clock7_calendar((unsigned)app->year74,(unsigned)app->month74,1,0,0,0,&first))leading=(first.days+5)%7;
    int cell=(w->bounds.width-48)/7;
    for(unsigned i=0;i<7;++i)text(x+(int)i*cell+12,y+64,week[i],1,ink());
    unsigned days=days74((unsigned)app->year74,(unsigned)app->month74);
    for(unsigned day=1;day<=days;++day){unsigned p=leading+day-1;decimal(part,day);
        button(token,x+(int)(p%7)*cell,y+90+(int)(p/7)*40,cell-8,part,DAY74+day,app->day74==day);}
    text(x,y+342,"Private note for the selected day",1,ink());
    ui73_round(x,y+369,w->bounds.width-48,84,7,preferences.dark?0x10212a:0xffffff);
    note_view74(x+10,y+375,w->bounds.width-68,app);
    button(token,x,y+465,150,"Save note",NOTE_SAVE74,1);button(token,x+165,y+465,160,"Clear note",NOTE_DELETE74,0);
    ui73_fit(x,w->bounds.y+w->bounds.height-28,w->bounds.width-48,app->notice[0]?app->notice:app->dirty?"Unsaved note. Ctrl+S saves.":"Choose a day, type a note, then save. Notes stay on this computer.",1,ink());
}
static void draw74(desktop_window *w,application *app,uint64_t token){
#ifdef ENGINE_M8
    if(w->app==WINDOWS8){draw_windows8(w,app,token);return;}
#endif
    int x=w->bounds.x+24,y=w->bounds.y+64;
    if(w->app==DISKS74||w->app==RECOVERY74){disks_draw74(w,app,token);return;}
    if(app->closing==5){
        text(x,y,"Remove this application?",2,ink());text(x,y+48,titles[(unsigned)app->operand],2,ink());
        ui73_paragraph(x,y+100,w->bounds.width-48,4,"Only this application's installed package will be removed. Your documents, private app data and shared system services will be kept. Close all its windows first.",ink());
        button(token,x,y+225,130,"Keep app",PACKAGE_CANCEL74,1);button(token,x+145,y+225,170,"Remove app",PACKAGE_APPLY74,0);
        ui73_paragraph(x,y+285,w->bounds.width-48,3,app->notice,ink());return;
    }
    if(w->app==SOFTWARE74||w->app==SETTINGS)draw_software74(w,app,token);
    if(w->app==CLOCK74)draw_clock74(w,app,token);
    if(w->app==CALENDAR74)draw_calendar74(w,app,token);
    formats_draw74(w,app,token);
    archives_draw74(w,app,token);
    browser_draw74(w,app,token);
    media_draw74(w,app,token);
    if(w->app==HELP74){
        text(x,y,"Make yourself at home",2,ink());
        app->source_crc=page74(x,y+48,w->bounds.width-48,(unsigned)(w->bounds.height-170)/24,app->text,app->offset);
        button(token,x,w->bounds.y+w->bounds.height-60,140,"Previous",PACKAGE_PREV74,0);
        button(token,x+155,w->bounds.y+w->bounds.height-60,140,"Read more",PACKAGE_NEXT74,0);
    }
}
static unsigned action74(desktop_window *w,application *app,uint64_t token,unsigned command){
    (void)token;
    if(command<10000)return 0;
#ifdef ENGINE_M8
    if(action_windows8(w,app,token,command))return 1;
#endif
    if(calculator_action74(w,app,command))return 1;
    if(archives_action74(w,app,command))return 1;
    if(browser_action74(w,app,token,command))return 1;
    if(media_action74(w,app,command))return 1;
    if(disks_action74(w,app,token,command))return 1;
    if(formats_action74(w,app,token,command))return 1;
    if(command==MANAGE74&&w->app==SETTINGS){app->offset=74;notice(app,"");invalidate(w->bounds);return 1;}
    if(command==SETTINGS_BACK74&&w->app==SETTINGS){app->offset=0;invalidate(w->bounds);return 1;}
    if(command>=PACKAGE_OPEN74&&command<PACKAGE_OPEN74+APP74_KINDS){launch(command-PACKAGE_OPEN74,0);whole();return 1;}
    if((w->app==SOFTWARE74||w->app==SETTINGS)&&command>=PACKAGE_CHANGE74&&command<PACKAGE_CHANGE74+APP74_KINDS){
        unsigned kind=command-PACKAGE_CHANGE74;
        if(app74_required(kind)){notice(app,"This component is required by Engine OS and cannot be removed.");}
        else if(app7_available(kind)){app->operand=kind;app->closing=5;notice(app,"");}
        else{unsigned status=app7_package(kind,1);preferences.calculator=app7_available(CALCULATOR);
            notice(app,status?"Installation failed. The existing system was kept.":"Installed from the verified offline package cache.");}
    }else if(command==PACKAGE_CANCEL74){app->closing=0;notice(app,"Application kept.");}
    else if(command==PACKAGE_APPLY74&&app->closing==5){
        unsigned kind=(unsigned)app->operand,active=0;
        for(unsigned i=0;i<DESKTOP_WINDOWS;++i)if(model.windows[i].live&&model.windows[i].app==kind)active=1;
        unsigned status=active?V6_RO:app7_package(kind,0);
        if(!status){if(kind==CLOCK74)memset(&time74,0,sizeof(time74));app->closing=0;preferences.calculator=app7_available(CALCULATOR);notice(app,"Application removed. Personal files and shared services were kept.");}
        else notice(app,active?"Close every window of this app first, then try again.":"Removal failed. The installed package was kept.");
    }else if(command==PACKAGE_PREV74||command==PACKAGE_NEXT74){
        if(w->app==HELP74){
            if(command==PACKAGE_PREV74&&app->existed){--app->existed;memcpy(&app->offset,app->pending+app->existed*4,4);}
            else if(command==PACKAGE_NEXT74&&app->source_crc<app->length&&app->existed<64){memcpy(app->pending+app->existed*4,&app->offset,4);++app->existed;app->offset=app->source_crc;}
        }
        else if(w->app==SOFTWARE74||w->app==SETTINGS){
            unsigned *position=w->app==SETTINGS?&app->year74:&app->offset,rows=(unsigned)(w->bounds.height-220)/56,total=0;if(rows>6)rows=6;
            for(unsigned kind=1;kind<APP74_KINDS;++kind)if(app74_ready(kind))++total;
            if(command==PACKAGE_PREV74)*position=*position>=rows?*position-rows:0;
            else if(*position+rows<total)*position+=rows;
        }
    }else if(w->app==CLOCK74){
        uint64_t ticks=m3_clock_ticks();
        if(command>=CLOCK_WORLD74&&command<=CLOCK_ALARM74)app->offset=command-CLOCK_WORLD74;
        if(command==ZONE74)app->entered=(app->entered+1)%4;
        if(command==TIMER_LESS74&&app->operand>1)--app->operand;
        if(command==TIMER_MORE74&&app->operand<1440)++app->operand;
        if(command==TIMER_START74){time74.timer_deadline=ticks+(uint64_t)app->operand*6000;notice(app,"Timer started. You can close this window.");}
        if(command==TIMER_CANCEL74){time74.timer_deadline=0;notice(app,"Timer cancelled.");}
        if(command==WATCH_START74){if(time74.watch_running)time74.watch_accumulated+=ticks-time74.watch_started;else time74.watch_started=ticks;time74.watch_running^=1;}
        if(command==WATCH_RESET74){time74.watch_accumulated=0;time74.watch_started=ticks;}
        if(command==ALARM_HOUR74)time74.alarm_minute=(time74.alarm_minute+60)%1440;
        if(command==ALARM_MINUTE74)time74.alarm_minute=(time74.alarm_minute/60)*60+(time74.alarm_minute+1)%60;
        if(command==ALARM_TOGGLE74)time74.alarm_enabled^=1;
    }else if(w->app==CALENDAR74){
        if(command==NOTE_SAVE74){save_document(app);}
        else if(command==NOTE_DELETE74){send_app(app,APP7_CLEAR,0,0,0);notice(app,"Note cleared in this window. Save to keep the change.");}
        else if(command==MONTH_PREV74||command==MONTH_NEXT74||(command>DAY74&&command<=DAY74+31)){
            if(app->dirty){notice(app,"Save your note or clear and save it before choosing another date.");invalidate(w->bounds);return 1;}
            if(command==MONTH_PREV74){if(app->month74>1)--app->month74;else if(app->year74>2020){--app->year74;app->month74=12;}app->day74=1;}
            else if(command==MONTH_NEXT74){if(app->month74<12)++app->month74;else if(app->year74<2099){++app->year74;app->month74=1;}app->day74=1;}
            else app->day74=command-DAY74;
            notice(app,"");calendar_load74(app);
        }
    }
    invalidate(w->bounds);return 1;
}
static unsigned key74(desktop_window *w,application *app,uint64_t token,int key){
    if(browser_key74(w,app,token,key))return 1;
    if(disks_key74(w,app,key))return 1;
    if(w->app==CALENDAR74){
        if(key==19)action74(w,app,token,NOTE_SAVE74);
        else if((key>=32&&key<=126)||key==8||key==10||key==128||key==129||key==130||key==131||key==132){
            unsigned was_dirty=app->dirty;send_app(app,APP7_KEY,(unsigned)key,0,0);
            invalidate((desktop_rect){w->bounds.x+24,w->bounds.y+429,w->bounds.width-48,84});
            if(was_dirty!=app->dirty||app->stopped){if(app->dirty&&!app->stopped)notice(app,"");invalidate((desktop_rect){w->bounds.x+24,w->bounds.y+w->bounds.height-28,w->bounds.width-48,24});}
        }
        else return 0;return 1;
    }
    if(w->app==HELP74&&(key==133||key==134)){action74(w,app,token,key==133?PACKAGE_PREV74:PACKAGE_NEXT74);return 1;}
    return 0;
}
static void tick74(void){
    uint64_t ticks=m3_clock_ticks();
    if(time74.timer_deadline&&ticks>=time74.timer_deadline){time74.timer_deadline=0;copy(shell_notice,"Clock: your timer has finished.",sizeof(shell_notice));whole();}
    clock7_time now;
    if(!clock7_read(&now)){
        uint64_t minute=(uint64_t)now.days*1440+now.hour*60+now.minute;
        if(minute!=task_minute74){task_minute74=minute;invalidate((desktop_rect){(int)model.width-180,(int)model.height-60,180,60});}
        if(time74.alarm_enabled&&now.hour*60+now.minute==time74.alarm_minute&&time74.last_alarm_day!=now.days){time74.last_alarm_day=now.days;copy(shell_notice,"Clock: it is time for your daily alarm.",sizeof(shell_notice));whole();}
    }
    for(unsigned i=0;i<DESKTOP_WINDOWS;++i){desktop_window *w=&model.windows[i];application *app=&apps[i];
        if(w->live&&!w->minimized&&w->app==CLOCK74&&app->offset<3)
            invalidate((desktop_rect){w->bounds.x+24,w->bounds.y+144,w->bounds.width-48,64});
        if(w->live&&!w->minimized&&w->app==MEDIA74&&app->entered&&audio74_current().state==AUDIO74_PLAYING)
            invalidate((desktop_rect){w->bounds.x+24,w->bounds.y+274,w->bounds.width-48,65});
    }
}
