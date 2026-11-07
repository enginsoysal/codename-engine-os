#include <engine/desktop/session.h>
#include <engine/m4.h>
#include <engine/m3.h>
#include "build.h"

enum { FILES=1, EDITOR, SETTINGS, CALCULATOR, DOWNLOADS, DETAILS };
enum { OPEN_FILES=1, OPEN_EDITOR, OPEN_SETTINGS, OPEN_CALCULATOR, OPEN_DOWNLOADS,
       OPEN_DETAILS, SAVE=20, COPY, CUT, PASTE, FILE_UP, FILE_NEW, FILE_FOLDER,
       STYLE_MINT=40, STYLE_LAVENDER, STYLE_AMBER, STYLE_DARK, STYLE_PAPER,
       CALCULATOR_OFF, CALCULATOR_ON, PREFERENCES_SAVE,
       DISCARD=60, KEEP_EDITING, FILE_ROW=100, CALC_KEY=200 };

typedef struct {
    char path[256], text[8192], notice[96];
    unsigned length, cursor, dirty, closing, offset;
    int64_t number, operand;
    unsigned operation, entered, calc_error;
} application;
typedef struct { desktop_rect rect; uint64_t window; unsigned action; } control;
typedef struct { uint64_t magic; uint32_t accent, dark, calculator, reserved; } preferences7;

static desktop_model model;
static application apps[DESKTOP_WINDOWS];
static control controls[192];
static unsigned control_count, launcher, pointer_x, pointer_y, previous_buttons;
static unsigned dragging, drag_resize, drag_x, drag_y;
static uint64_t drag_window, paints, painted_pixels;
static desktop_rect drag_bounds;
static m6_preferences owner;
static preferences7 preferences;
static char clipboard[8192];
static unsigned clipboard_length;
static const os_boot_info_v0 *display;
static const char *titles[]={"", "Files", "Text Editor", "Settings", "Calculator", "Downloads", "System details"};

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
    text(x+9,y+9,label,1,primary?0x102d29:ink());
    if(control_count<sizeof(controls)/sizeof(*controls))controls[control_count++]=(control){r,window,action};
}
static void unique_path(char *out,const char *folder,const char *prefix,const char *suffix) {
    char number[24],base[256];root_path(base,folder);append(base,"/",256);
    for(unsigned i=1;i<10000;++i){copy(out,base,256);append(out,prefix,256);decimal(number,i);append(out,number,256);append(out,suffix,256);if(!entry(out))return;}
    out[0]=0;
}

static uint64_t launch(unsigned kind,const char *path) {
    if(kind==CALCULATOR&&!preferences.calculator)return 0;
    unsigned offset=model.count*26;
    uint64_t token=desktop_open(&model,kind,(desktop_rect){170+(int)offset,90+(int)offset,700,470});
    if(!token){serial_text("M7_WINDOW_LIMIT\n");return 0;}
    unsigned slot=(unsigned)(uint32_t)token-1;
    application *app=&apps[slot];memset(app,0,sizeof(*app));
    if(kind==FILES)root_path(app->path,"Documents");
    if(kind==EDITOR){
        if(path){
            const v6_entry *file=entry(path);unsigned bytes=0;
            if(!file||file->kind!=1||file->size>=sizeof(app->text)||v6_verify(m6_volume(),path)||
               v6_read(m6_volume(),path,0,app->text,sizeof(app->text)-1,&bytes)){
                notice(app,"Cannot open this file: integrity, size or I/O error.");
                unique_path(app->path,"Documents","Note",".txt");
            }else{copy(app->path,path,sizeof(app->path));app->length=bytes;app->cursor=bytes;app->text[bytes]=0;}
        }else unique_path(app->path,"Documents","Note",".txt");
    }
    launcher=0;task_damage();serial_text("M7_WINDOW_OPEN app=");serial_hex(kind);serial_text("\n");return token;
}

