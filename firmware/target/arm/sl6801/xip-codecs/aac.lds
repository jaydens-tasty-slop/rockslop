/* SPDX-License-Identifier: GPL-2.0-or-later */
/* aac (libfaad + libm4a) run from flash. libfaad is built with CODEC_XIP, so
 * its ICODE_ATTR/ICONST_ATTR/IBSS_ATTR objects are in the arena already;
 * this list adds codeclib/libm4a code and tables (built for in-arena codecs,
 * no IRAM attributes) that run per frame. */
/* First guess (unmeasured): per-frame code of the LC decode path and the
 * IMDCT/FFT with its twiddle tables. Bigger candidates left in flash:
 * iq_table (32 KiB, one load per nonzero coefficient), revtab (8 KiB). */
#define XIP_HOT_SECTIONS \
    *(.text.faad_getbits*) \
    *(.text.faad_showbits*) \
    *(.text.faad_flushbits*) \
    *(.text.spectral_data*) \
    *(.text.decode_scale_factors*) \
    *(.text.section_data*) \
    *(.text.scale_factor_data*) \
    *(.text.individual_channel*) \
    *(.text.side_info*) \
    *(.text.huffman_*) \
    *(.text.quant_to_spec*) \
    *(.text.reconstruct_*) \
    *(.text.ms_decode*) \
    *(.text.tns_decode_frame*) \
    *(.text.ifilter_bank*) \
    *(.text.ff_imdct_*) \
    *(.text.ff_fft_calc_c*) \
    *(.text.fft*) \
    *(.text.pass*) \
    *(.text.TRANSFORM*) \
    *(.rodata.sincos_lookup*) \
    SBR_HOT_SECTIONS

/* HE-AAC (SBR, libfaad FAAD_SBR_LOWMEM): its per-slot loops (QMF banks, HF
 * generation and adjustment with their fixed-point helpers and 64-bit
 * division), and what a profile of the rest found per frame from flash
 * (2026-11-01, 32 kbit/s: 16% of the CPU): the channel's band limits
 * (sbr_channel), envelope parsing, the saved state, the codec's memcpy and
 * memset calls. The other parsing stays in flash. */
#define SBR_HOT_SECTIONS \
    *(.text.sbr_qmf_analysis_32*) \
    *(.text.sbr_qmf_synthesis_64*) \
    *(.text.dct4_kernel*) \
    *(.text.hf_generation*) \
    *(.text.hf_adjustment*) \
    *(.text.sbr_channel*) \
    *(.text.sbr_save_prev_data*) \
    *(.text.sbr_envelope*) \
    *(.text.extract_envelope_data*) \
    *libcodec.a:codeclib.o(.text.memcpy .text.memset) \
    *(.text.get_S_mapped*) \
    *(.text.find_log2_E*) \
    *(.text.log2_int*) \
    *(.text.pow2_int*) \
    *(.text.pow2_fix*) \
    *libgcc.a:_aeabi_ldivmod.o(.text*) \
    *libgcc.a:_udivmoddi4.o(.text*) \
    *(.rodata.qmf_c*) \
    *(.rodata.log2_tab*)

#include "codec-xip.lds"
