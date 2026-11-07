#include <engine/desktop/session.h>
#include <engine/desktop/app.h>
#include <engine/network/network.h>
#include <engine/network/download.h>
#include <engine/network/clock.h>
#include <engine/m4.h>
#include <engine/m3.h>
#include "build.h"
#include "distro.h"

enum { FILES=1, EDITOR, SETTINGS, CALCULATOR, DOWNLOADS, DETAILS, TERMINAL };
enum { OPEN_FILES=1, OPEN_EDITOR, OPEN_SETTINGS, OPEN_CALCULATOR, OPEN_DOWNLOADS,
       OPEN_DETAILS, OPEN_TERMINAL, SAVE=20, COPY, CUT, PASTE, FILE_UP, FILE_NEW, FILE_FOLDER,
       STYLE_MINT=40, STYLE_LAVENDER, STYLE_AMBER, STYLE_DARK, STYLE_PAPER,
       CALCULATOR_OFF, CALCULATOR_ON, PREFERENCES_SAVE,
       DISCARD=60, KEEP_EDITING, CALCULATOR_CRASH=64, APP_SELFTEST, NETWORK_RETRY, DNS_PROBE, DOWNLOAD_START, DOWNLOAD_CANCEL, DOWNLOAD_FOLDER, URL_FOCUS, MEMORY_LIMIT,
       SAVE_AS=80,CONFIRM_FILENAME,FILENAME_FOCUS,TERMINAL_CLEAR,CALC_KEY=200,FILE_ROW=1000,FILE_DELETE_ROW=3000 };

typedef struct {
    union {
        app7_view view;
        struct {
            char text[APP7_TEXT];
            uint32_t length,cursor,dirty,calc_error;
            int64_t number,operand;
            uint32_t operation,entered;
        };
    };
    char path[256],notice[96];
    unsigned closing,offset,stopped,existed;
    uint32_t source_crc;
    char pending[256];
    unsigned name_cursor,name_selected;
} application;
typedef struct { desktop_rect rect; uint64_t window; unsigned action; } control;
typedef struct { uint64_t magic; uint32_t accent, dark, calculator, reserved; } preferences7;

static desktop_model model;
static application apps[DESKTOP_WINDOWS];
#define DESKTOP_CONTROLS 288u
static control controls[DESKTOP_CONTROLS];
static unsigned control_count, launcher, pointer_x, pointer_y, previous_buttons;
static unsigned dragging, drag_resize, drag_x, drag_y;
static uint64_t drag_window, paints, painted_pixels;
static desktop_rect drag_bounds;
static m6_preferences owner;
static preferences7 preferences;
static char clipboard[8192];
static char shell_notice[96];
static unsigned clipboard_length;
static unsigned pressure_on;
static unsigned control_active,focused_action;
static uint64_t focused_control_window;
static const os_boot_info_v0 *display;
static const char *titles[]={"", "Files", "Text Editor", "Settings", "Calculator", "Downloads", "System details", "Advanced Terminal"};

static unsigned length(const char *s) { unsigned n=0; while(s[n])++n; return n; }
static int equal(const char *a,const char *b) { while(*a&&*a==*b){++a;++b;}return *a==*b; }
static void copy(char *out,const char *in,unsigned capacity) {
    unsigned n=0;while(n+1<capacity&&in[n]){out[n]=in[n];++n;}out[n]=0;
}
static void append(char *out,const char *in,unsigned capacity) {
    unsigned n=length(out);if(n<capacity)copy(out+n,in,capacity-n);
}
static void decimal(char *out,uint64_t value) {
    char reverse[24];unsigned n=0;do{reverse[n++]=(char)('0'+value%10);value/=10;}while(value);
    unsigned i=0;while(n)out[i++]=reverse[--n];out[i]=0;
}
static void integer(char *out,int64_t value) {
    if(value<0){*out++='-';decimal(out,(uint64_t)(-(value+1))+1);}else decimal(out,(uint64_t)value);
}
static unsigned memory_source(void *context,uint64_t offset,void *out,unsigned bytes) {
    memcpy(out,(const uint8_t *)context+offset,bytes);return 0;
}
static void resource_log(void){
    page_stats stats=pages_stats();serial_text("M7_DESKTOP_STATS current=");serial_hex(stats.current_pages*4096);
    serial_text(" peak=");serial_hex(stats.peak_pages*4096);serial_text(" allocation_failures=");serial_hex(stats.allocation_failures);
    serial_text(" paints=");serial_hex(paints);serial_text(" pixels=");serial_hex(painted_pixels);
    serial_text(" visible_framebuffer=");serial_hex((uint64_t)display->framebuffer_stride_bytes*display->framebuffer_height);
    serial_text(" framebuffer_range=");serial_hex(display->framebuffer.byte_length);serial_text("\n");
}
static void root_path(char *out,const char *folder) {
    copy(out,"/Users/",256);append(out,owner.user,256);append(out,"/",256);append(out,folder,256);
}
static const v6_entry *entry(const char *path) {
    v6_volume *v=m6_volume();for(unsigned i=0;i<V6_ENTRIES;++i)
        if(v->entries[i].kind&&equal(v->entries[i].path,path))return &v->entries[i];
    return 0;
}
static void invalidate(desktop_rect r) { desktop_damage(&model,r); }
static void whole(void) { invalidate((desktop_rect){0,0,(int32_t)model.width,(int32_t)model.height}); }
static void task_damage(void) { invalidate((desktop_rect){0,(int32_t)model.height-60,(int32_t)model.width,60}); }
static void notice(application *app,const char *text) { copy(app->notice,text,sizeof(app->notice)); }
static void text(int x,int y,const char *value,unsigned scale,uint32_t color) {
    if(x>=0&&y>=0)gui6_text((unsigned)x,(unsigned)y,value,scale,color);
}
static void rectangle(desktop_rect r,uint32_t color) {
    if(r.width>0&&r.height>0&&r.x>=0&&r.y>=0)
        gui6_rect((unsigned)r.x,(unsigned)r.y,(unsigned)r.width,(unsigned)r.height,color);
}
static uint32_t ink(void) { return preferences.dark?0xe5eef2:0x1b3441; }
static uint32_t paper(void) { return preferences.dark?0x172933:0xf8fafb; }

