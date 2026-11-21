/* SPDX-License-Identifier: GPL-2.0-or-later */
/* wma (libwma, WMA v1/v2) run from flash. Per-block code in the arena:
 * block and coefficient decode, the bit reader, exponents, noise,
 * windowing, the IMDCT/FFT with its main twiddle table (sincos_lookup0,
 * 4 KB), the superframe/bit reservoir handling, and libgcc's 64-bit
 * division (noise band gains); for the setup at each track start, the
 * window sines (CORDIC) and the VLC table builder, which took ~0.4 s from
 * flash. Left in flash: the rest of the setup, ASF parsing and seeking, the Huffman source tables, revtab and
 * sincos_lookup1 (~3K reads per 2048-sample IMDCT). The buffers (up to
 * 106 KB at 44.1 kHz stereo, WMA_HEAP_BUFFERS in libwma) take the arena's
 * rest plus the playback memory lent while WMA plays (frame_out, 32 KB). */
#define XIP_HOT_SECTIONS \
    *(.text.wma_decode_block*) \
    *(.text.get_bits*) \
    *(.text.skip_bits*) \
    *(.text.wma_noise*) \
    *(.text.wma_decode_frame*) \
    *(.text.wma_decode_superframe_*) \
    *(.text.wma_window*) \
    *(.text.decode_exp_vlc*) \
    *(.text.decode_exp_lsp*) \
    *(.text.wma_lsp_to_curve*) \
    *(.text.pow_m1_4*) \
    *(.text.noise_band_gain*) \
    *(.text.fixdiv64*) \
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
    *(.rodata.fft_dispatch*) \
    *(.rodata.sincos_lookup0*) \
    *libgcc.a:_udivmoddi4.o(.text*) \
    *libgcc.a:_aeabi_ldivmod.o(.text*) \
    *libgcc.a:_aeabi_uldivmod.o(.text*) \
    *(.text.fsincos*) \
    *(.rodata.atan_table*) \
    *(.text.build_table*) \
    *(.text.compare_vlcspec*) \
    *wma.o(.text.codec_run*)

#include "codec-xip.lds"
