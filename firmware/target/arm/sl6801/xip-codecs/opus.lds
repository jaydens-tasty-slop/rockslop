/* SPDX-License-Identifier: GPL-2.0-or-later */
/* opus (libopus, CELT path) run from flash. Per-frame code of the CELT
 * decoder in the arena, picked from a profile of 108k music (FFT, MDCT,
 * band/PVQ dequantisation, bit allocation, range decoder, synthesis) and
 * the FFT/MDCT tables. Left in flash: SILK, packet parsing, loss
 * concealment; the Ogg buffers take most of the rest of the arena. */
#define XIP_HOT_SECTIONS \
    *(.text.opus_fft_impl*) \
    *(.text.clt_mdct_backward*) \
    *(.text.quant_band_stereo*) \
    *(.text.quant_partition*) \
    *(.text.quant_band*) \
    *(.text.compute_theta*) \
    *(.text.decode_pulses*) \
    *(.text.deemphasis*) \
    *(.text.celt_synthesis*) \
    *(.text.denormalise_bands*) \
    *(.text.normalise_residual*) \
    *(.text.exp_rotation*) \
    *(.text.alg_unquant*) \
    *(.text.renormalise_vector*) \
    *(.text.extract_collapse_mask*) \
    *(.text.comb_filter*) \
    *(.text.haar1*) \
    *(.text.special_hybrid_folding*) \
    *(.text.unquant_coarse_energy*) \
    *(.text.unquant_fine_energy*) \
    *(.text.unquant_energy_finalise*) \
    *(.text.ec_*) \
    *(.text.celt_udiv*) \
    *(.text.celt_rsqrt_norm*) \
    *(.text.celt_rcp*) \
    *(.text.celt_sqrt*) \
    *(.text.celt_exp2*) \
    *(.text.celt_cos_norm*) \
    *(.text.celt_lcg_rand*) \
    *(.text.isqrt32*) \
    *(.text.bitexact_*) \
    *(.text.celt_decode_with_ec*) \
    *(.text.clt_compute_allocation*) \
    *(.text.quant_all_bands*) \
    *(.text.init_caps*) \
    *(.text.celt_sudiv*) \
    *(.rodata.mdct_twiddles960*) \
    *(.rodata.fft_twiddles48000_960*) \
    *(.rodata.fft_bitrev480*) \
    *(.rodata.cache_*) \
    *(.rodata.band_allocation*) \
    *(.rodata.e_prob_model*) \
    *(.rodata.eband5ms*) \
    *(.rodata.LOG2_FRAC_TABLE*) \
    *(.rodata.logN400*) \
    *(.rodata.window120*)

#include "codec-xip.lds"
