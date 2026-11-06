#include <engine/m5.h>
#include <engine/fs.h>
#include "distro.h"
#include "build.h"
extern unsigned m51_prepare(unsigned,unsigned),m51_run(unsigned,unsigned),m51_is_live(void),m51_persistent(void);
extern void m51_cancel(void);
static const os_boot_info_v0 *screen;
enum {HOME,TRY,SELECT,CONFIRM,DONE,ERROR,DETAIL,ADVANCED,ABOUT};
static unsigned state,previous,repair,selected,last_status,last_percent;
static int equal(const char *a,const char *b){while(*a&&*a==*b){++a;++b;}return !*a&&!*b;}
static void clear(void){console_begin(screen);}
static void capacity(const storage_disk *d){console_decimal(d->sectors/2097152);console_text(" GiB (");console_decimal(d->sectors/2048);console_text(" MiB)");}
static void home(void){state=HOME;clear();console_text(DISTRO_WELCOME "\n\n1) Try / open my files\n2) Install " DISTRO_NAME "\n3) Repair an installation\n4) Advanced tools / terminal\n5) Technical details\n6) About this distribution\n\nType a number, then press Enter.\n");serial_text("M51_HOME_READY\n");}
static void selection(void){state=SELECT;clear();console_text(repair?"REPAIR - choose your installation disk\n":"INSTALL - choose a new disk\n");
    if(!m51_is_live()){console_text("Please boot the release ISO or USB to install or repair.\n0) Back\n");return;}
    if(!storage_count())console_text("No disks were detected.\nVMware: power off, add a SATA hard disk, then boot this ISO again.\n");
    for(unsigned i=0;i<storage_count();++i){const storage_disk *d=storage_get(i);console_decimal(i+1);console_text(") Disk ");console_decimal(i+1);console_text(" - ");capacity(d);console_text(d->transport==STORAGE_AHCI&&d->readable&&d->sector_bytes==512&&d->sectors>=M5_MIN_SECTORS?" - SATA; checked when selected\n":" - unavailable: needs readable SATA, 512-byte sectors, at least 256 MiB\n");}
    console_text("\n0) Back without changing any disk\nSelect the number shown beside your disk.\n");}
static void problem(unsigned code){state=ERROR;last_status=code;clear();
    if(code==B_DENIED)console_text("This disk cannot be used for this operation.\nInstallation needs a new SATA disk with blank boot metadata.\nRepair needs an installation made by this exact ISO/build.\nExisting partitions are not erased automatically.\n");
    else console_text("The disk could not be read, written or verified.\nThe operation has NOT completed successfully.\nPower off and check the SATA disk connection. For an incomplete\ninstallation, restart this same ISO and choose Repair.\n");
    console_text("\n0) Back to welcome\nD) Technical details\n");serial_text("M51_ACTION_REFUSED_OR_FAILED\n");}
static void confirmation(void){state=CONFIRM;clear();console_text(repair?"Review boot repair\n":"Review installation\n");console_text("Selected disk ");console_decimal(selected+1);console_text(": ");capacity(storage_get(selected));
    console_text(repair?"\nRestore this build's boot files. Initialized user data is preserved.\n":"\nThis prepares the WHOLE selected disk. Existing contents may be lost.\n64 MiB EFI + 16 MiB native data; remaining space stays unallocated.\nMaximum file size is currently 48 KiB. No desktop or Windows apps yet.\n");
    console_text(repair?"\n1) Repair this disk now\n":"\n1) Install on this disk now\n");console_text("0) Go back - do not write anything\nD) Show technical details\n");serial_text("M51_CONFIRM_READY\n");}
