static const char *file_basename11(const char*path){const char*name=path;for(unsigned n=0;path[n];++n)if(path[n]=='/')name=path+n+1;return name;}
static unsigned file_within11(const char*path,const char*root){unsigned n=0;while(root[n]&&path[n]==root[n])++n;return !root[n]&&(!path[n]||path[n]=='/');}
static unsigned lower11(unsigned c){return c>='A'&&c<='Z'?c+32:c;}
static int file_compare_text11(const char*a,const char*b){while(*a&&lower11((uint8_t)*a)==lower11((uint8_t)*b)){++a;++b;}unsigned x=lower11((uint8_t)*a),y=lower11((uint8_t)*b);return x<y?-1:x>y?1:0;}
static uint64_t file_path_hash11(const char*p){uint64_t value=UINT64_C(1469598103934665603);for(unsigned i=0;p[i]&&i<256;++i)value=(value^(uint8_t)p[i])*UINT64_C(1099511628211);return value;}
static void file_bin_path11(char*out,unsigned meta){copy(out,meta?"/System/RecycleMetadata/":"/System/Recycle/",256);append(out,owner.user,256);}
static unsigned file_is_bin11(const char*path){char bin[256];file_bin_path11(bin,0);return equal(path,bin);}
static unsigned file_in_bin11(const char*path){char bin[256];file_bin_path11(bin,0);return file_within11(path,bin);}
static unsigned file_direct_child11(const char*path,const char*folder){unsigned n=equal(folder,"/")?0:length(folder);if(memcmp(path,folder,n)||path[n]!='/')return 0;for(unsigned i=n+1;path[i];++i)if(path[i]=='/')return 0;return 1;}
static const char *file_kind11(const char*path){const char*name=file_basename11(path),*dot=0;for(unsigned i=0;name[i];++i)if(name[i]=='.')dot=name+i+1;return dot?dot:"";}
static int file_compare11(unsigned left,unsigned right,unsigned sort,unsigned descending){
 const v6_entry*a=&m6_volume()->entries[left],*b=&m6_volume()->entries[right];
 if(a->kind!=b->kind)return a->kind==2?-1:1;
 int result=0;if(sort==1&&a->size!=b->size)result=a->size<b->size?-1:1;
 else if(sort==2)result=file_compare_text11(file_kind11(a->path),file_kind11(b->path));
 if(!result)result=file_compare_text11(file_label11(a),file_label11(b));
 if(!result)result=left<right?-1:left>right?1:0;return descending?-result:result;
}
static void file_heap11(file_listing11*l,unsigned start,unsigned count){
 for(;;){unsigned child=start*2+1;if(child>=count)return;
  if(child+1<count&&file_compare11(l->order[child],l->order[child+1],l->sort,l->descending)<0)++child;
  if(file_compare11(l->order[start],l->order[child],l->sort,l->descending)>=0)return;
  uint16_t swap=l->order[start];l->order[start]=l->order[child];l->order[child]=swap;start=child;}
}
static file_listing11 *file_listing11_for(unsigned slot,const char*path,unsigned sort,unsigned descending){
 file_listing11*l=&file_lists11[slot];v6_volume*v=m6_volume();
 if(l->generation==v->generation&&equal(l->path,path)&&l->sort==sort&&l->descending==descending)return l;
 if(file_in_bin11(path))file_refresh_trash_names11();
 l->count=0;l->sort=sort;l->descending=descending;copy(l->path,path,256);
 for(unsigned i=0;i<V6_ENTRIES;++i)if(v->entries[i].kind&&file_direct_child11(v->entries[i].path,path))l->order[l->count++]=(uint16_t)i;
 for(unsigned i=l->count/2;i;--i)file_heap11(l,i-1,l->count);
 for(unsigned i=l->count;i>1;--i){uint16_t swap=l->order[0];l->order[0]=l->order[i-1];l->order[i-1]=swap;file_heap11(l,0,i-1);}
 l->generation=v->generation;return l;
}
static const v6_entry *ordered_child11(application*a,unsigned row){unsigned slot=(unsigned)(a-apps);file_window11*f=&file_windows11[slot];file_listing11*l=file_listing11_for(slot,a->path,f->sort,f->descending);return row<l->count?&m6_volume()->entries[l->order[row]]:0;}
static const v6_entry *desktop_child11(unsigned row){char path[256];root_path(path,"Desktop");file_listing11*l=file_listing11_for(DESKTOP_WINDOWS,path,desktop_sort11,desktop_descending11);return row<l->count?&m6_volume()->entries[l->order[row]]:0;}
static unsigned desktop_children11(void){char path[256];root_path(path,"Desktop");return file_listing11_for(DESKTOP_WINDOWS,path,desktop_sort11,desktop_descending11)->count;}
static unsigned selection_index11(application*a,unsigned row){if(a->usb_mode10==2)return row;const v6_entry*e=ordered_child11(a,row);return e?(unsigned)(e-m6_volume()->entries):V6_ENTRIES;}
static unsigned file_selected_count11(application*a){unsigned count=0,total=children72(a);if(a->usb_mode10==2){if(usb_listing10(a,0,1,&total))return 0;}for(unsigned n=0;n<total&&n<V6_ENTRIES;++n)count+=selected_row11(a,n);return count;}
static void file_visit11(application*a){
 unsigned slot=(unsigned)(a-apps);file_window11*f=&file_windows11[slot];f->address_active=0;f->view=0;f->sort=f->descending=0;
 for(unsigned i=0;i<file_views_state11.count;++i){file_view_record11*r=&file_views_state11.records[i];if(equal(r->path,a->path)){f->view=r->view;f->sort=r->sort;f->descending=r->descending;break;}}
 file_lists11[slot].generation=0;
}
static unsigned file_save_view11(application*a){
 unsigned slot=(unsigned)(a-apps),index=32;file_window11*f=&file_windows11[slot];
 for(unsigned i=0;i<file_views_state11.count;++i)if(equal(file_views_state11.records[i].path,a->path)){index=i;break;}
 if(index==32){index=file_views_state11.count<32?file_views_state11.count++:file_views_state11.next++%32;}
 file_view_record11*r=&file_views_state11.records[index];copy(r->path,a->path,256);r->view=f->view;r->sort=f->sort;r->descending=f->descending;
 file_views_state11.magic=UINT64_C(0x3153574549563146);file_views_state11.version=1;
 return v6_put(m6_volume(),"/System/Settings/files11.bin",sizeof(file_views_state11),memory_source,&file_views_state11);
}
