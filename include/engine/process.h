#ifndef ENGINE_PROCESS_H
#define ENGINE_PROCESS_H
#include <engine/base.h>
#include <engine/boot/boot_v0.h>
#define USER_BASE UINT64_C(0x100000000)
#define USER_DATA (USER_BASE+0x8000)
#define USER_STACK (USER_BASE+0x10000)
#define USER_TOP (USER_STACK+4096)
#define USER_SHARED (USER_BASE+0x20000)
#define USER_LIMIT (USER_BASE+0x200000)
#define KERNEL_STACK UINT64_C(0xffff800000001000)
#define KERNEL_STACK_TOP (KERNEL_STACK+16384)
typedef struct {
    uint64_t r15,r14,r13,r12,r11,r10,r9,r8,rbp,rdi,rsi,rdx,rcx,rbx,rax;
    uint64_t vector,error,rip,cs,rflags,rsp,ss;
} trap_frame;
_Static_assert(sizeof(trap_frame)==176,"x64 user trap layout");
typedef struct {uint64_t current_pages,peak_pages,total_pages,allocation_failures;} page_stats;
enum execution_abi { EXEC_NATIVE64=1,EXEC_WINDOWS_X86,EXEC_WINDOWS_X64,EXEC_WIN16,
                     EXEC_LINUX_X64,EXEC_DARWIN_X64 };
uint64_t page_alloc(void);
#ifdef ENGINE_M9
uint64_t pages_alloc_run(unsigned);
#endif
void page_free(uint64_t);
void pages_init(const os_boot_info_v0*);
page_stats pages_stats(void);
uint64_t kernel_space_init(const os_boot_info_v0*);
uint64_t kernel_pd(unsigned);
uint64_t kernel_stack_branch(void);
void interrupts_set_kernel_stack(uint64_t);
NORETURN void process_start(const os_boot_info_v0*,unsigned);
trap_frame *process_trap(trap_frame*);
NORETURN void arch_enter_user(trap_frame*);
NORETURN void arch_schedule_first(void);
#endif
