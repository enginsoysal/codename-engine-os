/* Provider checks and cooperative jobs. UI never treats a queued action as success. */
static desktop_window *file_owner_window11(uint64_t token){desktop_window*w=desktop_lookup(&model,token);return w&&w->app==FILES?w:0;}
static void file_notice11(uint64_t token,const char*message){desktop_window*w=file_owner_window11(token);if(w){notice(&apps[w-model.windows],message);invalidate(w->bounds);}else{copy(shell_notice,message,sizeof(shell_notice));task_damage();invalidate((desktop_rect){0,(int)model.height-108,(int)model.width,48});}}
static const char *file_error_text11(unsigned code){return code==V6_FULL?"Not enough free space or file slots. Completed items remain available.":code==V6_RO?"This disk is read-only. Use recovery to check it.":code==V6_IO?"The disk could not confirm the change. Writes are stopped; use recovery.":code==V6_EXISTS?"That name is already in use. Choose Keep both or Skip.":code==V6_LIMIT?"This operation exceeds a supported file, path or selection limit.":code==V6_NOTFOUND?"The item is no longer available. Refresh the folder and try again.":"The source changed or this operation is not supported. Check the selected items.";}
static void file_invalidate11(void){for(unsigned i=0;i<DESKTOP_WINDOWS;++i)if(model.windows[i].live&&model.windows[i].app==FILES)invalidate(model.windows[i].bounds);whole();}
static void file_clear_selections11(void){memset(selected11,0,sizeof(selected11));memset(desktop_files_selected11,0,sizeof(desktop_files_selected11));desktop_selection11=0;interaction11.last_valid=0;}
static void file_dialog_open11(unsigned mode,uint64_t owner_token,const char*path){memset(&file_dialog_state11,0,sizeof(file_dialog_state11));file_dialog_state11.mode=mode;file_dialog_state11.owner=owner_token;if(path)copy(file_dialog_state11.path,path,256);if(owner_token){desktop_focus(&model,owner_token);desktop_focus11=0;}else desktop_focus11=1;control_active=0;context11=0;whole();}
static unsigned file_user_item11(const char*path){char home[256];home72(home);return writable72(path)&&!equal(path,home);}
static unsigned file_open_under11(const char*path){
 if(transfer_owner74||w8_pending())return 1;
 for(unsigned i=0;i<DESKTOP_WINDOWS;++i)if(model.windows[i].live&&model.windows[i].app!=FILES&&apps[i].path[0]&&file_within11(apps[i].path,path))return 1;
 return 0;
}
static const v6_entry *file_case_entry11(const char*path){const v6_entry*exact=entry(path);if(exact)return exact;for(unsigned i=0;i<V6_ENTRIES;++i){const v6_entry*e=&m6_volume()->entries[i];if(e->kind&&!file_compare_text11(e->path,path))return e;}return 0;}
static unsigned file_join11(char*out,const char*folder,const char*name){if(length(folder)+length(name)+2>256)return V6_LIMIT;copy(out,folder,256);if(!equal(folder,"/"))append(out,"/",256);append(out,name,256);return V6_OK;}
static void file_parent11(char*out,const char*path){copy(out,path,256);unsigned n=length(out);while(n>1&&out[n-1]!='/')--n;if(n>1)out[n-1]=0;else copy(out,"/",256);}
static unsigned file_name_valid11(const char*name){unsigned n=length(name);if(!n||n>63||equal(name,".")||equal(name,"..")||name[n-1]==' '||name[n-1]=='.')return 0;for(unsigned i=0;i<n;++i){unsigned c=(uint8_t)name[i];if(c<32||c>126||c=='/'||c=='\\'||c==':'||c=='*'||c=='?'||c=='"'||c=='<'||c=='>'||c=='|')return 0;}return 1;}
static unsigned file_make_directories11(const char*path){char prefix[256];unsigned n=length(path);for(unsigned i=1;i<=n;++i)if(!path[i]||path[i]=='/'){memcpy(prefix,path,i);prefix[i]=0;const v6_entry*e=entry(prefix);if(e){if(e->kind!=2)return V6_INVALID;}else{unsigned status=v6_mkdir(m6_volume(),prefix);if(status)return status;}}return V6_OK;}
static unsigned file_bin_ready11(void){char path[256];file_bin_path11(path,0);unsigned status=file_make_directories11(path);if(status)return status;file_bin_path11(path,1);return file_make_directories11(path);}
static unsigned file_nul11(const char*p){for(unsigned i=0;i<256;++i)if(!p[i])return 1;return 0;}
static unsigned file_trash_record11(const char*payload,recycle_record11*record,char*metadata){char bin[256];file_bin_path11(bin,0);if(!file_direct_child11(payload,bin))return V6_INVALID;file_bin_path11(metadata,1);append(metadata,"/",256);append(metadata,file_basename11(payload),256);append(metadata,".bin",256);unsigned bytes=0;const v6_entry*e=entry(metadata);if(!e||e->size!=sizeof(*record)||v6_verify(m6_volume(),metadata)||v6_read(m6_volume(),metadata,0,record,sizeof(*record),&bytes)||bytes!=sizeof(*record))return V6_INVALID;
 if(record->magic!=UINT64_C(0x314e494243455246)||record->version!=1||record->reserved||(record->kind!=1&&record->kind!=2)||!file_nul11(record->original)||!file_user_item11(record->original))return V6_INVALID;return V6_OK;}