static void button(uint64_t window,int x,int y,int width,const char *label,unsigned action,unsigned primary) {
    desktop_rect r={x,y,width,30};
    rectangle(r,primary?preferences.accent:preferences.dark?0x2a414f:0xe6edf0);
    char fitted[64];unsigned columns=width>18?(unsigned)(width-18)/8:0;
    if(columns>sizeof(fitted)-1)columns=sizeof(fitted)-1;
    unsigned count=length(label);if(count>columns)count=columns;
    memcpy(fitted,label,count);fitted[count]=0;
    if(label[count]&&count>=3){fitted[count-1]='.';fitted[count-2]='.';fitted[count-3]='.';}
    text(x+9,y+9,fitted,1,primary?0x102d29:ink());
    if(control_active&&focused_control_window==window&&focused_action==action){
        rectangle((desktop_rect){x,y,width,2},ink());rectangle((desktop_rect){x,y+28,width,2},ink());
        rectangle((desktop_rect){x,y,2,30},ink());rectangle((desktop_rect){x+width-2,y,2,30},ink());
    }
    if(control_count<sizeof(controls)/sizeof(*controls))controls[control_count++]=(control){r,window,action};
}
static void unique_in(char *out,const char *directory,const char *prefix,const char *suffix) {
    char number[24],base[256];copy(base,directory,sizeof(base));if(length(base)&&base[length(base)-1]!='/')append(base,"/",256);
    for(unsigned i=1;i<10000;++i){copy(out,base,256);append(out,prefix,256);decimal(number,i);append(out,number,256);append(out,suffix,256);unsigned reserved=0;for(unsigned slot=0;slot<DESKTOP_WINDOWS;++slot)
            if(model.windows[slot].live&&model.windows[slot].app==EDITOR&&apps[slot].path!=out&&equal(apps[slot].path,out))reserved=1;
        if(!entry(out)&&!reserved)return;}
    out[0]=0;
}
static void unique_path(char *out,const char *folder,const char *prefix,const char *suffix){char directory[256];root_path(directory,folder);unique_in(out,directory,prefix,suffix);}

static unsigned send_app(application *app,unsigned command,unsigned key,const void *data,unsigned bytes) {
    unsigned slot=(unsigned)(app-apps);
    unsigned status=app->stopped?V6_INVALID:app7_request(slot,command,key,data,bytes,&app->view);
    if(status){app->stopped=1;notice(app,"Application stopped safely. Close and reopen it.");}
    return status;
}

static uint64_t launch(unsigned kind,const char *path) {
    if(kind==CALCULATOR&&!preferences.calculator)return 0;
    unsigned offset=model.count*26;
    uint64_t token=desktop_open(&model,kind,(desktop_rect){170+(int)offset,90+(int)offset,700,470});
    if(!token){copy(shell_notice,"Eight windows are open. Close one before opening another.",sizeof(shell_notice));whole();serial_text("M7_WINDOW_LIMIT\n");return 0;}
    shell_notice[0]=0;
    unsigned slot=(unsigned)(uint32_t)token-1;
    application *app=&apps[slot];memset(app,0,sizeof(*app));
    unsigned opened=(kind==EDITOR||kind==CALCULATOR)?app7_open(slot,kind):0;
    if(opened){
        app->stopped=1;notice(app,opened==V6_FULL?"Not enough memory. Close an app, then close this window and retry.":"Application could not start. Check its installed package.");
    }
    if(kind==FILES)root_path(app->path,"Documents");
    if(kind==TERMINAL){copy(app->text,"Engine OS advanced terminal. Type help for commands.\nUse menu, exit or Escape to close this window.\nengine> ",sizeof(app->text));app->length=length(app->text);}
    if(kind==DOWNLOADS){copy(app->text,"https://raw.githubusercontent.com/enginsoysal/codename-engine-os/main/LICENSE",sizeof(app->text));app->length=app->cursor=length(app->text);}
    if(kind==EDITOR){
        if(path){
            const v6_entry *file=entry(path);unsigned bytes=0;
            if(!file||file->kind!=1||file->size>=sizeof(app->text)||v6_verify(m6_volume(),path)||
               v6_read(m6_volume(),path,0,app->text,sizeof(app->text)-1,&bytes)){
                notice(app,"Cannot open this file: integrity, size or I/O error.");
                unique_path(app->path,"Documents","Note",".txt");
            }else{copy(app->path,path,sizeof(app->path));app->length=bytes;app->cursor=bytes;app->text[bytes]=0;app->existed=1;app->source_crc=file->crc;}
        }else unique_path(app->path,"Documents","Note",".txt");
    }
    if(kind==EDITOR&&!app->stopped)send_app(app,APP7_LOAD,0,app->text,app->length);
    if(kind==DETAILS)resource_log();
    launcher=0;task_damage();serial_text("M7_WINDOW_OPEN app=");serial_hex(kind);serial_text("\n");return token;
}

static void save_document(application *app) {
    if(!app->path[0]){notice(app,"No available document name. Open Files and try again.");return;}
    const v6_entry *current=entry(app->path);
    if((current&&(!app->existed||current->crc!=app->source_crc))||(!current&&app->existed)){
        notice(app,"This document changed on disk. Reopen it before overwriting.");return;
    }
    unsigned status=v6_put(m6_volume(),app->path,app->length,memory_source,app->text);
    if(status){notice(app,"Save failed. Your unsaved text is still here.");serial_text("M7_DOCUMENT_SAVE_FAILED\n");}
    else {app->existed=1;const v6_entry *saved=entry(app->path);if(saved)app->source_crc=saved->crc;send_app(app,APP7_SAVED,0,0,0);notice(app,"Saved to the displayed file path.");serial_text("M7_DOCUMENT_SAVED\n");}
}