void m51_progress(unsigned percent){if(percent/10==last_percent/10&&percent!=100)return;last_percent=percent;console_text("Progress: ");console_decimal(percent);console_text("%\n");}
void m51_start(const os_boot_info_v0 *boot){screen=boot;home();console_text("Choice> ");}
unsigned m51_menu_active(void){return state!=ADVANCED;}
unsigned m51_line(const char *line){
    if(state==ADVANCED){if(equal(line,"menu")){home();return 1;}if(equal(line,"install")||equal(line,"plan")||equal(line,"repair")){console_text("For guided setup, type menu and choose Install or Repair.\n");return 1;}return 0;}
    if(equal(line,"d")||equal(line,"D")||(state==HOME&&equal(line,"5"))){previous=state;state=DETAIL;clear();console_text("Technical details (optional)\nBuild: " BUILD_ID "\n");storage_inventory();m5_command("volume");console_text("Last operation code: ");console_decimal(last_status);console_text("\nFull boot diagnostics are also sent to COM1.\n0) Return\n");return 1;}
    if(equal(line,"0")){m51_cancel();if(state==DETAIL&&previous==CONFIRM){selection();return 1;}home();return 1;}
    if(state==HOME){
        if(equal(line,"1")){state=TRY;clear();console_text(m51_persistent()?"Your installed files are persistent.\n":"Try mode: file changes are temporary and disappear on reboot.\n");console_text("This release has a file preview, not a desktop.\n\n1) Create a sample note\n2) Read the sample note\n3) List my documents\n0) Back\n");}
        else if(equal(line,"2")||equal(line,"3")){repair=equal(line,"3");selection();}
        else if(equal(line,"4")){state=ADVANCED;clear();console_text("Advanced terminal. Type menu to return to guided setup.\nType help for diagnostic commands.\n");serial_text("M51_ADVANCED_READY\n");}
        else if(equal(line,"6")){state=ABOUT;clear();console_text(DISTRO_NAME " " DISTRO_VERSION "\n" DISTRO_WELCOME "\nOriginal Engine OS kernel. Distribution profile: /System/Distro/profile.json\nMIT source; custom profiles choose branding, colour and data packages.\n0) Back\n");}
        else console_text("Choose one of the displayed numbers, for example 2 to install.\n");return 1;
    }
    if(state==TRY){unsigned s=0;if(equal(line,"1")){s=fs_write("/Users/test/Documents/Sample.txt","My first Engine OS note.",24);console_text(s?"The note could not be saved. The volume may be read-only or full.\n":"Sample note saved. Choose 2 to read it.\n");}
        else if(equal(line,"2")){char data[128];unsigned bytes=0;s=fs_read("/Users/test/Documents/Sample.txt",data,127,&bytes);if(s)console_text("No sample note yet. Choose 1 to create one.\n");else{data[bytes]=0;console_text(data);console_text("\n");}}
        else if(equal(line,"3")){s=fs_list("/Users/test/Documents");if(s)console_text("Documents are unavailable on this volume.\n");}else console_text("Choose 1, 2, 3 or 0 to go back.\n");return 1;}
    if(state==SELECT){unsigned n=0;const char *p=line;while(*p>='0'&&*p<='9'&&n<=STORAGE_MAX){n=n*10+(unsigned)(*p++-'0');}if(*p||!n||n>storage_count()||!m51_is_live()){console_text("Select an available disk number, or 0 to go back.\n");return 1;}selected=n-1;unsigned s=m51_prepare(selected,repair);if(s)problem(s);else confirmation();return 1;}
    if(state==CONFIRM&&equal(line,"1")){clear();console_text(repair?"Repairing boot files. Please wait; do not disconnect the disk.\n":"Installing. Please wait; do not disconnect the disk.\n");last_percent=101;unsigned s=m51_run(selected,repair);if(s)problem(s);else{state=DONE;clear();console_text(repair?"Repair complete.\n":"Installation complete.\n");console_text("1. Power off this VM using VMware.\n2. Disconnect the ISO/CD or USB installation media.\n3. Start the VM again to boot your installed system.\n\n0) Return to welcome\nD) Technical details\n");serial_text("M51_GUIDED_COMPLETE\n");}return 1;}
    console_text("Choose an option shown above, or 0 to return.\n");return 1;
}
