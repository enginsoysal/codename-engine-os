#include <engine/m4.h>
#include <engine/fs.h>
#ifdef ENGINE_M45
#include <engine/storage.h>
#endif
#ifdef ENGINE_M5
#include <engine/m5.h>
#include <engine/m5_volume.h>
#endif
static uint8_t record[512];
static int valid_record(void){uint32_t length,crc;memcpy(&length,record+16,4);memcpy(&crc,record+508,4);return !memcmp(record,M4_RECORD,8)&&length<=120&&crc==m4_crc(record,508);}
static unsigned load(void){unsigned s=block_read(1,record);if(s)return s;return valid_record()?B_OK:B_IO;}
static unsigned store(const char *text){
    if(fs_readonly())return B_DENIED;
    unsigned length=0;while(text[length])if(++length>120)return B_DENIED;
    unsigned s=load();if(s)return s;uint64_t generation;memcpy(&generation,record+8,8);if(generation==UINT64_MAX)return B_DENIED;++generation;
    memset(record,0,512);memcpy(record,M4_RECORD,8);memcpy(record+8,&generation,8);memcpy(record+16,&length,4);memcpy(record+24,text,length);uint32_t crc=m4_crc(record,508);memcpy(record+508,&crc,4);
    uint8_t expected[512];memcpy(expected,record,512);
    s=block_write_record(record);if(!s)s=block_flush();if(!s)s=load();if(!s&&memcmp(expected,record,512))s=B_IO;return s;
}
static void show(void){unsigned s=load();if(s){console_text("Read error: ");console_hex(s);console_text("\n");return;}
    uint64_t generation;uint32_t length;memcpy(&generation,record+8,8);memcpy(&length,record+16,4);char text[121];memcpy(text,record+24,length);text[length]=0;
    console_text("Record generation ");console_hex(generation);console_text(": ");console_text(text);console_text("\n");}
