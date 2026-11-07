/* One semantic type scale, shared spacing and measured wrapped copy per step. */
static void illustration73(unsigned x,unsigned y,unsigned w){
 ui73_round(x,y,w,140,12,0x183747);ui73_round(x+24,y+20,w-48,99,8,0xf7fafc);
 gui6_rect(x+24,y+44,w-48,1,0xdce6ec);ui73_round(x+36,y+29,6,6,3,accent);
 for(unsigned i=0;i<3;++i){unsigned fx=x+54+i*(w-92)/3;ui73_folder(fx,y+62,37,0xf1ce88);ui73_round(fx,y+102,48,3,1,0xc7d7df);}
}
static volatile unsigned painting73;
static void progress73(unsigned x,unsigned y,unsigned cw,uint32_t ink,uint32_t muted){
 gui6_rect(x,y+216,cw,208,0xf6f8fa);ui73_paragraph(x,y+228,cw,2,stage?stage:"Preparing your system",muted);
 ui73_round(x,y+294,cw,10,5,0xdce6ec);if(percent)ui73_round(x,y+294,cw*percent/100,10,5,0x229d80);
 char number[24];decimal(percent,number);label(x,y+326,number,3,ink);label(x+65,y+332,"percent copied and verified",1,muted);ui73_paragraph(x,y+378,cw,2,"Keep the disk connected while your system is being saved.",muted);
}
static void render72(void){
 painting73=1;
 static unsigned last=UINT32_MAX;unsigned old=focus;controls=0;gui7_clip_reset();
 unsigned x=body+10,y=top+92,cw=width-280;uint32_t ink=0x193746,muted=0x48616f;
 if(step==5&&last==5){gui7_clip(x,y+210,cw,216);progress73(x,y,cw,ink,muted);goto finished73;}last=step;
 gui6_rect(0,0,screen->framebuffer_width,screen->framebuffer_height,0x152b3a);
 ui73_shadow(left,top,width,height);ui73_round(left,top,width,height,14,0xf6f8fa);
 ui73_round(left,top,196,height,14,0x102a3a);gui6_rect(left+180,top,16,height,0x102a3a);
 ui73_fit(left+24,top+30,148,DISTRO_NAME,3,0xf7fafc);ui73_fit(left+24,top+78,148,"Make it yours.",1,0xb7cbd8);
 const char*names[]={"Welcome","Your disk","Your setup","Your style","Review","Installing","Your desktop"};
 for(unsigned i=0;i<7;++i){unsigned sy=top+132+i*49;if(i==step)ui73_round(left+12,sy-7,172,38,7,0x254857);
 char digit[2]={(char)('1'+i),0};label(left+26,sy,digit,2,i<=step?accent:0x8daaba);label(left+52,sy,names[i],1,i==step?0xffffff:0xb7cbd8);}
 ui73_fit(left+24,top+height-48,148,"Feel at home.",1,0xb7cbd8);
 label(x,top+28,step==6?"READY TO EXPLORE":"WELCOME TO YOUR WORKSPACE",1,0x486b7b);
 for(unsigned i=0;i<7;++i)ui73_round(x+i*31,top+64,23,3,1,i<=step?0x219d80:0xd4e0e7);
 if(step==0){label(x,y,"A fresh start. Made yours.",4,ink);
 ui73_paragraph(x,y+54,cw,2,DISTRO_WELCOME,muted);
 illustration73(x,y+132,cw);ui73_paragraph(x,y+295,cw,2,"Choose a disk, make it yours and review the changes. Nothing is erased until you choose Install.",muted);
 if(validation)ui73_paragraph(x,y+355,cw,2,validation,0x9a4434);
 else ui73_paragraph(x,y+355,cw,2,"UEFI setup adds a startup entry. Your disk still needs a separate installation confirmation.",muted);
 }
 if(step==1){label(x,y,"Where should Engine OS live?",4,ink);
 ui73_paragraph(x,y+52,cw,2,maintenance?"Choose the existing Engine OS disk to maintain.":"Select your installation disk. Its capacity and connection are shown below.",muted);
 unsigned count=storage_count();if(!count)ui73_paragraph(x,y+124,cw,3,"No supported disk is connected. In VMware, add a SATA hard disk and enable Connect at power on.",0x9a4434);
 for(unsigned row=0;row<4;++row){unsigned i=disk_page*4+row;if(i>=count)break;const storage_disk*d=storage_get(i);unsigned dy=y+100+row*48;char name[24]="Disk ";decimal(i+1,name+5);
 button(x,dy,120,name,100+i,chosen==i);char size[40];decimal(d->sectors<(UINT64_C(2097152))?d->sectors/2048:d->sectors/2097152,size);unsigned n=0;while(size[n])++n;const char*tail=d->sectors<UINT64_C(2097152)?" MiB":" GiB";for(unsigned j=0;tail[j];++j)size[n++]=tail[j];size[n]=0;
 label(x+144,dy+10,size,2,ink);ui73_fit(x+255,dy+10,cw-255,d->transport==STORAGE_AHCI&&d->sector_bytes==512&&d->sectors>=UINT64_C(4194304)&&d->sectors<=UINT64_C(4294967296)?"SATA - ready for installation":"Available for inspection only",1,muted);}
 if(count>4){button(x+cw-132,y+286,56,"<",94,0);button(x+cw-66,y+286,56,">",95,0);}
 if(!maintenance){button(x,y+322,220,"Manage disks",90,0);button(x+235,y+322,310,replace_selected?"Erase and install: On":"Erase and install: Off",91,replace_selected);}
 ui73_paragraph(x,y+380,cw,2,validation?validation:maintenance?"Maintenance keeps personal files; review before applying changes.":"Installation uses the whole selected disk. Existing data requires a separate erase confirmation.",validation?0x9a4434:muted);}
 if(step==2){label(x,y,"The essentials, ready for you.",4,ink);illustration73(x,y+66,cw);
 label(x,y+228,"Desktop and Files are included.",3,ink);ui73_paragraph(x,y+270,cw,2,"Your personal files stay separate from system files. Add the welcome guide if you would like a quick introduction.",muted);
 button(x,y+338,310,preferences.guide?"Include welcome guide: On":"Include welcome guide: Off",20,0);}
 if(step==3){label(x,y,"Make yourself at home.",4,ink);label(x,y+62,"Your name",2,ink);button(x,y+94,350,preferences.user,30,0);
 ui73_paragraph(x,y+150,cw,2,"Use letters, numbers, a dash or an underscore. Your name and choices will be saved on disk.",muted);
 label(x,y+224,"Choose an accent",2,ink);button(x,y+264,135,"Mint",31,accent==0x62e5bd);button(x+150,y+264,135,"Lavender",32,accent==0x9caeff);button(x+300,y+264,135,"Amber",33,accent==0xf4c47c);
 ui73_paragraph(x,y+330,cw,2,validation?validation:"You can change the appearance again in desktop Settings.",validation?0x9a4434:muted);}
 if(step==4){label(x,y,"Ready when you are.",4,ink);ui73_paragraph(x,y+52,cw,2,maintenance?"Review this boot maintenance operation. Personal files will be kept.":"Review your choices. Installation will use the whole selected disk.",muted);
 ui73_round(x,y+106,cw,192,10,0xe8eff4);char disk[24],space[40],color[16]="#";decimal(chosen+1,disk);decimal((m6_plan()->data.last-m6_plan()->data.first+1)/2048,space);
 const char*hex="0123456789ABCDEF";for(unsigned i=0;i<6;++i)color[1+i]=hex[(accent>>(20-i*4))&15];color[7]=0;
 const char*keys[]={"Installation disk","Your name","Data partition (MiB)","Welcome guide","Accent"};const char*values[]={disk,preferences.user,space,preferences.guide?"Included":"Not included",color};
 for(unsigned i=0;i<5;++i){label(x+20,y+123+i*32,keys[i],1,muted);ui73_fit(x+235,y+123+i*32,cw-255,values[i],2,ink);}
 if(!maintenance&&replace_selected)button(x,y+328,520,replace_consent?"[X] Erase ALL data on this disk":"[ ] Erase ALL data on this disk",92,0);
 if(validation)ui73_paragraph(x,y+382,cw,2,validation,0x9a4434);
 else if(!replace_selected)ui73_paragraph(x,y+330,cw,2,"Go Back to change anything. Choose Install only when you are ready.",muted);}
 if(step==5){label(x,y,"Making it yours.",4,ink);illustration73(x,y+58,cw);progress73(x,y,cw,ink,muted);}
 if(step>=6){label(x,y,maintenance?"Your system is ready.":"Welcome home.",4,ink);illustration73(x,y+62,cw);
 label(x,y+226,maintenance?"Boot files have been verified.":"Your desktop and folders are ready.",3,ink);
 ui73_paragraph(x,y+272,cw,2,maintenance?"Power off, disconnect only the installation ISO, and start the VM again.":"Choose Open desktop to start exploring. Your files are stored on the installed disk.",muted);
 ui73_round(x,y+338,cw,82,8,0xe8eff4);ui73_paragraph(x+16,y+352,cw-32,2,"For later boots, keep the SATA hard disk connected. Disconnect only the CD/DVD installation media.",muted);}
 gui6_rect(left+196,bottom-1,width-196,1,0xd8e3e9);label(x,bottom+25,"Mouse or Tab and Enter",1,muted);
 if(!busy&&step<5){if(step)button(left+width-300,bottom+14,120,"Back",1,0);button(left+width-160,bottom+14,135,step==4?(maintenance?"Confirm":"Install"):"Next",2,1);}
 if(step==0){button(x,y+412,300,"Open installed desktop",83,0);button(x+315,y+412,135,"Recovery",50,0);}
 if(step==6)button(left+width-230,bottom+14,205,maintenance?"Finish":"Open desktop",3,1);
 finished73:
 focus=old<controls?old:0;gui7_clip_reset();painting73=0;serial_text("M73_VIEW step=");serial_hex(step);serial_text("\n");
}
