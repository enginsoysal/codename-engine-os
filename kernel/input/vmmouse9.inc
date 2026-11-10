/* Original implementation of the public VMware absolute-pointer interface. */
static unsigned absolute9;
typedef struct {uint32_t a,b,c,d;} vm9_result;
static vm9_result vm9_call(unsigned command,unsigned argument){
 uint32_t a=0x564d5868,b=argument,c=command,d=0x5658,s=0,t=0;
 __asm__ volatile("inl %%dx,%%eax":"+a"(a),"+b"(b),"+c"(c),"+d"(d),"+S"(s),"+D"(t)::"memory");
 return (vm9_result){a,b,c,d};
}
static void vm9_start(void){
 uint32_t a,b,c,d;__asm__ volatile("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(0x40000000u),"c"(0));
 if(b!=0x61774d56||c!=0x4d566572||d!=0x65726177)return;
 vm9_result version=vm9_call(10,~0x564d5868u);if(version.b!=0x564d5868u||version.a==UINT32_MAX)return;
 vm9_call(41,0x45414552);vm9_result status=vm9_call(40,0);
 if((status.a&0xffff0000)==0xffff0000||!(status.a&65535)||vm9_call(39,1).a!=0x3442554a){vm9_call(41,0xf5);return;}
 vm9_call(86,1);vm9_call(41,0x53424152);absolute9=1;mouse_bytes=3;mouse_index=0;serial_text("M9_VMWARE_ABSOLUTE_POINTER_ACTIVE\n");
}
static unsigned vm9_input(void){
 if(!absolute9)return 0;
 for(unsigned count=0;count<64;++count){
  unsigned status=vm9_call(40,0).a,words=status&65535;
  if((status&0xffff0000)==0xffff0000||(words&3)){absolute9=0;vm9_call(41,0xf5);serial_text("M9_VMWARE_POINTER_FALLBACK\n");return 0;}
  if(!words)return 1;vm9_result packet=vm9_call(39,4);

#ifdef ENGINE_M10
  if((int8_t)packet.d)input10_wheel(-(int8_t)packet.d);
#endif
  unsigned buttons=((packet.a&0x20)?1:0)|((packet.a&0x10)?2:0)|((packet.a&8)?4:0);
  if(packet.a&0x10000)m6_pointer_report((int32_t)packet.b,-(int32_t)packet.c,buttons);
  else if(packet.b<=65535&&packet.c<=65535){extern void m9_pointer_absolute(unsigned,unsigned,unsigned);m9_pointer_absolute(packet.b,packet.c,buttons);}
 }
 return 1;
}
