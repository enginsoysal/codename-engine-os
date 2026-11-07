/* UEFI 2.11 section 3.1: a short-form File Path load option searches ESPs.
   Use the stable fallback path, including after verified older-slot rollback.
   Preserve foreign Boot#### values and their relative BootOrder ordering. */
static efi_guid boot72_global={0x8be4df61,0x93ca,0x11d2,{0xaa,0x0d,0x00,0xe0,0x98,0x03,0x2b,0x8c}};
#define BOOT72_NOT_FOUND UINT64_C(0x800000000000000e)
static void boot72_name(uint16_t *name,unsigned number){
 const char*hex="0123456789ABCDEF";name[0]='B';name[1]='o';name[2]='o';name[3]='t';
 for(unsigned i=0;i<4;++i)name[4+i]=(uint16_t)hex[(number>>(12-i*4))&15];name[8]=0;
}
static efi_status boot72_register(efi_runtime_services*rt){
 if(!rt||!rt->get_variable||!rt->set_variable)return EFI_INVALID_PARAMETER;
 const uint16_t description[]=u"Engine OS";
 const uint16_t path[]=u"\\EFI\\BOOT\\BOOTX64.EFI";
 uint8_t option[256]={0},current[256];uint16_t order[256],next[257],name[9];
 unsigned path_bytes=4+sizeof(path)+4,bytes=6+sizeof(description)+path_bytes;
 option[0]=1;option[4]=(uint8_t)path_bytes;option[5]=(uint8_t)(path_bytes>>8);
 memcpy(option+6,description,sizeof(description));unsigned at=6+sizeof(description);
 option[at]=4;option[at+1]=4;option[at+2]=(uint8_t)(4+sizeof(path));option[at+3]=0;
 memcpy(option+at+4,path,sizeof(path));at+=4+sizeof(path);option[at]=0x7f;option[at+1]=0xff;option[at+2]=4;
 uint64_t size=sizeof(order);uint32_t attrs=0;efi_status status=rt->get_variable((uint16_t*)u"BootOrder",&boot72_global,&attrs,&size,order);
 if(status==BOOT72_NOT_FOUND)size=0;
 else if(EFI_ERROR(status))return status;
 else if(attrs!=7||size>sizeof(order)||(size&1))return EFI_INVALID_PARAMETER;
 unsigned count=(unsigned)size/2,free=UINT32_MAX,owned=UINT32_MAX;
 for(unsigned n=0xe000;n<0xe100;++n){uint64_t length=sizeof(current);uint32_t attribute=0;boot72_name(name,n);
 status=rt->get_variable(name,&boot72_global,&attribute,&length,current);
 if(status==BOOT72_NOT_FOUND){if(free==UINT32_MAX)free=n;}
 else if(!EFI_ERROR(status)&&attribute==7&&length==bytes&&!memcmp(current,option,bytes)){owned=n;break;}
 else if(EFI_ERROR(status)&&status!=EFI_BUFFER_TOO_SMALL)return status;}
 if(owned==UINT32_MAX){if(free==UINT32_MAX)return EFI_INVALID_PARAMETER;owned=free;boot72_name(name,owned);
 status=rt->set_variable(name,&boot72_global,7,bytes,option);if(EFI_ERROR(status))return status;}
 boot72_name(name,owned);uint64_t length=sizeof(current);attrs=0;
 status=rt->get_variable(name,&boot72_global,&attrs,&length,current);
 if(EFI_ERROR(status))return status;if(attrs!=7||length!=bytes||memcmp(current,option,bytes))return EFI_INVALID_PARAMETER;
 unsigned used=1;next[0]=(uint16_t)owned;for(unsigned i=0;i<count;++i)if(order[i]!=owned)next[used++]=order[i];
 if(used>256)return EFI_INVALID_PARAMETER;
 if(size!=used*2||memcmp(order,next,used*2)){
 status=rt->set_variable((uint16_t*)u"BootOrder",&boot72_global,7,used*2,next);if(EFI_ERROR(status))return status;}
 size=sizeof(order);attrs=0;status=rt->get_variable((uint16_t*)u"BootOrder",&boot72_global,&attrs,&size,order);
 if(EFI_ERROR(status))return status;return attrs==7&&size==used*2&&!memcmp(order,next,used*2)?0:EFI_INVALID_PARAMETER;
}
