#ifndef ENGINE_M3_H
#define ENGINE_M3_H
#include <engine/process.h>
#define M3_VM (USER_BASE+0x80000)
#define M3_PAGES 16
#define M3_THREADS 8
#define M3_CPUS 4
typedef struct {uint64_t kernel_cr3,entry,count;const uint64_t *pages,*vas,*flags;const uint8_t *data;const os_boot_info_v0 *boot;} m3_image;
extern _Atomic unsigned m3_active;
NORETURN void m3_start(const m3_image*,void (*)(void));
trap_frame *m3_trap(trap_frame*);
void m3_tick(void);
unsigned m3_cpu_interrupt(uint64_t*);
unsigned m3_smp_start(const os_boot_info_v0*,uint64_t);
void m3_smp_test(void);
unsigned m3_smp_pressure(unsigned);
void m3_smp_stop(void);
unsigned m3_cpu(void);
uint64_t m3_clock_ticks(void);
void m3_clock_probe(void);
void m3_smp_launch(unsigned);
NORETURN void m3_ap_enter(void);
NORETURN void m3_ap_return(unsigned);
uint64_t m3_smp_pages(void);
void pages_limit(uint64_t);
uint64_t pages_window_peak(unsigned);
uint64_t kernel_low_pt(void);
void interrupts_ap(unsigned,uint64_t,uint64_t);
static inline uint64_t m3_tsc(void){uint32_t a,d;__asm__ volatile("lfence; rdtsc":"=a"(a),"=d"(d)::"memory");return ((uint64_t)d<<32)|a;}
#endif
