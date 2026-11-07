/* Select only the validated active monitor's preferred mode, not an arbitrary
   largest mode. A single active GOP/EDID output is the qualified profile. */
static void preferred_display73(efi_gop*gop){
 typedef struct{uint32_t bytes;uint8_t*data;} edid73;
 static efi_guid guid={0xbd8c1056,0x9f36,0x44ec,{0x92,0xa8,0xa6,0x33,0x7f,0x81,0x79,0x86}};
 for(unsigned mode=0;gop&&gop->mode&&gop->query_mode&&mode<gop->mode->max_mode&&mode<256;++mode){uint64_t size=0;efi_gop_info*info=0;efi_status status=((efi_status(*)(efi_gop*,uint32_t,uint64_t*,efi_gop_info**))gop->query_mode)(gop,mode,&size,&info);if(!EFI_ERROR(status)&&info){serial_text("M73_GOP_MODE ");serial_hex(info->width);serial_text("x");serial_hex(info->height);serial_text("\n");}if(info)((efi_status(*)(void*))bs->free_pool)(info);}
 edid73*edid=0;if(!gop||!gop->mode||!gop->query_mode||!gop->set_mode||gop->mode->max_mode>256||EFI_ERROR(bs->locate_protocol(&guid,0,(void**)&edid))||!edid||!edid->data||edid->bytes<128)return;
 const uint8_t*p=edid->data;static const uint8_t header[8]={0,255,255,255,255,255,255,0};if(memcmp(p,header,8))return;
 uint8_t sum=0;for(unsigned i=0;i<128;++i)sum=(uint8_t)(sum+p[i]);if(sum||!(p[54]|p[55]))return;
 unsigned width=p[56]|((unsigned)(p[58]&0xf0)<<4),height=p[59]|((unsigned)(p[61]&0xf0)<<4);
 if(width<1024||height<768||width>3840||height>2160)return;
 for(unsigned mode=0;mode<gop->mode->max_mode;++mode){uint64_t size=0;efi_gop_info*info=0;
 efi_status s=((efi_status(*)(efi_gop*,uint32_t,uint64_t*,efi_gop_info**))gop->query_mode)(gop,mode,&size,&info);
 unsigned match=!EFI_ERROR(s)&&info&&size>=36&&info->format<=1&&info->width==width&&info->height==height;
 if(info)((efi_status(*)(void*))bs->free_pool)(info);
 if(match){s=((efi_status(*)(efi_gop*,uint32_t))gop->set_mode)(gop,mode);serial_text(EFI_ERROR(s)?"M73_EDID_MODE_REFUSED\n":"M73_EDID_MODE_SELECTED\n");return;}}
}
