.text
.globl kernel_handoff
kernel_handoff:
    cli
    cld
    movq %rcx,%r10
    movq %rdx,%rax
    movq %r8,%rsp
    andq $-16,%rsp
    movq %r9,%cr3
    lgdt transition_gdtr(%rip)
    pushq $8
    leaq transition_ready(%rip),%rdx
    pushq %rdx
    lretq
transition_ready:
    movw $16,%dx
    movw %dx,%ds
    movw %dx,%es
    movw %dx,%ss
    xorw %dx,%dx
    movw %dx,%fs
    movw %dx,%gs
    xorq %rbp,%rbp
    movq %r10,%rdi
    callq *%rax
1:  cli
    hlt
    jmp 1b
.p2align 3
transition_gdt:
    .quad 0
    .quad 0x00af9a000000ffff
    .quad 0x00cf92000000ffff
transition_gdtr:
    .word 23
    .quad transition_gdt
