static void appearance_path11(char*path){copy(path,"/System/Settings/appearance11-",256);append(path,owner.user,256);append(path,".bin",256);}
static void appearance_load11(void){appearance11=(appearance_record11){0x41313145,1,preferences.accent,preferences.dark,ui73_scale_choice(),DISTRO_DESKTOP_WALLPAPER,0,0,{{0}}};char path[256];appearance_path11(path);appearance_record11 p;unsigned bytes=0;
 if(v6_verify(m6_volume(),path)||v6_read(m6_volume(),path,0,&p,sizeof(p),&bytes)||bytes!=sizeof(p)||p.magic!=appearance11.magic||p.version!=1||p.accent>0xffffff||p.dark>1||p.wallpaper>0xffffff||p.pattern>2||p.reserved||(p.scale&& (p.scale<2||p.scale>4)))return;
 for(unsigned i=0;i<20;++i){desktop_rect r=p.normal[i];if(r.width&&(r.width<320||r.width>4096||r.height<200||r.height>2160||r.x< -4096||r.x>4096||r.y<0||r.y>2160))return;}
 appearance11=p;preferences.accent=p.accent;preferences.dark=p.dark;const os_boot_info_v0*changed=ui73_set_scale(p.scale);if(!changed){changed=ui73_set_scale(0);copy(shell_notice,"Saved scale needs a larger display. Automatic scale is active for this session.",96);}if(changed){display=changed;model.width=changed->framebuffer_width;model.height=changed->framebuffer_height;}
}
static unsigned appearance_save11(void){appearance11.accent=preferences.accent;appearance11.dark=preferences.dark;appearance11.scale=ui73_scale_choice();for(unsigned i=0;i<model.count;++i){unsigned slot=model.order[i];desktop_window*w=&model.windows[slot];window_state11*s=window_state_for11(desktop_token(&model,slot));if(w->app<20)appearance11.normal[w->app]=s->mode?s->normal:w->bounds;}char path[256];appearance_path11(path);return v6_put(m6_volume(),path,sizeof(appearance11),memory_source,&appearance11);}
static uint32_t desktop_ink11(void){unsigned c=appearance11.wallpaper,r=c>>16,g=(c>>8)&255,b=c&255;return r*299+g*587+b*114>150000?0x183240:0xe5f0f3;}
static void appearance_settings11(desktop_window*w,application*a,uint64_t token){int x=w->bounds.x+24,y=w->bounds.y+56;ui73_fit(x,y,w->bounds.width-48,"Make Engacer OS yours",4,ink());
 text(x,y+48,"Window accent",1,ink());const char*names[]={"Mint","Lavender","Amber"};const unsigned colors[]={0x62e5bd,0x9caeff,0xf4c47c};for(unsigned i=0;i<3;++i)button(token,x+(int)i*112,y+74,104,names[i],STYLE_MINT+i,preferences.accent==colors[i]);
 text(x,y+124,"Window appearance",1,ink());button(token,x,y+150,148,"Light",STYLE_PAPER,!preferences.dark);button(token,x+160,y+150,148,"Dark",STYLE_DARK,preferences.dark);
 text(x,y+200,"Wallpaper",1,ink());button(token,x,y+226,104,"Ocean",45000,appearance11.wallpaper==0x142c35);button(token,x+112,y+226,104,"Night",45001,appearance11.wallpaper==0x182039);button(token,x+224,y+226,104,"Paper",45002,appearance11.wallpaper==0xd7e4e9);button(token,x+336,y+226,168,appearance11.pattern==0?"Gradient":appearance11.pattern==1?"Solid":"Horizon",45003,0);
 text(x,y+276,"Display size",1,ink());const char*sizes[]={"Auto","100%","150%","200%"};for(unsigned i=0;i<4;++i)button(token,x+(int)i*104,y+302,96,sizes[i],SCALE_AUTO+i,ui73_scale_choice()==(i?i+1:0));
 button(token,x,y+352,170,"Save preferences",PREFERENCES_SAVE,1);button(token,x+182,y+352,238,"Double-click speed...",32100,0);button(token,x+432,y+352,184,"Start / taskbar...",45004,0);
 button(token,x+432,y+74,184,"Devices",DEVICES10,0);button(token,x+432,y+150,184,"Applications",10000,0);
 ui73_paragraph(x,w->bounds.y+w->bounds.height-64,w->bounds.width-48,2,a->notice[0]?a->notice:"Preview changes now. Save keeps your appearance and normal window sizes.",ink());
}

static uint32_t desktop_label_ink11(unsigned row){return desktop_selection11&(1u<<row)?0xe5f0f3:desktop_ink11();}
