#define ENGINE_M3
#define ENGINE_M9
#include "../../kernel/memory/pages.c"
__declspec(dllexport) void reset9(uint64_t base,unsigned pages){
 memset(free_bits,0,sizeof(free_bits));memset(managed,0,sizeof(managed));memset(&stats,0,sizeof(stats));cursor=0;window_peak=0;admission_limit=UINT64_MAX;allocator_lock=0;
 for(unsigned i=0;i<pages;++i){unsigned page=(unsigned)(base/4096)+i;free_bits[page/64]|=UINT64_C(1)<<(page%64);managed[page/64]|=UINT64_C(1)<<(page%64);++stats.total_pages;}
}
