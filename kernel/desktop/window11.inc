/* Native window management. Bounds remain logical, rasterization physical. */
static window_state11*window_state_for11(uint64_t token){desktop_window*w=desktop_lookup(&model,token);if(!w)return 0;window_state11*s=&window_geometry11[w-model.windows];if(s->token!=token){s->token=token;s->normal=w->bounds;s->mode=0;}return s;}
static void window_minimum11(unsigned app,int*width,int*height){*width=480;*height=420;
 if(app==SETTINGS){*width=700;*height=550;}if(app==DOWNLOADS)*height=460;
 if(app==CALCULATOR){*width=660;*height=650;}if(app==CLOCK74){*width=700;*height=510;}
 if(app==CALENDAR74){*width=760;*height=640;}if(app==DISKS74||app==RECOVERY74){*width=850;*height=620;}
 if(app==SOFTWARE74){*width=820;*height=620;}if(app==IMAGES74)*width=640;
 if(app==SCREENSHOT74||app==ARCHIVES74){*width=660;*height=530;}
 if(app==MEDIA74){*width=760;*height=580;}if(app==BROWSER74){*width=850;*height=650;}
 if(app==HELP74){*width=700;*height=560;}if(app==WINDOWS8){*width=960;*height=650;}
 if(*width>(int)model.width)*width=model.width;if(*height>(int)model.height-60)*height=model.height-60;
}
static desktop_rect window_fit11(desktop_rect r){if(r.width>(int)model.width)r.width=model.width;if(r.height>(int)model.height-60)r.height=model.height-60;if(r.x+r.width>(int)model.width)r.x=(int)model.width-r.width;if(r.y+r.height>(int)model.height-60)r.y=(int)model.height-60-r.height;if(r.x<0)r.x=0;if(r.y<0)r.y=0;return r;}
static void window_admit11(uint64_t token){desktop_window*w=desktop_lookup(&model,token);if(!w)return;int minw,minh;window_minimum11(w->app,&minw,&minh);desktop_rect r=w->bounds;if(w->app<20&&appearance11.normal[w->app].width)r=appearance11.normal[w->app];if(r.width<minw)r.width=minw;if(r.height<minh)r.height=minh;desktop_place(&model,token,window_fit11(r));window_state11*s=window_state_for11(token);s->normal=w->bounds;s->mode=0;}
static unsigned window_snap_allowed11(desktop_window*w){int minw,minh;window_minimum11(w->app,&minw,&minh);return (int)model.width/2>=minw;}
static void window_mode11(uint64_t token,unsigned mode){desktop_window*w=desktop_lookup(&model,token);window_state11*s=window_state_for11(token);if(!w||!s)return;
 if(mode>=2&&!window_snap_allowed11(w)){copy(shell_notice,"This app needs more room. Maximize it or use a wider desktop.",96);whole();return;}
 if(!s->mode&&mode)s->normal=w->bounds;
 desktop_rect r=s->normal;if(mode==1)r=(desktop_rect){0,0,model.width,model.height-60};
 if(mode==2||mode==3){int left=mode==2?0:(int)model.width/2;r=(desktop_rect){left,0,(int)model.width-left,(int)model.height-60};if(mode==2)r.width=model.width/2;}
 launcher=context11=0;s->mode=mode;desktop_place(&model,token,r);desktop_focus11=0;desktop_focus(&model,token);control_active=0;whole();
}
static unsigned window_edges11(desktop_window*w,int x,int y){if(!w||file_block_owner11(desktop_token(&model,w-model.windows)))return 0;window_state11*s=window_state_for11(desktop_token(&model,w-model.windows));if(s&&s->mode==1)return 0;desktop_rect r=w->bounds;unsigned e=0;if(x<r.x+6)e|=1;if(x>=r.x+r.width-6)e|=2;if(y<r.y+6)e|=4;if(y>=r.y+r.height-6)e|=8;if(x>=r.x+r.width-18&&y>=r.y+r.height-18)e=10;return e;}
static void window_caption11(desktop_window*w){desktop_rect r=w->bounds;uint64_t token=desktop_token(&model,w-model.windows);application*a=&apps[w-model.windows];char title[320];copy(title,titles[w->app],sizeof(title));
 if((w->app==FILES||w->app==EDITOR)&&a->path[0]){const char*name=a->path;for(unsigned i=0;a->path[i];++i)if(a->path[i]=='/'&&a->path[i+1])name=a->path+i+1;copy(title,a->dirty?"* ":"",sizeof(title));append(title,equal(a->path,"/")?"System drive":name,sizeof(title));append(title," - ",sizeof(title));append(title,titles[w->app],sizeof(title));}
 if(model.focused==(unsigned)(w-model.windows)&&!desktop_focus11&&!launcher)rectangle((desktop_rect){r.x+12,r.y+38,r.width-24,2},preferences.accent);
 ui73_fit(r.x+16,r.y+11,r.width-166,title,1,ink());
 unsigned hover_y=pointer_y>=(unsigned)r.y+2&&pointer_y<(unsigned)r.y+38;
 for(unsigned i=0;i<3;++i){int x=r.x+r.width-140+(int)i*44;if(hover_y&&(int)pointer_x>=x&&(int)pointer_x<x+40)ui73_round(x,r.y+2,40,36,5,i==2?0xc84d58:preferences.dark?0x3c5665:0xd7e3e9);}
 uint32_t color=ink();int x=r.x+r.width-128,y=r.y+18;rectangle((desktop_rect){x,y+6,13,2},color);x+=44;
 window_state11*s=window_state_for11(token);if(s&&s->mode){rectangle((desktop_rect){x+3,y-3,12,2},color);rectangle((desktop_rect){x+13,y-3,2,11},color);}
 rectangle((desktop_rect){x,y,12,2},color);rectangle((desktop_rect){x,y+10,12,2},color);rectangle((desktop_rect){x,y,2,12},color);rectangle((desktop_rect){x+10,y,2,12},color);x+=44;
 for(int i=0;i<12;++i){rectangle((desktop_rect){x+i,y+i,2,2},color);rectangle((desktop_rect){x+11-i,y+i,2,2},color);}
}
static void window_drag_begin11(i11_target target){desktop_window*w=desktop_lookup(&model,target.window);window_state11*s=window_state_for11(target.window);if(!w||!s)return;window_before11=w->bounds;window_mode_before11=s->mode;window_normal_before11=s->normal;
 dragging=1;drag_window=target.window;drag_x=interaction11.x;drag_y=interaction11.y;window_drag_edges11=target.action>=31010&&target.action<=31025?target.action-31010:target.action==31003?10:0;drag_resize=!!window_drag_edges11;
 if(!drag_resize&&s->mode){int offset=(interaction11.x-w->bounds.x)*s->normal.width/w->bounds.width;desktop_rect r=s->normal;r.x=interaction11.x-offset;r.y=interaction11.y-18;s->mode=0;desktop_place(&model,target.window,r);}else if(drag_resize)s->mode=0;
 drag_bounds=w->bounds;window_snap_preview11=0;
}
static void window_drag_update11(unsigned x,unsigned y){desktop_window*w=desktop_lookup(&model,drag_window);if(!w){dragging=window_snap_preview11=0;return;}desktop_rect r=drag_bounds;int dx=(int)x-(int)drag_x,dy=(int)y-(int)drag_y,minw,minh;window_minimum11(w->app,&minw,&minh);
 if(window_drag_edges11){if(window_drag_edges11&1){r.x+=dx;r.width-=dx;}if(window_drag_edges11&2)r.width+=dx;if(window_drag_edges11&4){r.y+=dy;r.height-=dy;}if(window_drag_edges11&8)r.height+=dy;
 if(r.width<minw){if(window_drag_edges11&1)r.x-=minw-r.width;r.width=minw;}if(r.height<minh){if(window_drag_edges11&4)r.y-=minh-r.height;r.height=minh;}}
 else{r.x+=dx;r.y+=dy;}
 unsigned preview=!drag_resize?(y<8?1:x<12?2:x>model.width-13?3:0):0;if(preview>=2&&!window_snap_allowed11(w))preview=0;
 if(preview!=window_snap_preview11){window_snap_preview11=preview;whole();}
 desktop_rect old=w->bounds;unsigned cache=!drag_resize&&!window_snap_preview11&&!model.damaged&&model.count&&model.order[model.count-1]==(unsigned)(w-model.windows);desktop_place(&model,drag_window,r);if(cache)(void)move_cached9(old,w->bounds);
}
static void window_drag_end11(unsigned cancel){unsigned redraw=cancel||window_snap_preview11;desktop_window*w=desktop_lookup(&model,drag_window);window_state11*s=window_state_for11(drag_window);if(w&&s){if(cancel){s->normal=window_normal_before11;s->mode=window_mode_before11;desktop_place(&model,drag_window,window_before11);}else if(window_snap_preview11){s->normal=window_mode_before11?window_normal_before11:window_before11;s->mode=1;window_mode11(drag_window,window_snap_preview11);}else if(!s->mode)s->normal=w->bounds;}dragging=window_keyboard_move11=window_snap_preview11=0;if(redraw)whole();}
static unsigned window_menu11(i11_target t,unsigned x,unsigned y){desktop_window*w=desktop_lookup(&model,t.window);if(!w||!(t.kind==I11_CAPTION||(t.action>=31000&&t.action<=31025)||t.action==70))return 0;window_state11*s=window_state_for11(t.window);file_menu_begin11(t.window,x,y);
 file_menu_add11("Restore",W11_RESTORE,s->mode!=0||w->minimized);file_menu_add11("Move (arrow keys)",W11_MOVE,!s->mode);file_menu_add11("Size (arrow keys)",W11_SIZE,!s->mode);file_menu_add11("Minimize",31002,1);file_menu_add11("Maximize",W11_MAX,s->mode!=1);file_menu_add11("Snap left",W11_LEFT,window_snap_allowed11(w));file_menu_add11("Snap right",W11_RIGHT,window_snap_allowed11(w));file_menu_add11("Close",31001,!file_block_owner11(t.window));copy(file_menu_reason11,"Move/size: arrow keys, Enter to keep, Escape to cancel.",128);file_menu_fit11();return 1;}