static void save_document(application *app) {
    if(!app->path[0]){notice(app,"No available document name. Open Files and try again.");return;}
    unsigned status=v6_put(m6_volume(),app->path,app->length,memory_source,app->text);
    if(status){notice(app,"Save failed. Your unsaved text is still here.");serial_text("M7_DOCUMENT_SAVE_FAILED\n");}
    else {app->dirty=0;notice(app,"Saved to your Documents folder.");serial_text("M7_DOCUMENT_SAVED\n");}
}

static void draw_editor(desktop_window *w,application *app,uint64_t token) {
    int x=w->bounds.x+14,y=w->bounds.y+48,right=w->bounds.x+w->bounds.width-14;
    button(token,x,y,75,"Save",SAVE,1);button(token,x+85,y,75,"Copy all",COPY,0);
    button(token,x+170,y,75,"Cut all",CUT,0);button(token,x+255,y,75,"Paste",PASTE,0);
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
        char label[96];copy(label,file->kind==2?"[Folder] ":"[Text]   ",sizeof(label));append(label,file->path+length(app->path)+1,sizeof(label));
        button(token,x,y+70+(int)i*30,w->bounds.width-30,label,FILE_ROW+app->offset+i,0);
    }
    text(x,w->bounds.y+w->bounds.height-24,app->notice[0]?app->notice:"Click a folder or a text file. Arrow keys scroll the list.",1,ink());
}
static void draw_settings(desktop_window *w,uint64_t token) {
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
    button(token,x,y+205,150,preferences.calculator?"Disable Calculator":"Enable Calculator",preferences.calculator?CALCULATOR_OFF:CALCULATOR_ON,0);
    button(token,x,y+260,170,"Save preferences",PREFERENCES_SAVE,1);
    text(x,w->bounds.y+w->bounds.height-28,"Personal files and required components stay available.",1,ink());
}
static void draw_calculator(desktop_window *w,application *app,uint64_t token) {
    int x=w->bounds.x+22,y=w->bounds.y+55;char number[32];integer(number,app->number);
    rectangle((desktop_rect){x,y,w->bounds.width-44,50},preferences.dark?0x10212a:0xe5eeed);
    text(x+14,y+16,app->calc_error?"Out of range / invalid operation":number,2,ink());
    const char *labels[]={"7","8","9","/","4","5","6","*","1","2","3","-","C","0","=","+"};
    for(unsigned i=0;i<16;++i)button(token,x+(int)(i%4)*62,y+72+(int)(i/4)*42,52,labels[i],CALC_KEY+i,0);
    text(x,y+270,"Checked signed integer arithmetic. Type keys or click.",1,ink());
}