static void save_as(application *app){
    unsigned count=length(app->pending);
    if(!count||count>63||app->pending[count-1]==' '||app->pending[count-1]=='.'){
        notice(app,"Choose a filename of 1-63 characters, ending with a letter or extension.");return;}
    for(unsigned n=0;n<count;++n){unsigned c=(uint8_t)app->pending[n];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c==' '||c=='.'||c=='-'||c=='_')){notice(app,"Use letters, numbers, spaces, a dot, dash or underscore.");return;}}
    char path[256];root_path(path,"Documents");append(path,"/",sizeof(path));append(path,app->pending,sizeof(path));
    if(entry(path)){notice(app,"That filename already exists. Choose a new name; no file was overwritten.");return;}
    if(v6_put(m6_volume(),path,app->length,memory_source,app->text)){notice(app,"Save failed. Your original document and unsaved text are unchanged.");return;}
    copy(app->path,path,sizeof(app->path));app->existed=1;app->source_crc=entry(path)->crc;
    send_app(app,APP7_SAVED,0,0,0);app->closing=0;notice(app,"Saved with your chosen filename in Documents.");serial_text("M7_DOCUMENT_SAVE_AS\n");
}
static void draw_editor(desktop_window *w,application *app,uint64_t token) {
    int x=w->bounds.x+14,y=w->bounds.y+48,right=w->bounds.x+w->bounds.width-14;
    button(token,x,y,75,"Save",SAVE,1);button(token,x+85,y,85,"Copy all",COPY,0);
    button(token,x+170,y,75,"Cut all",CUT,0);button(token,x+255,y,75,"Paste",PASTE,0);button(token,x+340,y,100,"Save as",SAVE_AS,0);
    text(x,y+44,app->path,1,ink());
    rectangle((desktop_rect){x,y+66,right-x,w->bounds.height-155},preferences.dark?0x10212a:0xffffff);
    int tx=x+10,ty=y+78;unsigned columns=(unsigned)(right-x-20)/12;if(columns<1)columns=1;
    unsigned rows=(unsigned)(w->bounds.height>180?w->bounds.height-180:20)/18;
    unsigned cursor_line=0,column=0;
    for(unsigned i=0;i<app->cursor;++i){if(app->text[i]=='\n'||++column>=columns){++cursor_line;column=0;}}
    unsigned first_line=cursor_line>=rows&&rows?cursor_line-rows+1:0;
    unsigned line=0;column=0;
    for(unsigned i=0;i<=app->length;++i){
        if(i==app->cursor&&line>=first_line&&line<first_line+rows)
            rectangle((desktop_rect){tx+(int)column*12,ty+(int)(line-first_line)*18,2,15},preferences.accent);
        if(i==app->length)break;
        char value=app->text[i];
        if(value!='\n'&&line>=first_line&&line<first_line+rows){char glyph[2]={value,0};text(tx+(int)column*12,ty+(int)(line-first_line)*18,glyph,2,ink());}
        if(value=='\n'||++column>=columns){++line;column=0;}
    }
    int bottom=w->bounds.y+w->bounds.height;
    text(x,bottom-43,app->dirty?"Unsaved changes":"Saved document",1,app->dirty?0xd49644:ink());
    text(x,bottom-24,app->notice[0]?app->notice:"Ctrl+S save  Ctrl+C copy all  Ctrl+V paste  arrows move",1,ink());
}

static const v6_entry *child_at(application *app,unsigned wanted) {
    v6_volume *v=m6_volume();unsigned size=length(app->path),count=0;
    for(unsigned i=0;i<V6_ENTRIES;++i){const v6_entry *file=&v->entries[i];
        if(!file->kind||memcmp(file->path,app->path,size)||file->path[size]!='/')continue;
        const char *tail=file->path+size+1;unsigned n=0;while(tail[n]&&tail[n]!='/')++n;
        if(!tail[n]&&count++==wanted)return file;
    }
    return 0;
}
static void draw_files(desktop_window *w,application *app,uint64_t token) {
    int x=w->bounds.x+14,y=w->bounds.y+48;
    button(token,x,y,60,"Up",FILE_UP,0);button(token,x+70,y,110,"New text",FILE_NEW,1);
    button(token,x+190,y,110,"New folder",FILE_FOLDER,0);
    text(x,y+45,app->path,1,ink());
    unsigned rows=(unsigned)(w->bounds.height-145)/30;
    if(rows>14)rows=14;
    for(unsigned i=0;i<rows;++i){const v6_entry *file=child_at(app,app->offset+i);if(!file)break;
        char label[96];copy(label,file->kind==2?"[Folder] ":"[File]   ",sizeof(label));append(label,file->path+length(app->path)+1,sizeof(label));
        button(token,x,y+70+(int)i*30,w->bounds.width-120,label,FILE_ROW+app->offset+i,0);
        button(token,x+w->bounds.width-110,y+70+(int)i*30,80,"Delete",FILE_DELETE_ROW+app->offset+i,0);
    }
    if(!child_at(app,app->offset))text(x,y+80,"This folder is empty. Create a text file or a folder above.",1,ink());
    text(x,w->bounds.y+w->bounds.height-24,app->notice[0]?app->notice:"Click a folder or a text file. Arrow keys scroll the list.",1,ink());
}
static void draw_settings(desktop_window *w,application *app,uint64_t token) {
    int x=w->bounds.x+20,y=w->bounds.y+55;
    text(x,y,"Make Engine OS yours",2,ink());
    text(x,y+40,"Accent",1,ink());
    button(token,x,y+60,90,"Mint",STYLE_MINT,preferences.accent==0x62e5bd);
    button(token,x+100,y+60,90,"Lavender",STYLE_LAVENDER,preferences.accent==0x9caeff);
    button(token,x+200,y+60,90,"Amber",STYLE_AMBER,preferences.accent==0xf4c47c);
    text(x,y+110,"Appearance",1,ink());
    button(token,x,y+130,130,"Dark windows",STYLE_DARK,preferences.dark);
    button(token,x+140,y+130,130,"Light windows",STYLE_PAPER,!preferences.dark);
    text(x,y+185,"Optional Calculator component",1,ink());
    button(token,x,y+205,150,preferences.calculator?"Remove Calculator":"Install Calculator",preferences.calculator?CALCULATOR_OFF:CALCULATOR_ON,0);
    button(token,x,y+260,170,"Save preferences",PREFERENCES_SAVE,1);
    text(x,w->bounds.y+w->bounds.height-28,app->notice[0]?app->notice:"Personal files and required components stay available.",1,ink());
}
static void draw_calculator(desktop_window *w,application *app,uint64_t token) {
    int x=w->bounds.x+22,y=w->bounds.y+55;char number[32];integer(number,app->number);
    rectangle((desktop_rect){x,y,w->bounds.width-44,50},preferences.dark?0x10212a:0xe5eeed);
    text(x+14,y+16,app->calc_error?"Out of range / invalid operation":number,2,ink());
    const char *labels[]={"7","8","9","/","4","5","6","*","1","2","3","-","C","0","=","+"};
    for(unsigned i=0;i<16;++i)button(token,x+(int)(i%4)*62,y+72+(int)(i/4)*42,52,labels[i],CALC_KEY+i,0);
    button(token,x+290,y+72,150,"Crash test",CALCULATOR_CRASH,0);
    text(x,y+270,"Checked signed integer arithmetic. Type keys or click.",1,ink());
}
static void terminal_output(char c,void *context){
    application *app=context;
    if(c==8){if(app->length)app->text[--app->length]=0;return;}
    if(c!='\n'&&c!='\t'&&(c<32||c>126))return;
    if(app->length>=sizeof(app->text)-1){unsigned remove=0;while(remove<app->length&&app->text[remove]!='\n')++remove;if(remove<app->length)++remove;else remove=app->length/2;
        for(unsigned n=remove;n<app->length;++n)app->text[n-remove]=app->text[n];app->length-=remove;}
    app->text[app->length++]=c;app->text[app->length]=0;
}
static void draw_terminal(desktop_window *w,application *app,uint64_t token){
    desktop_rect r=w->bounds;button(token,r.x+16,r.y+45,100,"Clear",TERMINAL_CLEAR,0);
    text(r.x+135,r.y+56,"Optional advanced commands. Type help; Escape closes.",1,ink());
    rectangle((desktop_rect){r.x+12,r.y+88,r.width-24,r.height-104},preferences.dark?0x10212a:0xffffff);
    unsigned columns=(unsigned)(r.width-44)/8,visible=(unsigned)(r.height-130)/14,rows=1,column=0;
    for(unsigned n=0;n<app->length;++n){if(app->text[n]=='\n'){++rows;column=0;}else if(++column>=columns){++rows;column=0;}}
    unsigned first=rows>visible?rows-visible:0,row=0;column=0;
    for(unsigned n=0;n<app->length;++n){char c=app->text[n];
        if(c=='\n'){++row;column=0;continue;}
        if(row>=first&&row<first+visible){char value[2]={c=='\t'?' ':c,0};text(r.x+22+(int)column*8,r.y+100+(int)(row-first)*14,value,1,ink());}
        if(++column>=columns){++row;column=0;}
    }
}

