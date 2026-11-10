/* Original M7.5 guided installer: persistent frame, explicit disk list,
   large hit targets and damage-only focus. No second framebuffer allocation. */
static volatile unsigned painting73;
static unsigned full_frames75,content_frames75,focus_frames75,name_frames75,last_step75=UINT32_MAX;
static uint64_t longest_frame75,paint_cycles75;
static unsigned framed75,previous_focus75=UINT32_MAX,previous_count75,previous_mode75=UINT32_MAX;
static const char*title75[24];static unsigned kind75[24];
static uint32_t signature75,previous_accent75;
static unsigned chosen_state75;
static uint64_t counter75(void){uint32_t a,d;__asm__ volatile("rdtsc":"=a"(a),"=d"(d));return ((uint64_t)d<<32)|a;}
static void outline75(control c,uint32_t color){
 gui6_rect(c.x,c.y,c.w,2,color);gui6_rect(c.x,c.y+c.h-2,c.w,2,color);gui6_rect(c.x,c.y,2,c.h,color);gui6_rect(c.x+c.w-2,c.y,2,c.h,color);
}
static void tap75(unsigned x,unsigned y,unsigned w,unsigned h,const char*text,unsigned action,unsigned kind){
 uint32_t bg=kind==1?accent:kind==2?0xeff4f7:kind==3?0xf8e9e4:0xffffff,ink=kind==1?0x102c25:kind==2?0x6a7e89:0x193746;
 ui73_round(x,y,w,h,8,kind==2?0xdce5e9:0xbacbd4);ui73_round(x+2,y+2,w-4,h-4,6,bg);
 if(kind!=2&&controls<24){unsigned i=controls++;buttons[i]=(control){x,y,w,h,action};title75[i]=text;kind75[i]=kind;}
 ui73_fit(x+16,y+(h-22)/2,w-32,text,2,ink);
}
static unsigned suitable75(const storage_disk*d){return d&&d->readable&&m6_target_writable(d)&&d->sector_bytes==512&&d->sectors>=UINT64_C(4194304)&&d->sectors<=UINT64_C(4294967296);}
static void inspect75(unsigned disk){const storage_disk*d=storage_get(disk);chosen_state75=3;if(!suitable75(d))return;unsigned status=m6_prepare(disk);chosen_state75=status==B_OK?1:status==B_IO?4:2;m6_cancel();}
static void diskname75(unsigned disk,char*out){memcpy(out,"Disk ",5);decimal(disk+1,out+5);}
static void capacity75(const storage_disk*d,char*out){decimal(d->sectors/2097152,out);unsigned n=0;while(out[n])++n;memcpy(out+n," GiB",5);}
static void art75(unsigned x,unsigned y,unsigned w){
 ui73_round(x,y,w,128,12,0xe4f2ed);unsigned mid=x+w/2;
 ui73_round(mid-111,y+20,222,83,9,0x193c4c);ui73_round(mid-104,y+27,208,65,5,0xf5fafc);
 ui73_folder(mid-58,y+44,32,0xf0c87c);ui73_folder(mid-3,y+44,32,0x98d9c2);gui6_rect(mid-1,y+103,3,10,0x193c4c);ui73_round(mid-40,y+112,80,4,2,0x193c4c);
}
static uint32_t model75(void){uint32_t h=2166136261u;const uint8_t*p=(const uint8_t*)&preferences;for(unsigned i=0;i<sizeof(preferences);++i)h=(h^p[i])*16777619u;h^=chosen_state75*53+chosen*31+replace_selected*7+replace_consent*19+disk_page*37+percent*43;h^=(uint32_t)(uintptr_t)validation;return h;}
static void render72(void){
 painting73=1;uint64_t began=counter75();unsigned oldfocus=focus;uint32_t sig=model75();gui7_clip_reset();
 unsigned x=body+12,y=top+98,cw=width-276;uint32_t ink=0x193746,muted=0x506976;
 if(framed75&&previous_mode75==0&&last_step75==5&&step==5){
  gui7_clip(x,y+260,cw,168);gui6_rect(x,y+260,cw,168,0xf8fafb);ui73_paragraph(x,y+272,cw,2,stage?stage:"Preparing the installation",ink);
  ui73_round(x,y+335,cw,14,7,0xd8e4eb);if(percent)ui73_round(x,y+335,cw*percent/100,14,7,0x229d80);char value[24];decimal(percent,value);unsigned at=0;while(value[at])++at;memcpy(value+at,"%",2);label(x,y+373,value,3,ink);signature75=sig;goto done75;
 }
 if(framed75&&previous_mode75==0&&last_step75==3&&step==3&&previous_accent75==accent&&signature75!=sig){
  gui6_rect(x,y+94,cw,64,0xf8fafb);ui73_round(x,y+97,cw,56,8,0xbacbd4);ui73_round(x+2,y+99,cw-4,52,6,0xffffff);
  ui73_fit(x+16,y+114,cw-32,preferences.user[0]?preferences.user:"Click here and type your name",2,ink);
  if(focus<controls)outline75(buttons[focus],0x19846e);gui6_rect(x,y+378,cw,70,0xf8fafb);ui73_paragraph(x,y+382,cw,2,validation?validation:"You can change your desktop appearance again in Settings.",validation?0x994334:muted);
  signature75=sig;++name_frames75;goto done75;
 }
 /* Tab/Shift+Tab changes only the two focus rings, preserving all content. */
 if(framed75&&previous_mode75==0&&last_step75==step&&signature75==sig&&previous_count75==controls){
  if(previous_focus75<controls)outline75(buttons[previous_focus75],kind75[previous_focus75]==1?accent:0xbacbd4);
  if(focus<controls)outline75(buttons[focus],0x19846e);++focus_frames75;goto done75;
 }
 if(!framed75||previous_mode75!=0){
  gui6_rect(0,0,screen->framebuffer_width,screen->framebuffer_height,0x173340);ui73_shadow(left,top,width,height);ui73_round(left,top,width,height,16,0xf8fafb);
  ui73_round(left,top,196,height,16,0x102c3b);gui6_rect(left+180,top,16,height,0x102c3b);
  ui73_fit(left+24,top+28,152,DISTRO_NAME,3,0xffffff);label(left+24,top+73,"SETUP",1,0xa9c7d4);
  label(left+24,top+height-52,"Your space.",1,0xb4ced8);label(left+24,top+height-32,"Your system.",1,0xb4ced8);framed75=1;++full_frames75;
 }
 ++content_frames75;
 /* The outer frame/background never clears on Next. Only changing panes paint. */
 gui6_rect(left+196,top+16,width-212,height-16,0xf8fafb);
 const char*names[]={"Welcome","Choose a disk","Your apps","Your style","Review","Installing","Ready"};
 for(unsigned i=0;i<7;++i){unsigned sy=top+128+i*54;gui6_rect(left+12,sy-8,172,42,0x102c3b);ui73_round(left+12,sy-8,172,42,8,i==step?0x264e5d:0x102c3b);char n[2]={(char)('1'+i),0};label(left+25,sy,n,2,i<=step?accent:0x7394a5);label(left+52,sy+2,names[i],1,i==step?0xffffff:0xb4ced8);}
 char steptext[32]="STEP ";decimal(step+1,steptext+5);unsigned n=0;while(steptext[n])++n;memcpy(steptext+n," OF 7",6);label(x,top+29,steptext,1,muted);
 gui6_rect(x,top+65,cw,4,0xdbe6ec);gui6_rect(x,top+65,cw*(step+1)/7,4,0x239e80);
 controls=0;
 if(step==0){label(x,y,"Let's set up " ENGINE_PRODUCT_NAME ".",4,ink);ui73_paragraph(x,y+55,cw,2,"A few simple choices. Your own desktop, ready to use.",muted);art75(x,y+116,cw);
  label(x,y+268,"1. Choose where to install",2,ink);label(x,y+307,"2. Make it yours",2,ink);label(x,y+346,"3. Review, install and restart",2,ink);
  ui73_paragraph(x,y+395,cw,2,validation?validation:"We will ask before erasing anything. You do not need an internet connection.",validation?0x994334:muted);
 }
 if(step==1){label(x,y,maintenance?"Choose your " ENGINE_PRODUCT_NAME " disk":"Choose an installation disk",4,ink);ui73_paragraph(x,y+49,cw,2,"Click a row in the list. The selected row is highlighted.",muted);
  unsigned count=storage_count();ui73_round(x,y+98,cw,266,10,0xc5d5dd);ui73_round(x+2,y+100,cw-4,262,8,0xffffff);
  label(x+20,y+113,"INSTALLATION DISKS",1,muted);label(x+cw-118,y+113,"CAPACITY",1,muted);
  if(!count)ui73_paragraph(x+22,y+166,cw-44,2,"No disk found. Power off VMware, add a supported hard disk and connect it at power on. Then start again.",0x994334);
  for(unsigned row=0;row<4;++row){unsigned i=disk_page*4+row;if(i>=count)break;const storage_disk*d=storage_get(i);unsigned dy=y+145+row*53;char name[32],size[32];diskname75(i,name);capacity75(d,size);
   uint32_t bg=chosen==i?0xe1f5ec:0xffffff;gui6_rect(x+4,dy,cw-8,52,bg);if(chosen==i)gui6_rect(x+4,dy,4,52,0x229d80);
   if(controls<24){unsigned at=controls++;buttons[at]=(control){x+4,dy,cw-8,52,100+i};title75[at]="";kind75[at]=0;}
   ui73_round(x+20,dy+17,18,18,9,chosen==i?0x238d73:0xa7bcc8);ui73_round(x+23,dy+20,12,12,6,bg);if(chosen==i)ui73_round(x+26,dy+23,6,6,3,0x238d73);
   label(x+52,dy+6,name,2,ink);ui73_fit(x+52,dy+31,cw-205,suitable75(d)?(chosen==i?(chosen_state75==1?"Selected - empty disk":chosen_state75==2?"Selected - contains existing data":"Selected - check connection"):"supported hard disk") : "Cannot install here - inspection only",1,muted);label(x+cw-118,dy+15,size,2,ink);
  }
  if(count>4){tap75(x,y+373,82,42,"Prev",94,0);tap75(x+92,y+373,82,42,"Next",95,0);}
  else if(!maintenance){tap75(x,y+377,180,44,"Disk tools...",90,chosen<count?0:2);if(chosen_state75==2)tap75(x+194,y+377,cw-194,44,replace_selected?"[x] Erase this disk and install":"[ ] Erase this disk and install",91,0);else if(chosen_state75==1)label(x+200,y+390,"Empty disk - ready to install",2,ink);}
  ui73_paragraph(x,y+435,cw,1,validation?validation:chosen>=count?"Select a disk above to enable Next.":chosen_state75==1?"The installer will prepare this empty disk automatically.":chosen_state75==3?"This disk cannot be used for installation. Choose a supported disk, 2 GiB to 2 TiB.":chosen_state75==4?"This disk could not be read. Check its disk connection and try again.":replace_selected?"Review will ask for separate permission before erasing this disk.":"This disk contains files. Back them up before choosing Erase this disk and install.",validation?0x994334:muted);
 }
 if(step==2){label(x,y,"Your everyday essentials.",4,ink);ui73_paragraph(x,y+53,cw,2,"Files, Settings and the desktop are ready for you. Standard apps are included with this build.",muted);art75(x,y+119,cw);
  label(x,y+270,"Make changes whenever you like.",3,ink);ui73_paragraph(x,y+315,cw,2,"Remove or reinstall optional apps in Settings. Your files stay separate, and the system keeps working.",muted);tap75(x,y+390,cw,52,preferences.guide?"[x] Include the getting-started guide":"[ ] Include the getting-started guide",20,0);
 }
 if(step==3){label(x,y,"Make yourself at home.",4,ink);label(x,y+63,"Your account name",2,ink);tap75(x,y+97,cw,56,preferences.user[0]?preferences.user:"Click here and type your name",30,0);
  ui73_paragraph(x,y+171,cw,2,"Click the box to type. Use letters, numbers, - or _. Backspace removes a letter.",muted);label(x,y+249,"Choose your colour",2,ink);
  unsigned w=(cw-24)/3;tap75(x,y+291,w,56,accent==0x62e5bd?"(o) Mint":"( ) Mint",31,0);tap75(x+w+12,y+291,w,56,accent==0x9caeff?"(o) Lavender":"( ) Lavender",32,0);tap75(x+2*(w+12),y+291,w,56,accent==0xf4c47c?"(o) Amber":"( ) Amber",33,0);
  ui73_paragraph(x,y+382,cw,2,validation?validation:"You can change your desktop appearance again in Settings.",validation?0x994334:muted);
 }
 if(step==4){label(x,y,maintenance?"Review the repair":"Ready to install?",4,ink);ui73_paragraph(x,y+54,cw,2,maintenance?"Boot files will be verified. Personal files will be kept.":"Check the disk and your choices below. Nothing has been erased yet.",muted);
  ui73_round(x,y+114,cw,210,10,0xeaf0f4);char disk[32],size[32];diskname75(chosen,disk);capacity75(storage_get(chosen),size);
  const char*keys[]={"Selected disk","Disk capacity","Your account","Getting-started guide","Your colour"};const char*values[]={disk,size,preferences.user,preferences.guide?"Included":"Not included",accent==0x62e5bd?"Mint":accent==0x9caeff?"Lavender":"Amber"};
  for(unsigned i=0;i<5;++i){label(x+20,y+132+i*36,keys[i],2,muted);ui73_fit(x+cw/2,y+132+i*36,cw/2-20,values[i],2,ink);}
  if(!maintenance){if(replace_selected)tap75(x,y+346,cw,54,replace_consent?"[x] Erase all files on the selected disk":"[ ] I agree to erase this entire disk",92,3);
   ui73_paragraph(x,y+(replace_selected?416:350),cw,2,validation?validation:replace_selected?"This cannot be undone. Other disks are not selected for installation.":"This disk will be prepared automatically. Use Back to change any choice.",validation?0x994334:muted);}
 }
 if(step==5){label(x,y,"Making it yours.",4,ink);ui73_paragraph(x,y+56,cw,2,"Keep the computer on. We are saving and checking your system.",muted);art75(x,y+119,cw);ui73_paragraph(x,y+272,cw,2,stage?stage:"Preparing the installation",ink);
  ui73_round(x,y+335,cw,14,7,0xd8e4eb);if(percent)ui73_round(x,y+335,cw*percent/100,14,7,0x229d80);char value[24];decimal(percent,value);unsigned at=0;while(value[at])++at;memcpy(value+at,"%",2);label(x,y+373,value,3,ink);
 }
 if(step==6){label(x,y,"You're ready to go.",4,ink);ui73_paragraph(x,y+54,cw,2,"" ENGINE_PRODUCT_NAME " is installed on your disk. Your choices have been saved.",muted);art75(x,y+116,cw);
  label(x,y+268,"Next: start your installed system.",3,ink);ui73_paragraph(x,y+312,cw,2,"Choose Restart. " ENGINE_PRODUCT_NAME " will start from the hard disk. You can leave this matching installer connected.",muted);
  ui73_paragraph(x,y+383,cw,2,"Keep the hard disk connected. You can disconnect the installation ISO later.",muted);if(!maintenance)tap75(x,y+434,250,42,"Open desktop now",3,0);
 }
 gui6_rect(left+196,bottom-8,width-214,1,0xdce6ec);
 if(step==0){tap75(x,bottom+8,188,48,"Recovery & details",50,0);tap75(x+202,bottom+8,228,48,"Open installed system",83,0);}
 else if(step<5)tap75(x,bottom+8,120,48,"Back",1,0);
 if(step<5){unsigned disabled=step==1&&(chosen>=storage_count()||(!maintenance&&(chosen_state75==3||chosen_state75==4||(chosen_state75==2&&!replace_selected))));if(step==4&&!maintenance&&replace_selected&&!replace_consent)disabled=1;
  tap75(left+width-242,bottom+8,214,48,step==4?(maintenance?"Apply repair":"Install " ENGINE_PRODUCT_NAME ""):"Next",2,disabled?2:1);
 }
 if(step==6)tap75(left+width-298,bottom+8,270,48,"Restart to " ENGINE_PRODUCT_NAME "",84,1);
 if(last_step75!=step){focus=0;for(unsigned i=0;i<controls;++i)if(kind75[i]==1)focus=i;}
 else focus=oldfocus<controls?oldfocus:0;
 if(focus<controls)outline75(buttons[focus],0x19846e);
 signature75=sig;previous_accent75=accent;last_step75=step;previous_mode75=0;previous_count75=controls;
 done75:
 previous_focus75=focus;gui7_clip_reset();painting73=0;paint_cycles75=counter75()-began;if(paint_cycles75>longest_frame75)longest_frame75=paint_cycles75;
 serial_text("M73_VIEW step=");serial_hex(step);serial_text("\n");serial_text("M75_FRAME full=");serial_hex(full_frames75);serial_text(" content=");serial_hex(content_frames75);serial_text(" focus=");serial_hex(focus_frames75);serial_text("\n");
}
