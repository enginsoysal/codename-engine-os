static unsigned devices_rows10(desktop_window*w){unsigned n=w->bounds.height>350?(unsigned)(w->bounds.height-310)/56:1;return n>6?6:n?n:1;}
static rem10_entry usb_rows10[6];
static unsigned usb_visible10(desktop_window*w){unsigned rows=w->bounds.height>350?(unsigned)(w->bounds.height-350)/43:1;return rows>6?6:rows?rows:1;}
static void draw_devices10(desktop_window*w,application*app,uint64_t token){
 int x=w->bounds.x+22,y=w->bounds.y+54,width=w->bounds.width-44;
 text(x,y,"Devices",2,ink());button(token,x+width-240,y-8,110,"Refresh",DEVICES_REFRESH10,0);button(token,x+width-120,y-8,110,"Back",DEVICES_BACK10,0);
 ui73_fit(x,y+42,width,"Support is built in. Detected does not mean fully supported.",1,ink());
 usb10_info usb=usb10_current();char summary[160]="USB: ",n[24];decimal(n,usb.keyboards);append(summary,n,160);append(summary," keyboard, ",160);decimal(n,usb.mice);append(summary,n,160);append(summary," mouse, ",160);decimal(n,usb.disks);append(summary,n,160);append(summary," storage, ",160);decimal(n,usb.hubs);append(summary,n,160);append(summary," hub. Display: software 2D.",160);ui73_fit(x,y+72,width,summary,1,ink());
 unsigned count=hw10_count(),rows=devices_rows10(w);
 for(unsigned i=0;i<rows&&app->offset+i<count;++i){const hw10_device*d=hw10_get(app->offset+i);int top=y+112+(int)i*56;char label[96];copy(label,d->name,96);append(label,"  /  ",96);append(label,d->state==HW10_READY?"Ready":d->state==HW10_LIMITED?"Limited":d->state==HW10_UNAVAILABLE?"Unavailable":"Detected",96);text(x+8,top,label,1,ink());ui73_fit(x+8,top+23,width-16,d->detail,1,ink());rectangle((desktop_rect){x,top+46,width,1},preferences.dark?0x35515a:0xd7e1d8);}
 int bottom=w->bounds.y+w->bounds.height-135;button(token,x,bottom,95,"Previous",DEVICES_PREV10,0);button(token,x+105,bottom,95,"Next",DEVICES_NEXT10,0);
 text(x,bottom+48,"Keyboard layout",1,ink());button(token,x+170,bottom+35,135,"US QWERTY",LAYOUT_US10,!input10_layout());button(token,x+315,bottom+35,135,"US Dvorak",LAYOUT_DVORAK10,input10_layout()==1);
 ui73_fit(x,w->bounds.y+w->bounds.height-28,width,app->notice[0]?app->notice:"Keyboard choice is saved automatically. Hardware support expands by tested profiles.",1,ink());
}
static void usb_notice10(application*app,unsigned status){notice(app,status==B_ABSENT?"Drive disconnected. Your existing files were kept.":status==B_UNSUPPORTED?"Unsupported format, file size, name or free-space layout. Nothing was replaced.":status==B_DENIED?"Copy refused: check the name, write protection or volume health. No file was replaced.":"I/O failed. Do not remove as safe; reconnect and check the drive before writing.");}
static unsigned usb_listing10(application*app,unsigned start,unsigned rows,unsigned*total){return rem10_list(app->usb_disk10,app->usb_context10,app->usb_part10,app->usb_path10,start,usb_rows10,rows,total);}
static void draw_usb10(desktop_window*w,application*app,uint64_t token){
 int x=w->bounds.x+22,y=w->bounds.y+54,width=w->bounds.width-44;
 text(x,y,"USB drives",2,ink());button(token,x+width-130,y-8,130,"Your folders",USB_CLOSE10,0);
 if(app->usb_mode10==1){ui73_paragraph(x,y+48,width,1,"Choose a connected drive. Opening it never formats or erases it.",ink());unsigned row=0;
  for(unsigned i=0;i<storage_count()&&row<6;++i){const storage_disk*d=storage_get(i);if(d->transport!=STORAGE_USB||!d->readable)continue;char label[128]="USB drive ",n[24];decimal(n,i+1);append(label,n,128);append(label,"  -  ",128);decimal(n,d->sectors*d->sector_bytes/(1024*1024));append(label,n,128);append(label," MiB",128);append(label,storage_writable(d)?"  /  read and write":"  /  read only",128);button(token,x,y+105+(int)row*55,width,label,USB_DISK10+i,0);++row;}
  if(!row)ui73_paragraph(x,y+115,width,2,"No readable USB drive is connected. Connect one, then reopen USB drives. Other devices are listed in Settings > Devices.",ink());
 }else{
  button(token,x,y+44,95,"Drives",USB_DRIVES10,0);button(token,x+105,y+44,80,"Up",USB_UP10,0);ui73_fit(x+200,y+55,width-205,app->usb_path10,1,ink());
  unsigned rows=usb_visible10(w),total=0,status=usb_listing10(app,app->offset,rows,&total);if(status){usb_notice10(app,status);ui73_paragraph(x,y+115,width,2,app->notice,ink());}
  else for(unsigned i=0;i<rows&&app->offset+i<total;++i){rem10_entry*e=&usb_rows10[i];int top=y+105+(int)i*43;button(token,x,top,width,"",USB_ROW10+i,
#ifdef ENGINE_M11
 selected_row11(app,app->offset+i)
#else
 app->usb_selected10==app->offset+i+1
#endif
 );if(e->kind==2)folder_icon72(x+12,top+8,20,0xf0ce87);else{rectangle((desktop_rect){x+15,top+7,16,24},0xf7f8ee);rectangle((desktop_rect){x+18,top+13,10,2},0x88a4a0);}ui73_fit(x+44,top+10,width-55,e->name,1,ink());}
  if(!status&&!total)text(x,y+128,"This folder is empty.",2,ink());
  int bottom=w->bounds.y+w->bounds.height-145;button(token,x,bottom,80,"<",USB_PREV10,0);button(token,x+90,bottom,80,">",USB_NEXT10,0);
  if(app->usb_source10[0])button(token,x+185,bottom,width-185,"Copy here",USB_EXPORT10,1);else button(token,x+185,bottom,width-185,"Copy selected to Downloads",USB_IMPORT10,1);
  button(token,x,bottom+48,175,"Safe remove",USB_EJECT10,0);ui73_fit(x+190,bottom+61,width-190,app->usb_source10[0]?app->usb_source10:"Select a file to copy, or open a folder.",1,ink());
 }
 ui73_fit(x,w->bounds.y+w->bounds.height-28,width,app->notice[0]?app->notice:"FAT32 file browsing and create-only copies. Safe remove before unplugging.",1,ink());
}
static unsigned devices_action10(desktop_window*w,application*app,uint64_t token,unsigned command){
 if(w->app==SETTINGS){
  if(command==DEVICES10){app->hardware_view10=1;app->offset=0;hw10_refresh();}
  else if(command==DEVICES_BACK10){app->hardware_view10=0;app->offset=0;}
  else if(command==DEVICES_REFRESH10)hw10_refresh();
  else if(command==DEVICES_PREV10)app->offset=app->offset>=devices_rows10(w)?app->offset-devices_rows10(w):0;
  else if(command==DEVICES_NEXT10){if(app->offset+devices_rows10(w)<hw10_count())app->offset+=devices_rows10(w);}
  else if(command==LAYOUT_US10||command==LAYOUT_DVORAK10){unsigned saved[2]={0x30314c4b,command==LAYOUT_DVORAK10};input10_set_layout(saved[1]);unsigned status=v6_put(m6_volume(),"/System/Settings/keyboard10.bin",sizeof(saved),memory_source,saved);notice(app,status?"Layout changed for this session; saving failed.":"Keyboard layout saved.");}
  else return 0;invalidate(w->bounds);return 1;
 }
 if(w->app!=FILES)return 0;
#ifdef ENGINE_M11
 if(command==USB_OPEN10||command==USB_CLOSE10||command==USB_DRIVES10||command==USB_UP10||(command>=USB_DISK10&&command<USB_DISK10+STORAGE_MAX)){memset(selected11[app-apps],0,sizeof(selected11[0]));interaction11.last_valid=0;}
 if(command==USB_IMPORT10){unsigned count=0;for(unsigned n=0;n<V6_ENTRIES;++n)count+=selected_row11(app,n);if(count>1){notice(app,"USB transfer accepts one selected file at a time. Select one file to copy.");invalidate(w->bounds);return 1;}}
#endif
 if(command>=USB_SEND10&&command<USB_SEND10+V6_ENTRIES){const v6_entry*f=child_at(app,command-USB_SEND10);if(f&&f->kind==1&&writable72(f->path)){copy(app->usb_source10,f->path,256);app->usb_mode10=1;app->offset=0;app->notice[0]=0;}}
 else if(command==USB_OPEN10){app->usb_mode10=1;app->offset=0;app->usb_source10[0]=0;app->notice[0]=0;}
 else if(command==USB_CLOSE10){app->usb_mode10=0;app->offset=0;app->usb_source10[0]=0;}
 else if(command==USB_DRIVES10){app->usb_mode10=1;app->offset=0;}
 else if(command>=USB_DISK10&&command<USB_DISK10+STORAGE_MAX){unsigned disk=command-USB_DISK10,part,context,status=rem10_volume(disk,&part,&context);if(status)usb_notice10(app,status);else{app->usb_disk10=disk;app->usb_context10=context;app->usb_part10=part;app->usb_mode10=2;app->offset=app->usb_selected10=0;copy(app->usb_path10,"/",256);app->notice[0]=0;}}
 else if(app->usb_mode10==2){
  if(command==USB_UP10){unsigned n=length(app->usb_path10);while(n>1&&app->usb_path10[n-1]!='/')--n;if(n>1)app->usb_path10[n-1]=0;else copy(app->usb_path10,"/",256);app->offset=app->usb_selected10=0;}
  else if(command==USB_PREV10)app->offset=app->offset>=usb_visible10(w)?app->offset-usb_visible10(w):0;
  else if(command==USB_NEXT10){unsigned total;if(!usb_listing10(app,app->offset,6,&total)&&app->offset+usb_visible10(w)<total)app->offset+=usb_visible10(w);}
  else if(command>=USB_ROW10&&command<USB_ROW10+6){unsigned total,status=usb_listing10(app,app->offset,6,&total),row=command-USB_ROW10;if(status)usb_notice10(app,status);else if(app->offset+row<total){rem10_entry*e=&usb_rows10[row];if(e->kind==2){if(length(app->usb_path10)+length(e->name)+2>=256)notice(app,"This path is too long.");else{if(!equal(app->usb_path10,"/"))append(app->usb_path10,"/",256);append(app->usb_path10,e->name,256);app->offset=app->usb_selected10=0;app->notice[0]=0;
#ifdef ENGINE_M11
 memset(selected11[app-apps],0,sizeof(selected11[0]));interaction11.last_valid=0;
#endif
}}else {app->usb_selected10=app->offset+row+1;
#ifdef ENGINE_M11
 notice(app,"Copy this USB file to Downloads, then open it there.");
#endif
 }}}
  else if(command==USB_EJECT10){unsigned status=usb10_eject(app->usb_disk10,app->usb_context10);if(status)usb_notice10(app,status);else{app->usb_mode10=1;app->offset=0;notice(app,"Safe to remove the USB drive. Reconnect it to use it again.");hw10_refresh();}}
  else if(command==USB_EXPORT10&&app->usb_source10[0]){notice(app,"Copying to USB. Please keep the drive connected.");invalidate(w->bounds);render();unsigned status=rem10_export(app->usb_disk10,app->usb_context10,app->usb_part10,app->usb_path10,app->usb_source10);if(status)usb_notice10(app,status);else{notice(app,"Copied and flushed to USB. Use Safe remove before unplugging.");app->usb_source10[0]=0;app->offset=0;}}
  else if(command==USB_IMPORT10){unsigned total,status=app->usb_selected10?usb_listing10(app,app->usb_selected10-1,1,&total):B_DENIED;if(status||!app->usb_selected10||app->usb_selected10>total||usb_rows10[0].kind!=1)notice(app,"Select a file first.");else{char source[256],target[256];copy(source,app->usb_path10,256);if(!equal(source,"/"))append(source,"/",256);if(length(source)+length(usb_rows10[0].name)>=256||length(usb_rows10[0].name)>55){notice(app,"Use a shorter file name before copying to your Downloads.");invalidate(w->bounds);return 1;}append(source,usb_rows10[0].name,256);root_path(target,"Downloads");append(target,"/",256);append(target,usb_rows10[0].name,256);if(entry(target))unique_path(target,"Downloads",usb_rows10[0].name,"");notice(app,"Copying to Downloads. Please keep the drive connected.");invalidate(w->bounds);render();status=target[0]?rem10_import(app->usb_disk10,app->usb_context10,app->usb_part10,source,target):B_DENIED;if(status)usb_notice10(app,status);else notice(app,"Copied to Downloads. Your existing files were kept.");}}
  else return 0;
 }else return 0;
 invalidate(w->bounds);(void)token;return 1;
}
