#include <engine/storage.h>
static unsigned run(uint8_t *data){
    unsigned size=0,status=ext_list(0,0,"/");if(status)return status;
    if(ext_read(0,0,"/BIG.BIN",data,65536,&size)||size!=65536)return B_IO;
    for(unsigned i=0;i<size;++i)if(data[i]!=(i>=32768&&i<36864?0:(uint8_t)(i*37+11)))return B_IO;
    if(ext_read(0,0,"/Docs/R\xc3\xa9sum\xc3\xa9.txt",data,65536,&size)||size!=65536)return B_IO;
    for(unsigned i=0;i<size;++i)if(data[i]!=(i>=32768&&i<36864?0:(uint8_t)(i*37+11)))return B_IO;
    if(ext_read(0,0,"/Docs/\xc0\xaf",data,65536,&size)!=B_DENIED||ext_read(0,0,"/big.bin",data,65536,&size)!=B_ABSENT||ext_read(0,0,"/../BIG.BIN",data,65536,&size)!=B_DENIED||ext_read(0,0,"/BIG.BIN",data,65535,&size)!=B_UNSUPPORTED||ext_read(0,0,"/link",data,65536,&size)!=B_UNSUPPORTED)return B_IO;
    return B_OK;
}
unsigned ext_selftest(void){
    uint64_t pages[18],baseline=pages_stats().current_pages;unsigned allocated=0;for(;allocated<18;++allocated)if(!(pages[allocated]=page_alloc()))break;if(allocated!=18){while(allocated)page_free(pages[--allocated]);return B_IO;}
    uint64_t *pd=(void*)(uintptr_t)pages[0],*pt=(void*)(uintptr_t)pages[1],*branch=(void*)(uintptr_t)kernel_stack_branch();if(branch[2]){for(unsigned i=0;i<18;++i)page_free(pages[i]);return B_IO;}pd[0]=pages[1]|3;for(unsigned i=0;i<16;++i)pt[i]=pages[i+2]|UINT64_C(0x8000000000000003);branch[2]=pages[0]|3;
    unsigned status=run((void*)(uintptr_t)UINT64_C(0xffff800080000000));branch[2]=0;uint64_t cr3;__asm__ volatile("movq %%cr3,%0;movq %0,%%cr3":"=r"(cr3)::"memory");for(unsigned i=0;i<18;++i)page_free(pages[i]);if(pages_stats().current_pages!=baseline)return B_IO;serial_text("M45_EXT2_TEST_BUFFERS_RECLAIMED\n");return status;
}
