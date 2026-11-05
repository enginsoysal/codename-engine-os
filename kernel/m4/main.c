#include <engine/m4.h>
static uint8_t record[512];
static int valid_record(void){uint32_t length,crc;memcpy(&length,record+16,4);memcpy(&crc,record+508,4);return !memcmp(record,M4_RECORD,8)&&length<=120&&crc==m4_crc(record,508);}
static unsigned load(void){unsigned s=block_read(1,record);if(s)return s;return valid_record()?B_OK:B_IO;}
static unsigned store(const char *text){
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
NORETURN void m4_start(const os_boot_info_v0 *boot,unsigned mode){
    serial_text("M4_STORAGE_BEGIN\n");unsigned s=block_init();serial_text("M4_BLOCK_INIT status=");serial_hex(s);serial_text("\n");
    page_stats memory=pages_stats();serial_text("M4_MEMORY retained_owned_bytes=");serial_hex(memory.current_pages*4096);serial_text(" peak_owned_bytes=");serial_hex(memory.peak_pages*4096);serial_text("\n");
    if(mode==1||mode==4){
        uint8_t ignored[512];if(s||block_read(M4_SECTORS,ignored)!=B_DENIED||load()!=B_OK){serial_text("M4_STORAGE_UNAVAILABLE_OR_INVALID\n");test_exit(0x32);}
        uint64_t before;memcpy(&before,record+8,8);s=store("ENGINE OS M4 DURABLE RECORD");
        if(s){serial_text("M4_STORAGE_IO_ERROR status=");serial_hex(s);serial_text("\n");test_exit(0x32);}
        uint64_t after;memcpy(&after,record+8,8);if(after!=before+1){serial_text("M4_RECORD_CHECK_FAIL\n");test_exit(0x32);}
        serial_text("M4_STORAGE_PASS generation=");serial_hex(after);block_info i=block_stats();serial_text(" commands=");serial_hex(i.commands);serial_text(" dma_bytes=");serial_hex(i.dma_pages*4096);serial_text("\n");test_exit(0x10);
    }
    console_begin(boot);console_text("Own kernel storage and interrupt-driven input.\n");
    console_text(s?"No marked data disk. Storage writes disabled.\n":"Validated disposable data volume ready.\n");
    console_text("Persistent sector record only; ext2/VFS/USB are pending.\n");input_init();serial_text("M4_CONSOLE_READY\n");console_text("engine> ");
    char line[128];unsigned length=0;int overflow=0;
    for(;;){__asm__ volatile("cli");int c=input_pop();if(c<0){__asm__ volatile("sti; hlt":::"memory");continue;}__asm__ volatile("sti");
        if(c==8){if(length&&!overflow){--length;console_text("\b");}continue;}
        if(c==10){line[length]=0;console_text("\n");
            if(overflow)console_text("Command too long.\n");
            else if(equal(line,"help"))console_text("help  info  show  store TEXT  sync  halt\nWrites affect only sector 1 of the marked test volume.\n");
            else if(equal(line,"info"))info();else if(equal(line,"show"))show();
            else if(length>=6&&store_prefix(line)){unsigned result=store(line+6);console_text(result?"Store failed: ":"Stored and flushed: ");console_hex(result);console_text("\n");}
            else if(equal(line,"sync")){console_text("Flush status: ");console_hex(block_flush());console_text("\n");}
            else if(equal(line,"halt")){console_text("CPU halted.\n");out8(0x21,255);out8(0x3f9,0);halt_forever();}
            else if(length)console_text("Unknown command. Type help.\n");
            length=0;overflow=0;console_text("engine> ");
        }else if(c>=32&&c<=126){if(length<120&&!overflow){line[length++]=(char)c;char echo[2]={(char)c,0};console_text(echo);}else overflow=1;}
    }
}
