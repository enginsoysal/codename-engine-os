static void file_text_delete11(char*buffer,unsigned start,unsigned end){unsigned n=length(buffer);if(start>end||end>n)return;for(unsigned i=start;i+end-start<=n;++i)buffer[i]=buffer[i+end-start];}
static unsigned file_text_key11(char*buffer,unsigned*cursor,unsigned*anchor,int key){unsigned n=length(buffer),lo=*cursor<*anchor?*cursor:*anchor,hi=*cursor>*anchor?*cursor:*anchor;if(*cursor>n)*cursor=n;if(*anchor>n)*anchor=n;
 if(key==1){*anchor=0;*cursor=n;return 1;}
 if(key==128||key==129||key==130||key==131){unsigned next=*cursor;if(key==130)next=0;else if(key==131)next=n;else if(key==128){if(!(input11_modifiers()&2)&&lo!=hi)next=lo;else if(next)--next;}else{if(!(input11_modifiers()&2)&&lo!=hi)next=hi;else if(next<n)++next;}*cursor=next;if(!(input11_modifiers()&2))*anchor=next;return 1;}
 if(key==3||key==24){if(lo!=hi){unsigned size=hi-lo;memcpy(clipboard,buffer+lo,size);clipboard[size]=0;clipboard_length=size;clipboard_kind11=1;file_clip11.count=0;if(key==24){file_text_delete11(buffer,lo,hi);*cursor=*anchor=lo;}}return 1;}
 if(key==22){if(clipboard_kind11==2)return 1;unsigned size=clipboard_length;if(size>255-(n-(hi-lo)))return 1;for(unsigned i=0;i<size;++i)if((uint8_t)clipboard[i]<32||(uint8_t)clipboard[i]>126)return 1;file_text_delete11(buffer,lo,hi);n=length(buffer);for(unsigned i=n+1;i>lo;--i)buffer[i+size-1]=buffer[i-1];memcpy(buffer+lo,clipboard,size);*cursor=*anchor=lo+size;return 1;}
 if(key==8||key==132){if(lo!=hi){file_text_delete11(buffer,lo,hi);*cursor=*anchor=lo;}else if(key==8&&*cursor){file_text_delete11(buffer,*cursor-1,*cursor);--*cursor;*anchor=*cursor;}else if(key==132&&*cursor<n){file_text_delete11(buffer,*cursor,*cursor+1);*anchor=*cursor;}return 1;}
 if(key>=32&&key<=126){if(n-(hi-lo)>=255)return 1;file_text_delete11(buffer,lo,hi);n=length(buffer);for(unsigned i=n+1;i>lo;--i)buffer[i]=buffer[i-1];buffer[lo]=(char)key;*cursor=*anchor=lo+1;return 1;}return 0;
}
static unsigned file_dialog_owner_focused11(void){if(!file_dialog_state11.mode)return 0;if(!file_dialog_state11.owner)return desktop_focus11||model.focused>=DESKTOP_WINDOWS;return model.focused<DESKTOP_WINDOWS&&desktop_token(&model,model.focused)==file_dialog_state11.owner;}
static unsigned file_dialog_key11(int key){file_dialog11*d=&file_dialog_state11;if(!file_dialog_owner_focused11())return 0;if(key==137||key==135)return 0;
 if(key==27){if(file_job_state11.waiting)file_job_state11.waiting=0;d->mode=0;i11_cancel(&interaction11);whole();return 1;}
 unsigned text_mode=d->mode==F11_DLG_RENAME||d->mode==F11_DLG_NEW_FOLDER||d->mode==F11_DLG_NEW_TEXT;
 if(text_mode){if(key==9||key==136){d->focus=(d->focus+(key==9?1:2))%3;whole();return 1;}if(key==10){if(d->focus==2)file_action11(d->owner,F11_DIALOG_CANCEL);else file_dialog_confirm11();return 1;}if(d->focus==0){if(file_text_key11(d->input,&d->cursor,&d->anchor,key)){d->error[0]=0;whole();}return 1;}if(key==32){file_action11(d->owner,d->focus==2?F11_DIALOG_CANCEL:F11_DIALOG_OK);return 1;}return 1;}
 if(d->mode==F11_DLG_CONFLICT){unsigned count=4;if(key==9||key==136||key==133||key==134){int forward=key==9||key==134;do{d->focus=forward?(d->focus+1)%count:(d->focus+count-1)%count;}while(d->focus==2&&!file_job_state11.replace_allowed);whole();return 1;}if(key==10||key==32){unsigned commands[]={F11_KEEP_BOTH,F11_SKIP,F11_REPLACE,F11_DIALOG_CANCEL};file_action11(d->owner,commands[d->focus%4]);}return 1;}
 if(d->mode==F11_DLG_REMOVE||d->mode==F11_DLG_EMPTY){if(key==9||key==136||key==128||key==129){d->focus^=1;whole();return 1;}if(key==10||key==32)file_action11(d->owner,d->focus?F11_DIALOG_CANCEL:F11_DIALOG_OK);return 1;}
 if(d->mode==F11_DLG_OPEN_WITH){unsigned handlers[6],count=0;const unsigned candidates[]={EDITOR,IMAGES74,ARCHIVES74,MEDIA74,BROWSER74,WINDOWS8};for(unsigned i=0;i<6;++i)if(app7_available(candidates[i]))handlers[count++]=candidates[i];if(key==9||key==134){d->focus=(d->focus+1)%(count+2);whole();return 1;}if(key==136||key==133){d->focus=(d->focus+count+1)%(count+2);whole();return 1;}if(key==10||key==32){if(d->focus<count)file_action11(d->owner,F11_ASSOC_APP+handlers[d->focus]);else file_action11(d->owner,d->focus==count?F11_ALWAYS:F11_DIALOG_CANCEL);}return 1;}
 if(key==10||key==32)file_action11(d->owner,F11_DIALOG_CANCEL);return 1;
}
static unsigned file_address_commit11(desktop_window*w,application*a){file_window11*f=&file_windows11[a-apps];char path[256];const char*value=f->address;
 if(!file_compare_text11(value,"Home"))home72(path);else if(!file_compare_text11(value,"Desktop"))root_path(path,"Desktop");else if(!file_compare_text11(value,"Documents"))root_path(path,"Documents");else if(!file_compare_text11(value,"Downloads"))root_path(path,"Downloads");else if(!file_compare_text11(value,"Pictures"))root_path(path,"Pictures");else if(!file_compare_text11(value,"System drive"))copy(path,"/",256);else if(!file_compare_text11(value,"Recycle Bin")){file_action11(desktop_token(&model,(unsigned)(w-model.windows)),F11_BIN);return 1;}else if(!file_compare_text11(value,"USB drives")){devices_action10(w,a,desktop_token(&model,(unsigned)(w-model.windows)),USB_OPEN10);f->address_active=0;return 1;}else if(value[0]=='/')copy(path,value,256);else if(file_join11(path,a->path,value)){notice(a,"This path is too long.");return 1;}
 if(!equal(path,"/")){const v6_entry*e=file_case_entry11(path);if(!e||e->kind!=2){notice(a,"That folder is not available. Your current folder was kept.");invalidate(w->bounds);return 1;}copy(path,e->path,256);}visit72(a,path,1);f->address_active=0;invalidate(w->bounds);return 1;
}
static unsigned files_key11(int key){
 if(file_drag11){if(key==27||key==135||key==137){files_cancel_drag11();if(key==27)return 1;}else return 1;}
 if(file_dialog_key11(key))return 1;if(context11)return 0;
 desktop_window*w=model.focused<DESKTOP_WINDOWS?&model.windows[model.focused]:0;uint64_t token=w&&w->live?desktop_token(&model,model.focused):0;application*a=w&&w->live&&w->app==FILES?&apps[w-model.windows]:0;unsigned desktop=desktop_focus11||!token;
 if(!desktop&&!a)return 0;
 if(a&&file_windows11[a-apps].address_active){file_window11*f=&file_windows11[a-apps];if(key==27){f->address_active=0;invalidate(w->bounds);return 1;}if(key==10)return file_address_commit11(w,a);if(file_text_key11(f->address,&f->cursor,&f->anchor,key)){invalidate(w->bounds);return 1;}return key!=9&&key!=136;}
 if(a&&a->closing)return 0;unsigned modifiers=input11_modifiers();uint64_t owner_token=desktop?0:token;
 if(key==27&&file_job_state11.active&&file_job_state11.owner==owner_token){file_job_state11.cancel=1;return 1;}
 if(key==3||key==24||key==22||key==26){unsigned command=key==3?F11_COPY:key==24?F11_CUT:key==22?F11_PASTE:F11_UNDO;file_action11(owner_token,command);return 1;}
 if(key==139){file_action11(owner_token,F11_RENAME);return 1;}
 if(key==132){file_action11(owner_token,a&&file_is_bin11(a->path)?F11_PERMANENT:F11_RECYCLE);return 1;}
 if(key==14){if(modifiers&2)file_action11(owner_token,FILE_FOLDER);else{char path[256];if(a&&!a->usb_mode10)copy(path,a->path,256);else home72(path);launch(FILES,path);whole();}return 1;}
 if((modifiers&4)&&(key=='d'||key=='D')){if(a)file_action11(token,F11_ADDRESS);return 1;}
 if(a&&(modifiers&4)&&(key==128||key==129||key==133)){files_action72(w,a,token,key==128?NAV_BACK:key==129?NAV_FORWARD:FILE_UP);return 1;}
 if(key==10&&(modifiers&4)){file_action11(owner_token,F11_PROPERTIES);return 1;}
 if(control_active&&(key==10||key==32)){if(focused_control_window==owner_token){unsigned command=focused_action;control_active=0;action(owner_token,command);whole();return 1;}return 0;}
 if(desktop){unsigned total=desktop_children11()+3;if(key==1){desktop_selection11=7;memset(desktop_files_selected11,0,sizeof(desktop_files_selected11));for(unsigned row=0;row+3<total;++row){const v6_entry*e=desktop_child11(row);unsigned index=(unsigned)(e-m6_volume()->entries);desktop_files_selected11[index/8]|=(uint8_t)(1u<<(index%8));}whole();return 1;}
  if(key==133||key==134||key==128||key==129||key==130||key==131){unsigned row=desktop_item_focus11;if(row>=total)row=0;if(key==130)row=0;else if(key==131)row=total-1;else if(key==133||key==128){if(row)--row;}else if(row+1<total)++row;desktop_select11(row,modifiers,0);whole();return 1;}
  if(key==10){if(desktop_item_focus11<3&&(desktop_selection11&(1u<<desktop_item_focus11))){unsigned index=desktop_item_focus11;desktop_focus11=0;if(index==2)launch(SETTINGS,0);else{char path[256];if(index==1)root_path(path,"Documents");else home72(path);launch(FILES,path);}whole();}else file_open_selection11(0);return 1;}
  if(key==27){desktop_selection11=0;memset(desktop_files_selected11,0,sizeof(desktop_files_selected11));i11_cancel(&interaction11);whole();return 1;}return 0;
 }
 if(a->usb_mode10){
  if(a->usb_mode10!=2)return 0;unsigned total=0,slot=(unsigned)(a-apps),visible=usb_visible10(w);
  if(usb_listing10(a,0,1,&total)){file_notice11(token,"The USB folder is unavailable. Reconnect the drive or return to your folders.");return 1;}
  if(key==1){memset(selected11[slot],0,sizeof(selected11[slot]));for(unsigned row=0;row<total&&row<V6_ENTRIES;++row)selected11[slot][row/8]|=(uint8_t)(1u<<(row%8));a->usb_selected10=total?1:0;invalidate(w->bounds);return 1;}
  if(key==133||key==134||key==130||key==131||key==32){if(!total)return 1;unsigned row=a->usb_selected10?a->usb_selected10-1:0;if(key==130)row=0;else if(key==131)row=total-1;else if(key==133){if(row)--row;}else if(key==134&&row+1<total)++row;
   select_row11(a,row,key==32?modifiers&1:modifiers&2,0);a->usb_selected10=row+1;if(row<a->offset)a->offset=row;if(row>=a->offset+visible)a->offset=row-visible+1;invalidate(w->bounds);return 1;}
  if(key==10&&a->usb_selected10){unsigned row=a->usb_selected10-1;a->offset=row;devices_action10(w,a,token,USB_ROW10);return 1;}
  if(key==27){memset(selected11[slot],0,sizeof(selected11[slot]));a->usb_selected10=0;invalidate(w->bounds);return 1;}
  return 0;
 }
 unsigned total=children72(a),slot=(unsigned)(a-apps),columns=file_columns11(w),visible=file_page_size11(w);
 if(key==1){memset(selected11[slot],0,sizeof(selected11[slot]));for(unsigned row=0;row<total;++row){unsigned index=selection_index11(a,row);if(index<V6_ENTRIES)selected11[slot][index/8]|=(uint8_t)(1u<<(index%8));}invalidate(w->bounds);return 1;}
 if(key==138){int wheel=input10_take_wheel(),position=(int)a->offset-wheel*(int)(file_windows11[slot].view?columns:3);if(position<0)position=0;if(total&&(unsigned)position>=total)position=(int)total-1;if(file_windows11[slot].view)position=position/(int)columns*(int)columns;a->offset=(unsigned)position;invalidate(w->bounds);return 1;}
 if(key==133||key==134||key==128||key==129||key==130||key==131||key==32){if(!total)return 1;unsigned row=selection_focus11[slot];if(row>=total)row=0;if(key==130)row=0;else if(key==131)row=total-1;else if(key==133)row=row>=columns?row-columns:0;else if(key==134)row=row+columns<total?row+columns:total-1;else if(key==128){if(row)--row;}else if(key==129){if(row+1<total)++row;}
  if((modifiers&1)&&key!=32&&!(modifiers&2))selection_focus11[slot]=row;else select_row11(a,row,key==32?modifiers&1:modifiers&2,0);
  if(row<a->offset)a->offset=file_windows11[slot].view?row/columns*columns:row;if(row>=a->offset+visible)a->offset=file_windows11[slot].view?row/columns*columns:row-visible+1;invalidate(w->bounds);return 1;}
 if(key==10){file_open_selection11(token);return 1;}
 if(key==27){memset(selected11[slot],0,sizeof(selected11[slot]));i11_cancel(&interaction11);invalidate(w->bounds);return 1;}
 return 0;
}
