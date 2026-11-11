/* Native M11 interaction broker. The control catalogue remains paint-owned. */
static unsigned selected_row11(application*a,unsigned row){return row<V6_ENTRIES&&!!(selected11[a-apps][row/8]&(1u<<(row%8)));}
static void select_row11(application*a,unsigned row,unsigned modifiers,unsigned preserve){
 unsigned slot=(unsigned)(a-apps),total=children72(a);if(a->usb_mode10==2&&usb_listing10(a,0,1,&total))return;if(row>=total)return;if(row>=V6_ENTRIES){notice(a,"Selection supports up to 1024 items in one folder.");return;}
 if(preserve&&selected_row11(a,row))return;
 if(!(modifiers&3))memset(selected11[slot],0,sizeof(selected11[slot]));
 if(modifiers&2){unsigned anchor=selection_anchor11[slot];if(anchor>=total)anchor=row;if(!(modifiers&1))memset(selected11[slot],0,sizeof(selected11[slot]));unsigned lo=anchor<row?anchor:row,hi=anchor>row?anchor:row;for(unsigned i=lo;i<=hi;++i)selected11[slot][i/8]|=(uint8_t)(1u<<(i%8));}
 else {if(modifiers&1)selected11[slot][row/8]^=(uint8_t)(1u<<(row%8));else selected11[slot][row/8]|=(uint8_t)(1u<<(row%8));selection_anchor11[slot]=row;}
 selection_focus11[slot]=row;
 serial_text("M11_SELECTION row=");serial_hex(row);serial_text("\n");
}
static i11_target target11(int x,int y){
 uint64_t token=desktop_hit(&model,x,y);
 for(unsigned i=control_count;i;--i){control*c=&controls[i-1];desktop_rect r=c->rect;
  if(x>=r.x&&x<r.x+r.width&&y>=r.y&&y<r.y+r.height&&((context11&&c->action>=32000&&c->action<=32003)||(launcher?c->window==0:c->window==token)||y>=(int)model.height-60)){
   desktop_window*w=desktop_lookup(&model,c->window);
   unsigned item=w&&w->app==FILES&&!apps[w-model.windows].closing&&c->action>=FILE_ROW&&c->action<FILE_ROW+V6_ENTRIES;
   if(w&&w->app==FILES&&apps[w-model.windows].usb_mode10==2&&c->action>=USB_ROW10&&c->action<USB_ROW10+6)item=1;
   return(i11_target){c->window,c->action,item?I11_ITEM:I11_BUTTON};
  }
 }
 if(context11)return(i11_target){0,0,I11_BACKGROUND};
 desktop_window*w=desktop_lookup(&model,token);
 if(w){desktop_rect r=w->bounds;
  if(y<r.y+35){unsigned command=x>r.x+r.width-52?31001:x>r.x+r.width-90?31002:31000;return(i11_target){token,command,command==31000?I11_CAPTION:I11_BUTTON};}
  if(x>=r.x+r.width-18&&y>=r.y+r.height-18)return(i11_target){token,31003,I11_CAPTION};
  return(i11_target){token,0,I11_BACKGROUND};
 }
 if(!launcher&&x>=30&&x<150&&y>=110&&y<385)return(i11_target){0,30000+(y<195?0u:y<290?1u:2u),I11_ITEM};
 return(i11_target){0,0,I11_BACKGROUND};
}
static void draw_context11(void){
 if(!context11)return;
 int x=(int)context_x11,y=(int)context_y11;ui73_round(x,y,244,142,8,paper());
 button(context_window11,x+6,y+6,232,context_action11?"Open":"Open Files",32000,context_choice11==0);
 button(context_window11,x+6,y+52,232,"About this selection",32001,context_choice11==1);
 button(context_window11,x+6,y+98,232,"Close menu",32002,context_choice11==2);
}
static void open_target11(i11_target t){
 desktop_focus11=0;
 if(t.action>=30000&&t.action<=30002){if(t.action==30002)launch(SETTINGS,0);else {uint64_t n=launch(FILES,0);desktop_window*w=desktop_lookup(&model,n);if(w&&t.action==30001)files_action72(w,&apps[w-model.windows],n,NAV_DOCUMENTS);}whole();return;}
 action(t.window,t.action);
}
static void dispatch11(i11_target t,unsigned event,unsigned x,unsigned y){
 if(context11&&!(t.action>=32000&&t.action<=32003)){context11=0;whole();if(!(event&I11_CONTEXT))return;}
 if(t.window)desktop_focus11=0;else if(t.kind==I11_ITEM||t.kind==I11_BACKGROUND)desktop_focus11=1;
 if(event&I11_CONTEXT){
  if(t.kind==I11_ITEM&&t.window){desktop_window*w=desktop_lookup(&model,t.window);if(w&&w->app==FILES)select_row11(&apps[w-model.windows],t.action>=USB_ROW10&&t.action<USB_ROW10+6?apps[w-model.windows].offset+t.action-USB_ROW10:t.action-FILE_ROW,0,1);}
  if(!t.window&&t.kind==I11_ITEM&&t.action>=30000&&t.action<=30002){unsigned row=t.action-30000;if(!(desktop_selection11&(1u<<row)))desktop_selection11=1u<<row;desktop_item_focus11=row;}
  context11=1;context_choice11=0;context_window11=t.window;context_action11=t.kind==I11_ITEM?t.action:0;
  context_x11=model.width>260&&x>model.width-250?model.width-250:x;
  context_y11=model.height>210&&y>model.height-210?model.height-210:y;
  launcher=0;control_active=0;whole();serial_text("M11_CONTEXT_OPEN\n");return;
 }
 if(t.action>=32000&&t.action<=32003){unsigned choice=t.action;uint64_t window=context_window11;unsigned command=context_action11;context11=0;whole();if(choice==32000){if(command)open_target11((i11_target){window,command,I11_ITEM});else launch(FILES,0);}else if(choice==32001){desktop_window*w=desktop_lookup(&model,window);if(w)notice(&apps[w-model.windows],"Selected item. File operations arrive in M11.2.");else copy(shell_notice,"Engacer OS desktop. Right-click actions are context dependent.",sizeof(shell_notice));}return;}
 if((event&I11_SELECT)&&t.kind==I11_ITEM){
  if(t.window){desktop_window*w=desktop_lookup(&model,t.window);if(w&&w->app==FILES){application*a=&apps[w-model.windows];unsigned row=t.action>=USB_ROW10&&t.action<USB_ROW10+6?a->offset+t.action-USB_ROW10:t.action-FILE_ROW;select_row11(a,row,dispatch_modifiers11,0);if(a->usb_mode10==2)a->usb_selected10=row+1;invalidate(w->bounds);}}
  else {unsigned row=t.action-30000,mod=dispatch_modifiers11;if(row<3){if(mod&2){unsigned lo=desktop_anchor11<row?desktop_anchor11:row,hi=desktop_anchor11>row?desktop_anchor11:row;if(!(mod&1))desktop_selection11=0;for(unsigned i=lo;i<=hi;++i)desktop_selection11|=1u<<i;}else{if(mod&1)desktop_selection11^=1u<<row;else desktop_selection11=1u<<row;desktop_anchor11=row;}desktop_item_focus11=row;}whole();}
  control_active=0;
 }
 if((event&I11_SELECT)&&t.kind==I11_BACKGROUND){desktop_window*w=desktop_lookup(&model,t.window);if(w&&w->app==FILES){memset(selected11[w-model.windows],0,sizeof(selected11[0]));invalidate(w->bounds);}else{desktop_selection11=0;whole();}}
 if(event&I11_ACTIVATE){
  if(t.action==31001){close_window(t.window,0);task_damage();}
  else if(t.action==31002){desktop_minimize(&model,t.window,1);task_damage();}
  else if(t.kind==I11_ITEM||t.kind==I11_BUTTON)open_target11(t);
 }
}
static void marquee_update11(int x,int y){
 desktop_window*w=desktop_lookup(&model,marquee_window11);if(marquee_window11&&!w){marquee11=0;return;}
 int left=x<interaction11.x?x:interaction11.x,top=y<interaction11.y?y:interaction11.y,right=x>interaction11.x?x:interaction11.x,bottom=y>interaction11.y?y:interaction11.y;
 marquee_rect11=(desktop_rect){left,top,right-left+1,bottom-top+1};
 if(!w){desktop_selection11=(input11_modifiers()&1)?marquee_desktop_saved11:0;for(unsigned i=0;i<3;++i){int yy=110+(int)i*95;if(30<right&&150>left&&yy<bottom&&yy+85>top){desktop_selection11|=1u<<i;desktop_item_focus11=i;}}whole();return;}
 unsigned slot=(unsigned)(w-model.windows);
 if(input11_modifiers()&1)memcpy(selected11[slot],marquee_saved11,sizeof(marquee_saved11));else memset(selected11[slot],0,sizeof(selected11[slot]));
 for(unsigned i=0;i<control_count;++i){control*c=&controls[i];desktop_rect r=c->rect;if(c->window!=marquee_window11||c->action<FILE_ROW||c->action>=FILE_ROW+V6_ENTRIES)continue;
 if(r.x<right&&r.x+r.width>left&&r.y<bottom&&r.y+r.height>top){unsigned row=c->action-FILE_ROW;selected11[slot][row/8]|=(uint8_t)(1u<<(row%8));selection_focus11[slot]=row;}}
 whole();
}
static unsigned keyboard11(int key){
 dispatch_modifiers11=input11_modifiers();
 if(key==141){context11=marquee11=dragging=0;i11_cancel(&interaction11);whole();return 1;}
 if(key==27&&(context11||interaction11.captured||marquee11||dragging)){context11=marquee11=0;i11_cancel(&interaction11);dragging=0;whole();return 1;}
 desktop_window*w=model.focused<DESKTOP_WINDOWS?&model.windows[model.focused]:0;
 if(key==140){i11_target t={0,0,I11_BACKGROUND};if(desktop_focus11&&desktop_selection11){t.kind=I11_ITEM;t.action=30000+desktop_item_focus11;}else if(!desktop_focus11&&w&&w->live){t.window=desktop_token(&model,model.focused);if(w->app==FILES&&!apps[model.focused].usb_mode10&&children72(&apps[model.focused])){t.kind=I11_ITEM;t.action=FILE_ROW+selection_focus11[model.focused];}else if(w->app==FILES&&apps[model.focused].usb_mode10==2&&apps[model.focused].usb_selected10){t.kind=I11_ITEM;t.action=USB_ROW10+apps[model.focused].usb_selected10-1-apps[model.focused].offset;}}dispatch11(t,I11_CONTEXT,pointer_x,pointer_y);return 1;}
 if(context11){if(key==10){dispatch11((i11_target){context_window11,32000+context_choice11,I11_BUTTON},I11_ACTIVATE,0,0);return 1;}if(key==9||key==134){context_choice11=(context_choice11+1)%3;whole();}else if(key==136||key==133){context_choice11=(context_choice11+2)%3;whole();}return 1;}
 if(key==27)interaction11.last_valid=0;
 if(launcher)return 0;
 if(desktop_focus11||!w||!w->live){
  if(key==10&&desktop_selection11){open_target11((i11_target){0,30000+desktop_item_focus11,I11_ITEM});desktop_focus11=0;return 1;}
  if(key==1){desktop_selection11=7;whole();return 1;}
  if(key==27){desktop_selection11=0;whole();return 1;}
  if(key==133||key==134||key==128||key==129){unsigned row=desktop_item_focus11;if(key==133||key==128){if(row)--row;}else if(row<2)++row;dispatch11((i11_target){0,30000+row,I11_ITEM},I11_SELECT,0,0);return 1;}
  return 0;
 }
 application*a=&apps[w-model.windows];if(w->app!=FILES||a->closing)return 0;
 if(a->usb_mode10){if(a->usb_mode10==2&&key==10&&!control_active&&a->usb_selected10){unsigned row=a->usb_selected10-1;if(row>=a->offset&&row<a->offset+6)action(desktop_token(&model,(unsigned)(w-model.windows)),USB_ROW10+row-a->offset);return 1;}if(key==27){memset(selected11[w-model.windows],0,sizeof(selected11[0]));a->usb_selected10=0;invalidate(w->bounds);return 1;}return 0;}
 unsigned total=children72(a),slot=(unsigned)(w-model.windows);uint64_t token=desktop_token(&model,slot);
 if(key==1){memset(selected11[slot],0,sizeof(selected11[slot]));for(unsigned i=0;i<total;++i)selected11[slot][i/8]|=(uint8_t)(1u<<(i%8));invalidate(w->bounds);return 1;}
 if(key==133||key==134||key==128||key==129||key==130||key==131){if(!total)return 1;unsigned row=selection_focus11[slot];if(key==130)row=0;else if(key==131)row=total-1;else if(key==133||key==128){if(row)--row;}else if(row+1<total)++row;select_row11(a,row,input11_modifiers()&2,0);if(row<a->offset)a->offset=row;unsigned visible=w->bounds.height>220?(unsigned)(w->bounds.height-220)/44:1;if(row>=a->offset+visible)a->offset=row-visible+1;invalidate(w->bounds);return 1;}
 if(key==10&&!control_active){for(unsigned i=0;i<total;++i)if(selected_row11(a,i)){action(token,FILE_ROW+i);break;}return 1;}
 if(key==27){memset(selected11[slot],0,sizeof(selected11[slot]));invalidate(w->bounds);return 1;}
 return 0;
}
