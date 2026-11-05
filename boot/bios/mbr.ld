ENTRY(_start)
PHDRS { boot PT_LOAD FLAGS(5); }
SECTIONS { . = 0x7c00; .text : { *(.text*) } :boot /DISCARD/ : { *(.comment) *(.note*) } }
