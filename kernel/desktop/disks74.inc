/* Desktop frontends for protected disk operations and verified boot recovery. */
enum { DISK_NEXT74=12100,DISK_PREV_PAGE74,DISK_NEXT_PAGE74,DISK_CREATE74,DISK_FORMAT74,DISK_DELETE74,
       DISK_INITIALIZE74,DISK_ERASE74,DISK_CANCEL74,DISK_CONSENT74,DISK_APPLY74,
       DISK_SIZE_LESS74,DISK_SIZE_MORE74,DISK_SIZE_ALL74,DISK_NAME74,DISK_PREPARE74,
       DISK_FAT74,DISK_NATIVE74,RECOVERY_REPAIR74,RECOVERY_ROLLBACK74,RECOVERY_GUIDE74,
       DISK_ROW74=12200 };
static const char *disk_op74(unsigned op){
    static const char *names[]={"","Erase disk","Initialize GPT","Create partition","Delete partition","Format FAT32","Format Engine","Repair GPT","Repair GPT"};
    return op<9?names[op]:"Disk operation";
}
static void disk_select74(application *app,unsigned disk){
    app->number=disk;app->operation=UINT32_MAX;app->offset=app->day74=app->entered=0;
    unsigned status=disk74_select(disk);notice(app,status?"This disk is unavailable.":disk==system_disk74()?"The running system disk is protected. Use installation media to change its partitions.":"");
}
static void disk_review74(application *app,unsigned op){
    const d71_disk *state=disk74_state();unsigned row=app->operation,index=0;
    if(op==D71_CREATE){if(row<state->count||row>=state->count+state->gaps){notice(app,"Choose an unallocated space row first.");return;}index=row-state->count;}
    else if(op==D71_DELETE||op==D71_FAT32||op==D71_NATIVE){if(row>=state->count){notice(app,"Choose a partition first.");return;}index=state->parts[row].index;}
    unsigned status=disk74_prepare(op,index,(uint64_t)app->operand,app->pending);
    if(status){notice(app,d71_error(status));return;}
    app->year74=op;app->closing=6;app->day74=0;app->entered=0;notice(app,"");
}
static void disks_init74(unsigned kind,application *app){
    unsigned slot=(unsigned)(app-apps);uint64_t token=desktop_token(&model,slot);
    if(kind==DISKS74){desktop_place(&model,token,(desktop_rect){((int)model.width-850)/2,45,850,620});disk_select74(app,0);}
}
static void disks_draw74(desktop_window *w,application *app,uint64_t token){
    int x=w->bounds.x+24,y=w->bounds.y+62,width=w->bounds.width-48;
    if(w->app==RECOVERY74){
        if(app->closing==6){text(x,y,app->year74==1?"Repair boot files?":"Return to the previous system?",2,ink());
            ui73_paragraph(x,y+55,width,5,"" ENGINE_PRODUCT_NAME " will verify a complete recovery slot before replaying its boot files. Your documents and the existing partition table are kept. Keep the disk connected. Restart afterwards to use the selected system.",ink());
            button(token,x,y+220,220,app->entered?"[X] I understand":"[ ] I understand",DISK_CONSENT74,0);
            button(token,x,y+275,140,"Cancel",DISK_CANCEL74,0);button(token,x+155,y+275,170,"Apply recovery",DISK_APPLY74,1);
        }else{
            text(x,y,"Keep " ENGINE_PRODUCT_NAME " recoverable",2,ink());
            ui73_paragraph(x,y+48,width,4,"Repair restores verified boot files for this installation. Rollback needs a complete previous recovery slot. Keep your installation ISO available for offline recovery and system updates.",ink());
            button(token,x,y+175,210,"Repair boot files",RECOVERY_REPAIR74,0);button(token,x+225,y+175,225,"Previous system",RECOVERY_ROLLBACK74,0);
            button(token,x,y+235,230,"Read recovery guidance",RECOVERY_GUIDE74,0);
        }
        ui73_paragraph(x,w->bounds.y+w->bounds.height-90,width,3,app->notice,ink());return;
    }
    if(w->app!=DISKS74)return;
    const d71_disk *state=disk74_state();char value[128],part[24];decimal(value,(uint64_t)app->number+1);copy(part,value,24);copy(value,"Disk ",128);append(value,part,128);
    append(value,"  -  ",128);decimal(part,state->sectors/2048);append(value,part,128);append(value," MiB",128);
    text(x,y,value,2,ink());
    if(app->closing==6){
        const d71_plan *plan=disk74_plan();text(x,y+52,disk_op74(plan->operation),2,ink());
        copy(value,"Sectors ",128);decimal(part,plan->first);append(value,part,128);append(value," to ",128);decimal(part,plan->last);append(value,part,128);
        if(plan->operation==D71_ERASE)copy(value,"All partitions and data on the selected disk",128);
        text(x,y+95,value,1,ink());
        ui73_paragraph(x,y+135,width,4,"This changes the selected disk. Formatting or deleting loses data in the selected partition; erasing affects the whole disk. Other disks are not changed. Review the disk and operation before continuing.",ink());
        button(token,x,y+255,285,app->entered?"[X] Change this disk":"[ ] Change this disk",DISK_CONSENT74,0);
        button(token,x,y+315,140,"Cancel",DISK_CANCEL74,1);button(token,x+155,y+315,170,"Apply changes",DISK_APPLY74,0);
    }else if(app->day74){
        text(x,y+55,app->day74==1?"Create a partition":"Choose the filesystem",2,ink());
        if(app->day74==1){
            decimal(value,(uint64_t)app->operand);append(value," MiB",128);text(x,y+105,value,2,ink());
            button(token,x,y+145,110,"-1 MiB",DISK_SIZE_LESS74,0);button(token,x+125,y+145,110,"+1 MiB",DISK_SIZE_MORE74,0);button(token,x+250,y+145,140,"Use all space",DISK_SIZE_ALL74,0);
            text(x,y+210,"Partition name - click to edit",1,ink());button(token,x,y+240,390,app->pending,DISK_NAME74,app->entered);
            button(token,x,y+315,160,"Review",DISK_PREPARE74,1);
        }else{
            button(token,x,y+110,200,"FAT32",DISK_FAT74,0);button(token,x+215,y+110,210,"Engine filesystem",DISK_NATIVE74,0);
            ui73_paragraph(x,y+175,width,3,"Quick format initializes filesystem metadata. It does not securely erase every old data sector. Formatting requires a separate review and confirmation.",ink());
        }
        button(token,x+180,y+315,140,"Cancel",DISK_CANCEL74,0);
    }else{
        button(token,x+width-145,y-6,145,"Next disk",DISK_NEXT74,0);
        text(x,y+44,app->number==system_disk74()?"Running system disk - inspection only":state->editable?"Select a partition or unallocated space":state->scheme==D71_BLANK?"Blank disk - initialize GPT first":"Unsupported or damaged layout - inspection only",1,ink());
        unsigned rows=5,total=state->count+state->gaps;
        for(unsigned i=0;i<rows&&app->offset+i<total;++i){unsigned row=app->offset+i;value[0]=0;
            if(row<state->count){const d71_partition *p=&state->parts[row];copy(value,p->name[0]?p->name:"Partition",128);append(value," - ",128);append(value,p->format,128);
                decimal(part,(p->last-p->first+1)/2048);append(value," - ",128);append(value,part,128);append(value," MiB",128);if(p->protected)append(value," - protected",128);
            }else{d71_gap gap=state->free[row-state->count];decimal(part,(gap.last-gap.first+1)/2048);copy(value,"Unallocated - ",128);append(value,part,128);append(value," MiB",128);}
            button(token,x,y+85+(int)i*45,width,value,DISK_ROW74+row,app->operation==row);
        }
        button(token,x,y+325,105,"Previous",DISK_PREV_PAGE74,0);button(token,x+115,y+325,95,"Next",DISK_NEXT_PAGE74,0);
        button(token,x,y+380,165,"Create partition",DISK_CREATE74,0);button(token,x+180,y+380,150,"Format",DISK_FORMAT74,0);button(token,x+345,y+380,165,"Delete partition",DISK_DELETE74,0);
        button(token,x,y+430,165,"Initialize GPT",DISK_INITIALIZE74,0);button(token,x+180,y+430,150,"Erase disk",DISK_ERASE74,0);
    }
    ui73_paragraph(x,w->bounds.y+w->bounds.height-65,width,2,app->notice,ink());
}
static unsigned disks_action74(desktop_window *w,application *app,uint64_t token,unsigned command){
    (void)token;
    if((w->app!=DISKS74&&w->app!=RECOVERY74)||command<12100||command>=12500)return 0;
    if(command==DISK_CANCEL74){app->closing=app->day74=app->entered=0;notice(app,"Cancelled. Nothing was written.");}
    else if(command==DISK_CONSENT74&&app->closing==6)app->entered^=1;
    else if(command==DISK_APPLY74&&app->closing==6){
        if(!app->entered)notice(app,"Confirm the checkbox first, or choose Cancel.");
        else{unsigned status=w->app==DISKS74?disk74_apply():recovery74_apply();app->closing=app->day74=app->entered=0;
            notice(app,status?(w->app==DISKS74?d71_error(status):"Recovery could not finish. Keep the ISO for offline repair."):
                w->app==DISKS74?"Disk operation completed. Your other disks are unchanged.":"Verified boot files restored. Restart to use this system.");}
    }else if(w->app==RECOVERY74){
        if(command==RECOVERY_GUIDE74){launch(HELP74,0);whole();return 1;}
        if(command==RECOVERY_REPAIR74||command==RECOVERY_ROLLBACK74){unsigned mode=command==RECOVERY_REPAIR74?1:3,status=recovery74_prepare(mode);
            if(status)notice(app,mode==3?"No complete previous recovery slot is available. Nothing was changed.":"The current recovery slot could not be verified. Use your installation ISO.");
            else{app->year74=mode;app->closing=6;app->entered=0;notice(app,"");}}
    }else if(command>=DISK_ROW74&&command<DISK_ROW74+257){app->operation=command-DISK_ROW74;notice(app,"");}
    else if(command==DISK_NEXT74){unsigned count=storage_count();if(count)disk_select74(app,((unsigned)app->number+1)%count);}
    else if(command==DISK_PREV_PAGE74){app->offset=app->offset>=5?app->offset-5:0;}
    else if(command==DISK_NEXT_PAGE74){const d71_disk *s=disk74_state();if(app->offset+5<s->count+s->gaps)app->offset+=5;}
    else if(command==DISK_CREATE74){const d71_disk *s=disk74_state();unsigned row=app->operation;
        if(row<s->count||row>=s->count+s->gaps)notice(app,"Select an unallocated space row first.");
        else{d71_gap gap=s->free[row-s->count];app->operand=(int64_t)((gap.last-gap.first+1)/2048);app->day74=1;app->entered=0;copy(app->pending,"Data",256);}}
    else if(command==DISK_FORMAT74){const d71_disk *s=disk74_state();if(app->operation>=s->count)notice(app,"Select a partition first.");else app->day74=2;}
    else if(command==DISK_DELETE74)disk_review74(app,D71_DELETE);
    else if(command==DISK_INITIALIZE74)disk_review74(app,D71_INITIALIZE);
    else if(command==DISK_ERASE74)disk_review74(app,D71_ERASE);
    else if(command==DISK_PREPARE74&&app->day74==1)disk_review74(app,D71_CREATE);
    else if(command==DISK_FAT74&&app->day74==2)disk_review74(app,D71_FAT32);
    else if(command==DISK_NATIVE74&&app->day74==2)disk_review74(app,D71_NATIVE);
    else if(command==DISK_SIZE_LESS74&&app->operand>1)--app->operand;
    else if(command==DISK_SIZE_MORE74&&app->operand<2097152)++app->operand;
    else if(command==DISK_SIZE_ALL74){const d71_disk *s=disk74_state();if(app->operation>=s->count&&app->operation<s->count+s->gaps){d71_gap g=s->free[app->operation-s->count];app->operand=(int64_t)((g.last-g.first+1)/2048);}}
    else if(command==DISK_NAME74)app->entered=1;
    invalidate(w->bounds);return 1;
}
static unsigned disks_key74(desktop_window *w,application *app,int key){
    if(w->app!=DISKS74||app->day74!=1||!app->entered)return 0;
    unsigned n=length(app->pending);if(key==8&&n)app->pending[n-1]=0;
    else if(key>=32&&key<=126&&n<36){app->pending[n]=(char)key;app->pending[n+1]=0;}
    else if(key==10)app->entered=0;else return 0;
    invalidate(w->bounds);return 1;
}
