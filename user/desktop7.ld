ENTRY(app7_main)
PHDRS { text PT_LOAD FLAGS(5); rodata PT_LOAD FLAGS(4); }
SECTIONS {
  . = 0x100000000;
  .text : { *(.text .text.* .ltext .ltext.*) } :text
  ASSERT(. <= 0x100004000, "desktop app code exceeds 16 KiB")
  . = 0x100004000;
  .rodata : { *(.rodata .rodata.* .lrodata .lrodata.*) } :rodata
  ASSERT(. <= 0x100008000, "desktop app constants exceed 16 KiB")
  .data : { *(.data .data.* .ldata .ldata.*) }
  .bss : { *(COMMON) *(.bss .bss.* .lbss .lbss.*) }
  ASSERT(SIZEOF(.data) + SIZEOF(.bss) == 0, "desktop app private state belongs in its mailbox")
  /DISCARD/ : { *(.eh_frame*) *(.comment*) *(.note*) }
}
