ENTRY(entry)
SECTIONS {
    . = 0x01018000;
    .text : { *(.text.start) *(.text .text.*) *(.rodata .rodata.*) }
    /DISCARD/ : { *(.ARM.*) *(.comment) *(.note.*) }
    ASSERT(. <= 0x01018800, "gadget overlaps its args at 0x01018800")
}
