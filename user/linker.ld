ENTRY(user_main)
PHDRS { text PT_LOAD FLAGS(5); rodata PT_LOAD FLAGS(4); data PT_LOAD FLAGS(6); }
SECTIONS {
  . = 0x100000000;
  .text : { *(.text .text.* .ltext .ltext.*) } :text
  ASSERT(. <= 0x100004000, "user text exceeds 16 KiB")
  . = 0x100004000;
  .rodata : { *(.rodata .rodata.* .lrodata .lrodata.*) } :rodata
  ASSERT(. <= 0x100008000, "user rodata exceeds 16 KiB")
  . = 0x100008000;
  .data : { *(.data .data.* .ldata .ldata.*) } :data
  .bss : { *(COMMON) *(.bss .bss.* .lbss .lbss.*) } :data
  ASSERT(. <= 0x100009000, "user private data exceeds 4 KiB")
  /DISCARD/ : { *(.eh_frame*) *(.comment*) *(.note*) }
}
