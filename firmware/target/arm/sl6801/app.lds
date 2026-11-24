/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "cpu.h"

#ifdef HAVE_EXACT_MODULE_API
#include "module_build_id.lds"
#endif

ENTRY(yp3_link_only_entry)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(target/arm/sl6801/yp3/crt0.o)

/* Flash: XIP code at SL6801_FLASH_BASE (flash offset 0x10000). The stock
 * bootloader copies the first 4 KiB of FIRM ([0xE430, 0xF430) in flash, filled
 * by the packager from the start of .text) to SRAM 0x804c00 and runs it.
 * RAM: core data/bss/stacks from 0x804c00 up to the shared codec/plugin
 * arena at SL6801_MODULE_BASE; the top YP3_DIAG_RETAINED_BYTES hold records
 * that survive a watchdog reset. */
MEMORY
{
#ifdef HAVE_XIP_CODECS
    FLASH (rx) : ORIGIN = SL6801_FLASH_BASE,
                 LENGTH = XIP_CODEC_AREA_START - SL6801_FLASH_BASE
#else
    FLASH (rx) : ORIGIN = SL6801_FLASH_BASE, LENGTH = SL6801_FLASH_SIZE
#endif
    RAM (rw) : ORIGIN = SL6801_CORE_RAM_BASE,
               LENGTH = SL6801_MODULE_BASE - SL6801_CORE_RAM_BASE
    RETAINED (rw) : ORIGIN = SL6801_RAM_BASE + SL6801_RAM_SIZE
                             - YP3_DIAG_RETAINED_BYTES,
                    LENGTH = YP3_DIAG_RETAINED_BYTES
}

/* Core functions run from SRAM instead of uncached XIP flash, as
 * F(archive, member, function). The core is built with -ffunction-sections:
 * each one's .text.<function> (and its clones' .text.<function>.*) goes to
 * .data, which crt0 copies to SRAM, and the rest of its object's code to
 * .text_rest. The link fails if one matched nothing (renamed, moved to
 * another file, inlined away); static functions the compiler inlines into
 * these need no entry. Code that must run from SRAM wherever it is built
 * still uses ICODE_ATTR.
 * - text rendering hot paths: font cache, glyph widths, UTF-8, bidi;
 * - directory scans (long-name parsing, path walk): every lookup parses
 *   whole directories without a dircache; from uncached flash one lookup
 *   in a 136-entry folder took ~150 ms;
 * - software volume, which scales each 4 KiB chunk in the DMA interrupt:
 *   from flash 7.4 ms a chunk (32% of the CPU at 192 MHz, interrupts off);
 * - per-sample DSP stages: from flash the EQ, bass enhancement, fatigue
 *   reduction and compressor stuttered. */
#define SRAM_FUNCTIONS(F) \
    /* text rendering */ \
    F(libfirmware.a, bidi.o, bidi_l2v) \
    F(libfirmware.a, font.o, font_get_width) \
    F(libfirmware.a, font.o, font_get_bits) \
    F(libfirmware.a, font_cache.o, search) \
    F(libfirmware.a, font_cache.o, font_cache_get) \
    F(libfirmware.a, lru.o, lru_touch) \
    F(libfirmware.a, lru.o, lru_data) \
    F(libfirmware.a, unicode.o, utf8length) \
    F(libfirmware.a, unicode.o, utf8decode) \
    F(libfirmware.a, lcd-16bit.o, lcd_putsxyofs) \
    /* directory scans */ \
    F(libfirmware.a, disk_cache.o, dc_cache_probe) \
    F(libfirmware.a, file_internal.o, open_path_component) \
    F(libfirmware.a, unicode.o, utf8_ucs_get_extra_bytes_count) \
    F(libfirmware.a, unicode.o, utf8encode_internal) \
    F(libfirmware.a, unicode.o, utf8encode) \
    F(libfirmware.a, fat.o, parse_short_direntry) \
    F(libfirmware.a, fat.o, longent_char_next) \
    F(libfirmware.a, fat.o, fatlong_parse_start) \
    F(libfirmware.a, fat.o, get_next_cluster32) \
    F(libfirmware.a, fat.o, fat_seek) \
    F(libfirmware.a, fat.o, fat_readdir) \
    /* software volume */ \
    F(libfirmware.a, pcm_sw_volume.o, pcm_scale_buffer_cut) \
    F(libfirmware.a, pcm_sw_volume.o, pcm_scale_buffer_boost) \
    F(libfirmware.a, pcm_sw_volume.o, pcm_scale_buffer_trans) \
    F(libfirmware.a, pcm_sw_volume.o, pcm_play_dma_status_callback_int_swvol) \
    /* DSP stages */ \
    F(librbcodec.a, dsp_filter.o, filter_process) \
    F(librbcodec.a, dsp_filter.o, dequeue) \
    F(librbcodec.a, dsp_filter.o, enqueue) \
    F(librbcodec.a, afr.o, afr_reduce_process) \
    F(librbcodec.a, compressor.o, compressor_process) \
    F(librbcodec.a, pbe.o, pbe_process) \
    F(librbcodec.a, eq.o, eq_process)
