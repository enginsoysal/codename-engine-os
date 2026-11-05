.section .rodata
.p2align 4
.globl ap_trampoline,ap_trampoline_end
ap_trampoline:
.code16
cli
cld
xorw %ax,%ax
movw %ax,%ds
movw %ax,%es
movw %ax,%ss
lgdtl 0x8000+(ap_gdtr-ap_trampoline)
movl %cr0,%eax
orl $1,%eax
movl %eax,%cr0
ljmpl $8,$0x8000+(ap_protected-ap_trampoline)
.code32
ap_protected:
movw $16,%ax
movw %ax,%ds
movw %ax,%es
movw %ax,%ss
movl %cr4,%eax
orl $0x20,%eax
movl %eax,%cr4
movl 0x8f00,%eax
movl %eax,%cr3
movl $0xc0000080,%ecx
rdmsr
orl $0x900,%eax
wrmsr
movl %cr0,%eax
orl $0x80010000,%eax
movl %eax,%cr0
ljmpl $24,$0x8000+(ap_long-ap_trampoline)
.code64
ap_long:
movq 0x8f08,%rsp
xorq %rbp,%rbp
movq 0x8f10,%rdi
movq 0x8f18,%rax
call *%rax
1: cli
hlt
jmp 1b
.p2align 3
ap_gdt:
.quad 0,0x00cf9a000000ffff,0x00cf92000000ffff,0x00af9a000000ffff
ap_gdtr:
.word 31
.long 0x8000+(ap_gdt-ap_trampoline)
ap_trampoline_end:
.section .note.GNU-stack,"",@progbits

.text
.globl m3_invalidate_shared
.type m3_invalidate_shared,@function
m3_invalidate_shared:
movabsq $0xffff800000080000,%rax
invlpg (%rax)
ret
.size m3_invalidate_shared,.-m3_invalidate_shared
