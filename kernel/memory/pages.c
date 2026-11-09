#include <engine/process.h>
#include <engine/boot/validate_v0.h>
#define WORDS 4096u /* Manage at most 1 GiB: two 32 KiB bitmaps, no per-page heap objects. */
static uint64_t free_bits[WORDS],managed[WORDS];
static unsigned cursor;
#ifdef ENGINE_M3
static unsigned allocator_lock;
static uint64_t admission_limit=UINT64_MAX;
static void lock(void){while(__atomic_exchange_n(&allocator_lock,1,__ATOMIC_ACQUIRE))__asm__ volatile("pause");}
static void unlock(void){__atomic_store_n(&allocator_lock,0,__ATOMIC_RELEASE);}
void pages_limit(uint64_t value){lock();admission_limit=value?value:UINT64_MAX;unlock();}
#else
static void lock(void){} static void unlock(void){}
#endif
static page_stats stats;
#ifdef ENGINE_M3
static uint64_t window_peak;
uint64_t pages_window_peak(unsigned reset){lock();if(reset)window_peak=stats.current_pages;uint64_t result=window_peak;unlock();return result;}
#endif
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
    lock();
#ifdef ENGINE_M3
    if(stats.current_pages>=admission_limit){++stats.allocation_failures;unlock();return 0;}
#endif
    for(unsigned n=0;n<WORDS;++n) {
        unsigned word=(cursor+n)%WORDS;uint64_t value=free_bits[word];
        if(value) {
            unsigned bit=(unsigned)__builtin_ctzll(value);free_bits[word]&=~(UINT64_C(1)<<bit);cursor=word;
            ++stats.current_pages;if(stats.current_pages>stats.peak_pages)stats.peak_pages=stats.current_pages;
#ifdef ENGINE_M3
            if(stats.current_pages>window_peak)window_peak=stats.current_pages;
#endif
            uint64_t address=((uint64_t)word*64+bit)*4096;unlock();memset((void*)(uintptr_t)address,0,4096);return address;
        }
    }
    ++stats.allocation_failures;unlock();return 0;
}
#ifdef ENGINE_M9
/* Optional contiguous graphics storage; all pages retain normal allocator ownership. */
uint64_t pages_alloc_run(unsigned count) {
    if(!count||count>8192)return 0;
    lock();
    if(stats.current_pages>admission_limit||count>admission_limit-stats.current_pages){++stats.allocation_failures;unlock();return 0;}
    unsigned run=0,start=0;
    for(unsigned page=256;page<WORDS*64u;++page){
        if(free_bits[page/64]&(UINT64_C(1)<<(page%64))){if(!run)start=page;if(++run==count){
            for(unsigned at=start;at<start+count;++at)free_bits[at/64]&=~(UINT64_C(1)<<(at%64));
            stats.current_pages+=count;if(stats.current_pages>stats.peak_pages)stats.peak_pages=stats.current_pages;
            if(stats.current_pages>window_peak)window_peak=stats.current_pages;
            unlock();uint64_t address=(uint64_t)start*4096;memset((void*)(uintptr_t)address,0,(uint64_t)count*4096);return address;
        }}else run=0;
    }
    ++stats.allocation_failures;unlock();return 0;
}
#endif
void page_free(uint64_t address) {
    lock();
    if((address&4095)||address>=UINT64_C(0x40000000))corrupt();
    uint64_t p=address/4096,bit=UINT64_C(1)<<(p%64);unsigned w=(unsigned)(p/64);
    if(!(managed[w]&bit)||(free_bits[w]&bit)||!stats.current_pages)corrupt();
    free_bits[w]|=bit;--stats.current_pages;if(w<cursor)cursor=w;unlock();
}
page_stats pages_stats(void) {lock();page_stats copy=stats;unlock();return copy;}
