.text
.globl w8_enter
w8_enter:
    pushfq
    cli
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %rsp,w8_saved_rsp(%rip)
    movq %rsi,%cr3
    movq %rdi,%rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
    addq $16,%rsp
    iretq
.globl w8_escape
w8_escape:
    cli
    movq w8_kernel_cr3(%rip),%rax
    movq %rax,%cr3
    movq w8_saved_rsp(%rip),%rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    popfq
    ret
.section .note.GNU-stack,"",@progbits
