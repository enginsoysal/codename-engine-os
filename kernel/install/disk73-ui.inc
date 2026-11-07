/* Same verified disk plans; measured typography and explicit consequences. */
static void u71_draw(unsigned x,unsigned y){
 unsigned cw=width-280;uint32_t muted=0xb7cbd8,warn=0xffc3a9;
 label(x,y,u71_mode==1?"Manage your disks":u71_mode==2?"Create a partition":u71_mode==3?"Choose a format":u71_mode==5?"Repair a partition table":"Review disk changes",4,0xffffff);
 u71_identity(x,y+48);
 if(u71_mode==1){ui73_fit(x,y+104,cw,u71_scheme(),1,muted);button(x+cw-132,y+43,58,"<",201,0);button(x+cw-66,y+43,58,">",202,0);
  unsigned total=u71_state.count+u71_state.gaps;
  for(unsigned row=0;row<4;++row){unsigned item=u71_page*4+row;if(item>=total)break;unsigned pos=y+134+row*48;char name[64];uint64_t first,last;
   if(item<u71_state.count){const d71_partition*p=u71_state.parts+item;memcpy(name,"Partition ",10);decimal(p->index+1,name+10);first=p->first;last=p->last;button(x,pos,160,name,300+row,u71_row==item);ui73_fit(x+175,pos,cw-325,p->name,1,muted);ui73_fit(x+175,pos+22,cw-325,p->protected?"Protected":p->format,1,muted);}
   else{d71_gap g=u71_state.free[item-u71_state.count];first=g.first;last=g.last;button(x,pos,160,"Free space",300+row,u71_row==item);}
   textnum(x+cw-140,pos+10,(last-first+1)/2048," MiB");
  }
  button(x,y+326,56,"<",203,0);button(x+66,y+326,56,">",204,0);button(x+138,y+326,145,"Refresh",205,0);
  button(x+300,y+326,175,"New partition",210,0);button(x+490,y+326,100,"Delete",211,0);
  button(x,y+382,135,"Format",212,0);button(x+150,y+382,135,"New GPT",213,0);button(x+300,y+382,135,"Clear disk",214,0);button(x+450,y+382,140,"Repair GPT",215,0);
 }else if(u71_mode==2){label(x,y+116,"Size in MiB",1,muted);button(x,y+146,210,u71_size,220,0);button(x+225,y+146,170,"Use all free",221,0);
  if(u71_state.scheme==D71_GPT){label(x,y+210,"Partition name",1,muted);button(x,y+240,cw-30,u71_name,222,0);}
  ui73_paragraph(x,y+304,cw,2,"Creates an unformatted data partition. Existing partitions stay unchanged. Format the new partition afterward.",muted);button(x,y+382,230,"Review creation",223,1);
 }else if(u71_mode==3){ui73_paragraph(x,y+120,cw,2,"Formatting removes files from this partition. Other partitions are kept.",warn);button(x,y+188,330,"FAT32 (64 MiB minimum)",224,0);button(x,y+246,330,"Engine data (1 MiB minimum)",225,0);ui73_paragraph(x,y+320,cw,2,"Quick format does not securely erase old contents. Resizing and conversion are not available.",muted);
 }else if(u71_mode==5){ui73_paragraph(x,y+120,cw,3,"Choose the verified table copy to keep. Choosing the wrong copy can hide partitions. Filesystem data is not written.",warn);label(x,y+210,(u71_state.valid_copies&1)?"Primary: verified":"Primary: invalid",1,muted);label(x+285,y+210,(u71_state.valid_copies&2)?"Backup: verified":"Backup: invalid",1,muted);button(x,y+266,240,"Use primary GPT",229,0);button(x+255,y+266,240,"Use backup GPT",230,0);
 }else{ui73_paragraph(x,y+116,cw,2,u71_title(u71_op),warn);
  if(u71_op==D71_ERASE)ui73_paragraph(x,y+176,cw,2,"All partitions on this disk will become inaccessible.",warn);
  else if(u71_op==D71_DELETE||u71_op==D71_FAT32||u71_op==D71_NATIVE){textnum(x,y+176,u71_plan.index+1," - partition number");ui73_paragraph(x,y+218,cw,2,"Only this partition changes. Its existing files will be lost.",warn);}
  else if(u71_op==D71_CREATE){ui73_fit(x,y+176,cw,u71_plan.name,1,0xffffff);textnum(x,y+218,(u71_plan.last-u71_plan.first+1)/2048," MiB new partition");}
  else ui73_paragraph(x,y+176,cw,2,"Check the disk identity above. Review the operation before applying changes.",muted);
  ui73_paragraph(x,y+270,cw,2,"Quick operations do not securely erase old file contents.",muted);button(x,y+330,440,u71_consent?"[X] I understand these disk changes":"[ ] I understand these disk changes",226,0);button(x,y+386,160,"Keep / Cancel",227,1);button(x+175,y+386,200,"Apply changes",228,0);
 }
 if(u71_notice)ui73_paragraph(x,y+442,cw,2,u71_notice,warn);
 button(x,bottom+14,185,u71_mode==1?"Back to setup":"Back",200,0);
}