static void draw_window(unsigned slot) {
    desktop_window *w=&model.windows[slot];if(w->minimized)return;
    application *app=&apps[slot];desktop_rect r=w->bounds;uint64_t token=desktop_token(&model,slot);
    rectangle(r,model.focused==slot?preferences.accent:0x58717c);
    rectangle((desktop_rect){r.x+1,r.y+1,r.width-2,r.height-2},paper());
    rectangle((desktop_rect){r.x+1,r.y+1,r.width-2,34},preferences.dark?0x253c49:0xdce8ec);
    text(r.x+14,r.y+12,titles[w->app],2,ink());
    text(r.x+r.width-77,r.y+12,"_",2,ink());text(r.x+r.width-42,r.y+12,"x",2,ink());
    if(app->closing){text(r.x+22,r.y+80,"Keep your unsaved changes?",2,ink());
        button(token,r.x+22,r.y+130,150,"Keep editing",KEEP_EDITING,1);
        button(token,r.x+185,r.y+130,150,"Discard changes",DISCARD,0);return;}
    if(w->app==EDITOR)draw_editor(w,app,token);
    if(w->app==FILES)draw_files(w,app,token);
    if(w->app==SETTINGS)draw_settings(w,token);
    if(w->app==CALCULATOR)draw_calculator(w,app,token);
    if(w->app==DOWNLOADS){text(r.x+20,r.y+65,"Downloads",3,ink());text(r.x+20,r.y+120,"Networking is not implemented in this development slice.",1,ink());text(r.x+20,r.y+150,"M7 is incomplete until real HTTPS tasks pass.",1,ink());}
    if(w->app==DETAILS){char value[32];text(r.x+20,r.y+65,"Engine OS M7 development",2,ink());text(r.x+20,r.y+105,BUILD_ID,1,ink());
        decimal(value,pages_stats().current_pages*4096);text(r.x+20,r.y+145,"Managed bytes",1,ink());text(r.x+230,r.y+145,value,1,ink());
        decimal(value,pages_stats().peak_pages*4096);text(r.x+20,r.y+175,"Managed peak",1,ink());text(r.x+230,r.y+175,value,1,ink());
        decimal(value,paints);text(r.x+20,r.y+205,"Damage repaints",1,ink());text(r.x+230,r.y+205,value,1,ink());
        text(r.x+20,r.y+250,"Static kernel, app and firmware memory are additional.",1,ink());}
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
    for(unsigned y=0;y<model.height;y+=32)gui6_rect(0,y,model.width,32,0x10252f+(y/64)*0x010101);
    text(36,30,"Engine OS",3,0xd8e9ec);text(38,64,"Your space. Your system.",1,0x8aabb7);
    const char *icons[]={"Files","Text Editor","Settings","Calculator","Downloads"};
    for(unsigned i=0;i<5;++i){if(i==3&&!preferences.calculator)continue;
        gui6_rect(38,110+i*83,38,36,preferences.accent);text(49,121+i*83,i==0?"F":i==1?"T":i==2?"S":i==3?"+":"D",2,0x12352e);
        text(38,155+i*83,icons[i],1,0xc4d9df);}
    for(unsigned i=0;i<model.count;++i)draw_window(model.order[i]);
    int bar=(int)model.height-60;
    gui6_rect(0,(unsigned)bar,model.width,60,0x0b1922);
    button(0,16,bar+15,100,"E  Start",0,launcher);
    for(unsigned i=0;i<model.count;++i){unsigned slot=model.order[i];desktop_window *w=&model.windows[slot];
        button(desktop_token(&model,slot),130+(int)i*100,bar+15,92,titles[w->app],70,model.focused==slot);}
    text((int)model.width-180,bar+23,"M7 development",1,0x95b1bd);
    if(launcher){
        gui6_rect(16,(unsigned)bar-305,300,295,paper());text(34,bar-284,owner.user,2,ink());
        unsigned row=0;for(unsigned i=1;i<=6;++i){if(i==CALCULATOR&&!preferences.calculator)continue;
            button(0,30,bar-248+(int)row++*38,270,titles[i],i,0);}
    }
    gui7_clip_reset();usb_mouse_draw();++paints;painted_pixels+=(uint64_t)dirty.width*dirty.height;
}

static void calculator(application *app,char key) {
    if(key=='C'||key=='c'){app->number=app->operand=0;app->operation=app->entered=app->calc_error=0;return;}
    if(key>='0'&&key<='9'){
        if(!app->entered){app->number=0;app->entered=1;}
        int64_t value;if(__builtin_mul_overflow(app->number,(int64_t)10,&value)||__builtin_add_overflow(value,(int64_t)(key-'0'),&value))app->calc_error=1;else app->number=value;return;
    }
    if(key=='='){
        int64_t value=0;int error=0;
        if(app->operation=='+')error=__builtin_add_overflow(app->operand,app->number,&value);
        else if(app->operation=='-')error=__builtin_sub_overflow(app->operand,app->number,&value);
        else if(app->operation=='*')error=__builtin_mul_overflow(app->operand,app->number,&value);
        else if(app->operation=='/'){
            if(!app->number||(app->operand==INT64_MIN&&app->number==-1))error=1;else value=app->operand/app->number;
        }else return;
        app->calc_error=(unsigned)error;if(!error)app->number=value;app->operation=0;app->entered=0;
    }else if(key=='+'||key=='-'||key=='*'||key=='/'){app->operand=app->number;app->operation=(unsigned)key;app->entered=0;}
}

