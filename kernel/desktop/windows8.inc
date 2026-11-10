/* Native frontend for the bounded original Windows-console compatibility profile. */
#define PRODUCT_ROOT8 "/Applications/Windows/{23170F69-40C1-2702-2606-000001000000}"
enum {PREVIEW8=12000,INSTALL8,REMOVE8,CONFIRM8,BACK8,INFO8,ZIP8,EXTRACT8,CANCEL8,OUTPUT_PREV8,OUTPUT_NEXT8,LICENSE8};
extern const char windows8_licenses[];
static unsigned installed8(void){
 static uint64_t generation=UINT64_MAX;static unsigned valid;
 v6_volume*v=m6_volume();if(generation!=v->generation){generation=v->generation;const v6_entry*e=entry(PRODUCT_ROOT8 "/product.bin");valid=e&&e->kind==1&&!v6_verify(v,PRODUCT_ROOT8 "/product.bin");}
 return valid;
}
static void init_windows8(application*app,uint64_t token){
 if(!app->path[0])root_path(app->path,"Downloads/7z2606-x64.msi");app->number=0;app->offset=0;
 int width=(int)model.width-80;if(width>940)width=940;int height=(int)model.height-120;if(height>730)height=730;
 desktop_place(&model,token,(desktop_rect){((int)model.width-width)/2,35,width,height});
}
static void draw_windows8(desktop_window*w,application*app,uint64_t token){
 int x=w->bounds.x+24,y=w->bounds.y+62,width=w->bounds.width-48;const w8_info*info=w8_current();
 if(app->calc_error){
  text(x,y,"Software licenses",2,ink());
  app->source_crc=page74(x,y+50,(unsigned)width,(unsigned)(w->bounds.height-190)/24,windows8_licenses,app->offset);
  button(token,x,w->bounds.y+w->bounds.height-57,140,"Previous",OUTPUT_PREV8,0);
  button(token,x+154,w->bounds.y+w->bounds.height-57,140,"Read more",OUTPUT_NEXT8,0);
  button(token,x+308,w->bounds.y+w->bounds.height-57,140,"Back to app",BACK8,1);return;
 }
 unsigned installed=installed8();
 ui73_round(x,y,64,64,12,preferences.accent);text(x+13,y+18,"7z",2,0x102d29);
 text(x+84,y+4,"7-Zip Console",2,ink());
 text(x+84,y+38,"26.06  /  Igor Pavlov  /  Windows x64",1,ink());
 ui73_fit(x+width-150,y+9,150,info->state==1?"Working...":installed?"Installed":"Not installed",1,ink());
 ui73_paragraph(x,y+84,width,2,"Runs the original Windows program. Its Windows file-manager interface and Explorer menus are not supported yet.",ink());
 if(app->number){
  text(x,y+145,app->number==1?"Install 7-Zip Console?":"Remove 7-Zip Console?",2,ink());
  ui73_paragraph(x,y+194,width,5,app->number==1?"" ENGINE_PRODUCT_NAME " will verify the included MSI before changing files, then install its 106 original files in a separate application folder. Your documents stay separate. The console program is supported; the Windows GUI is unavailable.":"Unchanged application files and product registrations will be removed. Your documents and modified application files will be kept. You can install the application again later.",ink());
  button(token,x,y+340,150,"Go back",BACK8,0);button(token,x+166,y+340,190,app->number==1?"Install 7-Zip":"Remove 7-Zip",CONFIRM8,1);
  button(token,x+372,y+340,150,"Licenses",LICENSE8,0);return;
 }
 if(!installed){
  ui73_round(x,y+132,width,76,8,preferences.dark?0x142b35:0xe8f0f4);
  text(x+14,y+145,"Included installer",1,ink());ui73_fit(x+14,y+177,width-28,app->path,1,ink());
 }
 int actions=y+(installed?140:226);
 if(info->state==1){
  button(token,x,actions,150,"Cancel task",CANCEL8,0);
 }else{
  button(token,x,actions,160,installed?"Open 7-Zip":"Install 7-Zip",installed?INFO8:INSTALL8,1);
  button(token,x+174,actions,170,installed?"Remove app":"Review installer",installed?REMOVE8:PREVIEW8,0);
  button(token,x+358,actions,130,"Licenses",LICENSE8,0);
  if(entry(PRODUCT_ROOT8)&&!installed)button(token,x+502,actions,190,"Recover / remove",REMOVE8,0);
 }
 if(installed&&info->state!=1){
  button(token,x,y+194,195,"Create test archive",ZIP8,0);button(token,x+209,y+194,195,"Extract test archive",EXTRACT8,0);
  ui73_fit(x,y+246,width,"The test uses a separate sample document in your Documents folder.",1,ink());
 }
 const char*message=app->notice[0]?app->notice:info->message[0]?info->message:
  info->state==1?"7-Zip is running...":info->state==2&&info->command==W8_RUN?"Finished successfully.":installed?"Ready. Open the original Windows app or try creating and extracting a test archive.":"Choose Install 7-Zip to get started. Other Windows installers are not supported in this first release.";
 ui73_paragraph(x,y+294,width,info->command==W8_RUN?2:3,message,ink());
 if(info->state==1&&info->command!=W8_RUN){
  unsigned filled=(unsigned)width*info->percent/100;ui73_round(x,y+374,width,8,4,preferences.dark?0x233c47:0xdce8ee);
  if(filled)ui73_round(x,y+374,(int)filled,8,4,preferences.accent);
  char progress[40];decimal(progress,info->percent);append(progress,"% complete",40);text(x,y+390,progress,1,ink());
 }
 if(w->bounds.height>600&&info->output[0]){
  int top=y+(info->command==W8_RUN?382:442);unsigned rows=(unsigned)(w->bounds.y+w->bounds.height-80-top)/24;
  text(x,top-28,"Output from the original Windows program",1,ink());
  app->source_crc=page74(x,top,(unsigned)width,rows,info->output,app->offset);
  button(token,x,w->bounds.y+w->bounds.height-57,140,"Previous",OUTPUT_PREV8,0);button(token,x+154,w->bounds.y+w->bounds.height-57,140,"More output",OUTPUT_NEXT8,0);
 }
}
static void sample_name8(char*out,const char*base,unsigned number,const char*extension){
 copy(out,base,256);if(number){char suffix[24];decimal(suffix,number+1);append(out,"-",256);append(out,suffix,256);}append(out,extension,256);
}
static unsigned action_windows8(desktop_window*w,application*app,uint64_t token,unsigned command){
 (void)token;if(w->app!=WINDOWS8||command<PREVIEW8||command>LICENSE8)return 0;
 if(command==LICENSE8){app->calc_error=1;app->offset=0;invalidate(w->bounds);return 1;}
 if(command==BACK8){app->number=0;app->calc_error=0;app->offset=0;invalidate(w->bounds);return 1;}
 if(command==OUTPUT_PREV8){app->offset=0;invalidate(w->bounds);return 1;}
 if(command==OUTPUT_NEXT8){const char*source=app->calc_error?windows8_licenses:w8_current()->output;if(source[app->source_crc])app->offset=app->source_crc;invalidate(w->bounds);return 1;}
 if(command==CANCEL8){w8_cancel();invalidate(w->bounds);return 1;}
 if(w8_pending()){notice(app,"A Windows task is already running. Wait or cancel it.");invalidate(w->bounds);return 1;}
 if(command==INSTALL8||command==REMOVE8){app->number=command==INSTALL8?1:2;invalidate(w->bounds);return 1;}
 unsigned job=0;const char*source=app->path,*arguments="";char sample_arguments[512];
 if(command==CONFIRM8){job=app->number==1?W8_INSTALL:app->number==2?W8_REMOVE:0;app->number=0;}
 if(command==PREVIEW8)job=W8_PREVIEW;
 if(command==INFO8||command==ZIP8||command==EXTRACT8){job=W8_RUN;source=PRODUCT_ROOT8 "/7z.exe";arguments=command==INFO8?"i":command==ZIP8?"a -tzip -mx=1 -mmt=off -y EngineOS-Windows.zip EngineOS-Windows.txt":"x -mmt=off -y -oEngineOS-Windows-Extracted EngineOS-Windows.zip";}
 if(command==ZIP8||command==EXTRACT8){
  char path[256],name[256],archive[256];unsigned chosen=100,latest=100;
  for(unsigned i=0;i<100;++i){sample_name8(name,"EngineOS-Windows",i,".zip");root_path(path,"Documents/");append(path,name,256);if(entry(path))latest=i;}chosen=latest==100?0:latest+1;
  if(command==ZIP8){
   if(chosen==100){notice(app,"The sample archive limit was reached. Move or remove old sample archives in Documents, then try again.");invalidate(w->bounds);return 1;}
   sample_name8(archive,"EngineOS-Windows",chosen,".zip");copy(sample_arguments,"a -tzip -mx=1 -mmt=off -y ",512);append(sample_arguments,archive,512);append(sample_arguments," EngineOS-Windows.txt",512);
  }else{
   if(latest==100){notice(app,"Create a test archive first, then choose Extract test archive.");invalidate(w->bounds);return 1;}
   sample_name8(archive,"EngineOS-Windows",latest,".zip");chosen=100;
   for(unsigned i=0;i<100;++i){sample_name8(name,"EngineOS-Windows-Extracted",i,"");root_path(path,"Documents/");append(path,name,256);if(!entry(path)){chosen=i;break;}}
   if(chosen==100){notice(app,"The sample folder limit was reached. Move or remove old extraction folders in Documents, then try again.");invalidate(w->bounds);return 1;}
   sample_name8(name,"EngineOS-Windows-Extracted",chosen,"");copy(sample_arguments,"x -mmt=off -y -o",512);append(sample_arguments,name,512);append(sample_arguments," ",512);append(sample_arguments,archive,512);
  }arguments=sample_arguments;
 }
 if(job){
  char user[256];root_path(user,"");unsigned n=length(user);if(n&&user[n-1]=='/')user[n-1]=0;
  if(command==ZIP8){char path[256];root_path(path,"Documents/EngineOS-Windows.txt");if(!entry(path)){static const char sample[]="Created in " ENGINE_PRODUCT_NAME " and archived by the original Windows 7-Zip application.\n";if(v6_put(m6_volume(),path,sizeof(sample)-1,memory_source,(void*)sample)){notice(app,"The sample document could not be created.");invalidate(w->bounds);return 1;}}}
  unsigned result=w8_start(job,source,PRODUCT_ROOT8,user,arguments);notice(app,result?"The Windows task could not start. Open System Monitor for memory details.":"");app->offset=0;
 }invalidate(w->bounds);return 1;
}
