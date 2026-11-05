ENTRY(kernel_main)
PHDRS { text PT_LOAD FLAGS(5); rodata PT_LOAD FLAGS(4); data PT_LOAD FLAGS(6); }
SECTIONS {
  . = 0x200000;
  __text_start = .;
  .text : { *(.text .text.*) } :text
  . = ALIGN(4096);
  __text_end = .;
  __rodata_start = .;
  .rodata : { *(.rodata .rodata.*) } :rodata
  . = ALIGN(4096);
  __rodata_end = .;
  .data : { *(.data .data.*) } :data
  .bss : { *(COMMON) *(.bss .bss.*) } :data
  /DISCARD/ : { *(.eh_frame*) *(.comment*) *(.note*) }
}
