#include <engine/process.h>
#include <engine/boot/validate_v0.h>
#define WORDS 4096u /* Manage at most 1 GiB: two 32 KiB bitmaps, no per-page heap objects. */
static uint64_t free_bits[WORDS],managed[WORDS];
static unsigned cursor;
static page_stats stats;
static NORETURN void corrupt(void) {serial_text("ALLOCATOR_CORRUPTION\n");test_exit(0x31);}
void pages_init(const os_boot_info_v0 *boot) {
    memset(free_bits,0,sizeof(free_bits));memset(managed,0,sizeof(managed));memset(&stats,0,sizeof(stats));cursor=0;
    for(uint64_t off=0;off<boot->memory_map.byte_length;off+=boot->memory_descriptor_size) {
        const uint8_t *d=(const uint8_t*)(uintptr_t)(boot->memory_map.physical_address+off);
        if(os_read32(d)!=7)continue; /* Only conventional memory, never loader/firmware/reserved data. */
        uint64_t start=os_read64(d+8)/4096,pages=os_read64(d+24);
        if(start>=WORDS*64u)continue;
        if(pages>WORDS*64u-start)pages=WORDS*64u-start;
        for(uint64_t p=start;p<start+pages;++p)if(p>=256) {
            uint64_t bit=UINT64_C(1)<<(p%64);free_bits[p/64]|=bit;managed[p/64]|=bit;++stats.total_pages;
        }
    }
    serial_text("CONVENTIONAL_PAGES=");serial_hex(stats.total_pages);serial_text("\n");
}
uint64_t page_alloc(void) {
    for(unsigned n=0;n<WORDS;++n) {
        unsigned word=(cursor+n)%WORDS;uint64_t value=free_bits[word];
        if(value) {
            unsigned bit=(unsigned)__builtin_ctzll(value);free_bits[word]&=~(UINT64_C(1)<<bit);cursor=word;
            ++stats.current_pages;if(stats.current_pages>stats.peak_pages)stats.peak_pages=stats.current_pages;
            uint64_t address=((uint64_t)word*64+bit)*4096;memset((void*)(uintptr_t)address,0,4096);return address;
        }
    }
    ++stats.allocation_failures;return 0;
}
void page_free(uint64_t address) {
    if((address&4095)||address>=UINT64_C(0x40000000))corrupt();
    uint64_t p=address/4096,bit=UINT64_C(1)<<(p%64);unsigned w=(unsigned)(p/64);
    if(!(managed[w]&bit)||(free_bits[w]&bit)||!stats.current_pages)corrupt();
    free_bits[w]|=bit;--stats.current_pages;if(w<cursor)cursor=w;
}
page_stats pages_stats(void) {return stats;}
