#ifdef ENGINE_M7
#include <engine/desktop/session.h>
#endif
#include <engine/install/m6.h>
#include "build.h"
#include "distro.h"
#ifdef ENGINE_M71
#include <engine/install/disk71.h>
#endif
static const os_boot_info_v0*screen;
static unsigned command_overflow,terminal;static char command[256];static unsigned command_length;
static unsigned maintenance,step,focus,controls,chosen=UINT32_MAX,busy,percent,installed,diagnostics;
static uint32_t accent=DISTRO_ACCENT;static m6_preferences preferences={"Owner",DISTRO_ACCENT,DISTRO_M6_GUIDE};
static const char *failure,*stage,*notice,*validation;
#ifdef ENGINE_M7
static unsigned calculator_selection=DISTRO_DESKTOP_CALCULATOR;
unsigned m7_calculator_selection(void){return calculator_selection;}
#define DESKTOP_FINISH_TEXT "Start again to enter your installed Engine OS desktop."
#else
#define DESKTOP_FINISH_TEXT "The desktop will follow in a later release."
#endif
typedef struct{unsigned x,y,w,h,action;} control;
static control buttons[24];static unsigned left,top,width,height,body,bottom;
static void label(unsigned x,unsigned y,const char*s,unsigned size,uint32_t color){gui6_text(x,y,s,size,color);}
static void decimal(uint64_t n,char*b){char temp[24];unsigned i=0,j=0;do{temp[i++]=(char)('0'+n%10);n/=10;}while(n);while(i)b[j++]=temp[--i];b[j]=0;}
static void textnum(unsigned x,unsigned y,uint64_t n,const char*suffix){char b[64];decimal(n,b);unsigned i=0;while(b[i])++i;unsigned j=0;do{b[i+j]=suffix[j];}while(suffix[j++]);label(x,y,b,2,0xf0f5f5);}
static void button(unsigned x,unsigned y,unsigned w,const char*title,unsigned action,unsigned primary){unsigned n=controls++;buttons[n]=(control){x,y,w,40,action};uint32_t bg=primary?accent:0x20353e;gui6_rect(x,y,w,40,focus==n?0xe4fff7:0x3a535d);gui6_rect(x+2,y+2,w-4,36,bg);
#ifdef ENGINE_M72
 char fitted[64];unsigned columns=w>26?(w-26)/12:0;if(columns>63)columns=63;unsigned count=0;while(count<columns&&title[count]){fitted[count]=title[count];++count;}fitted[count]=0;
 if(title[count]&&count>=3){fitted[count-3]='.';fitted[count-2]='.';fitted[count-1]='.';}title=fitted;
#endif
 label(x+13,y+13,title,2,primary?0x112b24:0xffffff);}