#define SRAM_OBJECT(archive, member, function) *archive:member
#define SRAM_FUNCTION(archive, member, function) \
        __sram_##function##_begin = .; \
        *archive:member(.text.function .text.function.*) \
        __sram_##function##_end = .;
#define SRAM_MESSAGE(...) #__VA_ARGS__
#define SRAM_CHECK(archive, member, function) \
    ASSERT(__sram_##function##_end > __sram_##function##_begin, \
           SRAM_MESSAGE(function: missing from archive(member)))

/* One segment per area, none holding the ELF headers (by default ld maps
 * them into the lowest segment, which then overlaps others) */
PHDRS
{
    text PT_LOAD;
    data PT_LOAD;
    text_rest PT_LOAD;
    ram PT_LOAD;
    retained PT_LOAD;
}
SECTIONS
{
    .text :
    {
        loadaddress = .;
#if defined(YP3_RECOVERY_WATCHDOG)
        KEEP(*(.init.yp3_stock_stub));
        __yp3_early_code_begin = .;
        KEEP(*(.init.yp3_early_recovery_entry));
        KEEP(*(.init.yp3_early_recovery));
        KEEP(*(.init.yp3_recovery_tail));
        __yp3_early_code_end = .;
#endif
        KEEP(*(.init*));
        /* the objects of SRAM_FUNCTIONS: their code goes to .data and
         * .text_rest (an input section goes to its first match) */
        *(EXCLUDE_FILE(SRAM_FUNCTIONS(SRAM_OBJECT)) .text* .rodata* .glue_7*);
        *(.yp3_hal_provider_extra*);
        . = ALIGN(4);
    } > FLASH :text
    .vectors : ALIGN(1024)
    {
        KEEP(*(.vectors.arm));
        KEEP(*(.vectors.platform));
    } > RAM AT> FLASH :data
    __vectors_load = LOADADDR(.vectors);
    .data : ALIGN(4)
    {
        _databegin = .;
        /* Code that must run from SRAM while XIP is unavailable or
         * reconfigured. */
        __yp3_nor_clock_ramcode_begin = .;
        KEEP(*(.yp3_nor_clock_ramcode))
        __yp3_nor_clock_ramcode_end = .;
        *(.data*);
        *(.idata*);
        *(.irodata*);
        /* ICODE_ATTR code (SD read core): runs from SRAM, copied by crt0. */
        *(.icode*);
        SRAM_FUNCTIONS(SRAM_FUNCTION)
        . = ALIGN(4);
        _dataend = .;
    } > RAM AT> FLASH
    _datacopy = LOADADDR(.data);
    /* the rest of the code of SRAM_FUNCTIONS' objects */
    .text_rest : ALIGN(4)
    {
        *(.text*);
        . = ALIGN(4);
    } > FLASH :text_rest
    .bss (NOLOAD) : ALIGN(4)
    {
        _bssbegin = .;
        *(.bss*);
        *(.ibss*);
        *(COMMON);
        . = ALIGN(4);
        _bssend = .;
    } > RAM :ram
    .framebuffer (NOLOAD) : ALIGN(4)
    {
        *(.framebuffer);
    } > RAM
    .stack (NOLOAD) : ALIGN(8)
    {
        irqstackbegin = .;
#ifdef HAVE_BLUETOOTH
        /* Bluetooth's media tick (SysTick), its scheduler line and IRQ 26
         * nest: >= 1.7 KiB worst case by static analysis */
        . += 0x800;
#else
        . += 0x400;
#endif
        irqstackend = .;
        stackbegin = .;
        . += 0x1800;                    /* main thread: 2.6 KiB peak
                                           measured */
        stackend = .;
    } > RAM
    .yp3_fault_emergency (NOLOAD) : ALIGN(8)
    {
        __yp3_fault_emergency_stack_begin = .;
        . += 0x100;
        __yp3_fault_emergency_stack_end = .;
    } > RAM
    audiobuffer = ALIGN(., 4);
    audiobufend = SL6801_MODULE_BASE;
    codecbuf = SL6801_MODULE_BASE;
    pluginbuf = SL6801_MODULE_BASE;
    .yp3_retained (NOLOAD) : ALIGN(8)
    {
        __yp3_fault_noinit_begin = .;
        KEEP(*(.noinit.fault_record));
        __yp3_fault_noinit_end = .;
        KEEP(*(.noinit.dev_log));
        KEEP(*(.noinit.yp3_run_marker));
    } > RETAINED :retained
    __framebuffer_begin = ADDR(.framebuffer);
    __framebuffer_end = ADDR(.framebuffer) + SIZEOF(.framebuffer);
    __data_bytes = _dataend - _databegin;
    __bss_bytes = _bssend - _bssbegin;
    __framebuffer_bytes = SIZEOF(.framebuffer);
    _flashend = ADDR(.text_rest) + SIZEOF(.text_rest);
    /DISCARD/ : { *(.eh_frame*); *(.ARM.exidx*); *(.ARM.extab*); }
}
EXTERN(__vectors_arm);
EXTERN(__vectors_platform);
EXTERN(main);
/* Pull the fault entry archive member before vector weak defaults bind. */
EXTERN(yp3_fault_unhandled_entry);
ASSERT(yp3_stock_stub_entry == loadaddress,
       "copied stub must start FIRM text");
