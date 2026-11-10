/* Code+data in 0x806000..0x806a00, stack 0x806a00..0x806c00 (grows down
 * from 0x806c00), args/result at 0x806c00, bounce 0x806d00, stage 0x807000. */
ENTRY(_start)
MEMORY { CODE (rwx) : ORIGIN = 0x806000, LENGTH = 0xa00 }
SECTIONS {
    .text : { KEEP(*(.entry)) *(.text*) *(.rodata*) *(.data*) } > CODE
    .bss (NOLOAD) : { *(.bss*) *(COMMON) } > CODE
    /DISCARD/ : { *(.ARM.*) *(.comment) }
}
