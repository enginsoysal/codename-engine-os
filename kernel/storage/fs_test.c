#include <engine/fs.h>
static uint8_t *payload,*copy;
static unsigned run(uint64_t before){
    const char *path="/Users/test/Documents/reboot.bin";unsigned size=0,s;uint64_t value=0;
    if(before&&(fs_read(path,&value,8,&size)||size!=8||value!=before))return F_INVALID;
    value=before+1;if((s=fs_write(path,&value,8)))return s;
    for(unsigned n=0;n<FS_MAX_FILE;++n)payload[n]=(uint8_t)(n*17+before);
    if(fs_write("/Users/test/Documents/large.bin",payload,FS_MAX_FILE)||fs_read("/Users/test/Documents/large.bin",copy,FS_MAX_FILE,&size)||size!=FS_MAX_FILE||memcmp(copy,payload,size))return F_INVALID;
    s=fs_mkdir("/Users/test/Documents/M4");if(s&&s!=F_EXISTS)return s;
    if(fs_write("/Users/test/Documents/M4/Case","A",1)||fs_write("/Users/test/Documents/M4/case","b",1)||fs_write("/Users/test/Documents/M4/caf\xc3\xa9","C",1)||fs_write("/Users/test/Documents/M4/cafe\xcc\x81","D",1))return F_INVALID;
    if(fs_read("/Users/test/Documents/M4/Case",copy,FS_MAX_FILE,&size)||size!=1||copy[0]!='A'||fs_read("/Users/test/Documents/M4/case",copy,FS_MAX_FILE,&size)||copy[0]!='b')return F_INVALID;
    if(fs_write("/../escape","x",1)!=F_INVALID||fs_write("/Users/test/Documents/M4/\xc0\xaf","x",1)!=F_INVALID||fs_write(path,payload,FS_MAX_FILE+1)!=F_LIMIT)return F_INVALID;
    uint32_t h[8],extra;for(unsigned j=0;j<8;++j)if(fs_open(path,1,1,h+j))return F_INVALID;
    if(fs_open(path,1,1,&extra)!=F_LIMIT||fs_open(path,2,7,&extra)!=F_BUSY||fs_write(path,&value,8)!=F_BUSY||fs_remove(path)!=F_BUSY||fs_rename(path,"/Users/test/Documents/blocked")!=F_BUSY)return F_INVALID;
    for(unsigned j=0;j<8;++j)if(fs_close(h[j]))return F_INVALID;
    if(fs_read_handle(h[0],copy,FS_MAX_FILE,&size)!=F_INVALID||fs_close(h[0])!=F_INVALID)return F_INVALID;
    if(fs_open(path,1,0,&extra)||fs_read(path,copy,FS_MAX_FILE,&size)!=F_BUSY||fs_close(extra))return F_INVALID;
    if(fs_write("/Users/test/Documents/from.txt","first",5)||fs_rename("/Users/test/Documents/from.txt","/Users/test/Documents/M4/to.txt")||fs_read("/Users/test/Documents/from.txt",copy,FS_MAX_FILE,&size)!=F_NOTFOUND||fs_read("/Users/test/Documents/M4/to.txt",copy,FS_MAX_FILE,&size)||size!=5||memcmp(copy,"first",5)||fs_remove("/Users/test/Documents/M4/to.txt"))return F_INVALID;
    if(fs_check())return F_INVALID;serial_text("M4_EXT2_VFS_PASS generation=");serial_hex(value);serial_text(" max_file_bytes=");serial_hex(FS_MAX_FILE);serial_text("\n");return 0;
}
unsigned fs_selftest(uint64_t before){
    uint64_t pages[26];unsigned allocated=0;uint64_t baseline=pages_stats().current_pages;
    for(;allocated<26;++allocated)if(!(pages[allocated]=page_alloc()))break;
    if(allocated!=26){while(allocated)page_free(pages[--allocated]);return F_LIMIT;}
    uint64_t *pd=(uint64_t*)(uintptr_t)pages[0],*pt=(uint64_t*)(uintptr_t)pages[1];pd[0]=pages[1]|3;
    for(unsigned i=0;i<24;++i)pt[i]=pages[i+2]|UINT64_C(0x8000000000000003);
    uint64_t *branch=(uint64_t*)(uintptr_t)kernel_stack_branch();branch[2]=pages[0]|3;
    payload=(uint8_t*)(uintptr_t)UINT64_C(0xffff800080000000);copy=payload+FS_MAX_FILE;
    unsigned result=run(before);branch[2]=0;uint64_t cr3;__asm__ volatile("movq %%cr3,%0;movq %0,%%cr3":"=r"(cr3)::"memory");
    for(unsigned i=0;i<26;++i)page_free(pages[i]);payload=copy=0;
    if(pages_stats().current_pages!=baseline)return F_INVALID;serial_text("M4_FS_TEST_BUFFERS_RECLAIMED\n");return result;
}
