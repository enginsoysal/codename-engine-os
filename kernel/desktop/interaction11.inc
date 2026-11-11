/* Shared input broker; item identity survives sorting and unrelated file writes. */
static unsigned selected_row11(application*a,unsigned row){unsigned index=selection_index11(a,row);return index<V6_ENTRIES&&!!(selected11[a-apps][index/8]&(1u<<(index%8)));}
static void select_row11(application*a,unsigned row,unsigned modifiers,unsigned preserve){
 unsigned slot=(unsigned)(a-apps),total=children72(a);if(a->usb_mode10==2&&usb_listing10(a,0,1,&total))return;if(row>=total||row>=V6_ENTRIES)return;
 if(preserve&&selected_row11(a,row)){selection_focus11[slot]=row;return;}
 if(!(modifiers&3))memset(selected11[slot],0,sizeof(selected11[slot]));
 if(modifiers&2){unsigned anchor=selection_anchor11[slot];if(anchor>=total)anchor=row;if(!(modifiers&1))memset(selected11[slot],0,sizeof(selected11[slot]));unsigned lo=anchor<row?anchor:row,hi=anchor>row?anchor:row;for(unsigned i=lo;i<=hi;++i){unsigned index=selection_index11(a,i);if(index<V6_ENTRIES)selected11[slot][index/8]|=(uint8_t)(1u<<(index%8));}}
 else{unsigned index=selection_index11(a,row);if(index>=V6_ENTRIES)return;if(modifiers&1)selected11[slot][index/8]^=(uint8_t)(1u<<(index%8));else selected11[slot][index/8]|=(uint8_t)(1u<<(index%8));selection_anchor11[slot]=row;}
 selection_focus11[slot]=row;serial_text("M11_SELECTION row=");serial_hex(row);serial_text("\n");
}
static i11_target target11(int x,int y){
 if(launcher&&!context11&&y<(int)model.height-60&&(x<start_bounds11.x||x>=start_bounds11.x+start_bounds11.width||y<start_bounds11.y||y>=start_bounds11.y+start_bounds11.height))return(i11_target){0,0,I11_BACKGROUND};
 if(switch_open11){for(unsigned i=control_count;i;--i){control*c=&controls[i-1];desktop_rect r=c->rect;if(c->action>=W11_SWITCH&&c->action<W11_SWITCH+DESKTOP_WINDOWS&&x>=r.x&&x<r.x+r.width&&y>=r.y&&y<r.y+r.height)return(i11_target){0,c->action,I11_BUTTON};}return(i11_target){0,0,I11_BACKGROUND};}
 uint64_t token=desktop_hit(&model,x,y);desktop_window*edge_window=desktop_lookup(&model,token);unsigned edges=window_edges11(edge_window,x,y);if(edges&&!context11&&!launcher)return(i11_target){token,31010+edges,I11_CAPTION};
 for(unsigned i=control_count;i;--i){control*c=&controls[i-1];desktop_rect r=c->rect;if(launcher&&!context11&&y<(int)model.height-60&&(c->action<S11_PINNED||c->action>=S11_END))continue;if(file_block_control11(c->window,c->action))continue;
  if(x>=r.x&&x<r.x+r.width&&y>=r.y&&y<r.y+r.height&&(file_modal_control11(c->window,c->action)||(context11&&c->action>=32000&&c->action<=32031)||(launcher?c->window==0:c->window==token)||y>=(int)model.height-60)){
   desktop_window*w=desktop_lookup(&model,c->window);unsigned item=w&&w->app==FILES&&!apps[w-model.windows].closing&&c->action>=FILE_ROW&&c->action<FILE_ROW+V6_ENTRIES;
   if(w&&w->app==FILES&&apps[w-model.windows].usb_mode10==2&&c->action>=USB_ROW10&&c->action<USB_ROW10+6)item=1;
   if(!c->window&&c->action>=F11_DESKTOP_ITEM&&c->action<F11_DESKTOP_ITEM+V6_ENTRIES)item=1;
   return(i11_target){c->window,c->action,item?I11_ITEM:I11_BUTTON};
  }
 }
 if(context11||launcher)return(i11_target){0,0,I11_BACKGROUND};
 if(file_block_owner11(token))return(i11_target){token,0,I11_BACKGROUND};
 desktop_window*w=desktop_lookup(&model,token);if(w){desktop_rect r=w->bounds;if(y<r.y+40){unsigned command=x>=r.x+r.width-52?31001:x>=r.x+r.width-96?W11_MAX:x>=r.x+r.width-140?31002:31000;return(i11_target){token,command,command==31000?I11_CAPTION:I11_BUTTON};}if(x>=r.x+r.width-18&&y>=r.y+r.height-18)return(i11_target){token,31003,I11_CAPTION};return(i11_target){token,0,I11_BACKGROUND};}
 if(!launcher&&x>=30&&x<150&&y>=110&&y<385)return(i11_target){0,30000+(y<195?0u:y<290?1u:2u),I11_ITEM};return(i11_target){0,0,I11_BACKGROUND};
}
static void draw_context11(void){files_draw_context11();}
static void open_target11(i11_target target){
 if(target.action>=30000&&target.action<=30002){desktop_focus11=0;if(target.action==30002)launch(SETTINGS,0);else{char path[256];if(target.action==30001)root_path(path,"Documents");else home72(path);launch(FILES,path);}whole();return;}
 if(!target.window&&target.action>=F11_DESKTOP_ITEM&&target.action<F11_DESKTOP_ITEM+V6_ENTRIES){const v6_entry*e=desktop_child11(target.action-F11_DESKTOP_ITEM);if(e){char path[256];copy(path,e->path,256);desktop_focus11=1;file_open11(path,0);}return;}
 desktop_focus11=0;action(target.window,target.action);
}
static void dispatch11(i11_target target,unsigned event,unsigned x,unsigned y){
 if(switch_open11){if((event&I11_ACTIVATE)&&target.action>=W11_SWITCH&&target.action<W11_SWITCH+DESKTOP_WINDOWS)window_action11(0,target.action);return;}
 if(start_dispatch11(target,event,x,y))return;
 if(context11&&!(target.action>=32000&&target.action<=32031)){if(event&I11_CONTEXT)return;context11=0;whole();return;}
 if(target.action!=70&&target.action!=0&&!(target.action>=S11_PINNED&&target.action<S11_END)&&file_block_owner11(target.window)&&!file_modal_control11(target.window,target.action))return;
 if(target.window&&target.action!=70)desktop_focus11=0;else if(target.kind==I11_ITEM||target.kind==I11_BACKGROUND)desktop_focus11=1;
 if(event&I11_CONTEXT){if(target.kind==I11_ITEM&&target.window){desktop_window*w=file_owner_window11(target.window);if(w){application*a=&apps[w-model.windows];unsigned row=target.action>=USB_ROW10&&target.action<USB_ROW10+6?a->offset+target.action-USB_ROW10:target.action-FILE_ROW;select_row11(a,row,0,1);if(a->usb_mode10==2)a->usb_selected10=row+1;}}
  else if(!target.window&&target.kind==I11_ITEM){unsigned row=target.action>=F11_DESKTOP_ITEM?target.action-F11_DESKTOP_ITEM+3:target.action-30000;desktop_select11(row,0,1);}
  files_context_open11(target,x,y);serial_text("M11_CONTEXT_OPEN\n");return;
 }
 if(target.action>=32000&&target.action<=32031){files_context_invoke11(target.action-32000);return;}
 if((event&I11_SELECT)&&target.kind==I11_ITEM){if(target.window){desktop_window*w=file_owner_window11(target.window);if(w){application*a=&apps[w-model.windows];unsigned row=target.action>=USB_ROW10&&target.action<USB_ROW10+6?a->offset+target.action-USB_ROW10:target.action-FILE_ROW;select_row11(a,row,dispatch_modifiers11,0);if(a->usb_mode10==2)a->usb_selected10=row+1;invalidate(w->bounds);}}else{unsigned row=target.action>=F11_DESKTOP_ITEM?target.action-F11_DESKTOP_ITEM+3:target.action-30000;desktop_select11(row,dispatch_modifiers11,0);whole();}control_active=0;}
 if((event&I11_SELECT)&&target.kind==I11_BACKGROUND){desktop_window*w=file_owner_window11(target.window);if(w){memset(selected11[w-model.windows],0,sizeof(selected11[0]));invalidate(w->bounds);}else{desktop_selection11=0;memset(desktop_files_selected11,0,sizeof(desktop_files_selected11));whole();}}
 if(event&I11_ACTIVATE){if(target.action==W11_MAX||target.action==31000){window_state11*s=window_state_for11(target.window);if(s)window_mode11(target.window,s->mode?0:1);}else if(target.action==31001){close_window(target.window,0);task_damage();}else if(target.action==31002){desktop_minimize(&model,target.window,1);task_damage();}else if(target.kind==I11_ITEM||target.kind==I11_BUTTON)open_target11(target);}
}
static void marquee_update11(int x,int y){desktop_window*w=desktop_lookup(&model,marquee_window11);if(marquee_window11&&!w){marquee11=0;return;}int left=x<interaction11.x?x:interaction11.x,top=y<interaction11.y?y:interaction11.y,right=x>interaction11.x?x:interaction11.x,bottom=y>interaction11.y?y:interaction11.y;marquee_rect11=(desktop_rect){left,top,right-left+1,bottom-top+1};
 if(!w){if(dispatch_modifiers11&1){desktop_selection11=marquee_desktop_saved11;memcpy(desktop_files_selected11,marquee_desktop_files_saved11,sizeof(desktop_files_selected11));}else{desktop_selection11=0;memset(desktop_files_selected11,0,sizeof(desktop_files_selected11));}for(unsigned i=0;i<3;++i){int yy=110+(int)i*95;if(30<right&&150>left&&yy<bottom&&yy+85>top){desktop_set_selected11(i,1);desktop_item_focus11=i;}}for(unsigned i=0;i<control_count;++i){control*c=&controls[i];desktop_rect r=c->rect;if(c->window||c->action<F11_DESKTOP_ITEM||c->action>=F11_DESKTOP_ITEM+V6_ENTRIES)continue;if(r.x<right&&r.x+r.width>left&&r.y<bottom&&r.y+r.height>top){unsigned row=c->action-F11_DESKTOP_ITEM+3;desktop_set_selected11(row,1);desktop_item_focus11=row;}}whole();return;}
 unsigned slot=(unsigned)(w-model.windows);if(dispatch_modifiers11&1)memcpy(selected11[slot],marquee_saved11,sizeof(marquee_saved11));else memset(selected11[slot],0,sizeof(selected11[slot]));for(unsigned i=0;i<control_count;++i){control*c=&controls[i];desktop_rect r=c->rect;if(c->window!=marquee_window11||c->action<FILE_ROW||c->action>=FILE_ROW+V6_ENTRIES)continue;if(r.x<right&&r.x+r.width>left&&r.y<bottom&&r.y+r.height>top){unsigned row=c->action-FILE_ROW,index=selection_index11(&apps[slot],row);if(index<V6_ENTRIES)selected11[slot][index/8]|=(uint8_t)(1u<<(index%8));selection_focus11[slot]=row;}}whole();
}
static unsigned keyboard11(int key){dispatch_modifiers11=input11_modifiers();if(window_key11(key))return 1;if(key==141){files_cancel_drag11();context11=marquee11=dragging=0;i11_cancel(&interaction11);whole();return 1;}if(key==27&&(context11||interaction11.captured||marquee11||dragging||file_drag11)){files_cancel_drag11();context11=marquee11=0;i11_cancel(&interaction11);dragging=0;whole();return 1;}
 if(start_key11(key))return 1;
 if(files_key11(key))return 1;
 desktop_window*w=model.focused<DESKTOP_WINDOWS?&model.windows[model.focused]:0;
 if(key==140){i11_target target={0,0,I11_BACKGROUND};if(desktop_focus11&&(desktop_selection11||(desktop_item_focus11>=3&&file_selected_desktop11(desktop_item_focus11-3)))){target.kind=I11_ITEM;target.action=desktop_item_focus11<3?30000+desktop_item_focus11:F11_DESKTOP_ITEM+desktop_item_focus11-3;}else if(!desktop_focus11&&w&&w->live){target.window=desktop_token(&model,model.focused);application*a=&apps[model.focused];if(w->app==FILES&&!a->usb_mode10&&children72(a)){target.kind=I11_ITEM;target.action=FILE_ROW+selection_focus11[model.focused];}else if(w->app==FILES&&a->usb_mode10==2&&a->usb_selected10){a->offset=a->usb_selected10-1;target.kind=I11_ITEM;target.action=USB_ROW10;}}dispatch11(target,I11_CONTEXT,pointer_x,pointer_y);return 1;}
 if(context11){if(key==10||key==32){files_context_invoke11(context_choice11);return 1;}if(key==9||key==134){context_choice11=files_context_next11(context_choice11,1);whole();}else if(key==136||key==133){context_choice11=files_context_next11(context_choice11,-1);whole();}else if(key==138){int wheel=input10_take_wheel();while(wheel){context_choice11=files_context_next11(context_choice11,wheel<0?1:-1);wheel+=wheel<0?1:-1;}whole();}return 1;}
 return 0;
}
