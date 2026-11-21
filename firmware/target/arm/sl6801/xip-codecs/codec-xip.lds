/* SPDX-License-Identifier: GPL-2.0-or-later */
/* XIP codec: a decoder linked to run from flash (HAVE_XIP_CODECS).
 *
 * Included by <codec>.lds, which defines the codec's flash slot
 * (XIP_SLOT_START/XIP_SLOT_SIZE, CPU addresses) and XIP_HOT_SECTIONS, input
 * section patterns (".text.fn*", ".rodata.table*") of library code and tables
 * built without ICODE_ATTR that should run from SRAM too.
 *
 * Flash: header (struct codec_xip_header), the RAM image's load copy,
 * .text, .rodata. The .codec file on the card is a copy of the header.
 * Arena (codecbuf): the RAM image (.icode/.irodata/.idata, hot sections,
 * .data), then .bss/.ibss; the loader copies the RAM image, codec_start()
 * clears the bss, and the arena past plugin_end_addr is the codec's heap. */
#include "cpu.h"
#include "module_build_id.lds"

OUTPUT_FORMAT(elf32-littlearm)

MEMORY
{
    XIP_FLASH : ORIGIN = XIP_SLOT_START, LENGTH = XIP_SLOT_SIZE
    PLUGIN_RAM : ORIGIN = SL6801_MODULE_BASE, LENGTH = CODEC_SIZE
}

SECTIONS
{
    .header :
    {
        xip_flash_start = .;
        KEEP(*(.header))
    } > XIP_FLASH

    /* Listed before .text/.rodata: an input section goes to the first
       pattern that matches it, so hot sections land here. */
    .ram :
    {
        plugin_start_addr = .;
        _plugin_start_addr = .;
        *(.icode)
        XIP_HOT_SECTIONS
        *(.irodata)
        *(.idata)
        *(.data*)
        . = ALIGN(4);
        xip_data_end = .;
    } > PLUGIN_RAM AT> XIP_FLASH
    xip_data_load = LOADADDR(.ram);

    .text :
    {
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
    } > XIP_FLASH

    .rodata :
    {
        *(.rodata*)
        *(.ARM.exidx*)
        . = ALIGN(4);
        xip_flash_end = .;
    } > XIP_FLASH

    /DISCARD/ :
    {
        *(.eh_frame)
    }

    .bss (NOLOAD) :
    {
        plugin_bss_start = .;
        _plugin_bss_start = .;
        *(.ibss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
    } > PLUGIN_RAM

    .pluginend (NOLOAD) :
    {
        _plugin_end_addr = .;
        plugin_end_addr = .;
    } > PLUGIN_RAM

    .comment 0 :
    {
        KEEP(*(.comment))
    }
}

ASSERT(XIP_SLOT_START >= XIP_CODEC_AREA_START &&
       XIP_SLOT_START + XIP_SLOT_SIZE <= XIP_CODEC_AREA_START + XIP_CODEC_AREA_SIZE,
       "XIP codec slot outside the codec flash area")
ASSERT(LOADADDR(.ram) + SIZEOF(.ram) <= ADDR(.text), "RAM image overlaps .text")