static void info(void){block_info s=block_stats();console_text("AHCI validated=");console_hex(s.validated);console_text(" online=");console_hex(s.online);console_text("\nPCI BDF=");console_hex(s.pci);console_text(" port=");console_hex(s.port);console_text("\nSectors=");console_hex(s.sectors);console_text(" DMA pages=");console_hex(s.dma_pages);console_text("\nReads=");console_hex(s.reads);console_text(" writes=");console_hex(s.writes);console_text(" flushes=");console_hex(s.flushes);console_text("\nInput drops=");console_hex(input_dropped());console_text("\n");}
static char fold(char c){return c>='A'&&c<='Z'?(char)(c-'A'+'a'):c;}
static int equal(const char *a,const char *b){while(*a&&fold(*a)==fold(*b)){++a;++b;}return fold(*a)==fold(*b);}
static int store_prefix(const char *s){return fold(s[0])=='s'&&fold(s[1])=='t'&&fold(s[2])=='o'&&fold(s[3])=='r'&&fold(s[4])=='e'&&s[5]==' ';}
#ifdef ENGINE_M45
static unsigned number(const char **text,unsigned *value){const char *s=*text;unsigned n=0,digits=0;while(*s>='0'&&*s<='9'){if(n>10000)return 0;n=n*10+(unsigned)(*s++-'0');++digits;}if(!digits||(*s&&*s!=' '))return 0;while(*s==' ')++s;*text=s;*value=n;return 1;}
#endif
NORETURN void m4_start(const os_boot_info_v0 *boot,unsigned mode){
    serial_text("M4_STORAGE_BEGIN\n");unsigned s=block_init();serial_text("M4_BLOCK_INIT status=");serial_hex(s);serial_text("\n");
#ifdef ENGINE_M45
    (void)nvme_discover();
    (void)ide_discover();
    (void)virtio_discover();
    unsigned usb=usb_init();
#endif
    page_stats memory=pages_stats();serial_text("M4_MEMORY retained_owned_bytes=");serial_hex(memory.current_pages*4096);serial_text(" peak_owned_bytes=");serial_hex(memory.peak_pages*4096);serial_text("\n");
    #ifdef ENGINE_M5
    s=m5_volume_start();
    if(mode==7){extern unsigned m5_overlay_test(void);unsigned result=s?s:m5_overlay_test();serial_text(result?"M5_OVERLAY_FAIL\n":"M5_OVERLAY_PASS\n");test_exit(result?0x32:0x10);}
    #endif
    unsigned filesystem=s?F_IO:fs_mount();serial_text("M4_FS_MOUNT status=");serial_hex(filesystem);serial_text("\n");
#ifdef ENGINE_M45
    if(mode==6){extern unsigned ext_selftest(void);console_begin(boot);storage_inventory();if(ext_selftest()){serial_text("M45_EXT2_READ_FAIL\n");test_exit(0x32);}serial_text("M45_EXT2_READ_PASS\n");test_exit(0x10);}
    if(mode==5){extern unsigned fat_selftest(void);console_begin(boot);storage_inventory();if(fat_selftest()){serial_text("M45_FAT32_PAYLOAD_FAIL\n");test_exit(0x32);}serial_text("M45_FAT32_PAYLOAD_PASS\n");test_exit(0x10);}
    if(mode==1||mode==4){console_begin(boot);storage_inventory();if(storage_selftest()){serial_text("M45_DISCOVERY_FAIL\n");test_exit(0x32);}if(!filesystem){uint64_t before;if(load()){serial_text("M45_MARKED_WRITE_FAIL\n");test_exit(0x32);}memcpy(&before,record+8,8);if(fs_selftest(before)||store("ENGINE OS M45 MARKED REGRESSION")){serial_text("M45_MARKED_WRITE_FAIL\n");test_exit(0x32);}serial_text("M45_MARKED_WRITE_REGRESSION_PASS\n");}for(unsigned i=0;i<storage_count();++i){console_text("Partition inspection disk ");console_decimal(i);console_text("\n");unsigned status=storage_partitions(i);console_text("Partition status: ");console_decimal(status);console_text("\n");}nvme_diagnostics();virtio_diagnostics();usb_storage_diagnostics();serial_text("M45_DISCOVERY_PASS\n");test_exit(0x10);}
#else
    if(mode==1||mode==4){
        uint8_t ignored[512];if(s||block_read(M4_SECTORS,ignored)!=B_DENIED||load()!=B_OK){serial_text("M4_STORAGE_UNAVAILABLE_OR_INVALID\n");test_exit(0x32);}
        uint64_t before;memcpy(&before,record+8,8);if(filesystem||(s=fs_selftest(before))){serial_text("M4_FILESYSTEM_FAIL status=");serial_hex(filesystem?filesystem:s);serial_text("\n");test_exit(0x32);}s=store("ENGINE OS M4 DURABLE RECORD");
        if(s){serial_text("M4_STORAGE_IO_ERROR status=");serial_hex(s);serial_text("\n");test_exit(0x32);}
        uint64_t after;memcpy(&after,record+8,8);if(after!=before+1){serial_text("M4_RECORD_CHECK_FAIL\n");test_exit(0x32);}
        serial_text("M4_STORAGE_PASS generation=");serial_hex(after);block_info i=block_stats();serial_text(" commands=");serial_hex(i.commands);serial_text(" dma_bytes=");serial_hex(i.dma_pages*4096);serial_text(" irqs=");serial_hex(i.irqs);serial_text(" wait_halts=");serial_hex(i.wait_halts);serial_text("\n");test_exit(0x10);
    }
#endif
    console_begin(boot);
#ifdef ENGINE_M5
    m5_welcome();
#endif
    console_text("Own kernel storage and interrupt-driven input.\n");
#ifdef ENGINE_M45
    storage_inventory();
#ifdef ENGINE_M5
    console_text(filesystem?"Native filesystem unavailable or read-only. Repair boot does not repair damaged user data.\n":"Native filesystem mounted; volume shows its persistence mode.\n");
#else
    console_text(filesystem?"No writable development filesystem mounted. Disks remain inspectable.\n":"Ext2 development volume mounted.\n");
#endif
    console_text("Use disks, parts N and hardware to inspect devices.\n");
#else
    console_text(s?"No marked data disk. Storage writes disabled.\n":"Validated disposable data volume ready.\n");
#endif
#ifndef ENGINE_M45
    console_text(filesystem?"Filesystem unavailable or read-only.\n":"Ext2 mounted. Files persist across reboot.\n");
#endif
#ifndef ENGINE_M45
    unsigned usb=usb_init();
#endif
    serial_text("M4_USB_INIT status=");serial_hex(usb);serial_text("\n");input_init();serial_text("M4_CONSOLE_READY\n");console_text("engine> ");
    char line[128];unsigned length=0;int overflow=0;
    for(;;){__asm__ volatile("cli");
#ifdef ENGINE_M45
        if(usb_work_pending()){usb_process_changes();continue;}
#endif
        int c=input_pop();if(c<0){__asm__ volatile("sti; hlt":::"memory");continue;}__asm__ volatile("sti");
        if(c==256){usb_mouse_draw();continue;}
        if(c==8){if(length&&!overflow){--length;console_text("\b");}continue;}
        if(c==10){line[length]=0;console_text("\n");
            if(overflow)console_text("Command too long.\n");
            else if(equal(line,"help")){console_text("help info show store TEXT sync halt\nls PATH | cat PATH | mkdir PATH | rm PATH\nwrite PATH TEXT | mv FROM TO\n");
#ifdef ENGINE_M45
                #ifdef ENGINE_M5
                console_text("plan N | install N ERASE-SECTORS | cancel\nrepairplan N | repair N REPAIR-SECTORS | volume\n");
                #endif
                console_text("disks | hardware | parts N | fatls N P | fatcat N P PATH\nextls N P PATH | extcat N P PATH\n");
#endif
            }
#ifdef ENGINE_M45
            #ifdef ENGINE_M5
            else if(m5_command(line)){}
            #endif
            else if(equal(line,"disks"))storage_inventory();
            else if(equal(line,"hardware"))hardware_inventory();
            else if(length>6&&!memcmp(line,"parts ",6)){unsigned n=0,i=6;while(line[i]>='0'&&line[i]<='9'&&n<STORAGE_MAX)n=n*10+(unsigned)(line[i++]-'0');unsigned status=line[i]?B_DENIED:storage_partitions(n);console_text("Partition status: ");console_decimal(status);console_text("\n");}
            else if(length>6&&!memcmp(line,"extls ",6)){const char *args=line+6;unsigned disk,part,status=B_DENIED;if(number(&args,&disk)&&number(&args,&part)&&*args)status=ext_list(disk,part,args);console_text("Ext2 status: ");console_decimal(status);console_text("\n");}
            else if(length>7&&!memcmp(line,"extcat ",7)){const char *args=line+7;unsigned disk,part,size=0,status=B_DENIED;char data[4097];if(number(&args,&disk)&&number(&args,&part)&&*args)status=ext_read(disk,part,args,data,4096,&size);if(!status){data[size]=0;console_text(data);console_text("\n");}console_text("Ext2 status: ");console_decimal(status);console_text("\n");}
            else if(length>6&&!memcmp(line,"fatls ",6)){const char *args=line+6;unsigned disk,part,status=B_DENIED;if(number(&args,&disk)&&number(&args,&part)&&!*args)status=fat_list(disk,part);console_text("FAT status: ");console_decimal(status);console_text("\n");}
            else if(length>7&&!memcmp(line,"fatcat ",7)){const char *args=line+7;unsigned disk,part,size=0,status=B_DENIED;char data[4097];if(number(&args,&disk)&&number(&args,&part)&&*args)status=fat_read(disk,part,args,data,4096,&size);if(!status){data[size]=0;console_text(data);console_text("\n");}console_text("FAT status: ");console_decimal(status);console_text("\n");}
#endif
            else if(equal(line,"ls")){console_text("Status: ");console_hex(fs_list("/"));console_text("\n");}
            else if(length>3&&!memcmp(line,"ls ",3)){console_text("Status: ");console_hex(fs_list(line+3));console_text("\n");}
            else if(length>4&&!memcmp(line,"cat ",4)){char data[4097];unsigned size=0,status=fs_read(line+4,data,4096,&size);if(!status){data[size]=0;console_text(data);console_text("\n");}else{console_text("Read status: ");console_hex(status);console_text("\n");}}
            else if(length>6&&!memcmp(line,"mkdir ",6)){console_text("Mkdir status: ");console_hex(fs_mkdir(line+6));console_text("\n");}
            else if(length>3&&!memcmp(line,"rm ",3)){console_text("Remove status: ");console_hex(fs_remove(line+3));console_text("\n");}
            else if(length>6&&!memcmp(line,"write ",6)){unsigned i=6;while(line[i]&&line[i]!=' ')++i;if(line[i]){line[i++]=0;console_text("Write status: ");console_hex(fs_write(line+6,line+i,length-i));console_text("\n");}else console_text("Usage: write PATH TEXT\n");}
            else if(length>3&&!memcmp(line,"mv ",3)){unsigned i=3;while(line[i]&&line[i]!=' ')++i;if(line[i]){line[i++]=0;console_text("Rename status: ");console_hex(fs_rename(line+3,line+i));console_text("\n");}else console_text("Usage: mv FROM TO\n");}
            else if(equal(line,"info")){info();usb_info();}else if(equal(line,"show"))show();
            else if(length>=6&&store_prefix(line)){unsigned result=store(line+6);console_text(result?"Store failed: ":"Stored and flushed: ");console_hex(result);console_text("\n");}
            else if(equal(line,"sync")){console_text("Flush status: ");console_hex(block_flush());console_text("\n");}
            else if(equal(line,"halt")){console_text("CPU halted.\n");out8(0x21,255);out8(0x3f9,0);halt_forever();}
            else if(length)console_text("Unknown command. Type help.\n");
            length=0;overflow=0;console_text("engine> ");
        }else if(c>=32&&c<=126){if(length<120&&!overflow){line[length++]=(char)c;char echo[2]={(char)c,0};console_text(echo);}else overflow=1;}
    }
}