static void draw_window(unsigned slot) {
    desktop_window *w=&model.windows[slot];if(w->minimized)return;
    application *app=&apps[slot];desktop_rect r=w->bounds;uint64_t token=desktop_token(&model,slot);
    rectangle(r,model.focused==slot?preferences.accent:0x58717c);
    rectangle((desktop_rect){r.x+1,r.y+1,r.width-2,r.height-2},paper());
    rectangle((desktop_rect){r.x+1,r.y+1,r.width-2,34},preferences.dark?0x253c49:0xdce8ec);
    text(r.x+14,r.y+12,titles[w->app],2,ink());
    text(r.x+r.width-77,r.y+12,"_",2,ink());text(r.x+r.width-42,r.y+12,"x",2,ink());
    if(app->stopped){text(r.x+22,r.y+80,"Application stopped",2,ink());text(r.x+22,r.y+120,app->notice,1,ink());return;}
    if(app->closing==3){
        text(r.x+22,r.y+75,"Save document as",2,ink());text(r.x+22,r.y+110,"New filename in your Documents folder",1,ink());
        rectangle((desktop_rect){r.x+22,r.y+135,r.width-44,32},app->name_selected?preferences.accent:preferences.dark?0x10212a:0xffffff);
        text(r.x+30,r.y+147,app->pending,1,app->name_selected?0x102d29:ink());
        if(control_count<DESKTOP_CONTROLS)controls[control_count++]=(control){{r.x+22,r.y+135,r.width-44,32},token,FILENAME_FOCUS};
        button(token,r.x+22,r.y+190,150,"Save as",CONFIRM_FILENAME,1);button(token,r.x+185,r.y+190,150,"Cancel",KEEP_EDITING,0);
        text(r.x+22,r.y+250,app->notice,1,ink());return;
    }
    if(app->closing){text(r.x+22,r.y+80,app->closing==2?"Delete this item?":"Keep your unsaved changes?",2,ink());
        if(app->closing==2){const char *name=app->pending;for(unsigned n=0;app->pending[n];++n)if(app->pending[n]=='/')name=app->pending+n+1;text(r.x+22,r.y+110,name,1,ink());}
        button(token,r.x+22,r.y+(app->closing==2?150:130),150,app->closing==2?"Keep item":"Keep editing",KEEP_EDITING,1);
        button(token,r.x+185,r.y+(app->closing==2?150:130),150,app->closing==2?"Delete item":"Discard changes",DISCARD,0);return;}
    if(w->app==EDITOR)draw_editor(w,app,token);
    if(w->app==FILES)draw_files(w,app,token);
    if(w->app==SETTINGS)draw_settings(w,app,token);
    if(w->app==CALCULATOR)draw_calculator(w,app,token);
    if(w->app==TERMINAL)draw_terminal(w,app,token);
    if(w->app==DOWNLOADS){
        net7_configuration network=net7_current();
        text(r.x+20,r.y+60,"Your network",3,ink());
        text(r.x+20,r.y+110,network.message,1,ink());
        char address[32];decimal(address,network.address>>24);append(address,".",32);char part[24];decimal(part,(network.address>>16)&255);append(address,part,32);append(address,".",32);decimal(part,(network.address>>8)&255);append(address,part,32);append(address,".",32);decimal(part,network.address&255);append(address,part,32);
        text(r.x+20,r.y+150,"IPv4 address",1,ink());text(r.x+150,r.y+150,address,2,ink());
        button(token,r.x+20,r.y+200,150,"Retry network",NETWORK_RETRY,0);
        button(token,r.x+185,r.y+200,190,"Resolve example.com",DNS_PROBE,0);
        uint32_t resolved=0;unsigned dns=net7_dns_status(&resolved);
        text(r.x+20,r.y+250,dns==NET7_DNS_READY?"Real DNS lookup completed.":dns==NET7_DNS_PENDING?"Resolving the name...":dns==NET7_DNS_FAILED?"DNS lookup failed. Retry or check the network.":"You can test name resolution here.",1,ink());
        text(r.x+20,r.y+278,"HTTPS address - Enter downloads; Ctrl+L selects the address",1,ink());
        rectangle((desktop_rect){r.x+20,r.y+298,r.width-40,30},app->entered?preferences.accent:0xffffff);
        unsigned columns=(unsigned)(r.width-60)/8,start=app->cursor>columns?app->cursor-columns:0;
        char visible[128];unsigned available=app->length-start;if(available>columns)available=columns;if(available>sizeof(visible)-1)available=sizeof(visible)-1;
        memcpy(visible,app->text+start,available);visible[available]=0;text(r.x+27,r.y+309,visible,1,0x12252e);
        if(control_count<DESKTOP_CONTROLS)controls[control_count++]=(control){{r.x+20,r.y+298,r.width-40,30},token,URL_FOCUS};
        button(token,r.x+20,r.y+340,145,"Download",DOWNLOAD_START,1);
        button(token,r.x+180,r.y+340,100,"Cancel",DOWNLOAD_CANCEL,0);
        button(token,r.x+295,r.y+340,160,"Open Downloads",DOWNLOAD_FOLDER,0);
        download7_info download=download7_current();
        text(r.x+20,r.y+390,download.message?download.message:"Download a file securely. Closing this window cancels it.",1,ink());
        char progress[64],number[24];decimal(number,download.bytes);copy(progress,number,sizeof(progress));append(progress," bytes",sizeof(progress));
        if(download.tls_error){append(progress," - TLS error ",sizeof(progress));decimal(number,download.tls_error);append(progress,number,sizeof(progress));}
        text(r.x+20,r.y+420,progress,1,ink());
        clock7_time utc;if(!clock7_read(&utc)){char clock[64];decimal(clock,utc.year);append(clock,"-",64);decimal(number,utc.month);append(clock,number,64);append(clock,"-",64);decimal(number,utc.day);append(clock,number,64);append(clock," UTC ",64);decimal(number,utc.hour);append(clock,number,64);append(clock,":",64);decimal(number,utc.minute);append(clock,number,64);text(r.x+200,r.y+420,clock,1,ink());}
    }
    if(w->app==DETAILS){char value[32];text(r.x+20,r.y+65,"Engine OS 0.7",2,ink());text(r.x+20,r.y+105,BUILD_ID,1,ink());
        decimal(value,pages_stats().current_pages*4096);text(r.x+20,r.y+145,"Managed bytes",1,ink());text(r.x+230,r.y+145,value,1,ink());
        decimal(value,pages_stats().peak_pages*4096);text(r.x+20,r.y+175,"Managed peak",1,ink());text(r.x+230,r.y+175,value,1,ink());
        decimal(value,paints);text(r.x+20,r.y+205,"Damage repaints",1,ink());text(r.x+230,r.y+205,value,1,ink());
        button(token,r.x+20,r.y+290,260,"Test app isolation",APP_SELFTEST,0);
        button(token,r.x+20,r.y+340,260,pressure_on?"Restore memory budget":"Limit new app memory",MEMORY_LIMIT,pressure_on);
        text(r.x+20,r.y+395,pressure_on?"Memory limit active. Existing documents can still be saved.":"Optional diagnostics. Closing Details restores the memory budget.",1,ink());
        net7_nic_info nic=net7_nic_status();decimal(value,nic.status);
        text(r.x+20,r.y+235,"Ethernet driver status",1,ink());text(r.x+230,r.y+235,value,1,ink());
        text(r.x+20,r.y+265,"Static kernel, app and firmware memory are additional.",1,ink());}
    rectangle((desktop_rect){r.x+r.width-12,r.y+r.height-4,9,2},0x6a858e);
    rectangle((desktop_rect){r.x+r.width-7,r.y+r.height-9,4,2},0x6a858e);
}

