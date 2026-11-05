#include "bios-config.h"
.section .start,"ax"
.code16
.global stage2_entry
stage2_entry:
    cli
    movw $0x1000,%ax
    movw %ax,%ds
    movw %ax,%es
    movw $0x6000,%ax
    movw %ax,%ss
    movw $0xf000,%sp
    cld
    movb %dl,(drive-stage2_entry)
    inb $0x92,%al
    andb $0xfe,%al
    orb $2,%al
    outb %al,$0x92
    sti
    xorw %ax,%ax
    movw %ax,%es
    movl $0x32454256,%es:0x4000
    movw $0x4000,%di
    movw $0x4f00,%ax
    int $0x10
    cmpw $0x004f,%ax
    jne no_video
    xorw %ax,%ax
    movw %ax,%es
    movw $0x4200,%di
    movw $BIOS_VIDEO_MODE,%cx
    movw $0x4f01,%ax
    int $0x10
    cmpw $0x004f,%ax
    jne no_video
    cmpb $32,%es:0x4219
    jne no_video
    movw $(0x4000+BIOS_VIDEO_MODE),%bx
    movw $0x4f02,%ax
    int $0x10
    cmpw $0x004f,%ax
    jne no_video
    movb $1,(bios_video-stage2_entry)
no_video:
    xorl %ebx,%ebx
    movw $0x5400,%di
e820_next:
    incw (iterations-stage2_entry)
    cmpw $128,(iterations-stage2_entry)
    ja failed
    xorw %ax,%ax
    movw %ax,%es
    movl $1,%es:20(%di)
    movl $0xe820,%eax
    movl $0x534d4150,%edx
    movl $24,%ecx
    int $0x15
    jc e820_done
    cmpl $0x534d4150,%eax
    jne failed
    cmpl $20,%ecx
    jb failed
    testb $1,%es:20(%di)
    jz e820_skip
    incw (bios_e820_count-stage2_entry)
    addw $24,%di
    cmpw $64,(bios_e820_count-stage2_entry)
    jb e820_skip
    testl %ebx,%ebx
    jnz failed
    jmp e820_done
e820_skip:
    testl %ebx,%ebx
    jnz e820_next
e820_done:
    cmpw $0,(bios_e820_count-stage2_entry)
    je failed
    movw (bios_e820_count-stage2_entry),%cx
    movw $0x5400,%si
kernel_range_next:
    cmpl $1,%es:16(%si)
    jne kernel_range_skip
    cmpl $0,%es:4(%si)
    jne kernel_range_skip
    movl %es:(%si),%eax
    cmpl $0x200000,%eax
    ja kernel_range_skip
    cmpl $0,%es:12(%si)
    jne kernel_range_ok
    addl %es:8(%si),%eax
    jc kernel_range_ok
    cmpl $(0x200000+BIOS_KERNEL_BYTES),%eax
    jae kernel_range_ok
kernel_range_skip:
    addw $24,%si
    loop kernel_range_next
    movw $(memory_error-stage2_entry),%si
    jmp report_failed
kernel_range_ok:
    movw $0x1000,%ax
    movw %ax,%ds
load_next:
    movw $(packet-stage2_entry),%si
    movb (drive-stage2_entry),%dl
    movb $0x42,%ah
    int $0x13
    jc failed
    call unreal
    cld
    movl $0x20000,%esi
    movl %cs:(destination-stage2_entry),%edi
    movl $8192,%ecx
    .byte 0x67
    rep movsl
    movw $0x1000,%ax
    movw %ax,%ds
    movw %ax,%es
    addl $32768,(destination-stage2_entry)
    addl $BIOS_STAGE_BLOCKS,(packet_lba-stage2_entry)
    adcl $0,(packet_lba+4-stage2_entry)
    decw (chunks-stage2_entry)
    sti
    jnz load_next
    cli
    lgdt (gdtr-stage2_entry)
    movl %cr0,%eax
    orl $1,%eax
    movl %eax,%cr0
    ljmpl $0x18,$protected32
unreal:
    cli
    lgdt (gdtr-stage2_entry)
    movl %cr0,%eax
    orl $1,%eax
    movl %eax,%cr0
    ljmp $0x20,$(unreal_protected-stage2_entry)
unreal_protected:
    movw $0x10,%ax
    movw %ax,%ds
    movw %ax,%es
    movl %cr0,%eax
    andl $0xfffffffe,%eax
    movl %eax,%cr0
    ljmp $0x1000,$(unreal_real-stage2_entry)
unreal_real:
    ret
failed:
    movw $(boot_error-stage2_entry),%si
report_failed:
    call print_error
    movw $0x3f8,%dx
    movb $'F',%al
    outb %al,%dx
    movw $0xf4,%dx
    movl $0x23,%eax
    outl %eax,%dx
    cli
1:  hlt
    jmp 1b
print_error:
    cld
2:  lodsb
    testb %al,%al
    jz 4f
    pushw %ax
    pushw %cx
    movw $0x3fd,%dx
    movw $0xffff,%cx
3:  inb %dx,%al
    testb $32,%al
    jnz 5f
    loop 3b
5:  popw %cx
    popw %ax
    movw $0x3f8,%dx
    outb %al,%dx
    jmp 2b
4:  ret
.code32
protected32:
    movw $0x10,%ax
    movw %ax,%ds
    movw %ax,%es
    movw %ax,%ss
    movl $0x6f000,%esp
    call bios_prepare
    movl %eax,%ebx
    movl $0x100000,%eax
    movl %eax,%cr3
    movl %cr4,%eax
    orl $0x20,%eax
    movl %eax,%cr4
    movl $0xc0000080,%ecx
    rdmsr
    orl $0x100,%eax
    wrmsr
    movl %cr0,%eax
    orl $0x80000001,%eax
    movl %eax,%cr0
    ljmpl $0x8,$long64
.code64
long64:
    movw $0x10,%ax
    movw %ax,%ds
    movw %ax,%es
    movw %ax,%ss
    movq $0x6eff8,%rsp
    xorq %rbp,%rbp
    movl %ebx,%edi
    movq $BIOS_KERNEL_ENTRY,%rax
    jmp *%rax
.p2align 3
gdt:
    .quad 0,0x00af9a000000ffff,0x00cf92000000ffff,0x00cf9a000000ffff,0x00009a010000ffff
gdtr: .word 39
    .long gdt
.p2align 2
packet: .byte 16,0
    .word BIOS_STAGE_BLOCKS
    .word 0,0x2000
packet_lba: .quad BIOS_KERNEL_LBA
destination: .long 0x200000
chunks: .word BIOS_KERNEL_CHUNKS
drive: .byte 0
iterations: .word 0
boot_error: .asciz "BIOS_REJECT: firmware disk/map failure\r\n"
memory_error: .asciz "BIOS_REJECT: kernel region unavailable before copy\r\n"
.global bios_e820_count,bios_video
bios_e820_count: .word 0
bios_video: .byte 0
.section .note.GNU-stack,"",@progbits