ASSERT(yp3_link_only_entry == loadaddress,
       "ELF entry must be the copied stub");
ASSERT(yp3_native_xip_entry < loadaddress + 0x1000,
       "XIP entry outside copied prefix");
#if defined(YP3_RECOVERY_WATCHDOG)
ASSERT(__yp3_early_code_end <= loadaddress + 0xbf0,
       "early recovery code overlaps the copied prefix's temporary stack");
#endif
ASSERT(audiobuffer <= audiobufend, "core exceeds SRAM budget");
ASSERT((ADDR(.vectors) & 1023) == 0, "vector table must be 1024-byte aligned");
ASSERT((__vectors_load & 1023) == 0, "flash vectors must be VTOR aligned");
ASSERT((__vectors_platform - ADDR(.vectors)) == 64,
       "platform vectors follow 16 core vectors");
ASSERT(SIZEOF(.vectors) == 576, "expected 16 core and 128 platform vectors");
ASSERT(CODEC_SIZE == PLUGIN_BUFFER_SIZE, "shared arena sizes differ");
ASSERT(SL6801_MODULE_BASE + CODEC_SIZE + YP3_DIAG_RETAINED_BYTES
       == SL6801_RAM_BASE + SL6801_RAM_SIZE,
       "module arena exceeds SRAM");
ASSERT(__yp3_fault_noinit_end - __yp3_fault_noinit_begin == 128,
       "fault record ABI changed");
ASSERT((__yp3_fault_emergency_stack_begin & 7) == 0, "fault stack alignment");
SRAM_FUNCTIONS(SRAM_CHECK)