static void render(void) {
    desktop_rect dirty;
    if(!model.damaged)return;
    /* Invalidate the old cursor before discarding its saved framebuffer pixels. */
    invalidate((desktop_rect){(int32_t)pointer_x,(int32_t)pointer_y,10,16});
    if(!desktop_take_damage(&model,&dirty))return;
    gui7_clip((unsigned)dirty.x,(unsigned)dirty.y,(unsigned)dirty.width,(unsigned)dirty.height);
    control_count=0;
    gui6_rect(0,0,model.width,model.height,DISTRO_DESKTOP_WALLPAPER);
    text(36,30,DISTRO_NAME,3,0xd8e9ec);text(38,64,DISTRO_DESKTOP_TAGLINE,1,0x8aabb7);
    const char *icons[]={"Files","Text Editor","Settings","Calculator","Downloads"};
    for(unsigned i=0;i<5;++i){if(i==3&&!preferences.calculator)continue;
        gui6_rect(38,110+i*83,38,36,preferences.accent);text(49,121+i*83,i==0?"F":i==1?"T":i==2?"S":i==3?"+":"D",2,0x12352e);
        text(38,155+i*83,icons[i],1,0xc4d9df);}
    for(unsigned i=0;i<model.count;++i){
        desktop_rect window=model.windows[model.order[i]].bounds;
        int x=window.x>dirty.x?window.x:dirty.x,y=window.y>dirty.y?window.y:dirty.y;
        int right=window.x+window.width<dirty.x+dirty.width?window.x+window.width:dirty.x+dirty.width;
        int bottom=window.y+window.height<dirty.y+dirty.height?window.y+window.height:dirty.y+dirty.height;
        gui7_clip((unsigned)x,(unsigned)y,right>x?(unsigned)(right-x):0,bottom>y?(unsigned)(bottom-y):0);
        draw_window(model.order[i]);
    }
    gui7_clip((unsigned)dirty.x,(unsigned)dirty.y,(unsigned)dirty.width,(unsigned)dirty.height);
    int bar=(int)model.height-60;
    if(shell_notice[0]){gui6_rect(20,(unsigned)bar-40,model.width-40,30,paper());text(30,bar-30,shell_notice,1,ink());}
    gui6_rect(0,(unsigned)bar,model.width,60,0x0b1922);
    button(0,16,bar+15,100,"E  Start",0,launcher);
    unsigned available=model.width-140-(model.width>=1000?180:0),step=model.count?available/model.count:100;
    if(step>100)step=100;
    for(unsigned i=0;i<model.count;++i){unsigned slot=model.order[i];desktop_window *w=&model.windows[slot];
        button(desktop_token(&model,slot),130+(int)(i*step),bar+15,(int)step-8,titles[w->app],70,model.focused==slot);}
    if(model.width>=1000)text((int)model.width-180,bar+23,
#ifdef ENGINE_M71
 "Engine OS 0.7.1",
#else
 "Engine OS 0.7",
#endif
1,0x95b1bd);
    if(launcher){
        gui6_rect(16,(unsigned)bar-350,300,340,paper());text(34,bar-329,owner.user,2,ink());
        unsigned row=0;for(unsigned i=1;i<=7;++i){if(i==CALCULATOR&&!preferences.calculator)continue;
            button(0,30,bar-293+(int)row++*38,270,titles[i],i,0);}
    }
    gui7_clip_reset();usb_mouse_draw();++paints;painted_pixels+=(uint64_t)dirty.width*dirty.height;
}

static void calculator(application *app,char key) {
    send_app(app,APP7_KEY,(unsigned char)key,0,0);
}