#ifdef ENGINE_M71
static void render(void);
#include "disk71-ui.inc"
#endif
#ifdef ENGINE_M72
#include "wizard72-ui.inc"
#endif
static void render(void){
#ifdef ENGINE_M72
 if(!u71_mode&&!diagnostics&&!failure&&!installed){render72();return;}
#endif
 unsigned old=focus;controls=0;gui6_rect(0,0,screen->framebuffer_width,screen->framebuffer_height,0x0c171d);gui6_rect(left,top,width,height,0x3b535c);gui6_rect(left+1,top+1,width-2,height-2,0x101d24);gui6_rect(left+1,top+70,width-2,1,0x35464e);
 label(left+25,top+25,"E",3,accent);label(left+65,top+26,DISTRO_NAME,2,0xffffff);label(left+width-210,top+32,installed?"INSTALLED SYSTEM":"INSTALLATION",2,accent);
 gui6_rect(left+1,top+71,195,height-142,0x14242b);label(left+20,top+100,"MAKE IT YOURS",1,0x98adb5);
 const char*names[]={"Welcome","Your disk","Components","Your style","Review","Installing","Finish"};for(unsigned i=0;i<7;++i){char n[4]={(char)('1'+i),0};label(left+22,top+145+i*44,n,2,i==step?accent:0x7f949e);label(left+48,top+145+i*44,names[i],2,i==step?0xffffff:0x8fa3ad);}
 unsigned x=body,y=top+110;
#ifdef ENGINE_M71
 if(u71_mode){u71_draw(x,y);focus=old<controls?old:0;return;}
#endif
 if(diagnostics){label(x,y,"Technical details",3,0xffffff);label(x,y+50,"Build",2,0x95aab4);label(x,y+80,BUILD_ID,1,0xc5d9dd);textnum(x,y+115,pages_stats().current_pages*4096/1024," KiB managed RAM");textnum(x,y+150,pages_stats().peak_pages*4096/1024," KiB peak RAM");if(installed){button(x,y+200,230,"Write test file",60,0);button(x,y+250,230,"Verify test file",61,0);}button(x,y+300,230,"Advanced terminal",64,0);if(notice)label(x,y+355,notice,1,accent);button(x,bottom-65,160,"Return",62,0);
#ifdef ENGINE_M72
 if(!installed){button(x,y+200,140,"Repair",80,0);button(x+150,y+200,140,"Update",81,0);button(x+300,y+200,140,"Rollback",82,0);}
#endif
 return;}
 if(failure){label(x,y,"We could not finish.",3,0xffffff);label(x,y+65,failure,1,0xf4b29d);label(x,y+100,"No installation success is claimed.",1,0xa5bac2);label(x,y+125,"Keep this ISO for recovery. Check Details.",1,0xa5bac2);button(x,bottom-60,160,"Back",63,0);button(x+180,bottom-60,160,"Details",50,0);return;}
 if(installed){label(x,y,"Your system is ready.",3,0xffffff);label(x,y+55,preferences.user,2,accent);label(x,y+90,"Booted from disk. Your settings are persistent.",1,0xa5bac2);textnum(x,y+125,v6_capacity(m6_volume())/(1024*1024)," MiB data capacity");label(x,y+165,DESKTOP_FINISH_TEXT,1,0xa5bac2);button(x,y+210,230,"Change accent",70,0);button(x,y+265,230,"Save preferences",71,1);if(preferences.guide)button(x,y+320,230,"Remove guide",72,0);if(notice)label(x,y+375,notice,1,accent);button(x,bottom-65,180,"Details",50,0);return;}
 if(step==0){label(x,y,"A fresh start.",4,0xffffff);label(x,y+45,"Made yours.",4,0xffffff);label(x,y+115,"A guided setup, with room to choose.",2,0xa5bac2);label(x,y+150,"Review every change before installation.",1,0xa5bac2);gui6_rect(x,y+195,width-250,75,0x1a2c34);label(x+15,y+212,"Your system. Your choices.",2,0xffffff);label(x+15,y+244,DISTRO_WELCOME,1,0xa5bac2);label(x,y+300,"Unsigned BIOS/UEFI media: disable Secure Boot.",1,0x9fb2ba);}
 if(step==1){label(x,y,"Choose your disk.",3,0xffffff);label(x,y+50,
#ifdef ENGINE_M71
 maintenance?"Select an existing Engine OS system.":"Select a SATA disk. Manage partitions or choose whole-disk replacement.",
#else
 "Only a new supported SATA disk can be installed.",
#endif
1,0x9fb2ba);unsigned count=storage_count();if(!count)label(x,y+100,"No disk detected. Add a SATA disk in VMware.",1,0xf4b29d);for(unsigned row=0;row<4;++row){unsigned i=row;
#ifdef ENGINE_M71
 i+=disk_page*4;
#endif
 if(i>=count)break;const storage_disk*d=storage_get(i);char n[20]="Disk ";n[5]=(char)('1'+i);n[6]=0;button(x,y+90+row*55,135,n,100+i,chosen==i);textnum(x+155,y+103+row*55,d->sectors/2097152," GiB");}
#ifdef ENGINE_M71
 if(count>4){button(x+420,y+90,60,"<",94,0);button(x+490,y+90,60,">",95,0);}
 if(!maintenance){button(x,y+340,220,"Manage disks",90,0);button(x+235,y+340,310,replace_selected?"Erase and install: ON":"Erase and install: OFF",91,replace_selected);}
 if(validation)label(x,y+405,validation,1,0xf4b29d);
#endif
label(x,y+310,maintenance?"Select an Engine OS installation to maintain.":"Installation prepares the WHOLE selected disk.",1,0xf4b29d);}
 if(step==2){label(x,y,"Keep what you need.",3,0xffffff);label(x,y+55,"Base system and recovery are required.",1,0x9fb2ba);button(x,y+105,300,preferences.guide?"Welcome guide: On":"Welcome guide: Off",20,0);
#ifdef ENGINE_M7
 button(x,y+160,300,calculator_selection?"Calculator: On":"Calculator: Off",21,0);
 label(x,y+225,"Desktop, Files and Text Editor are included.",1,0x9fb2ba);
 label(x,y+255,"Calculator and the guide are optional. Personal files stay separate.",1,0x9fb2ba);
#else
 label(x,y+170,"The guide is optional and independently removable.",1,0x9fb2ba);label(x,y+205,"Desktop apps are not included in this release.",1,0x9fb2ba);label(x,y+235,"Your files stay separate from the base system.",1,0x9fb2ba);
#endif
}
 if(step==3){label(x,y,"Make yourself at home.",3,0xffffff);label(x,y+55,"Your account name (letters, digits, - or _)",1,0x9fb2ba);button(x,y+85,300,preferences.user,30,0);label(x,y+150,"Setup accent",2,0x9fb2ba);button(x,y+185,135,"Mint",31,accent==0x62e5bd);button(x+150,y+185,135,"Lavender",32,accent==0x9caeff);button(x+300,y+185,135,"Amber",33,accent==0xf4c47c);label(x,y+260,"Click the name field to edit. Tab changes focus.",1,0x9fb2ba);label(x,y+290,"The name and accent survive installed boots.",1,0x9fb2ba);if(validation)label(x,y+320,validation,1,0xf4b29d);}
 if(step==4){label(x,y,"Everything look right?",3,0xffffff);label(x,y+60,maintenance?"Boot files change; user data is preserved.":"Data on the selected disk may be lost.",1,0xf4b29d);textnum(x,y+110,chosen+1," - Selected disk number");textnum(x,y+150,(m6_plan()->data.last-m6_plan()->data.first+1)/2048," MiB data partition");label(x,y+195,"User",1,0x9fb2ba);label(x+90,y+190,preferences.user,2,0xffffff);label(x,y+240,preferences.guide?"Welcome guide included":"Welcome guide excluded",1,0x9fb2ba);
#ifdef ENGINE_M7
 label(x+320,y+240,calculator_selection?"Calculator included":"Calculator excluded",1,0x9fb2ba);
#endif

#ifdef ENGINE_M71
 if(!maintenance&&replace_selected){button(x,y+330,520,replace_consent?"[X] Erase ALL data on the selected disk":"[ ] Erase ALL data on the selected disk",92,0);if(validation)label(x,y+390,validation,1,0xf4b29d);}
#endif
 label(x,y+280,maintenance==1?"Repair replays the active verified boot slot.":maintenance==2?"Update stages this ISO into the inactive slot.":maintenance==3?"Rollback restores the previous verified boot slot.":"Back changes choices. Install begins disk writes.",1,0xf4b29d);}
 if(step==5){label(x,y,"Getting things ready.",3,0xffffff);label(x,y+70,stage?stage:"Preparing",1,0xa5bac2);gui6_rect(x,y+125,width-250,12,0x35464c);gui6_rect(x,y+125,(width-250)*percent/100,12,accent);textnum(x,y+165,percent," percent verified");label(x,y+230,"Do not disconnect the disk during installation.",1,0xa5bac2);}
 if(step>=6){label(x,y,maintenance==1?"Boot repair complete.":maintenance==2?"Update complete.":maintenance==3?"Rollback complete.":"Installation complete.",3,0xffffff);label(x,y+65,maintenance?"Verified boot files. User data was not formatted.":"Verified system files and persistent user setup.",1,0xa5bac2);label(x,y+115,"1. Power off this virtual machine.",2,0xffffff);label(x,y+155,"2. Disconnect the ISO or USB media.",2,0xffffff);label(x,y+195,"3. Start it again to boot Engine OS.",2,0xffffff);label(x,y+270,DESKTOP_FINISH_TEXT,1,accent);}
 gui6_rect(left+1,bottom-1,width-2,1,0x35464e);label(left+20,bottom+28,"Tab / Enter or mouse",1,0x9fb2ba);if(!busy&&step<5){if(step)button(left+width-300,bottom+14,120,"Back",1,0);button(left+width-160,bottom+14,135,step==4?(maintenance?"Confirm":"Install"):"Next",2,1);}if(step==6)button(left+width-160,bottom+14,135,"Finish",3,1);if(!busy&&step==0){button(body,bottom+14,120,"Details",50,0);button(body+140,bottom+14,120,"Repair",80,0);button(body+280,bottom+14,120,"Update",81,0);button(body+420,bottom+14,140,"Rollback",82,0);}focus=old<controls?old:0;
}
void m6_progress(unsigned p,const char*s){percent=p;stage=s;render();serial_text("M6_PROGRESS ");serial_hex(p);serial_text("\n");}
static void testpath(char*p){const char*prefix="/Users/";unsigned n=0;while(*prefix)p[n++]=*prefix++;for(unsigned i=0;preferences.user[i];++i)p[n++]=preferences.user[i];const char*tail="/Documents/large-test.bin";do{p[n++]=*tail;}while(*tail++);}
static unsigned pattern(void*ctx,uint64_t off,void*p,unsigned n){(void)ctx;for(unsigned i=0;i<n;++i)((uint8_t*)p)[i]=(uint8_t)((off+i)*37+19);return 0;}
static void terminal_command(void){
 if(command_overflow){command_length=command_overflow=0;console_text("Command too long. No command was executed.\nengine> ");return;}command[command_length]=0;v6_volume*v=m6_volume();
 if(!memcmp(command,"menu",5)){terminal=0;command_length=0;render();return;}
 if(!memcmp(command,"help",5))console_text("menu | help | volume | ls | cat PATH | write PATH TEXT | rm PATH\n");
 else if(!v)console_text("Boot the installed system to access persistent files.\n");
 else if(!memcmp(command,"volume",7)){console_text("Data capacity bytes: ");console_decimal(v6_capacity(v));console_text("\nCurrent file allocation bytes: ");console_decimal(v6_used(v));console_text("\n");}
 else if(!memcmp(command,"ls",3)){for(unsigned i=0;i<V6_ENTRIES;++i)if(v->entries[i].kind){console_text(v->entries[i].path);console_text(v->entries[i].kind==2?"/\n":"\n");}}
 else if(!memcmp(command,"cat ",4)){char data[1025];unsigned bytes=0,s=v6_verify(v,command+4);if(!s)s=v6_read(v,command+4,0,data,1024,&bytes);if(s){console_text("Read/integrity error: ");console_decimal(s);console_text("\n");}else{data[bytes]=0;console_text(data);console_text("\n");}}
 else if(!memcmp(command,"rm ",3)){const char*p=command+3;unsigned s=memcmp(p,"/Users/",7)?V6_RO:v6_remove(v,p);console_text("Remove status: ");console_decimal(s);console_text("\n");}
 else if(!memcmp(command,"write ",6)){char*p=command+6;unsigned n=0;while(p[n]&&p[n]!=' ')++n;if(!p[n]||memcmp(p,"/Users/",7))console_text("Use write /Users/account/Documents/name.txt TEXT\n");else{p[n++]=0;unsigned bytes=0;while(p[n+bytes])++bytes;extern unsigned m6_write_text(const char*,const void*,unsigned);unsigned s=m6_write_text(p,p+n,bytes);console_text("Write status: ");console_decimal(s);console_text("\n");}}
 else console_text("Unknown command. Type help.\n");command_length=0;console_text("engine> ");
}
#ifdef ENGINE_M7
void m7_terminal_command(const char *line,unsigned bytes){
 if(!line||bytes>=sizeof(command)){console_text("Command too long. No command was executed.\nengine> ");return;}
 memset(command,0,sizeof(command));memcpy(command,line,bytes);command_length=bytes;command_overflow=0;
 if(!memcmp(command,"menu",5)){console_text("Close the terminal window to return.\nengine> ");return;}
 terminal_command();
}
#endif
static void action(unsigned a){
#ifdef ENGINE_M71
 if(u71_mode){(void)u71_action(a);return;}
 if(a==90){if(chosen>=storage_count()){validation="Select a disk row first.";render();return;}u71_open(chosen);render();return;}
 if(a==91){replace_selected^=1;replace_consent=0;validation=0;m6_cancel();render();return;}
 if(a==92){replace_consent^=1;validation=0;render();return;}
 if(a==94||a==95){if(a==94&&disk_page)--disk_page;if(a==95&&(disk_page+1)*4<storage_count())++disk_page;render();return;}
 if(a>=100&&a<116){chosen=a-100;replace_selected=replace_consent=0;validation=0;m6_cancel();render();return;}
#endif
 if(a==64){terminal=1;command_length=command_overflow=0;console_begin(screen);console_text("Engine OS advanced terminal. Type menu to return.\nPersistent file access is available after installed boot.\nType help for commands.\nengine> ");serial_text("M6_TERMINAL_READY\n");return;}

 if(a==50){diagnostics=1;focus=0;render();return;}if(a==62){diagnostics=0;focus=0;render();return;}if(a==63){failure=0;maintenance=0;step=0;m6_cancel();focus=0;render();return;}
 if(a==60||a==61){char path[256];testpath(path);unsigned status=a==60?v6_put(m6_volume(),path,2*1024*1024+113,pattern,0):v6_verify(m6_volume(),path);notice=status?"The storage check failed. Keep the ISO for recovery.":a==60?"The test file was saved and committed.":"File integrity verification passed.";serial_text(status?"M6_LARGE_FILE_FAIL\n":"M6_LARGE_FILE_PASS\n");render();return;}
 if(a==72){char path[256]="/Users/";unsigned n=7;for(unsigned i=0;preferences.user[i];++i)path[n++]=preferences.user[i];const char*tail="/Documents/Welcome.txt";do{path[n++]=*tail;}while(*tail++);unsigned s=v6_remove(m6_volume(),path);if(!s||s==V6_NOTFOUND){preferences.guide=0;s=m6_save_settings(&preferences);}notice=s?"The guide could not be removed.":"Guide removed. Other features remain available.";serial_text(s?"M6_GUIDE_REMOVE_FAIL\n":"M6_GUIDE_REMOVED\n");render();return;}
 if(a==70){accent=accent==0x62e5bd?0x9caeff:0x62e5bd;preferences.accent=accent;render();return;}if(a==71){unsigned s=m6_save_settings(&preferences);notice=s?"Preferences could not be saved.":"Your preferences were saved to disk.";serial_text(s?"M6_SETTINGS_SAVE_FAIL\n":"M6_SETTINGS_SAVED\n");render();return;}
 if(a>=100&&a<104){chosen=a-100;render();return;}if(a==20){preferences.guide^=1;render();return;}
#ifdef ENGINE_M7
 if(a==21){calculator_selection^=1;render();return;}
#endif
 if(a>=31&&a<=33){const uint32_t colors[]={0x62e5bd,0x9caeff,0xf4c47c};accent=colors[a-31];preferences.accent=accent;render();return;}if(a==30)return;
 if(a>=80&&a<=82){maintenance=a-79;diagnostics=0;step=1;focus=0;render();return;}
 if(a==1){if(maintenance&&step==4)step=1;else if(step)--step;if(!step)maintenance=0;focus=0;render();return;}
 if(a==3){
#ifdef ENGINE_M72
 if(!maintenance&&m6_volume()&&!m6_settings(&preferences)){
 serial_text("M72_FINISH_TO_PERSISTENT_DESKTOP\n");desktop_start(screen,&preferences);}
#endif
 step=7;focus=0;render();return;}
#ifdef ENGINE_M72
 if(a==83){unsigned s=m6_open();if(!s)s=m6_settings(&preferences);if(!s){serial_text("M72_ISO_TO_PERSISTENT_DESKTOP\n");desktop_start(screen,&preferences);}
 validation="No single matching installation found. Use this ISO to install or recover.";render();return;}
#endif
 if(a==2){if(step==1){
#ifdef ENGINE_M71
 if(chosen>=storage_count()){validation="Select a disk row first, then choose Next.";render();return;}
 unsigned s=maintenance?m6_maintenance_prepare(chosen,maintenance):replace_selected?m71_prepare_replace(chosen):m6_prepare(chosen);
 if(s){validation=s==B_IO?"Disk I/O failed. Check the SATA connection.":maintenance?"No compatible Engine OS installation was found.":replace_selected?"Use a readable SATA disk, 512-byte sectors, 2 GiB to 2 TiB.":"Disk is not blank/eligible. Open Manage disks or choose Erase and install.";render();return;}validation=0;
#else
 unsigned s=maintenance?m6_maintenance_prepare(chosen,maintenance):m6_prepare(chosen);if(s){failure=s==B_DENIED?"Choose a blank SATA disk, 512-byte sectors, 2 GiB or larger.":"Disk I/O failed. Check the connection and try again.";render();return;}
#endif
}
 if(step==1&&maintenance){step=4;focus=0;render();return;}
 if(step==3&&!preferences.user[0]){validation="Please enter an account name to continue.";render();return;}if(step==4){
#ifdef ENGINE_M71
 if(!maintenance&&replace_selected&&!replace_consent){validation="Confirm whole-disk erasure above, or go Back.";render();return;}
#endif
busy=1;step=5;focus=0;render();unsigned s=maintenance?m6_maintenance():m6_install(&preferences);busy=0;if(s){failure="Disk copy or verification failed. Installation is incomplete.";serial_text("M6_INSTALL_FAILED code=");serial_hex(s);serial_text("\n");}else step=6;render();return;}++step;focus=0;render();}
}
NORETURN void m6_start(const os_boot_info_v0*boot){screen=boot;left=20;top=20;width=boot->framebuffer_width>1200?1160:boot->framebuffer_width-40;height=boot->framebuffer_height>740?700:boot->framebuffer_height-40;body=left+220;bottom=top+height-70;serial_text("M6_STORAGE_BEGIN\n");usb_mouse_limits(boot->framebuffer_width,boot->framebuffer_height);(void)block_init();(void)nvme_discover();(void)ide_discover();(void)virtio_discover();(void)usb_init();input_init();
 if(!m6_live()){unsigned s=m6_open();if(s)failure="Installed data volume could not be opened. Boot the ISO for recovery.";else{installed=1;if(m6_settings(&preferences))failure="Saved user settings failed integrity validation.";else accent=preferences.accent;}}
#ifdef ENGINE_M7
 if(installed&&!failure)desktop_start(boot,&preferences);
#endif
 render();serial_text("M6_WIZARD_READY\n");unsigned last_buttons=0;
 for(;;){__asm__ volatile("cli");if(usb_work_pending()){usb_process_changes();continue;}int key=input_pop();if(key<0){__asm__ volatile("sti; hlt":::"memory");continue;}__asm__ volatile("sti");
 if(terminal){if(key==256){unsigned x,y,b;usb_mouse_state(&x,&y,&b);usb_mouse_draw();continue;}if(key==8&&command_length){--command_length;console_text("\b");}else if(key==10){console_text("\n");terminal_command();}else if(key>=32&&key<=126){if(command_length<sizeof(command)-1&&!command_overflow){command[command_length++]=(char)key;char echo[2]={(char)key,0};console_text(echo);}else command_overflow=1;}continue;}
 if(key==256){unsigned x,y,b;usb_mouse_state(&x,&y,&b);if((b&1)&&!(last_buttons&1)){for(unsigned i=0;i<controls;++i){control*c=&buttons[i];if(x>=c->x&&x<c->x+c->w&&y>=c->y&&y<c->y+c->h){focus=i;action(c->action);break;}}}last_buttons=b;usb_mouse_draw();continue;}

#ifdef ENGINE_M7
 if(key==136&&controls){focus=(focus+controls-1)%controls;render();continue;}
#endif
 if(key==9&&controls){focus=(focus+1)%controls;render();continue;}if(key==10&&controls){action(buttons[focus].action);continue;}
#ifdef ENGINE_M71
 if(u71_key(key))continue;
#endif
 if(step==3&&!diagnostics&&focus<controls&&buttons[focus].action==30){unsigned n=0;while(preferences.user[n])++n;if(key==8&&n)preferences.user[n-1]=0;else if(n<31&&((key>='a'&&key<='z')||(key>='A'&&key<='Z')||(key>='0'&&key<='9')||key=='-'||key=='_')){preferences.user[n]=(char)key;preferences.user[n+1]=0;}validation=0;render();}
 }
}
