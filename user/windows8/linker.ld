ENTRY(windows8_main)
PHDRS { text PT_LOAD FLAGS(5); rodata PT_LOAD FLAGS(4); data PT_LOAD FLAGS(6); }
SECTIONS {
 . = 0x100000000;
 .text : { *(.text .text.* .ltext .ltext.*) } :text
 . = ALIGN(4096);
 .rodata : { *(.rodata .rodata.* .lrodata .lrodata.*) } :rodata
 . = ALIGN(4096);
 .data : { *(.data .data.* .ldata .ldata.*) } :data
 .bss : { *(COMMON) *(.bss .bss.* .lbss .lbss.*) } :data
 ASSERT(. <= 0x100100000,"Windows runtime exceeds its 1-MiB bound")
 /DISCARD/ : { *(.eh_frame*) *(.comment*) *(.note*) }
}