static void close_window(uint64_t token,int discard) {
    desktop_window *w=desktop_lookup(&model,token);if(!w)return;
    application *app=&apps[w-model.windows];
    if(w->app==EDITOR&&app->dirty&&!discard){app->closing=1;invalidate(w->bounds);return;}
    if(w->app==DOWNLOADS)download7_cancel();
    if(w->app==DETAILS&&pressure_on){pages_limit(0);pressure_on=0;}
    app7_close((unsigned)(w-model.windows));desktop_close(&model,token);memset(app,0,sizeof(*app));task_damage();
}
static void insert(application *app,const char *bytes,unsigned count) {
    if(count>sizeof(app->text)-1-app->length){notice(app,"This editor has reached its 8191-byte limit. Save your text.");return;}
    send_app(app,APP7_INSERT,0,bytes,count);
}

static void action(uint64_t token,unsigned command) {
    if(!token){if(!command){launcher^=1;whole();return;}launch(command,0);whole();return;}
    desktop_window *w=desktop_lookup(&model,token);if(!w)return;
    unsigned slot=(unsigned)(w-model.windows);application *app=&apps[slot];
    if(command==70){desktop_focus(&model,token);task_damage();return;}
    if(command==DISCARD){
        if(app->closing==2&&w->app==FILES){unsigned result=v6_remove(m6_volume(),app->pending);app->closing=0;app->pending[0]=0;
            notice(app,!result?"Item deleted. Other files are unchanged.":result==V6_INVALID?"The folder is not empty. Nothing was deleted.":"The item could not be deleted. Nothing else changed.");
            if(!result)serial_text("M7_FILE_DELETED\n");whole();
        }else close_window(token,1);return;
    }
    if(command==KEEP_EDITING){app->closing=0;app->pending[0]=0;invalidate(w->bounds);return;}
    if(app->stopped)return;
    if(command==SAVE&&w->app==EDITOR)save_document(app);
    if(command==SAVE_AS&&w->app==EDITOR){const char *name=app->path;for(unsigned n=0;app->path[n];++n)if(app->path[n]=='/')name=app->path+n+1;
        copy(app->pending,name,sizeof(app->pending));app->name_cursor=length(app->pending);app->name_selected=1;app->closing=3;notice(app,"Choose a new name. The original file will be kept.");}
    if(command==CONFIRM_FILENAME&&w->app==EDITOR&&app->closing==3)save_as(app);
    if(command==FILENAME_FOCUS&&app->closing==3){app->name_selected=0;app->name_cursor=length(app->pending);}
    if((command==COPY||command==CUT)&&w->app==EDITOR){memcpy(clipboard,app->text,app->length+1);clipboard_length=app->length;notice(app,"Document copied to the clipboard.");
        if(command==CUT)send_app(app,APP7_CLEAR,0,0,0);}
    if(command==PASTE&&w->app==EDITOR)insert(app,clipboard,clipboard_length);
    if(command==FILE_UP&&w->app==FILES){char root[256];root_path(root,"");unsigned n=length(app->path),min=length(root)-1;while(n>min&&app->path[n-1]!='/')--n;if(n>min)app->path[n-1]=0;app->offset=0;}
    if(command==FILE_NEW&&w->app==FILES){uint64_t opened=launch(EDITOR,0);desktop_window *created=desktop_lookup(&model,opened);
        if(created)unique_in(apps[created-model.windows].path,app->path,"Note",".txt");task_damage();}
    if(command==FILE_FOLDER&&w->app==FILES){char path[256];unique_in(path,app->path,"Folder","");unsigned result=path[0]?v6_mkdir(m6_volume(),path):V6_FULL;notice(app,result?"Folder could not be created.":"Folder created here.");}
    if(command>=FILE_ROW&&command<FILE_ROW+V6_ENTRIES&&w->app==FILES){const v6_entry *file=child_at(app,command-FILE_ROW);if(file){if(file->kind==2){copy(app->path,file->path,sizeof(app->path));app->offset=0;}else launch(EDITOR,file->path);}}
    if(command>=FILE_DELETE_ROW&&command<FILE_DELETE_ROW+V6_ENTRIES&&w->app==FILES){
        const v6_entry *file=child_at(app,command-FILE_DELETE_ROW);if(file){unsigned open=0;
            for(unsigned n=0;n<DESKTOP_WINDOWS;++n)if(model.windows[n].live&&model.windows[n].app==EDITOR&&equal(apps[n].path,file->path))open=1;
            if(open)notice(app,"Close the editor for this file before deleting it.");
            else {copy(app->pending,file->path,sizeof(app->pending));app->closing=2;}
        }
    }
    if(command==TERMINAL_CLEAR&&w->app==TERMINAL){app->length=app->name_cursor=0;app->text[0]=app->pending[0]=0;copy(app->text,"engine> ",sizeof(app->text));app->length=8;}
    if(command>=STYLE_MINT&&command<=STYLE_AMBER&&w->app==SETTINGS){const uint32_t colors[]={0x62e5bd,0x9caeff,0xf4c47c};preferences.accent=colors[command-STYLE_MINT];whole();}
    if((command==STYLE_DARK||command==STYLE_PAPER)&&w->app==SETTINGS){preferences.dark=command==STYLE_DARK;whole();}
    if(command==CALCULATOR_OFF&&w->app==SETTINGS){
        for(unsigned i=0;i<DESKTOP_WINDOWS;++i)if(model.windows[i].live&&model.windows[i].app==CALCULATOR)close_window(desktop_token(&model,i),1);
        unsigned status=app7_package(CALCULATOR,0);
        if(!status){preferences.calculator=0;notice(app,"Calculator removed. Your documents are unchanged.");serial_text("M7_CALCULATOR_REMOVED\n");}
        else notice(app,"Calculator could not be removed. No base component was removed.");
        whole();
    }
    if(command==CALCULATOR_ON&&w->app==SETTINGS){
        unsigned status=app7_package(CALCULATOR,1);
        if(!status){preferences.calculator=1;notice(app,"Calculator installed from the offline package cache.");serial_text("M7_CALCULATOR_INSTALLED\n");}
        else notice(app,"Calculator installation failed. Keep the recovery ISO.");
        whole();
    }
    if(command==NETWORK_RETRY&&w->app==DOWNLOADS){download7_cancel();net7_retry();}
    if(command==DNS_PROBE&&w->app==DOWNLOADS){download7_info d=download7_current();if(d.state<DOWNLOAD7_RESOLVING||d.state>DOWNLOAD7_RECEIVING)(void)net7_resolve("example.com");}
    if(command==URL_FOCUS&&w->app==DOWNLOADS){app->entered=0;app->cursor=app->length;}
    if(command==DOWNLOAD_START&&w->app==DOWNLOADS){unique_path(app->path,"Downloads","Download",".bin");if(app->path[0])(void)download7_start(m6_volume(),app->text,app->path);}
    if(command==DOWNLOAD_CANCEL&&w->app==DOWNLOADS)download7_cancel();
    if(command==MEMORY_LIMIT&&w->app==DETAILS){pressure_on^=1;pages_limit(pressure_on?pages_stats().current_pages:0);serial_text(pressure_on?"M7_MEMORY_LIMIT_ACTIVE\n":"M7_MEMORY_LIMIT_RESTORED\n");resource_log();}
    if(command==DOWNLOAD_FOLDER&&w->app==DOWNLOADS){uint64_t opened=launch(FILES,0);desktop_window *files=desktop_lookup(&model,opened);if(files){root_path(apps[files-model.windows].path,"Downloads");whole();}}
    if(command==CALCULATOR_CRASH&&w->app==CALCULATOR)send_app(app,APP7_CRASH,0,0,0);
    if(command==APP_SELFTEST&&w->app==DETAILS){
        unsigned free_slot=0;while(free_slot<DESKTOP_WINDOWS&&model.windows[free_slot].live)++free_slot;
        unsigned passed=free_slot<DESKTOP_WINDOWS&&app7_available(CALCULATOR);
        uint64_t baseline=pages_stats().current_pages;
        for(unsigned probe=APP7_CRASH;passed&&probe<=APP7_BAD_RESULT;++probe){
            if(app7_open(free_slot,CALCULATOR)||!app7_request(free_slot,probe,0,0,0,&apps[free_slot].view))passed=0;
            app7_close(free_slot);if(pages_stats().current_pages!=baseline)passed=0;
        }
        if(!app7_package(EDITOR,0))passed=0;
        notice(app,passed?"Five real app faults contained; private allocations reclaimed.":"Isolation check failed or unavailable. Keep the ISO.");
        serial_text(passed?"M7_APP_ISOLATION_PASS\n":"M7_APP_ISOLATION_FAILED\n");
    }
    if(command==PREFERENCES_SAVE&&w->app==SETTINGS){unsigned result=v6_put(m6_volume(),"/System/Settings/desktop7.bin",sizeof(preferences),memory_source,&preferences);notice(app,result?"Preferences could not be saved.":"Preferences saved.");serial_text(result?"M7_PREFERENCES_SAVE_FAILED\n":"M7_PREFERENCES_SAVED\n");}
    if(command>=CALC_KEY&&command<CALC_KEY+16&&w->app==CALCULATOR){const char keys[]="789/456*123-C0=+";calculator(app,keys[command-CALC_KEY]);}
    invalidate(w->bounds);
}

