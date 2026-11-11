static void file_menu_add11(const char*label,unsigned command,unsigned enabled){if(file_menu_count11<12)file_menu_items11[file_menu_count11++]=(file_menu11){label,command,enabled};}
static void file_menu_begin11(uint64_t token,unsigned x,unsigned y){file_menu_count11=file_menu_scroll11=0;file_menu_reason11[0]=0;context_window11=token;context_x11=x;context_y11=y;context_choice11=0;context11=1;launcher=0;control_active=0;}
static unsigned file_menu_visible11(void){unsigned available=model.height>130?model.height-130:114,rows=available/38;if(rows>file_menu_count11)rows=file_menu_count11;return rows?rows:1;}
static void file_menu_fit11(void){unsigned width=model.width>300?272:model.width-20,rows=file_menu_visible11(),height=rows*38+42;if(context_x11+width>model.width-8)context_x11=model.width-width-8;if(context_y11+height>model.height-68)context_y11=model.height>height+68?model.height-height-68:8;context_choice11=files_context_next11(file_menu_count11?file_menu_count11-1:0,1);whole();}
static unsigned files_context_next11(unsigned current,int direction){if(!file_menu_count11)return 0;for(unsigned i=0;i<file_menu_count11;++i){current=direction>0?(current+1)%file_menu_count11:(current+file_menu_count11-1)%file_menu_count11;if(file_menu_items11[current].enabled)return current;}return 0;}
static void files_places_menu11(uint64_t token){file_menu_begin11(token,pointer_x,pointer_y);const char*names[]={"Home","Desktop","Documents","Downloads","Pictures","System drive","USB drives","Recycle Bin"};for(unsigned i=0;i<8;++i)file_menu_add11(names[i],F11_PLACE+i,1);file_menu_fit11();}
static void files_sort_menu11(uint64_t token){file_menu_begin11(token,pointer_x,pointer_y);file_menu_add11("Name",F11_SORT_NAME,1);file_menu_add11("Size",F11_SORT_SIZE,1);file_menu_add11("Type",F11_SORT_TYPE,1);file_menu_add11("Reverse order",F11_DIRECTION,1);file_menu_fit11();}
static const v6_entry *file_target_entry11(i11_target t){desktop_window*w=file_owner_window11(t.window);if(w&&t.action>=FILE_ROW&&t.action<FILE_ROW+V6_ENTRIES)return child_at(&apps[w-model.windows],t.action-FILE_ROW);if(!t.window&&t.action>=F11_DESKTOP_ITEM&&t.action<F11_DESKTOP_ITEM+V6_ENTRIES)return desktop_child11(t.action-F11_DESKTOP_ITEM);return 0;}
static void files_context_open11(i11_target t,unsigned x,unsigned y){
 file_menu_begin11(t.window,x,y);context_action11=t.action;file_context_path11[0]=0;desktop_window*w=desktop_lookup(&model,t.window);application*a=w?&apps[w-model.windows]:0;
 if(w&&(t.kind==I11_CAPTION||(t.action>=31000&&t.action<=31003)||w->app!=FILES)){
  if(w->app==EDITOR&&t.kind!=I11_CAPTION&&t.action<31000){file_menu_add11("Copy document",COPY,1);file_menu_add11("Cut document",CUT,1);file_menu_add11("Paste text",PASTE,clipboard_kind11!=2&&clipboard_length!=0);}
  file_menu_add11("Minimize",31002,1);file_menu_add11("Close",31001,1);file_menu_fit11();return;
 }
 if(a&&a->usb_mode10==1){if(t.action>=USB_DISK10&&t.action<USB_DISK10+STORAGE_MAX)file_menu_add11("Open drive",t.action,1);file_menu_add11("Open Home",F11_PLACE,1);file_menu_add11("Settings",OPEN_SETTINGS+100,1);copy(file_menu_reason11,"Open a supported USB drive to browse its files.",128);file_menu_fit11();return;}
 if(a&&a->usb_mode10==2){
  if(t.action>=USB_ROW10&&t.action<USB_ROW10+6){unsigned total,row=t.action-USB_ROW10;if(!usb_listing10(a,a->offset,6,&total)&&a->offset+row<total){rem10_entry*e=&usb_rows10[row];file_join11(file_context_path11,a->usb_path10,e->name);file_menu_add11(e->kind==2?"Open folder":"Copy to Downloads",e->kind==2?t.action:USB_IMPORT10,1);file_menu_add11("Copy",F11_COPY,e->kind==1&&file_selected_count11(a)==1);file_menu_add11("Properties",F11_PROPERTIES,file_selected_count11(a)==1);}}
  file_menu_add11("Safe remove",USB_EJECT10,1);copy(file_menu_reason11,"USB supports one-file, create-only copies. Native items support more actions.",128);file_menu_fit11();return;
 }
 const v6_entry*e=file_target_entry11(t);if(e){copy(file_context_path11,e->path,256);unsigned count=a?file_selected_count11(a):0;if(!a){for(unsigned row=0;row<desktop_children11();++row)count+=file_selected_desktop11(row);}
  if(a&&file_is_bin11(a->path)){if(e->kind==2)file_menu_add11("Open folder",t.action,1);file_menu_add11("Restore",F11_RESTORE,count!=0);file_menu_add11("Copy",F11_COPY,count!=0);file_menu_add11("Properties",F11_PROPERTIES,count==1);file_menu_add11("Delete permanently...",F11_PERMANENT,count!=0);copy(file_menu_reason11,"Restore returns items to their original folders. Permanent deletion needs review.",128);}
  else{unsigned own=file_user_item11(e->path),in_use=file_open_under11(e->path);file_menu_add11("Open",F11_OPEN,writable72(e->path)||e->kind==2);file_menu_add11("Open with...",F11_OPEN_WITH,count==1&&e->kind==1&&writable72(e->path)&&!file_suffix11(e->path,"link"));file_menu_add11("Copy",F11_COPY,count!=0);file_menu_add11("Cut",F11_CUT,own&&!in_use);file_menu_add11("Rename...",F11_RENAME,count==1&&own&&!in_use);file_menu_add11("Move to Recycle Bin",F11_RECYCLE,own&&!in_use);file_menu_add11("Create desktop shortcut",F11_SHORTCUT,1);if(a&&e->kind==1&&count==1&&own)file_menu_add11("Copy to USB...",USB_SEND10+(t.action-FILE_ROW),1);file_menu_add11("Properties",F11_PROPERTIES,count==1);if(!own)copy(file_menu_reason11,"System items are protected. Copy readable files to Home before opening.",128);else if(in_use)copy(file_menu_reason11,"Close the app using this item before moving, renaming or recycling it.",128);}
 }else if(!t.window&&t.action>=30000&&t.action<=30002){file_menu_add11("Open",t.action,1);file_menu_add11("Create desktop shortcut",F11_SHORTCUT,1);file_menu_add11("Personalize",OPEN_SETTINGS,1);copy(file_menu_reason11,"Built-in launchers are independent of your desktop files.",128);}
 else{char path[256];if(a)copy(path,a->path,256);else root_path(path,"Desktop");copy(file_context_path11,path,256);unsigned writable=writable72(path)&&!file_in_bin11(path)&&!m6_volume()->readonly;
  if(file_is_bin11(path)){file_menu_add11("Restore selected items",F11_RESTORE,a&&file_selected_count11(a));file_menu_add11("Empty Recycle Bin...",F11_EMPTY,a&&children72(a));file_menu_add11("Properties",F11_PROPERTIES,1);}
  else{file_menu_add11("New folder...",FILE_FOLDER,writable);file_menu_add11("New text file...",F11_NEW_TEXT,writable);file_menu_add11("Paste",F11_PASTE,writable&&clipboard_kind11==2&&file_clip11.count);file_menu_add11("Undo last supported action",F11_UNDO,file_undo_state11.kind!=0);if(a){file_menu_add11(file_windows11[a-apps].view?"Use details view":"Use icon view",F11_VIEW,1);file_menu_add11("Sort by...",F11_SORT,1);}else file_menu_add11("Open Desktop in Files",F11_PLACE+1,1);if(!t.window)file_menu_add11("Personalize",OPEN_SETTINGS,1);if(!writable)copy(file_menu_reason11,"This location is protected. Use Home for your own files.",128);}
 }
 file_menu_fit11();
}
static void files_context_invoke11(unsigned index){if(index>=file_menu_count11||!file_menu_items11[index].enabled)return;unsigned command=file_menu_items11[index].command;uint64_t token=context_window11;context11=0;whole();
 if(command==31001){close_window(token,0);task_damage();return;}if(command==31002){desktop_minimize(&model,token,1);task_damage();return;}
 if(!token&&command>=30000&&command<=30002){if(command==30002)launch(SETTINGS,0);else{char path[256];if(command==30001)root_path(path,"Documents");else home72(path);launch(FILES,path);}desktop_focus11=0;whole();return;}
 action(token,command);
}

static uint64_t file_item_identity11(i11_target target){
 const v6_entry*entry=file_target_entry11(target);
 if(entry)return file_path_hash11(entry->path)^((uint64_t)(entry-m6_volume()->entries+1)<<32);
 desktop_window*w=file_owner_window11(target.window);
 if(w){application*a=&apps[w-model.windows];
  if(a->usb_mode10==2&&target.action>=USB_ROW10&&target.action<USB_ROW10+6){unsigned total,row=target.action-USB_ROW10;
   if(!usb_listing10(a,a->offset,6,&total)&&a->offset+row<total)return file_path_hash11(a->usb_path10)^file_path_hash11(usb_rows10[row].name)^((uint64_t)a->usb_disk10<<48);
  }
 }
 return target.window^((uint64_t)target.action<<32)^target.kind;
}
