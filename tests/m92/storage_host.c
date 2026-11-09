/* Actual registry implementation exercised with deterministic private callbacks. */

#include "../../kernel/storage/registry.c"
static unsigned reads,writes,flushes,result92;
void console_text(const char*s){(void)s;}void console_hex(uint64_t n){(void)n;}
void usb_hardware_inventory(unsigned b){(void)b;}
static unsigned read_test(unsigned c,uint64_t l,void*p){(void)c;(void)l;memset(p,0,512);++reads;return result92;}
static unsigned io_test(unsigned c,uint64_t l,void*p,unsigned n,unsigned w){(void)c;(void)l;(void)p;(void)n;if(w)++writes;else ++reads;return result92;}
static unsigned sync_test(unsigned c){(void)c;++flushes;return result92;}
__declspec(dllexport) unsigned storage92_cases(void){
 uint8_t data[4096];storage_disk d={STORAGE_IDE,8,0,512,1,0,4194304,0,0};
 count=reads=writes=flushes=result92=0;memset(transfers92,0,sizeof(transfers92));memset(syncs92,0,sizeof(syncs92));
 if(storage_register_io(d,read_test,io_test,sync_test)||!storage_writable(storage_get(0)))return 1;
 unsigned before=reads+writes;if(storage_transfer(0,UINT64_MAX,data,512,1)!=B_DENIED||storage_transfer(0,4194303,data,1024,1)!=B_DENIED||storage_transfer(0,0,data,513,1)!=B_DENIED||storage_transfer(0,0,data,8192,1)!=B_DENIED||storage_transfer(0,0,0,512,1)!=B_DENIED||storage_transfer(0,0,data,0,1)!=B_DENIED)return 2;
 if(reads+writes!=before||!storage_writable(storage_get(0)))return 3;
 if(storage_transfer(0,4194303,data,512,1)||writes!=1||storage_flush(0)||flushes!=1)return 4;
 d.transport=STORAGE_SCSI;d.bdf=16;d.context=1;if(storage_register(d,read_test)||storage_writable(storage_get(1)))return 5;
 if(storage_transfer(1,0,data,512,1)!=B_UNSUPPORTED||storage_flush(1)!=B_DENIED)return 6;
 d.bdf=24;d.context=2;d.sector_bytes=4096;if(storage_register_io(d,read_test,io_test,sync_test)||storage_writable(storage_get(2)))return 7;
 d.bdf=32;d.context=3;d.sector_bytes=512;if(storage_register_io(d,read_test,io_test,0)||storage_writable(storage_get(3)))return 8;
 result92=B_TIMEOUT;if(storage_transfer(0,0,data,512,1)!=B_TIMEOUT||storage_writable(storage_get(0))||storage_get(0)->status!=B_TIMEOUT)return 9;
 result92=B_OK;d.bdf=40;d.context=4;if(storage_register_io(d,read_test,io_test,sync_test))return 10;
 result92=B_IO;if(storage_flush(4)!=B_IO||storage_writable(storage_get(4)))return 11;
 return 0;
}

