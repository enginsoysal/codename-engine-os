/* Only owned installation media redirects to a single matching installed ESP.
   EFI LoadImage/StartImage load the actual disk loader and kernel, never the
   live kernel. Escape bypasses redirection for recovery/reinstallation. */
static void chain75_try(efi_handle image,efi_system_table*system,efi_loaded_image*loaded){
 typedef struct {void*reset;efi_status(*read)(void*,uint16_t*);} input75;
 input75*input=system->input;uint16_t key[2]={0};
 if(input&&input->read&&!EFI_ERROR(input->read(input,key))&&(key[0]==0x17||key[1]==27)){serial_text("M75_SETUP_OVERRIDE\n");return;}
 efi_handle handles[64],target=0;uint64_t size=sizeof(handles);unsigned count=0;
 if(!bs->locate_handle||EFI_ERROR(bs->locate_handle(2,&fs_guid,0,&size,handles))||size>sizeof(handles)||size%sizeof(efi_handle))return;
 for(unsigned i=0;i<size/sizeof(efi_handle);++i){
  if(handles[i]==loaded->device)continue;efi_filesystem*fs=0;efi_file*root=0,*file=0;
  if(EFI_ERROR(bs->handle_protocol(handles[i],&fs_guid,(void**)&fs))||!fs||EFI_ERROR(fs->open_volume(fs,&root))||!root)continue;
  uint8_t marker[41]={0};uint64_t bytes=sizeof(marker);unsigned match=0;
  if(!EFI_ERROR(root->open(root,&file,(uint16_t*)u"\\ENGINE.DSK",1,0))&&file){
   match=!EFI_ERROR(file->read(file,&bytes,marker))&&bytes==40&&!memcmp(marker,"ENGDISK1",8)&&!memcmp(marker+8,BUILD_ID,32);file->close(file);
  }
  root->close(root);if(match){target=handles[i];++count;}
 }
 if(count!=1){if(count)serial_text("M75_MULTIPLE_INSTALLATIONS_SETUP\n");return;}
 efi_guid dp_guid={0x09576e91,0x6d3f,0x11d2,{0x8e,0x39,0,0xa0,0xc9,0x69,0x72,0x3b}};
 uint8_t*source=0,path[1152];if(EFI_ERROR(bs->handle_protocol(target,&dp_guid,(void**)&source))||!source)return;
 unsigned at=0,hard=0;
 while(at<1024){unsigned n=(unsigned)source[at+2]|((unsigned)source[at+3]<<8);if(n<4||n>1024-at)return;
  if(source[at]==0x7f){if(source[at+1]!=0xff||n!=4)return;break;}
  if(source[at]==4&&source[at+1]==1&&n==42)hard=1;memcpy(path+at,source+at,n);at+=n;
 }
 if(at>=1024||!hard)return;const uint16_t name[]=u"\\EFI\\BOOT\\BOOTX64.EFI";unsigned n=4+sizeof(name);
 path[at]=4;path[at+1]=4;path[at+2]=(uint8_t)n;path[at+3]=(uint8_t)(n>>8);memcpy(path+at+4,name,sizeof(name));at+=n;memcpy(path+at,"\x7f\xff\x04\0",4);
 efi_handle child=0;efi_status status=bs->load_image(0,image,path,0,0,&child);
 if(!EFI_ERROR(status)&&child){serial_text("M75_CHAIN_DISK_LOADER\n");status=bs->start_image(child,0,0);}
 serial_text("M75_CHAIN_RETURNED status=");serial_hex(status);serial_text("\n");
}