static void file_refresh_trash_names11(void){v6_volume*v=m6_volume();if(trash_names_generation11==v->generation)return;memset(trash_names11,0,sizeof(trash_names11));char bin[256];file_bin_path11(bin,0);for(unsigned i=0;i<V6_ENTRIES;++i){const v6_entry*e=&v->entries[i];if(!e->kind||!file_direct_child11(e->path,bin))continue;recycle_record11 record;char metadata[256];if(!file_trash_record11(e->path,&record,metadata))copy(trash_names11[i],file_basename11(record.original),64);}trash_names_generation11=v->generation;}
static const char *file_label11(const v6_entry*e){unsigned i=(unsigned)(e-m6_volume()->entries);char bin[256];file_bin_path11(bin,0);if(file_direct_child11(e->path,bin)){file_refresh_trash_names11();if(i<V6_ENTRIES&&trash_names11[i][0])return trash_names11[i];}return file_basename11(e->path);}
static uint64_t file_tree_signature11(const char*path){uint64_t value=UINT64_C(0x6a09e667f3bcc909);unsigned count=0;for(unsigned i=0;i<V6_ENTRIES;++i){const v6_entry*e=&m6_volume()->entries[i];if(!e->kind||!file_within11(e->path,path))continue;value^=file_path_hash11(e->path)^e->first^(e->size*UINT64_C(1099511628211))^((uint64_t)e->crc<<16)^e->kind;++count;}return value^((uint64_t)count<<48);}
static void file_set_undo11(unsigned kind,const char*source,const char*target){file_undo_state11.kind=kind;copy(file_undo_state11.source,source?source:"",256);copy(file_undo_state11.target,target,256);file_undo_state11.generation=file_tree_signature11(target);}
static void file_created_folder11(const char*path){file_set_undo11(7,0,path);file_clear_selections11();file_invalidate11();}
static void file_follow_move11(const char*source,const char*target){unsigned n=length(source);for(unsigned i=0;i<DESKTOP_WINDOWS;++i)if(model.windows[i].live&&model.windows[i].app==FILES&&file_within11(apps[i].path,source)){char changed[256];copy(changed,target,256);append(changed,apps[i].path+n,256);copy(apps[i].path,changed,256);apps[i].offset=0;file_visit11(&apps[i]);}}
static unsigned file_selected_desktop11(unsigned row){const v6_entry*e=desktop_child11(row);unsigned i=e?(unsigned)(e-m6_volume()->entries):V6_ENTRIES;return i<V6_ENTRIES&&!!(desktop_files_selected11[i/8]&(1u<<(i%8)));}
static unsigned file_collect11(uint64_t owner_token,file_root11*out,unsigned*count){
 *count=0;desktop_window*w=file_owner_window11(owner_token);application*a=w?&apps[w-model.windows]:0;
 if(a&&a->usb_mode10){if(a->usb_mode10!=2||file_selected_count11(a)!=1||!a->usb_selected10)return V6_INVALID;unsigned total;if(usb_listing10(a,a->usb_selected10-1,1,&total)||a->usb_selected10>total||usb_rows10[0].kind!=1)return V6_INVALID;memset(out,0,sizeof(*out));out->provider=F11_USB;out->disk=a->usb_disk10;out->context=a->usb_context10;out->part=a->usb_part10;out->kind=1;out->size=usb_rows10[0].size;unsigned status=file_join11(out->source,a->usb_path10,usb_rows10[0].name);if(status)return status;*count=1;return V6_OK;}
 if(!w&&owner_token)return V6_INVALID;if(!w&&desktop_selection11)return V6_INVALID;
 unsigned total=a?children72(a):desktop_children11();
 for(unsigned row=0;row<total;++row){if(!(a?selected_row11(a,row):file_selected_desktop11(row)))continue;const v6_entry*e=a?child_at(a,row):desktop_child11(row);if(!e)continue;unsigned covered=0;
  for(unsigned other=0;other<total&&!covered;++other){if(other==row||!(a?selected_row11(a,other):file_selected_desktop11(other)))continue;const v6_entry*parent=a?child_at(a,other):desktop_child11(other);if(parent&&parent->kind==2&&file_within11(e->path,parent->path))covered=1;}
  if(covered)continue;if(*count==F11_ROOTS)return V6_LIMIT;file_root11*r=&out[(*count)++];memset(r,0,sizeof(*r));copy(r->source,e->path,256);r->kind=e->kind;r->size=e->size;r->crc=e->crc;
 }
 return *count?V6_OK:V6_NOTFOUND;
}
static unsigned file_destination11(uint64_t owner_token,char*out){desktop_window*w=file_owner_window11(owner_token);if(w){application*a=&apps[w-model.windows];if(a->usb_mode10)return V6_INVALID;copy(out,a->path,256);}else root_path(out,"Desktop");return writable72(out)&&!file_in_bin11(out)?V6_OK:V6_RO;}
static unsigned file_unique11(char*out,const char*requested,unsigned root){char folder[256],name[64],stem[64],extension[64];file_parent11(folder,requested);copy(name,file_basename11(requested),64);const char*dot=0;for(unsigned i=1;name[i];++i)if(name[i]=='.')dot=name+i;unsigned base=dot?(unsigned)(dot-name):length(name);copy(extension,dot?dot:"",64);if(length(extension)>20)extension[0]=0;copy(stem,name,base+1);
 for(unsigned candidate=2;candidate<10000;++candidate){char number[16],suffix[24],changed[64];decimal(number,candidate);copy(suffix," (",24);append(suffix,number,24);append(suffix,")",24);unsigned available=63-length(extension)-length(suffix);copy(changed,stem,available+1);append(changed,suffix,64);append(changed,extension,64);if(file_join11(out,folder,changed))return V6_LIMIT;if(file_case_entry11(out))continue;unsigned used=0;for(unsigned i=0;i<file_job_state11.count;++i)if(i!=root&&!file_job_state11.roots[i].skip&&!file_compare_text11(out,file_job_state11.roots[i].target))used=1;if(!used)return V6_OK;}return V6_FULL;
}
static unsigned file_job_nodes11(void){file_job11*j=&file_job_state11;v6_volume*v=m6_volume();j->node_count=0;j->total_bytes=0;
 for(unsigned root=0;root<j->count;++root){file_root11*r=&j->roots[root];if(r->skip)continue;if(r->provider==F11_USB){if(r->kind!=1||r->size>64u*1024*1024)return V6_LIMIT;j->total_bytes+=r->size*2;continue;}
  if(j->mode!=F11_JOB_COPY)continue;
  for(unsigned index=0;index<V6_ENTRIES;++index){const v6_entry*e=&v->entries[index];if(!e->kind||!file_within11(e->path,r->source))continue;if(j->node_count==V6_ENTRIES)return V6_LIMIT;
   char target[256];copy(target,r->target,256);if(length(target)+length(e->path)-length(r->source)>=256)return V6_LIMIT;append(target,e->path+length(r->source),256);
   if(e->kind==1&&e->size>64u*1024*1024)return V6_LIMIT;unsigned depth=0;for(unsigned n=length(r->source);e->path[n];++n)depth+=e->path[n]=='/';
   j->nodes[j->node_count++]=(file_node11){e->first,e->size,file_path_hash11(e->path),e->crc,(uint16_t)index,(uint16_t)root,(uint16_t)depth,(uint16_t)e->kind};if(e->kind==1)j->total_bytes+=e->size*2;
  }
 }
 /* At most 1024 short depth comparisons per insertion; no data buffers are copied. */
 for(unsigned i=1;i<j->node_count;++i){file_node11 node=j->nodes[i];unsigned k=i;while(k&&j->nodes[k-1].depth>node.depth){j->nodes[k]=j->nodes[k-1];--k;}j->nodes[k]=node;}
 return V6_OK;
}
static void file_job_finish11(unsigned status,unsigned cancelled){file_job11*j=&file_job_state11;v6_stream_abort(m6_volume());j->active=j->waiting=0;
 file_job_error11=status;char confirmed11[24];decimal(confirmed11,j->mode==F11_JOB_COPY?j->files+j->folders:j->completed);copy(file_job_result11,"Confirmed completed items: ",256);append(file_job_result11,confirmed11,256);append(file_job_result11,". ",256);append(file_job_result11,status?file_error_text11(status):cancelled?"Cancelled. Completed items remain. Unfinished copy sources were kept.":"The operation finished. Completed data is saved on disk.",256);
 if(!status&&!cancelled&&j->count==1&&!j->roots[0].skip&&!j->roots[0].replace&&(j->mode==F11_JOB_COPY||j->mode==F11_JOB_MOVE||j->mode==F11_JOB_RECYCLE||j->mode==F11_JOB_LINK))file_set_undo11(j->mode,j->roots[0].source,j->roots[0].target);
 if(j->mode==F11_JOB_MOVE||j->mode==F11_JOB_RESTORE||j->mode==F11_JOB_RECYCLE){file_clip11.count=0;if(clipboard_kind11==2)clipboard_kind11=0;}
 if(status)file_notice11(j->owner,"File operation stopped. Open Details for completed items and the reason.");else{char result[96],n[24];decimal(n,j->mode==F11_JOB_COPY?j->files:j->completed);copy(result,cancelled?"Cancelled. Completed ":"Completed ",96);append(result,n,96);append(result,j->mode==F11_JOB_COPY?" files":" items",96);if(j->mode==F11_JOB_COPY){decimal(n,j->folders);append(result,", ",96);append(result,n,96);append(result," folders",96);}if(j->skipped){decimal(n,j->skipped);append(result,"; skipped ",96);append(result,n,96);}append(result,". Other files are unchanged.",96);file_notice11(j->owner,result);}
 file_clear_selections11();file_invalidate11();serial_text("M11_FILE_JOB_END status=");serial_hex(status);serial_text(" cancelled=");serial_hex(cancelled);serial_text("\n");
}
static unsigned file_job_start11(unsigned mode,uint64_t owner_token,const file_root11*roots,unsigned count,const char*folder){
 if(file_job_state11.active||file_job_state11.waiting||file_dialog_state11.mode)return V6_LIMIT;if(!count||count>F11_ROOTS)return V6_LIMIT;
 file_job11*j=&file_job_state11;memset(j,0,sizeof(*j));j->mode=mode;j->owner=owner_token;j->count=count;j->replace_allowed=1;memcpy(j->roots,roots,count*sizeof(*roots));
 if(m6_volume()->readonly)return V6_RO;
 if(mode==F11_JOB_RECYCLE){unsigned status=file_bin_ready11();if(status)return status;}
 for(unsigned i=0;i<count;++i){file_root11*r=&j->roots[i];const v6_entry*source=r->provider==F11_NATIVE?entry(r->source):0;
  if(r->provider==F11_NATIVE&&!source)return V6_NOTFOUND;
  if((mode==F11_JOB_MOVE||mode==F11_JOB_RECYCLE)&&(!file_user_item11(r->source)||file_open_under11(r->source)))return V6_INVALID;
  if(r->provider==F11_USB&&mode!=F11_JOB_COPY)return V6_INVALID;
  if(folder){const char*name=source?file_label11(source):file_basename11(r->source);unsigned status=file_join11(r->target,folder,name);if(status)return status;}
  if(mode==F11_JOB_RECYCLE){char bin[256],id[32],number[24];file_bin_path11(bin,0);unsigned available=0;for(unsigned attempt=0;attempt<V6_ENTRIES&&!available;++attempt){decimal(number,m6_volume()->generation+i+attempt);copy(id,"R",32);append(id,number,32);unsigned status=file_join11(r->target,bin,id);if(status)return status;available=entry(r->target)==0;for(unsigned k=0;k<i;++k)if(equal(r->target,j->roots[k].target))available=0;}if(!available)return V6_FULL;}
  else if(mode!=F11_JOB_REMOVE){char parent[256];file_parent11(parent,r->target);const v6_entry*dir=entry(parent);if(!dir||dir->kind!=2||!writable72(parent))return V6_INVALID;}
  if(source&&source->kind==2&&file_within11(r->target,r->source)&&(mode==F11_JOB_COPY||mode==F11_JOB_MOVE))return V6_INVALID;
  if(mode==F11_JOB_MOVE&&equal(r->source,r->target)){r->skip=1;++j->skipped;continue;}
  if(mode==F11_JOB_RECYCLE||mode==F11_JOB_REMOVE)continue;
  const v6_entry*existing=file_case_entry11(r->target);if(mode==F11_JOB_MOVE&&existing==source)continue;unsigned duplicate=0;for(unsigned k=0;k<i;++k)if(!file_compare_text11(r->target,j->roots[k].target))duplicate=1;
  if(existing||duplicate){++j->conflicts;if(!existing||existing->kind!=1||r->kind!=1||r->provider!=F11_NATIVE||equal(r->source,existing->path)||duplicate||file_open_under11(existing->path))j->replace_allowed=0;if(existing)copy(r->target,existing->path,256);}
 }
 if(j->conflicts){j->waiting=1;file_dialog_open11(F11_DLG_CONFLICT,owner_token,0);return V6_OK;}
 unsigned status=file_job_nodes11();if(status)return status;j->active=1;file_job_error11=0;file_undo_state11.kind=0;file_notice11(owner_token,"Working on your files. You can keep using other windows.");return V6_OK;
}
static unsigned file_job_policy11(unsigned policy){file_job11*j=&file_job_state11;if(!j->waiting)return V6_INVALID;if(policy==F11_REPLACE&&!j->replace_allowed)return V6_INVALID;
 for(unsigned i=0;i<j->count;++i){file_root11*r=&j->roots[i];if(r->skip)continue;unsigned conflict=file_case_entry11(r->target)!=0;for(unsigned k=0;k<i;++k)if(!j->roots[k].skip&&!file_compare_text11(r->target,j->roots[k].target))conflict=1;if(!conflict)continue;
  if(policy==F11_SKIP){r->skip=1;++j->skipped;}else if(policy==F11_KEEP_BOTH){char changed[256];unsigned status=file_unique11(changed,r->target,i);if(status)return status;copy(r->target,changed,256);}else r->replace=1;
 }
 unsigned status=file_job_nodes11();if(status)return status;j->waiting=0;j->active=1;file_job_error11=0;file_dialog_state11.mode=0;file_undo_state11.kind=0;whole();return V6_OK;
}
static void file_job_progress11(void){uint64_t now=m3_clock_ticks();if(now-file_job_state11.last_paint>=5){file_job_state11.last_paint=now;desktop_window*w=file_owner_window11(file_job_state11.owner);if(w)invalidate(w->bounds);else invalidate((desktop_rect){10,(int)model.height-136,(int)model.width-20,74});}}
static void files_pump11(void){file_job11*j=&file_job_state11;if(!j->active)return;v6_volume*v=m6_volume();if(j->cancel){file_job_finish11(0,1);return;}unsigned status=0;
 if(j->mode!=F11_JOB_COPY){while(j->root<j->count&&j->roots[j->root].skip)++j->root;if(j->root==j->count){file_job_finish11(0,0);return;}file_root11*r=&j->roots[j->root];
  if(j->mode==F11_JOB_RECYCLE){const v6_entry*e=entry(r->source);if(!e){file_job_finish11(V6_NOTFOUND,0);return;}recycle_record11 record={.magic=UINT64_C(0x314e494243455246),.version=1,.kind=e->kind,.size=e->size,.crc=e->crc};copy(record.original,r->source,256);char meta[256];file_bin_path11(meta,1);append(meta,"/",256);append(meta,file_basename11(r->target),256);append(meta,".bin",256);status=v6_put(v,meta,sizeof(record),memory_source,&record);if(!status)status=v6_move_tree11(v,r->source,r->target,0);
  }else if(j->mode==F11_JOB_REMOVE)status=v6_remove_tree11(v,r->source);
  else if(j->mode==F11_JOB_LINK){shortcut_record11 record={.magic=UINT64_C(0x314b4e494c313145),.version=1,.kind=r->provider==F11_BUILTIN?3:r->kind==2?2:1,.app=r->provider==F11_BUILTIN?r->disk:0};copy(record.target,r->source,256);status=v6_put(v,r->target,sizeof(record),memory_source,&record);
  }else status=v6_move_tree11(v,r->source,r->target,r->replace);
  if(status){file_job_finish11(status,0);return;}
  if(j->mode==F11_JOB_MOVE||j->mode==F11_JOB_RESTORE||j->mode==F11_JOB_RECYCLE)file_follow_move11(r->source,r->target);
  if(j->mode==F11_JOB_RESTORE||j->mode==F11_JOB_REMOVE){recycle_record11 record;char meta[256];if(!file_trash_record11(r->source,&record,meta))status=v6_remove(v,meta);}
  ++j->completed;++j->root;if(status){file_job_finish11(status,0);return;}file_job_progress11();return;
 }
 if(j->phase==0){
  /* FAT32 retains its separately bounded, one-file create-only transfer. */
  while(j->root<j->count){file_root11*r=&j->roots[j->root++];if(r->skip||r->provider!=F11_USB)continue;status=rem10_import(r->disk,r->context,r->part,r->source,r->target);if(status){file_job_finish11(status==B_ABSENT?V6_NOTFOUND:V6_IO,0);return;}++j->files;j->done_bytes+=r->size*2;file_job_progress11();return;}
  if(j->node==j->node_count){file_job_finish11(0,0);return;}file_node11*n=&j->nodes[j->node];const v6_entry*e=&v->entries[n->index];file_root11*r=&j->roots[n->root];
  if(e->kind!=n->kind||e->first!=n->first||e->size!=n->size||e->crc!=n->crc||file_path_hash11(e->path)!=n->path_hash||!file_within11(e->path,r->source)){file_job_finish11(V6_INVALID,0);return;}
  copy(j->source,e->path,256);copy(j->target,r->target,256);append(j->target,e->path+length(r->source),256);j->source_first=e->first;j->source_size=e->size;j->source_crc=e->crc;j->offset=j->verified=0;j->staged[0]=0;
  if(e->kind==2){status=v6_mkdir(v,j->target);if(status){file_job_finish11(status,0);return;}++j->folders;++j->node;file_job_progress11();return;}
  if(r->replace){char stage[256],number[24];copy(stage,"/System/FileTransfers/",256);append(stage,owner.user,256);status=file_make_directories11(stage);decimal(number,v->generation);if(!status)status=file_join11(j->staged,stage,number);if(status){file_job_finish11(status,0);return;}}
  const char*target=j->staged[0]?j->staged:j->target;
  if(!e->size){status=v6_put(v,target,0,0,0);if(!status&&j->staged[0])status=v6_rename(v,j->staged,j->target,1);if(status){file_job_finish11(status,0);return;}++j->files;++j->node;file_job_progress11();return;}
  status=v6_stream_begin(v,target,e->size);if(status){file_job_finish11(status,0);return;}j->phase=1;
 }
 if(j->phase==1){const v6_entry*e=entry(j->source);if(!e||e->kind!=1||e->first!=j->source_first||e->size!=j->source_size||e->crc!=j->source_crc){file_job_finish11(V6_INVALID,0);return;}
  unsigned bytes=(unsigned)(j->source_size-j->offset>4096?4096:j->source_size-j->offset),got=0;status=v6_read(v,j->source,j->offset,file_buffer11,bytes,&got);if(!status&&got!=bytes)status=V6_IO;if(!status)status=v6_stream_append(v,file_buffer11,bytes);if(status){file_job_finish11(status,0);return;}j->offset+=bytes;j->done_bytes+=bytes;
  if(j->offset==j->source_size){status=v6_stream_seal11(v);if(status){file_job_finish11(status,0);return;}j->phase=2;}
 }else if(j->phase==2){const v6_entry*e=entry(j->source);if(!e||e->kind!=1||e->first!=j->source_first||e->size!=j->source_size||e->crc!=j->source_crc){file_job_finish11(V6_INVALID,0);return;}unsigned done=0;status=v6_stream_check11(v,&done);if(status){file_job_finish11(status,0);return;}unsigned bytes=(unsigned)(j->source_size-j->verified>4096?4096:j->source_size-j->verified);j->verified+=bytes;j->done_bytes+=bytes;
  if(done){status=v6_stream_finish_expected11(v,j->source_size,j->source_crc);if(!status&&j->staged[0])status=v6_rename(v,j->staged,j->target,1);if(status){file_job_finish11(status,0);return;}++j->files;++j->node;j->phase=0;}
 }
 file_job_progress11();
}
static unsigned files_pending11(void){return file_job_state11.active;}
static void files_poll11(void){uint64_t generation=m6_volume()->generation;if(generation==files_observed_generation11)return;files_observed_generation11=generation;trash_names_generation11=0;for(unsigned i=0;i<DESKTOP_WINDOWS;++i)if(model.windows[i].live&&model.windows[i].app==FILES)invalidate(model.windows[i].bounds);invalidate((desktop_rect){20,100,(int)model.width-40,(int)model.height-170});}