static void pointer(void) {
    unsigned x,y,buttons;usb_mouse_state(&x,&y,&buttons);
    unsigned pressed=(buttons&1)&&!(previous_buttons&1),released=!(buttons&1)&&(previous_buttons&1);
    if(released)dragging=0;
    if(dragging&&(buttons&1)){
        desktop_rect r=drag_bounds;int dx=(int)x-(int)drag_x,dy=(int)y-(int)drag_y;
        if(drag_resize){r.width+=dx;r.height+=dy;if(r.width<480)r.width=480;desktop_window *resizing=desktop_lookup(&model,drag_window);int minimum=resizing&&resizing->app==DOWNLOADS?460:420;if(r.height<minimum)r.height=minimum;}else{r.x+=dx;r.y+=dy;}
        desktop_place(&model,drag_window,r);
    }else if(pressed){
        if(control_active){control_active=0;whole();}
        uint64_t token=desktop_hit(&model,(int)x,(int)y);desktop_window *w=desktop_lookup(&model,token);
        int handled=0;
        /* Controls are generated in paint/stack order: last visible hit wins. */
        for(unsigned i=control_count;i;--i){control *control=&controls[i-1];desktop_rect r=control->rect;
            if((int)x>=r.x&&(int)x<r.x+r.width&&(int)y>=r.y&&(int)y<r.y+r.height&&
               ((launcher?control->window==0:control->window==token)||y>=model.height-60)){
                if(control->window)desktop_focus(&model,control->window);action(control->window,control->action);handled=1;break;}}
        if(!handled&&launcher){launcher=0;whole();handled=1;}
        if(!handled&&w){desktop_focus(&model,token);task_damage();desktop_rect r=w->bounds;
            if(y<(unsigned)r.y+35){if(x>(unsigned)(r.x+r.width-52))close_window(token,0);else if(x>(unsigned)(r.x+r.width-90))desktop_minimize(&model,token,1);else {dragging=1;drag_resize=0;}}
            else if(x>=(unsigned)(r.x+r.width-18)&&y>=(unsigned)(r.y+r.height-18)){dragging=1;drag_resize=1;}
            if(dragging){drag_window=token;drag_x=x;drag_y=y;drag_bounds=r;}
        }else if(!handled&&!token&&x>=30&&x<140&&y>=100&&y<520){unsigned kind=(y-100)/83+1;if(kind<=5)launch(kind,0);}
        else if(!handled&&launcher){launcher=0;whole();}
    }
    /* Preserve the previously drawn cursor region until damage has been consumed. */
    render();pointer_x=x;pointer_y=y;previous_buttons=buttons;usb_mouse_draw();
}

