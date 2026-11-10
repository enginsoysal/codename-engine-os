/* Bounded per-window navigation, independent of optional application workers. */
enum { NAV_BACK=90,NAV_FORWARD,NAV_HOME,NAV_DESKTOP,NAV_DOCUMENTS,NAV_DOWNLOADS,NAV_PICTURES,NAV_SYSTEM,NAV_PREVIOUS,NAV_NEXT,NAV_CREATE,NAV_NAME };
typedef struct {char path[8][256];unsigned count,position;} navigation72;
static navigation72 navigation[DESKTOP_WINDOWS];
static unsigned children72(application *app){
 unsigned total=0,size=equal(app->path,"/")?0:length(app->path);v6_volume*v=m6_volume();
 for(unsigned i=0;i<V6_ENTRIES;++i){const v6_entry*f=&v->entries[i];if(!f->kind||memcmp(f->path,app->path,size)||f->path[size]!='/')continue;
 const char*tail=f->path+size+1;unsigned n=0;while(tail[n]&&tail[n]!='/')++n;if(!tail[n])++total;}
 return total;
}
static void folder_icon72(int x,int y,unsigned size,uint32_t tint){
#ifdef ENGINE_M73
 ui73_folder((unsigned)x,(unsigned)y,size,tint);return;
#endif
 rectangle((desktop_rect){x,y,(int)size/2,7},0xbe974d);
 rectangle((desktop_rect){x,y+5,(int)size,(int)size*3/4},tint);
 rectangle((desktop_rect){x+2,y+8,(int)size-4,2},0xffe6ad);
}
static void home72(char *path){root_path(path,"");unsigned n=length(path);if(n&&path[n-1]=='/')path[n-1]=0;}
static int writable72(const char *path){char home[256];home72(home);unsigned n=length(home);return !memcmp(path,home,n)&&(!path[n]||path[n]=='/');}
static void visit72(application *app,const char *path,unsigned remember){
 navigation72 *nav=&navigation[app-apps];
 if(!equal(path,"/")){const v6_entry *folder=entry(path);if(!folder||folder->kind!=2){notice(app,"This folder is unavailable. Your current folder was kept.");return;}}
 if(remember&&(!nav->count||!equal(nav->path[nav->position],path))){
 nav->count=nav->count?nav->position+1:0;
 if(nav->count==8){for(unsigned i=1;i<8;++i)copy(nav->path[i-1],nav->path[i],256);nav->count=7;}
 copy(nav->path[nav->count],path,256);nav->position=nav->count++;}
 copy(app->path,path,256);app->offset=0;app->notice[0]=0;
 serial_text("M72_FOLDER_VISIT ");serial_text(path);serial_text("\n");
}
static void create_folder72(application *app){
 unsigned n=length(app->pending);
 if(!writable72(app->path)||!n||n>63||equal(app->pending,".")||equal(app->pending,"..")||app->pending[n-1]==' '||app->pending[n-1]=='.'){
 notice(app,"Use a folder name of 1-63 characters in your home folder.");return;}
 for(unsigned i=0;i<n;++i){unsigned c=(uint8_t)app->pending[i];if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c==' '||c=='.'||c=='-'||c=='_')){notice(app,"Use letters, numbers, spaces, dots, dashes or underscores.");return;}}
 if(length(app->path)+1+n>=256){notice(app,"This folder path is too long. Choose a shorter name.");return;}
 char path[256];copy(path,app->path,256);append(path,"/",256);append(path,app->pending,256);
 unsigned result=v6_mkdir(m6_volume(),path);
 if(result){notice(app,result==V6_EXISTS?"That name is already in use. Choose another.":"The folder could not be saved. Your files were kept.");return;}
 app->closing=0;app->offset=0;notice(app,"Folder created and saved to disk.");serial_text("M72_FOLDER_CREATED ");serial_text(path);serial_text("\n");
}
static void folder_dialog72(desktop_window*w,application*app,uint64_t token){
 int x=w->bounds.x+28,y=w->bounds.y+76;
 folder_icon72(x,y,44,0xf0ce87);text(x+65,y+8,"A new place for your files",2,ink());
 text(x,y+78,"Folder name",1,ink());
 rectangle((desktop_rect){x,y+103,w->bounds.width-56,42},preferences.dark?0x10212a:0xe6eee7);
 #ifdef ENGINE_M73
 ui73_fit(x+12,y+114,w->bounds.width-80,app->pending,1,ink());
#else
 text(x+12,y+119,app->pending,2,ink());
#endif
 if(control_count<DESKTOP_CONTROLS)controls[control_count++]=(control){{x,y+103,w->bounds.width-56,42},token,NAV_NAME};
 button(token,x,y+178,145,"Create folder",NAV_CREATE,1);button(token,x+158,y+178,120,"Cancel",KEEP_EDITING,0);
 text(x,y+242,app->notice[0]?app->notice:"Type a name, then choose Create folder. Escape cancels.",1,ink());
}
static void draw_files72(desktop_window *w,application *app,uint64_t token){
#ifdef ENGINE_M10
 if(app->usb_mode10){draw_usb10(w,app,token);return;}
#endif
 int x=w->bounds.x+16,y=w->bounds.y+49,cw=w->bounds.width;
 button(token,x,y,54,"<",NAV_BACK,0);button(token,x+62,y,54,">",NAV_FORWARD,0);button(token,x+124,y,54,"Up",FILE_UP,0);
 button(token,x+190,y,130,"New folder",FILE_FOLDER,1);
 if(cw>=760)button(token,x+332,y,128,"Appearance",OPEN_SETTINGS+100,0);
 int side=156,list=x+side+15,list_width=cw-side-47;
 rectangle((desktop_rect){x,y+48,side-4,w->bounds.height-115},preferences.dark?0x213b43:0xeaf0e9);
 text(x+12,y+65,"YOUR PLACES",1,ink());
 const char*places[]={"Home","Desktop","Documents","Downloads","Pictures","System drive"};
 for(unsigned i=0;i<6;++i){button(token,x+8,y+92+(int)i*37,side-20,"",NAV_HOME+i,0);unsigned scale=length(places[i])*12u<=(unsigned)(side-36)?2:1;text(x+16,y+101+(int)i*37+(scale==1?4:0),places[i],scale,ink());}
#ifdef ENGINE_M10
 button(token,x+8,y+92+6*37,side-20,"USB drives",USB_OPEN10,0);
#endif
 text(list,y+53,"LOCATION",1,ink());
 char path[128];unsigned columns=list_width>10?(unsigned)(list_width-10)/6:1,n=length(app->path);if(columns>127)columns=127;
 if(n>columns){copy(path,"...",128);append(path,app->path+n-(columns>3?columns-3:0),128);}else copy(path,app->path,128);
 #ifdef ENGINE_M73
 ui73_fit(list,y+74,list_width,app->path,1,ink());
#else
 text(list,y+74,path,1,ink());
#endif
 rectangle((desktop_rect){list,y+95,list_width,1},preferences.dark?0x35515a:0xd7e1d8);
 unsigned rows=w->bounds.height>220?(unsigned)(w->bounds.height-220)/44:1;if(rows>14)rows=14;if(!rows)rows=1;
 unsigned total=children72(app);
 if(app->offset>=total)app->offset=total?total-1:0;
 for(unsigned i=0;i<rows;++i){const v6_entry*file=child_at(app,app->offset+i);if(!file)break;int fy=y+106+(int)i*44;
 button(token,list,fy,list_width-
#ifdef ENGINE_M10
 160
#else
 78
#endif
 ,"",FILE_ROW+app->offset+i,0);
 if(file->kind==2)folder_icon72(list+10,fy+5,22,0xf0ce87);else {rectangle((desktop_rect){list+13,fy+4,17,23},0xf7f8ee);rectangle((desktop_rect){list+17,fy+10,9,2},0x88a4a0);}
 char name[64];unsigned limit=list_width>140?(unsigned)(list_width-140)/12:1;if(limit>63)limit=63;const char*full=file->path+(equal(app->path,"/")?1:length(app->path)+1);copy(name,full,limit+1);
 if(length(full)>limit&&limit>=3){name[limit-3]='.';name[limit-2]='.';name[limit-1]='.';}
 #ifdef ENGINE_M73
 ui73_fit(list+42,fy+8,list_width-
#ifdef ENGINE_M10
 210
#else
 128
#endif
 ,full,1,ink());
#else
 text(list+42,fy+8,name,2,ink());
#endif
#ifdef ENGINE_M10
 if(file->kind==1&&writable72(file->path))button(token,list+list_width-155,fy,78,"To USB",USB_SEND10+app->offset+i,0);
#endif
 if(writable72(file->path))button(token,list+list_width-70,fy,70,"Delete",FILE_DELETE_ROW+app->offset+i,0);
 }
 if(!total){folder_icon72(list+30,y+136,54,0xf0ce87);text(list+30,y+207,"Room for something new.",2,ink());text(list+30,y+242,"Choose New folder to get started.",1,ink());}
 int by=w->bounds.y+w->bounds.height-70;
 button(token,list,by,64,"<",NAV_PREVIOUS,0);button(token,list+72,by,64,">",NAV_NEXT,0);
 char count[48],value[24];decimal(value,total);copy(count,value,48);append(count," items",48);text(list+152,by+11,count,1,ink());
 text(x,w->bounds.y+w->bounds.height-23,app->notice[0]?app->notice:writable72(app->path)?"Click a folder to open it. Your files are saved on disk.":"System folders are read-only here. Your personal files are in Home.",1,ink());
}
static unsigned files_action72(desktop_window*w,application*app,uint64_t token,unsigned command){
 if(w->app!=FILES)return 0;
 navigation72*nav=&navigation[app-apps];
 if(command==OPEN_SETTINGS+100){launch(SETTINGS,0);whole();return 1;}
 if(command==NAV_NAME)return 1;
 if(command==NAV_CREATE){if(app->closing==4)create_folder72(app);invalidate(w->bounds);return 1;}
 if(command==NAV_BACK||command==NAV_FORWARD){
 if(command==NAV_BACK&&nav->position)--nav->position;
 if(command==NAV_FORWARD&&nav->position+1<nav->count)++nav->position;
 if(nav->count)visit72(app,nav->path[nav->position],0);invalidate(w->bounds);return 1;}
 if(command>=NAV_HOME&&command<=NAV_SYSTEM){char path[256];const char*folders[]={"","Desktop","Documents","Downloads","Pictures"};
 if(command==NAV_SYSTEM)copy(path,"/",256);else if(command==NAV_HOME)home72(path);else root_path(path,folders[command-NAV_HOME]);
 visit72(app,path,1);invalidate(w->bounds);return 1;}
 if(command==FILE_UP){char path[256];copy(path,app->path,256);unsigned n=length(path);while(n>1&&path[n-1]!='/')--n;if(n>1)path[n-1]=0;else copy(path,"/",256);visit72(app,path,1);invalidate(w->bounds);return 1;}
 if(command==NAV_PREVIOUS||command==NAV_NEXT){unsigned rows=w->bounds.height>220?(unsigned)(w->bounds.height-220)/44:1;if(rows>14)rows=14;if(!rows)rows=1;
 if(command==NAV_PREVIOUS)app->offset=app->offset>rows?app->offset-rows:0;else if(child_at(app,app->offset+rows))app->offset+=rows;invalidate(w->bounds);return 1;}
 if(command==FILE_FOLDER){if(!writable72(app->path))notice(app,"Choose Home or one of your folders to create a folder.");else{app->pending[0]=0;app->closing=4;app->notice[0]=0;}invalidate(w->bounds);return 1;}
 if(command>=FILE_ROW&&command<FILE_ROW+V6_ENTRIES){const v6_entry*file=child_at(app,command-FILE_ROW);if(file){if(file->kind==2){char path[256];copy(path,file->path,256);visit72(app,path,1);}else if(writable72(file->path)){
#ifdef ENGINE_M74
 open_file74(file->path);
#else
 launch(EDITOR,file->path);
#endif
 }else notice(app,"System files are protected here. Personal files open from Home.");}invalidate(w->bounds);return 1;}
 if(command>=FILE_DELETE_ROW&&command<FILE_DELETE_ROW+V6_ENTRIES){const v6_entry*file=child_at(app,command-FILE_DELETE_ROW);if(file&&!writable72(file->path)){notice(app,"System items are protected in Files.");invalidate(w->bounds);return 1;}}
 (void)token;return 0;
}