static void close_window(uint64_t token,int discard) {
    desktop_window *w=desktop_lookup(&model,token);if(!w)return;
    application *app=&apps[w-model.windows];
    if(w->app==EDITOR&&app->dirty&&!discard){app->closing=1;invalidate(w->bounds);return;}
    desktop_close(&model,token);memset(app,0,sizeof(*app));task_damage();
}
static void insert(application *app,const char *bytes,unsigned count) {
    if(count>sizeof(app->text)-1-app->length){notice(app,"This editor has reached its 8191-byte limit. Save your text.");return;}
    for(unsigned i=app->length+1;i>app->cursor;--i)app->text[i+count-1]=app->text[i-1];
    memcpy(app->text+app->cursor,bytes,count);app->cursor+=count;app->length+=count;app->dirty=1;
}

static void action(uint64_t token,unsigned command) {
    if(!token){if(!command){launcher^=1;whole();return;}launch(command,0);whole();return;}
    desktop_window *w=desktop_lookup(&model,token);if(!w)return;
    unsigned slot=(unsigned)(w-model.windows);application *app=&apps[slot];
    if(command==70){desktop_focus(&model,token);task_damage();return;}
    if(command==DISCARD){close_window(token,1);return;}
    if(command==KEEP_EDITING){app->closing=0;invalidate(w->bounds);return;}
    if(command==SAVE&&w->app==EDITOR)save_document(app);
    if((command==COPY||command==CUT)&&w->app==EDITOR){memcpy(clipboard,app->text,app->length+1);clipboard_length=app->length;notice(app,"Document copied to the clipboard.");
        if(command==CUT){app->length=app->cursor=0;app->text[0]=0;app->dirty=1;}}
    if(command==PASTE&&w->app==EDITOR)insert(app,clipboard,clipboard_length);
    if(command==FILE_UP&&w->app==FILES){char root[256];root_path(root,"");unsigned n=length(app->path),min=length(root)-1;while(n>min&&app->path[n-1]!='/')--n;if(n>min)app->path[n-1]=0;app->offset=0;}
    if(command==FILE_NEW&&w->app==FILES){launch(EDITOR,0);task_damage();}
    if(command==FILE_FOLDER&&w->app==FILES){char path[256];unique_path(path,"Documents","Folder","");unsigned result=path[0]?v6_mkdir(m6_volume(),path):V6_FULL;notice(app,result?"Folder could not be created.":"Folder created in Documents.");}
    if(command>=FILE_ROW&&command<CALC_KEY&&w->app==FILES){const v6_entry *file=child_at(app,command-FILE_ROW);if(file){if(file->kind==2){copy(app->path,file->path,sizeof(app->path));app->offset=0;}else launch(EDITOR,file->path);}}
    if(command>=STYLE_MINT&&command<=STYLE_AMBER&&w->app==SETTINGS){const uint32_t colors[]={0x62e5bd,0x9caeff,0xf4c47c};preferences.accent=colors[command-STYLE_MINT];whole();}
    if((command==STYLE_DARK||command==STYLE_PAPER)&&w->app==SETTINGS){preferences.dark=command==STYLE_DARK;whole();}
    if(command==CALCULATOR_OFF&&w->app==SETTINGS){preferences.calculator=0;for(unsigned i=0;i<DESKTOP_WINDOWS;++i)if(model.windows[i].live&&model.windows[i].app==CALCULATOR)close_window(desktop_token(&model,i),1);whole();}
    if(command==CALCULATOR_ON&&w->app==SETTINGS){preferences.calculator=1;whole();}
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
        if(drag_resize){r.width+=dx;r.height+=dy;}else{r.x+=dx;r.y+=dy;}
        desktop_place(&model,drag_window,r);
    }else if(pressed){
        uint64_t token=desktop_hit(&model,(int)x,(int)y);desktop_window *w=desktop_lookup(&model,token);
        int handled=0;
        /* Controls are generated in paint/stack order: last visible hit wins. */
        for(unsigned i=control_count;i;--i){control *control=&controls[i-1];desktop_rect r=control->rect;
            if((int)x>=r.x&&(int)x<r.x+r.width&&(int)y>=r.y&&(int)y<r.y+r.height&&
               (control->window==token||y>=model.height-60||launcher)){
                if(control->window)desktop_focus(&model,control->window);action(control->window,control->action);handled=1;break;}}
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
    if(key==27){if(launcher){launcher=0;whole();}else if(model.focused<DESKTOP_WINDOWS)close_window(desktop_token(&model,model.focused),0);return;}
    if(key==9){if(model.count){unsigned slot=model.order[0];desktop_focus(&model,desktop_token(&model,slot));task_damage();}return;}
    if(model.focused>=DESKTOP_WINDOWS)return;
    desktop_window *w=&model.windows[model.focused];application *app=&apps[model.focused];
    uint64_t token=desktop_token(&model,model.focused);
    if(app->closing){if(key==10){app->closing=0;invalidate(w->bounds);}return;}
    if(w->app==CALCULATOR){calculator(app,(char)(key==10?'=':key));invalidate(w->bounds);return;}
    if(w->app==FILES){if(key==133&&app->offset)--app->offset;if(key==134&&child_at(app,app->offset+1))++app->offset;invalidate(w->bounds);return;}
    if(w->app!=EDITOR)return;
    if(key==19){action(token,SAVE);return;}if(key==3){action(token,COPY);return;}if(key==24){action(token,CUT);return;}if(key==22){action(token,PASTE);return;}
    if(key==128&&app->cursor)--app->cursor;
    else if(key==129&&app->cursor<app->length)++app->cursor;
    else if(key==130)app->cursor=0;
    else if(key==131)app->cursor=app->length;
    else if(key==8&&app->cursor){for(unsigned i=app->cursor;i<=app->length;++i)app->text[i-1]=app->text[i];--app->cursor;--app->length;app->dirty=1;}
    else if(key==132&&app->cursor<app->length){for(unsigned i=app->cursor;i<app->length;++i)app->text[i]=app->text[i+1];--app->length;app->dirty=1;}
    else if((key>=32&&key<=126)||key==10){char value=(char)key;insert(app,&value,1);}
    invalidate(w->bounds);
}