static void keyboard(int key) {
    if(key==137){launcher^=1;control_active=0;whole();return;}
    if(key==135){if(model.count){desktop_focus(&model,desktop_token(&model,model.order[0]));control_active=0;whole();}return;}
    if(key==9||key==136){
        uint64_t target=launcher?0:model.focused<DESKTOP_WINDOWS?desktop_token(&model,model.focused):0;
        unsigned candidates[DESKTOP_CONTROLS],count=0,current=0;
        for(unsigned n=0;n<control_count;++n){control *c=&controls[n];
            if(c->window!=target||c->action==70||(launcher&&!c->action))continue;
            if(control_active&&focused_control_window==target&&focused_action==c->action)current=count+1;
            candidates[count++]=n;
        }
        if(count){unsigned selected=key==136?(current?current+count-2:count-1):(current?current:0);control *c=&controls[candidates[selected%count]];
            control_active=1;focused_control_window=c->window;focused_action=c->action;whole();}
        return;
    }
    if(key==10&&control_active){uint64_t target=focused_control_window;unsigned command=focused_action;control_active=0;action(target,command);whole();return;}
    if(control_active){control_active=0;whole();}
    if(key==27){if(launcher){launcher=0;whole();}else if(model.focused<DESKTOP_WINDOWS){
        application *focused=&apps[model.focused];if(focused->closing==3){focused->closing=0;invalidate(model.windows[model.focused].bounds);}else close_window(desktop_token(&model,model.focused),0);}return;}
    if(model.focused>=DESKTOP_WINDOWS)return;
    desktop_window *w=&model.windows[model.focused];application *app=&apps[model.focused];
    uint64_t token=desktop_token(&model,model.focused);
    if(app->closing==3){
        unsigned count=length(app->pending);
        if(key==10){save_as(app);invalidate(w->bounds);return;}
        if(key==1){app->name_selected=1;app->name_cursor=count;}
        else if(key==128){if(app->name_cursor)--app->name_cursor;app->name_selected=0;}
        else if(key==129){if(app->name_cursor<count)++app->name_cursor;app->name_selected=0;}
        else if(key==130){app->name_cursor=0;app->name_selected=0;}
        else if(key==131){app->name_cursor=count;app->name_selected=0;}
        else if(key==8||key==132){
            if(app->name_selected){app->pending[0]=0;app->name_cursor=0;app->name_selected=0;}
            else if((key==8&&app->name_cursor)||(key==132&&app->name_cursor<count)){
                if(key==8)--app->name_cursor;for(unsigned n=app->name_cursor;n<count;++n)app->pending[n]=app->pending[n+1];
            }
        }else if((key>='a'&&key<='z')||(key>='A'&&key<='Z')||(key>='0'&&key<='9')||key==' '||key=='.'||key=='-'||key=='_'){
            if(app->name_selected){count=0;app->name_cursor=0;app->pending[0]=0;app->name_selected=0;}
            if(count<63){for(unsigned n=count+1;n>app->name_cursor;--n)app->pending[n]=app->pending[n-1];app->pending[app->name_cursor++]=(char)key;}
        }
        invalidate(w->bounds);return;
    }
    if(app->closing){if(key==10){app->closing=0;invalidate(w->bounds);}return;}
    if(w->app==TERMINAL){
        if(key==10&&(equal(app->pending,"menu")||equal(app->pending,"exit"))){close_window(token,1);return;}
        console7_capture(terminal_output,app);
        if(key==10){console_text("\n");m7_terminal_command(app->pending,app->name_cursor);app->pending[0]=0;app->name_cursor=0;}
        else if(key==8&&app->name_cursor){app->pending[--app->name_cursor]=0;console_text("\b");}
        else if(key>=32&&key<=126&&app->name_cursor<sizeof(app->pending)-1){app->pending[app->name_cursor++]=(char)key;app->pending[app->name_cursor]=0;char echo[2]={(char)key,0};console_text(echo);}
        console7_capture(0,0);invalidate(w->bounds);return;
    }
    if(w->app==CALCULATOR){calculator(app,(char)(key==10?'=':key));invalidate(w->bounds);return;}
    if(w->app==FILES){if(key==133&&app->offset)--app->offset;if(key==134&&child_at(app,app->offset+1))++app->offset;invalidate(w->bounds);return;}
    if(w->app==DOWNLOADS){
        if(key==10){action(token,DOWNLOAD_START);return;}
        if(key==12){app->entered=1;app->cursor=app->length;}
        else if(key==128){if(app->cursor)--app->cursor;app->entered=0;}
        else if(key==129){if(app->cursor<app->length)++app->cursor;app->entered=0;}
        else if(key==130){app->cursor=0;app->entered=0;}
        else if(key==131){app->cursor=app->length;app->entered=0;}
        else if(key==8||key==132){
            if(app->entered){app->length=app->cursor=0;app->text[0]=0;app->entered=0;}
            else if((key==8&&app->cursor)||(key==132&&app->cursor<app->length)){
                if(key==8)--app->cursor;for(unsigned n=app->cursor;n<app->length;++n)app->text[n]=app->text[n+1];--app->length;
            }
        }else if(key>=32&&key<=126){
            if(app->entered){app->length=app->cursor=0;app->text[0]=0;app->entered=0;}
            if(app->length<3070){for(unsigned n=app->length+1;n>app->cursor;--n)app->text[n]=app->text[n-1];app->text[app->cursor++]=(char)key;++app->length;}
        }
        invalidate(w->bounds);return;
    }
    if(w->app!=EDITOR||app->stopped)return;
    if(key==19){action(token,SAVE);return;}if(key==3){action(token,COPY);return;}if(key==24){action(token,CUT);return;}if(key==22){action(token,PASTE);return;}
    send_app(app,APP7_KEY,(unsigned)key,0,0);
    invalidate(w->bounds);
}

NORETURN void desktop_start(const os_boot_info_v0 *boot,const m6_preferences *profile) {
    display=boot;owner=*profile;preferences=(preferences7){UINT64_C(0x3150464552374d45),profile->accent,DISTRO_DESKTOP_DARK,1,0};
    unsigned bytes=0;preferences7 stored;
    if(!v6_verify(m6_volume(),"/System/Settings/desktop7.bin")&&
       !v6_read(m6_volume(),"/System/Settings/desktop7.bin",0,&stored,sizeof(stored),&bytes)&&
       bytes==sizeof(stored)&&stored.magic==preferences.magic&&stored.accent<=0xffffff&&stored.dark<=1&&stored.calculator<=1&&!stored.reserved)preferences=stored;
    if(app7_prepare())serial_text("M7_APP_RUNTIME_MEMORY_UNAVAILABLE\n");
    preferences.calculator=app7_available(CALCULATOR);
    net7_init();
    desktop_init(&model,display->framebuffer_width,display->framebuffer_height);
    usb_mouse_state(&pointer_x,&pointer_y,&previous_buttons);
    launch(FILES,0);render();resource_log();serial_text("M7_DESKTOP_READY build=" BUILD_ID "\n");
    unsigned network_revision=net7_current().revision,download_revision=download7_current().revision;
    for(;;){
        __asm__ volatile("cli");
        if(usb_work_pending()){usb_process_changes();continue;}
        int key=input_pop();
        unsigned network_work=net7_work_pending();
        if(key<0&&!network_work){__asm__ volatile("sti; hlt":::"memory");continue;}
        __asm__ volatile("sti");
        if(network_work){net7_pump();unsigned revision=net7_current().revision;if(revision!=network_revision||download7_current().revision!=download_revision){network_revision=revision;download_revision=download7_current().revision;for(unsigned i=0;i<DESKTOP_WINDOWS;++i)if(model.windows[i].live&&(model.windows[i].app==DOWNLOADS||model.windows[i].app==DETAILS))invalidate(model.windows[i].bounds);}}
        if(key<0){render();continue;}
        if(key==256)pointer();else{keyboard(key);render();}
    }
}
