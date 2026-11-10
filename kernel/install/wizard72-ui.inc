/* Native framebuffer setup. Choices use the same verified storage transaction. */
static void preview72(unsigned x,unsigned y,unsigned w){
 gui6_rect(x+4,y+5,w,138,0xd5ddd9);gui6_rect(x,y,w,138,0x173d43);
 for(unsigned i=0;i<80;++i)gui6_rect(x+w-100+i,y+i/2,20,1,0x24595a);
 gui6_rect(x+22,y+18,w-80,94,0xfafbf8);gui6_rect(x+22,y+18,w-80,17,0xe3ece7);
 label(x+32,y+24,"Files",1,0x24483e);gui6_rect(x+22,y+35,65,77,0xeaf0eb);
 label(x+30,y+47,"Home",1,0x285349);label(x+30,y+64,"Documents",1,0x60766d);
 for(unsigned i=0;i<3;++i){unsigned fx=x+107+i*62;gui6_rect(fx,y+52,19,7,0xe1b866);gui6_rect(fx,y+58,32,24,0xf0ce87);}
 gui6_rect(x,y+122,w,16,0x0d252b);gui6_rect(x+10,y+127,28,5,accent);
}
static void render72(void){
 unsigned old=focus;controls=0;
 for(unsigned row=0;row<screen->framebuffer_height;++row){unsigned shade=18+row*12/screen->framebuffer_height;gui6_rect(0,row,screen->framebuffer_width,1,(shade<<16)|((shade+16)<<8)|(shade+22));}
 gui6_rect(left+6,top+8,width,height,0x07151a);gui6_rect(left,top,width,height,0xf6f7f1);
 gui6_rect(left,top,196,height,0x102c32);gui6_rect(left,top,196,5,accent);
 label(left+24,top+30,"E",4,accent);label(left+64,top+38,"" ENGINE_PRODUCT_NAME "",2,0xf1f7f2);
 label(left+24,top+86,"MAKE IT YOURS",1,0x9bc0ba);
 const char *names[]={"Welcome","Your disk","Your setup","Your style","Review","Installing","Your desktop"};
 for(unsigned i=0;i<7;++i){unsigned sy=top+132+i*49;
 if(i==step)gui6_rect(left+14,sy-9,168,35,0x21474a);
 char digit[2]={(char)('1'+i),0};label(left+27,sy,digit,2,i<=step?accent:0x789592);
 label(left+52,sy+3,names[i],1,i==step?0xffffff:0xacc4bd);}
 label(left+24,top+height-57,"YOUR SPACE.",1,0x87aaa5);label(left+24,top+height-37,"YOUR SYSTEM.",1,0x87aaa5);
 unsigned x=body+10,y=top+92;uint32_t ink=0x193a3b,muted=0x526d68;
 label(x,top+34,step==6?"READY WHEN YOU ARE":"LET'S GET YOU SETTLED",1,0x52766b);
 for(unsigned i=0;i<7;++i)gui6_rect(x+i*31,top+57,23,3,i<=step?accent:0xd5dfd5);
 if(step==0){label(x,y,"Make room",4,ink);label(x,y+42,"for your ideas.",4,ink);
 label(x,y+101,"A real desktop. Your folders. Your own space.",1,muted);
 preview72(x,y+137,width-295);
 label(x,y+301,"A few simple choices. No terminal commands.",1,muted);
 label(x,y+325,"Nothing is erased until you review and confirm.",1,muted);
 label(x,y+350,"UEFI media adds a startup entry; erasing a disk still needs Install.",1,muted);
 if(validation)label(x,y+367,validation,1,0x95472f);
 }
 if(step==1){label(x,y,"Where should we live?",3,ink);
 label(x,y+41,"Choose the disk for " ENGINE_PRODUCT_NAME ". Your files will live here.",1,muted);
 unsigned count=storage_count();if(!count){label(x,y+105,"No supported disk is connected.",2,0x95472f);label(x,y+143,"VMware: add a supported hard disk and connect it at power on.",1,muted);}
 for(unsigned row=0;row<4;++row){unsigned i=disk_page*4+row;if(i>=count)break;const storage_disk*d=storage_get(i);
 char name[24]="Disk ";decimal(i+1,name+5);unsigned dy=y+77+row*55;
 button(x,dy,135,name,100+i,chosen==i);char size[48];decimal(d->sectors/2097152,size);unsigned n=0;while(size[n])++n;const char*s=" GiB  /  ";for(unsigned j=0;s[j];++j)size[n++]=s[j];size[n]=0;label(x+154,dy+10,size,2,ink);
 label(x+310,dy+14,m6_target_writable(d)&&d->sector_bytes==512?"Disk - installation supported":"Browse only - unsupported install target",1,muted);}
 if(count>4){button(x+420,y+77,60,"<",94,0);button(x+490,y+77,60,">",95,0);}
 label(x,y+307,"Installation uses the whole disk. Review before erasing.",1,0x95472f);
 if(!maintenance){button(x,y+333,220,"Manage disks",90,0);button(x+235,y+333,310,replace_selected?"Erase and install: ON":"Erase and install: OFF",91,replace_selected);}
 if(validation)label(x,y+395,validation,1,0x95472f);
 }
 if(step==2){label(x,y,"The essentials. Ready.",3,ink);preview72(x,y+58,width-295);
 label(x,y+224,"Desktop and Files are included.",2,ink);label(x,y+257,"Browse your folders immediately after installation.",1,muted);
 button(x,y+300,310,preferences.guide?"Welcome guide: On":"Welcome guide: Off",20,0);
 label(x,y+358,"Your personal files stay separate from the system.",1,muted);}
 if(step==3){label(x,y,"Make it feel like you.",3,ink);label(x,y+55,"Your name (letters, numbers, - or _)",1,muted);
 button(x,y+83,320,preferences.user,30,0);label(x,y+151,"Choose your accent",2,ink);
 button(x,y+190,135,"Mint",31,accent==0x62e5bd);button(x+150,y+190,135,"Lavender",32,accent==0x9caeff);button(x+300,y+190,135,"Amber",33,accent==0xf4c47c);
 label(x,y+259,"Change the look later in desktop Settings.",1,muted);label(x,y+286,"Your name and choices are saved on disk.",1,muted);
 if(validation)label(x,y+331,validation,1,0x95472f);}
 if(step==4){label(x,y,"One last look.",4,ink);label(x,y+57,maintenance?"Verified boot maintenance; your personal files stay.":"" ENGINE_PRODUCT_NAME " will use the whole selected disk.",1,0x95472f);
 gui6_rect(x,y+92,width-295,155,0xe7eee5);char number[24];decimal(chosen+1,number);label(x+18,y+112,"INSTALLATION DISK",1,muted);label(x+220,y+108,number,2,ink);
 label(x+18,y+151,"YOUR NAME",1,muted);label(x+220,y+147,preferences.user,2,ink);
 decimal((m6_plan()->data.last-m6_plan()->data.first+1)/2048,number);label(x+18,y+190,"FILE SPACE (MiB)",1,muted);label(x+220,y+186,number,2,ink);
 label(x,y+260,preferences.guide?"Welcome guide: Included":"Welcome guide: Excluded",1,muted);
 char color[16]="Accent #";const char*hex="0123456789ABCDEF";for(unsigned i=0;i<6;++i)color[8+i]=hex[(accent>>(20-i*4))&15];color[14]=0;label(x+300,y+260,color,1,muted);
 label(x,y+277,"Next: a real desktop with your folders, ready to explore.",1,muted);
 if(!maintenance&&replace_selected){button(x,y+325,520,replace_consent?"[X] Erase ALL data on the selected disk":"[ ] Erase ALL data on the selected disk",92,0);}
 if(validation)label(x,y+390,validation,1,0x95472f);}
 if(step==5){label(x,y,"Making it yours.",4,ink);preview72(x,y+66,width-295);label(x,y+237,stage?stage:"Preparing your system",1,muted);
 gui6_rect(x,y+278,width-295,12,0xd7e3d6);gui6_rect(x,y+278,(width-295)*percent/100,12,0x2c997b);
 char progress[24];decimal(percent,progress);label(x,y+312,progress,2,ink);label(x+53,y+317,"percent copied and verified",1,muted);
 label(x,y+364,"Keep the disk connected. We are saving your system.",1,muted);}
 if(step>=6){label(x,y,maintenance?"All set.":"Welcome home.",4,ink);preview72(x,y+65,width-295);
 label(x,y+232,maintenance?"Boot files verified. Your personal files were kept.":"Your desktop and folders are ready on the installed disk.",1,muted);
 label(x,y+272,maintenance?"Power off, disconnect the ISO, then start again.":"Choose Open desktop. Start exploring right now.",2,ink);
 label(x,y+324,"For future boots: keep the supported hard disk connected.",1,muted);
 label(x,y+350,"Disconnect only the CD/DVD ISO, then start the VM.",1,muted);}
 gui6_rect(left+196,bottom-1,width-196,1,0xd4dfd5);label(x,bottom+30,"Mouse or Tab + Enter",1,muted);
 if(!busy&&step<5){if(step)button(left+width-300,bottom+14,120,"Back",1,0);button(left+width-160,bottom+14,135,step==4?(maintenance?"Confirm":"Install"):"Next",2,1);}
 if(step==0){button(x,y+412,300,"Open installed desktop",83,0);button(x+315,y+412,135,"Recovery",50,0);}
 if(step==6)button(left+width-230,bottom+14,205,maintenance?"Finish":"Open desktop",3,1);
 focus=old<controls?old:0;
}