NORETURN void desktop_start(const os_boot_info_v0 *boot,const m6_preferences *profile) {
    display=boot;owner=*profile;preferences=(preferences7){UINT64_C(0x3150464552374d45),profile->accent,0,1,0};
    unsigned bytes=0;preferences7 stored;
    if(!v6_verify(m6_volume(),"/System/Settings/desktop7.bin")&&
       !v6_read(m6_volume(),"/System/Settings/desktop7.bin",0,&stored,sizeof(stored),&bytes)&&
       bytes==sizeof(stored)&&stored.magic==preferences.magic&&stored.accent<=0xffffff&&stored.dark<=1&&stored.calculator<=1&&!stored.reserved)preferences=stored;
    desktop_init(&model,display->framebuffer_width,display->framebuffer_height);
    usb_mouse_state(&pointer_x,&pointer_y,&previous_buttons);
    launch(FILES,0);render();serial_text("M7_DESKTOP_READY build=" BUILD_ID "\n");
    for(;;){
        __asm__ volatile("cli");
        if(usb_work_pending()){usb_process_changes();continue;}
        int key=input_pop();
        if(key<0){__asm__ volatile("sti; hlt":::"memory");continue;}
        __asm__ volatile("sti");
        if(key==256)pointer();else{keyboard(key);render();}
    }
}
