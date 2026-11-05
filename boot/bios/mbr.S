#include "bios-config.h"
.section .text,"ax"
.code16
.global _start
_start:
    cli
    xorw %ax,%ax
    movw %ax,%ds
    movw %ax,%es
    movw %ax,%ss
    movw $0x7c00,%sp
    cld
    movb %dl,drive
    movw $0x3fb,%dx
    movb $0x80,%al
    outb %al,%dx
    movw $0x3f8,%dx
    movb $1,%al
    outb %al,%dx
    incw %dx
    xorb %al,%al
    outb %al,%dx
    movw $0x3fb,%dx
    movb $3,%al
    outb %al,%dx
    movw $0x3f8,%dx
    movb $'B',%al
    outb %al,%dx
    movb drive,%dl
    movw $0x55aa,%bx
    movb $0x41,%ah
    int $0x13
    jc failed
    cmpw $0xaa55,%bx
    jne failed
    testw $1,%cx
    jz failed
    movw $parameters,%si
    movb drive,%dl
    movb $0x48,%ah
    int $0x13
    jc failed
    cmpw $BIOS_SECTOR_BYTES,parameters+24
    jne failed
    sti
    movw $packet,%si
    movb drive,%dl
    movb $0x42,%ah
    int $0x13
    jc failed
    movb drive,%dl
    ljmp $0x1000,$0
failed:
    movw $0x3f8,%dx
    movb $'E',%al
    outb %al,%dx
    movw $0xf4,%dx
    movl $0x23,%eax
    outl %eax,%dx
    cli
1:  hlt
    jmp 1b
.p2align 2
packet:
    .byte 16,0
    .word BIOS_STAGE_BLOCKS
    .word 0,0x1000
    .quad BIOS_STAGE_LBA
drive: .byte 0
.p2align 1
parameters: .word 30
    .space 28
.org 446
    .space 64
    .word 0xaa55
