ENTRY(stage2_entry)
PHDRS { boot PT_LOAD FLAGS(7); }
SECTIONS {
 . = 0x10000;
 .text : { *(.start) *(.text*) } :boot
 .rodata : { *(.rodata*) } :boot
 .data : { *(.data*) } :boot
 .bss : { *(.bss*) *(COMMON) } :boot
 ASSERT(. <= 0x18000, "BIOS stage exceeds 32 KiB")
 /DISCARD/ : { *(.comment) *(.eh_frame*) *(.note*) }
}
