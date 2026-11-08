/* Search only bounded BIOS hard-disk handles. The committed activation identity
   must match this media; zero/multiple matches remain in setup. */
.section .start,"ax"
.code16
chain75:
    movb $1,%ah
    int $0x16
    jz 1f
    cmpb $1,%ah
    jne 1f
    xorb %ah,%ah
    int $0x16
    ret
1:  movb $0x80,(chain75_drive-stage2_entry)
    movb $0,(chain75_count-stage2_entry)
2:  movb (chain75_drive-stage2_entry),%dl
    cmpb (drive-stage2_entry),%dl
    je 5f
    movw $(chain75_params-stage2_entry),%si
    movw $30,(%si)
    movb $0x48,%ah
    int $0x13
    jc 5f
    cmpw $512,(chain75_params-stage2_entry+24)
    jne 5f
    movl $41,(chain75_packet-stage2_entry+8)
    movw $(chain75_packet-stage2_entry),%si
    movb (chain75_drive-stage2_entry),%dl
    movb $0x42,%ah
    int $0x13
    jc 5f
    xorw %ax,%ax
    movw %ax,%es
    cmpl $0x49474e45,%es:0x6000
    jne 5f
    cmpl $0x3649454e,%es:0x6004
    jne 5f
    cmpl $2,%es:0x6048
    jne 5f
    movw $(chain75_build-stage2_entry),%si
    movw $0x6018,%di
    movw $32,%cx
    repe cmpsb
    jne 5f
    incb (chain75_count-stage2_entry)
    movb (chain75_drive-stage2_entry),%al
    movb %al,(chain75_target-stage2_entry)
5:  incb (chain75_drive-stage2_entry)
    cmpb $0x90,(chain75_drive-stage2_entry)
    jb 2b
    cmpb $1,(chain75_count-stage2_entry)
    jne 8f
    movl $0,(chain75_packet-stage2_entry+8)
    movw $(chain75_packet-stage2_entry),%si
    movb (chain75_target-stage2_entry),%dl
    movb $0x42,%ah
    int $0x13
    jc 8f
    xorw %ax,%ax
    movw %ax,%es
    cmpw $0xaa55,%es:0x61fe
    jne 8f
    movb (chain75_target-stage2_entry),%dl
    cli
    movw %ax,%ds
    movw $0x6000,%si
    movw $0x7c00,%di
    movw $256,%cx
    rep movsw
    ljmp $0,$0x7c00
8:  ret
.p2align 2
chain75_packet:
    .byte 16,0
    .word 1,0x6000,0
    .quad 41
chain75_params: .word 30
    .space 28
chain75_drive: .byte 0
chain75_count: .byte 0
chain75_target: .byte 0
chain75_build: .ascii BIOS_BUILD75