static unsigned window_action11(uint64_t token,unsigned command){
 if(command>=W11_SWITCH&&command<W11_SWITCH+DESKTOP_WINDOWS){unsigned i=command-W11_SWITCH;if(switch_open11&&i<switch_count11&&desktop_lookup(&model,switch_tokens11[i])){desktop_focus11=0;desktop_focus(&model,switch_tokens11[i]);}switch_open11=0;control_active=0;whole();return 1;}
 if(command<W11_MAX||command>=W11_SWITCH)return 0;desktop_window*w=desktop_lookup(&model,token);if(!w)return 1;
 if(command==W11_MAX||command==W11_RESTORE||command==W11_LEFT||command==W11_RIGHT){window_mode11(token,command==W11_RESTORE?0:command==W11_LEFT?2:command==W11_RIGHT?3:1);return 1;}
 if(command==W11_MOVE||command==W11_SIZE){desktop_focus(&model,token);interaction11.x=w->bounds.x+30;interaction11.y=w->bounds.y+18;window_drag_begin11((i11_target){token,command==W11_SIZE?31020:31000,I11_CAPTION});window_keyboard_move11=1;whole();return 1;}return 1;
}
static void window_overlay11(void){
 if(window_snap_preview11){int x=window_snap_preview11==3?(int)model.width/2:0,width=window_snap_preview11==1?(int)model.width:(int)model.width/2,height=model.height-60;rectangle((desktop_rect){x+6,6,width-12,3},preferences.accent);rectangle((desktop_rect){x+6,height-9,width-12,3},preferences.accent);rectangle((desktop_rect){x+6,6,3,height-12},preferences.accent);rectangle((desktop_rect){x+width-9,6,3,height-12},preferences.accent);}
 if(!switch_open11)return;int width=560,height=90+(int)switch_count11*48,x=((int)model.width-width)/2,y=((int)model.height-60-height)/2;ui73_shadow(x,y,width,height);ui73_round(x,y,width,height,12,paper());text(x+20,y+18,"Switch windows",3,ink());
 for(unsigned i=0;i<switch_count11;++i){desktop_window*w=desktop_lookup(&model,switch_tokens11[i]);if(w)button(0,x+20,y+68+(int)i*48,width-40,titles[w->app],W11_SWITCH+i,i==switch_choice11);}
}
static unsigned window_key11(int key){
 if(key==135||key==143){if(dragging){window_drag_end11(1);i11_cancel(&interaction11);}if(file_drag11)files_cancel_drag11();control_active=0;}
 if(dragging&&(key==27||key==141)){window_drag_end11(1);i11_cancel(&interaction11);return 1;}
 if(window_keyboard_move11){if(key==10){window_drag_end11(0);return 1;}if((key==128||key==129||key==133||key==134)){int x=drag_x,y=drag_y;if(key==128)x-=16;if(key==129)x+=16;if(key==133)y-=16;if(key==134)y+=16;window_drag_update11(x,y);desktop_window*w=desktop_lookup(&model,drag_window);if(w)drag_bounds=w->bounds;whole();}return 1;}
 if(key==135||key==143){if(!switch_open11){switch_count11=0;for(unsigned i=model.count;i;--i)switch_tokens11[switch_count11++]=desktop_token(&model,model.order[i-1]);switch_choice11=0;switch_open11=!!switch_count11;launcher=context11=0;}if(switch_count11)switch_choice11=(switch_choice11+(key==143?switch_count11-1:1))%switch_count11;whole();return 1;}
 if(switch_open11){if(key==27||key==141){switch_open11=0;whole();return 1;}if(key==142||key==10){window_action11(0,W11_SWITCH+switch_choice11);return 1;}return 1;}
 if(key==142)return 1;
 uint64_t token=model.focused<DESKTOP_WINDOWS?desktop_token(&model,model.focused):0;desktop_window*w=desktop_lookup(&model,token);
 if(key==144){if(context11){context11=0;whole();return 1;}if(launcher){launcher=0;whole();return 1;}if(w){if(file_block_owner11(token))copy(shell_notice,"Finish or cancel this window's dialog first.",96);else if(apps[w-model.windows].closing)apps[w-model.windows].closing=0;else close_window(token,0);whole();}return 1;}
 if(key==145&&w){window_menu11((i11_target){token,31000,I11_CAPTION},w->bounds.x+12,w->bounds.y+40);return 1;}
 if(key>=146&&key<=149&&w){if(key==149){window_state11*s=window_state_for11(token);if(s->mode)window_mode11(token,0);else{desktop_minimize(&model,token,1);whole();}}else window_mode11(token,key==146?2:key==147?3:1);return 1;}
 if(key==150){launch(FILES,0);whole();return 1;}return 0;
}

static void window_reflow11(void){for(unsigned i=0;i<DESKTOP_WINDOWS;++i){desktop_window*w=&model.windows[i];if(!w->live)continue;uint64_t token=desktop_token(&model,i);window_state11*s=window_state_for11(token);s->normal=window_fit11(s->normal);desktop_rect r=window_fit11(w->bounds);if(s->mode==1)r=(desktop_rect){0,0,model.width,model.height-60};else if(s->mode>=2){if(!window_snap_allowed11(w)){s->mode=0;r=s->normal;}else{int left=s->mode==2?0:(int)model.width/2;r=(desktop_rect){left,0,s->mode==2?(int)model.width/2:(int)model.width-left,(int)model.height-60};}}desktop_place(&model,token,r);}}
