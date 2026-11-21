/* SPDX-License-Identifier: GPL-2.0-or-later */
/* vorbis (libtremor) run from flash. Per-frame code in the arena, picked
 * from a profile of q5 music: IMDCT/FFT, codebook and residue decode,
 * floor, windowing and the bit reader. Left in flash: the trig/window
 * tables (revtab, sincos, vwin: 20 KB) and setup; the heap (codebooks,
 * stream buffers) takes the rest of the arena plus the playback memory
 * lent while it plays (HAVE_CODEC_EXTRA_BUFFER). */
#define XIP_HOT_SECTIONS \
    *(.text.ff_imdct_half*) \
    *(.text.ff_fft_calc_c*) \
    *(.text.TRANSFORM*) \
    *(.text.pass*) \
    *(.text.fft4*) \
    *(.text.fft8*) \
    *(.text.fft16*) \
    *(.text.fft32*) \
    *(.text.fft64*) \
    *(.text.fft128*) \
    *(.text.fft256*) \
    *(.text.fft512*) \
    *(.text.fft1024*) \
    *(.text.fft2048*) \
    *(.rodata.fft_dispatch*) \
    *(.text.vorbis_book_decode*) \
    *(.text.decode_packed_*) \
    *(.text.bisect_codelist*) \
    *(.text.ff_vector_fmul_window_c*) \
    *(.text.mapping0_inverse*) \
    *(.text.res1_inverse*) \
    *(.text.res2_inverse*) \
    *(.text._01inverse*) \
    *(.text.floor1_inverse*) \
    *(.text.render_line*) \
    *(.text.vorbis_synthesis_blockin*) \
    *(.text.oggpack_*) \
    *(.rodata.FLOOR_fromdB_LOOKUP*)

#include "codec-xip.lds"
