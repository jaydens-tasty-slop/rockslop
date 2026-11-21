/* SPDX-License-Identifier: GPL-2.0-or-later */
/* mpa (libmad) run from flash. libmad has no IRAM selections on Cortex-M
 * (its ICODE_ATTRs are on ARM-classic assembly), so this list puts the
 * Layer III decode, IMDCT and synthesis code, the bit reader and the
 * Huffman tables in the arena. Left in flash: Layer I/II, the requantize
 * table rq_table (32 KiB, one load per nonzero sample), ASF and seeking. */
#define XIP_HOT_SECTIONS \
    *libmad.a:layer3.o(.text*) \
    *libmad.a:synth.o(.text*) \
    *libmad.a:bit.o(.text.mad_bit_read* .text.mad_bit_skip* .text.mad_bit_init* .text.mad_bit_length* .text.mad_bit_nextbyte*) \
    *libmad.a:frame.o(.text*) \
    *libmad.a:stream.o(.text.mad_stream_sync* .text.mad_stream_buffer* .text.mad_stream_skip*) \
    *libmad.a:huffman.o(.rodata*) \
    *mpa.o(.text.codec_run*)

#include "codec-xip.lds"
