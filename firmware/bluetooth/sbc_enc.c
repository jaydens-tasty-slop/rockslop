/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Portable SBC encoder for the A2DP source (bt_sbc.h), ported from BlueZ
 * libsbc (encoder only, generic C primitives; no mSBC, no decoder).
 *
 * Bluetooth low-complexity, subband codec (SBC) library
 * Copyright (C) 2004-2010  Marcel Holtmann <marcel@holtmann.org>
 * Copyright (C) 2004-2005  Henryk Ploetz <henryk@ploetzli.ch>
 * Copyright (C) 2005-2008  Brad Midgley <bmidgley@xmission.com>
 * Copyright (C) 2008-2010  Nokia Corporation
 * Copyright (C) 2012-2013  Intel Corporation
 *
 * The output is bit-exact with libsbc's encoder. All state lives in one
 * pinned buffer at the heap's top, so bt_sbc_encode() needs no allocation and
 * little stack and may run in interrupt context. The encode path and the
 * 8-subband tables live in IRAM where there is one: run from uncached XIP
 * flash (YP3) a frame took 14x real time.
 *
 * Per frame: sbc_input() reorders the PCM into the filter history,
 * sbc_analyze() runs the polyphase analysis filter (subband samples),
 * sbc_scalefactors() picks scale factors and the joint stereo subbands,
 * sbc_bit_alloc() spends the bitpool, sbc_pack() quantizes and writes. */
#include <string.h>
#include "config.h"
#include "core_alloc_top.h"
#include "bt_sbc.h"

/* Frame header: syncword; sampling frequency << 6 | (blocks / 4 - 1) << 4 |
 * channel mode << 2 | allocation method << 1 | (subbands == 8); bitpool;
 * CRC. The enum bt_sbc_* values are the field codes. */
#define SBC_SYNCWORD        0x9c
#define SBC_FRAME_HDR       4
#define SBC_FREQ_SHIFT      6
#define SBC_BLOCKS_SHIFT    4
#define SBC_MODE_SHIFT      2
#define SBC_ALLOC_SHIFT     1
#define SBC_SF_BITS         4       /* per scale factor */
#define SBC_MAX_BITS        16      /* per subband sample */
/* CRC-8 over the header from byte 1 (not the CRC itself), the joint
 * stereo flags and the scale factors */
#define SBC_CRC_INIT        0x0f
#define SBC_CRC_POLY        0x1d    /* x^8 + x^4 + x^3 + x^2 + 1 */
/* Subband samples carry this many fractional bits (libsbc SCALE_OUT_BITS) */
#define SCALE_OUT_BITS      15
/* Filter history per channel: newest samples at the bottom, reordered for
 * the analysis filter. At least 10 * subbands samples are always kept. */
#define SBC_X_BUFFER_SIZE   328

struct sbc_enc {
    uint8_t freq;           /* enum bt_sbc_freq */
    uint8_t mode;           /* enum bt_sbc_mode */
    uint8_t alloc;          /* enum bt_sbc_alloc */
    uint8_t channels;       /* 1 for mono, else 2 */
    uint8_t subbands;       /* 4 or 8 */
    uint8_t blocks;         /* 4, 8, 12 or 16 */
    uint8_t bitpool;
    uint16_t frame_bytes;   /* encoded frame length at this bitpool */
    int position;           /* index of the newest sample in X */
    int16_t X[2][SBC_X_BUFFER_SIZE];    /* filter history */
    int32_t sb_sample[16][2][8];        /* [block][channel][subband] */
    uint8_t scale_factor[2][8];
    uint8_t bits[2][8];                 /* bit allocation */
};

static int sbc_handle;          /* core_alloc_top() handle, 0: none */
static struct sbc_enc *sbc;     /* the open encoder, NULL: closed */

/* Loudness allocation offsets [sampling frequency][subband], 4 and 8
 * subbands. A2DP specification: Appendix B, page 69 */
static const int8_t sbc_offset4[4][4] = {
    { -1, 0, 0, 0 },
    { -2, 0, 0, 1 },
    { -2, 0, 0, 1 },
    { -2, 0, 0, 1 }
};

/* A2DP specification: Appendix B, page 69 */
static const int8_t sbc_offset8[4][8] ICONST_ATTR = {
    { -2, 0, 0, 0, 0, 0, 0, 1 },
    { -3, 0, 0, 0, 0, 0, 1, 2 },
    { -4, 0, 0, 0, 0, 0, 1, 2 },
    { -4, 0, 0, 0, 0, 0, 1, 2 }
};

/* Analysis filter constants (A2DP specification, section 12.8: the
 * prototype filter Proto_4_40 / Proto_8_80 and the cosine modulation), Q15.
 * Prototype values are premultiplied by 2; the accumulated sum is scaled
 * down by PROTO_SCALE bits before the cosine transform. Each row of the
 * prototype part is multiplied by a factor C0..C7 and the matching cosine
 * coefficients are divided by it, for precision. */
#define PROTO_SCALE         16
#define F_PROTO(x)          ((int32_t)(((x) * 2) * 32768.0 + 0.5))
#define F_COS(x)            ((int32_t)((x) * 32768.0 + 0.5))

/* Each table holds the reordered "proto" part followed by the reordered
 * "cos" part. Due to non-symmetrical reordering, separate tables for "even"
 * and "odd" blocks are needed. */
static const int16_t analysis_consts4_even[40 + 16] = {
#define C0 1.0932568993
#define C1 1.3056875580
#define C2 1.3056875580
#define C3 1.6772280856

#define F(x) F_PROTO(x)
     F(0.00000000E+00 * C0),  F(3.83720193E-03 * C0),
     F(5.36548976E-04 * C1),  F(2.73370904E-03 * C1),
     F(3.06012286E-03 * C2),  F(3.89205149E-03 * C2),
     F(0.00000000E+00 * C3), -F(1.49188357E-03 * C3),
     F(1.09137620E-02 * C0),  F(2.58767811E-02 * C0),
     F(2.04385087E-02 * C1),  F(3.21939290E-02 * C1),
     F(7.76463494E-02 * C2),  F(6.13245186E-03 * C2),
     F(0.00000000E+00 * C3), -F(2.88757392E-02 * C3),
     F(1.35593274E-01 * C0),  F(2.94315332E-01 * C0),
     F(1.94987841E-01 * C1),  F(2.81828203E-01 * C1),
    -F(1.94987841E-01 * C2),  F(2.81828203E-01 * C2),
     F(0.00000000E+00 * C3), -F(2.46636662E-01 * C3),
    -F(1.35593274E-01 * C0),  F(2.58767811E-02 * C0),
    -F(7.76463494E-02 * C1),  F(6.13245186E-03 * C1),
    -F(2.04385087E-02 * C2),  F(3.21939290E-02 * C2),
     F(0.00000000E+00 * C3),  F(2.88217274E-02 * C3),
    -F(1.09137620E-02 * C0),  F(3.83720193E-03 * C0),
    -F(3.06012286E-03 * C1),  F(3.89205149E-03 * C1),
    -F(5.36548976E-04 * C2),  F(2.73370904E-03 * C2),
     F(0.00000000E+00 * C3), -F(1.86581691E-03 * C3),
#undef F
#define F(x) F_COS(x)
     F(0.7071067812 / C0),  F(0.9238795325 / C1),
    -F(0.7071067812 / C0),  F(0.3826834324 / C1),
    -F(0.7071067812 / C0), -F(0.3826834324 / C1),
     F(0.7071067812 / C0), -F(0.9238795325 / C1),
     F(0.3826834324 / C2), -F(1.0000000000 / C3),
    -F(0.9238795325 / C2), -F(1.0000000000 / C3),
     F(0.9238795325 / C2), -F(1.0000000000 / C3),
    -F(0.3826834324 / C2), -F(1.0000000000 / C3),
#undef F

#undef C0
#undef C1
#undef C2
#undef C3
};

static const int16_t analysis_consts4_odd[40 + 16] = {
#define C0 1.3056875580
#define C1 1.6772280856
#define C2 1.0932568993
#define C3 1.3056875580

#define F(x) F_PROTO(x)
     F(2.73370904E-03 * C0),  F(5.36548976E-04 * C0),
    -F(1.49188357E-03 * C1),  F(0.00000000E+00 * C1),
     F(3.83720193E-03 * C2),  F(1.09137620E-02 * C2),
     F(3.89205149E-03 * C3),  F(3.06012286E-03 * C3),
     F(3.21939290E-02 * C0),  F(2.04385087E-02 * C0),
    -F(2.88757392E-02 * C1),  F(0.00000000E+00 * C1),
     F(2.58767811E-02 * C2),  F(1.35593274E-01 * C2),
     F(6.13245186E-03 * C3),  F(7.76463494E-02 * C3),
     F(2.81828203E-01 * C0),  F(1.94987841E-01 * C0),
    -F(2.46636662E-01 * C1),  F(0.00000000E+00 * C1),
     F(2.94315332E-01 * C2), -F(1.35593274E-01 * C2),
     F(2.81828203E-01 * C3), -F(1.94987841E-01 * C3),
     F(6.13245186E-03 * C0), -F(7.76463494E-02 * C0),
     F(2.88217274E-02 * C1),  F(0.00000000E+00 * C1),
     F(2.58767811E-02 * C2), -F(1.09137620E-02 * C2),
     F(3.21939290E-02 * C3), -F(2.04385087E-02 * C3),
     F(3.89205149E-03 * C0), -F(3.06012286E-03 * C0),
    -F(1.86581691E-03 * C1),  F(0.00000000E+00 * C1),
     F(3.83720193E-03 * C2),  F(0.00000000E+00 * C2),
     F(2.73370904E-03 * C3), -F(5.36548976E-04 * C3),
#undef F
#define F(x) F_COS(x)
     F(0.9238795325 / C0), -F(1.0000000000 / C1),
     F(0.3826834324 / C0), -F(1.0000000000 / C1),
    -F(0.3826834324 / C0), -F(1.0000000000 / C1),
    -F(0.9238795325 / C0), -F(1.0000000000 / C1),
     F(0.7071067812 / C2),  F(0.3826834324 / C3),
    -F(0.7071067812 / C2), -F(0.9238795325 / C3),
    -F(0.7071067812 / C2),  F(0.9238795325 / C3),
     F(0.7071067812 / C2), -F(0.3826834324 / C3),
#undef F

#undef C0
#undef C1
#undef C2
#undef C3
};

static const int16_t analysis_consts8_even[80 + 64] ICONST_ATTR = {
#define C0 2.7906148894
#define C1 2.4270044280
#define C2 2.8015616024
#define C3 3.1710363741
#define C4 2.5377944043
#define C5 2.4270044280
#define C6 2.8015616024
#define C7 3.1710363741

#define F(x) F_PROTO(x)
     F(0.00000000E+00 * C0),  F(2.01182542E-03 * C0),
     F(1.56575398E-04 * C1),  F(1.78371725E-03 * C1),
     F(3.43256425E-04 * C2),  F(1.47640169E-03 * C2),
     F(5.54620202E-04 * C3),  F(1.13992507E-03 * C3),
    -F(8.23919506E-04 * C4),  F(0.00000000E+00 * C4),
     F(2.10371989E-03 * C5),  F(3.49717454E-03 * C5),
     F(1.99454554E-03 * C6),  F(1.64973098E-03 * C6),
     F(1.61656283E-03 * C7),  F(1.78805361E-04 * C7),
     F(5.65949473E-03 * C0),  F(1.29371806E-02 * C0),
     F(8.02941163E-03 * C1),  F(1.53184106E-02 * C1),
     F(1.04584443E-02 * C2),  F(1.62208471E-02 * C2),
     F(1.27472335E-02 * C3),  F(1.59045603E-02 * C3),
    -F(1.46525263E-02 * C4),  F(0.00000000E+00 * C4),
     F(8.85757540E-03 * C5),  F(5.31873032E-02 * C5),
     F(2.92408442E-03 * C6),  F(3.90751381E-02 * C6),
    -F(4.91578024E-03 * C7),  F(2.61098752E-02 * C7),
     F(6.79989431E-02 * C0),  F(1.46955068E-01 * C0),
     F(8.29847578E-02 * C1),  F(1.45389847E-01 * C1),
     F(9.75753918E-02 * C2),  F(1.40753505E-01 * C2),
     F(1.11196689E-01 * C3),  F(1.33264415E-01 * C3),
    -F(1.23264548E-01 * C4),  F(0.00000000E+00 * C4),
     F(1.45389847E-01 * C5), -F(8.29847578E-02 * C5),
     F(1.40753505E-01 * C6), -F(9.75753918E-02 * C6),
     F(1.33264415E-01 * C7), -F(1.11196689E-01 * C7),
    -F(6.79989431E-02 * C0),  F(1.29371806E-02 * C0),
    -F(5.31873032E-02 * C1),  F(8.85757540E-03 * C1),
    -F(3.90751381E-02 * C2),  F(2.92408442E-03 * C2),
    -F(2.61098752E-02 * C3), -F(4.91578024E-03 * C3),
     F(1.46404076E-02 * C4),  F(0.00000000E+00 * C4),
     F(1.53184106E-02 * C5), -F(8.02941163E-03 * C5),
     F(1.62208471E-02 * C6), -F(1.04584443E-02 * C6),
     F(1.59045603E-02 * C7), -F(1.27472335E-02 * C7),
    -F(5.65949473E-03 * C0),  F(2.01182542E-03 * C0),
    -F(3.49717454E-03 * C1),  F(2.10371989E-03 * C1),
    -F(1.64973098E-03 * C2),  F(1.99454554E-03 * C2),
    -F(1.78805361E-04 * C3),  F(1.61656283E-03 * C3),
    -F(9.02154502E-04 * C4),  F(0.00000000E+00 * C4),
     F(1.78371725E-03 * C5), -F(1.56575398E-04 * C5),
     F(1.47640169E-03 * C6), -F(3.43256425E-04 * C6),
     F(1.13992507E-03 * C7), -F(5.54620202E-04 * C7),
#undef F
#define F(x) F_COS(x)
     F(0.7071067812 / C0),  F(0.8314696123 / C1),
    -F(0.7071067812 / C0), -F(0.1950903220 / C1),
    -F(0.7071067812 / C0), -F(0.9807852804 / C1),
     F(0.7071067812 / C0), -F(0.5555702330 / C1),
     F(0.7071067812 / C0),  F(0.5555702330 / C1),
    -F(0.7071067812 / C0),  F(0.9807852804 / C1),
    -F(0.7071067812 / C0),  F(0.1950903220 / C1),
     F(0.7071067812 / C0), -F(0.8314696123 / C1),
     F(0.9238795325 / C2),  F(0.9807852804 / C3),
     F(0.3826834324 / C2),  F(0.8314696123 / C3),
    -F(0.3826834324 / C2),  F(0.5555702330 / C3),
    -F(0.9238795325 / C2),  F(0.1950903220 / C3),
    -F(0.9238795325 / C2), -F(0.1950903220 / C3),
    -F(0.3826834324 / C2), -F(0.5555702330 / C3),
     F(0.3826834324 / C2), -F(0.8314696123 / C3),
     F(0.9238795325 / C2), -F(0.9807852804 / C3),
    -F(1.0000000000 / C4),  F(0.5555702330 / C5),
    -F(1.0000000000 / C4), -F(0.9807852804 / C5),
    -F(1.0000000000 / C4),  F(0.1950903220 / C5),
    -F(1.0000000000 / C4),  F(0.8314696123 / C5),
    -F(1.0000000000 / C4), -F(0.8314696123 / C5),
    -F(1.0000000000 / C4), -F(0.1950903220 / C5),
    -F(1.0000000000 / C4),  F(0.9807852804 / C5),
    -F(1.0000000000 / C4), -F(0.5555702330 / C5),
     F(0.3826834324 / C6),  F(0.1950903220 / C7),
    -F(0.9238795325 / C6), -F(0.5555702330 / C7),
     F(0.9238795325 / C6),  F(0.8314696123 / C7),
    -F(0.3826834324 / C6), -F(0.9807852804 / C7),
    -F(0.3826834324 / C6),  F(0.9807852804 / C7),
     F(0.9238795325 / C6), -F(0.8314696123 / C7),
    -F(0.9238795325 / C6),  F(0.5555702330 / C7),
     F(0.3826834324 / C6), -F(0.1950903220 / C7),
#undef F

#undef C0
#undef C1
#undef C2
#undef C3
#undef C4
#undef C5
#undef C6
#undef C7
};

static const int16_t analysis_consts8_odd[80 + 64] ICONST_ATTR = {
#define C0 2.5377944043
#define C1 2.4270044280
#define C2 2.8015616024
#define C3 3.1710363741
#define C4 2.7906148894
#define C5 2.4270044280
#define C6 2.8015616024
#define C7 3.1710363741

#define F(x) F_PROTO(x)
     F(0.00000000E+00 * C0), -F(8.23919506E-04 * C0),
     F(1.56575398E-04 * C1),  F(1.78371725E-03 * C1),
     F(3.43256425E-04 * C2),  F(1.47640169E-03 * C2),
     F(5.54620202E-04 * C3),  F(1.13992507E-03 * C3),
     F(2.01182542E-03 * C4),  F(5.65949473E-03 * C4),
     F(2.10371989E-03 * C5),  F(3.49717454E-03 * C5),
     F(1.99454554E-03 * C6),  F(1.64973098E-03 * C6),
     F(1.61656283E-03 * C7),  F(1.78805361E-04 * C7),
     F(0.00000000E+00 * C0), -F(1.46525263E-02 * C0),
     F(8.02941163E-03 * C1),  F(1.53184106E-02 * C1),
     F(1.04584443E-02 * C2),  F(1.62208471E-02 * C2),
     F(1.27472335E-02 * C3),  F(1.59045603E-02 * C3),
     F(1.29371806E-02 * C4),  F(6.79989431E-02 * C4),
     F(8.85757540E-03 * C5),  F(5.31873032E-02 * C5),
     F(2.92408442E-03 * C6),  F(3.90751381E-02 * C6),
    -F(4.91578024E-03 * C7),  F(2.61098752E-02 * C7),
     F(0.00000000E+00 * C0), -F(1.23264548E-01 * C0),
     F(8.29847578E-02 * C1),  F(1.45389847E-01 * C1),
     F(9.75753918E-02 * C2),  F(1.40753505E-01 * C2),
     F(1.11196689E-01 * C3),  F(1.33264415E-01 * C3),
     F(1.46955068E-01 * C4), -F(6.79989431E-02 * C4),
     F(1.45389847E-01 * C5), -F(8.29847578E-02 * C5),
     F(1.40753505E-01 * C6), -F(9.75753918E-02 * C6),
     F(1.33264415E-01 * C7), -F(1.11196689E-01 * C7),
     F(0.00000000E+00 * C0),  F(1.46404076E-02 * C0),
    -F(5.31873032E-02 * C1),  F(8.85757540E-03 * C1),
    -F(3.90751381E-02 * C2),  F(2.92408442E-03 * C2),
    -F(2.61098752E-02 * C3), -F(4.91578024E-03 * C3),
     F(1.29371806E-02 * C4), -F(5.65949473E-03 * C4),
     F(1.53184106E-02 * C5), -F(8.02941163E-03 * C5),
     F(1.62208471E-02 * C6), -F(1.04584443E-02 * C6),
     F(1.59045603E-02 * C7), -F(1.27472335E-02 * C7),
     F(0.00000000E+00 * C0), -F(9.02154502E-04 * C0),
    -F(3.49717454E-03 * C1),  F(2.10371989E-03 * C1),
    -F(1.64973098E-03 * C2),  F(1.99454554E-03 * C2),
    -F(1.78805361E-04 * C3),  F(1.61656283E-03 * C3),
     F(2.01182542E-03 * C4),  F(0.00000000E+00 * C4),
     F(1.78371725E-03 * C5), -F(1.56575398E-04 * C5),
     F(1.47640169E-03 * C6), -F(3.43256425E-04 * C6),
     F(1.13992507E-03 * C7), -F(5.54620202E-04 * C7),
#undef F
#define F(x) F_COS(x)
    -F(1.0000000000 / C0),  F(0.8314696123 / C1),
    -F(1.0000000000 / C0), -F(0.1950903220 / C1),
    -F(1.0000000000 / C0), -F(0.9807852804 / C1),
    -F(1.0000000000 / C0), -F(0.5555702330 / C1),
    -F(1.0000000000 / C0),  F(0.5555702330 / C1),
    -F(1.0000000000 / C0),  F(0.9807852804 / C1),
    -F(1.0000000000 / C0),  F(0.1950903220 / C1),
    -F(1.0000000000 / C0), -F(0.8314696123 / C1),
     F(0.9238795325 / C2),  F(0.9807852804 / C3),
     F(0.3826834324 / C2),  F(0.8314696123 / C3),
    -F(0.3826834324 / C2),  F(0.5555702330 / C3),
    -F(0.9238795325 / C2),  F(0.1950903220 / C3),
    -F(0.9238795325 / C2), -F(0.1950903220 / C3),
    -F(0.3826834324 / C2), -F(0.5555702330 / C3),
     F(0.3826834324 / C2), -F(0.8314696123 / C3),
     F(0.9238795325 / C2), -F(0.9807852804 / C3),
     F(0.7071067812 / C4),  F(0.5555702330 / C5),
    -F(0.7071067812 / C4), -F(0.9807852804 / C5),
    -F(0.7071067812 / C4),  F(0.1950903220 / C5),
     F(0.7071067812 / C4),  F(0.8314696123 / C5),
     F(0.7071067812 / C4), -F(0.8314696123 / C5),
    -F(0.7071067812 / C4), -F(0.1950903220 / C5),
    -F(0.7071067812 / C4),  F(0.9807852804 / C5),
     F(0.7071067812 / C4), -F(0.5555702330 / C5),
     F(0.3826834324 / C6),  F(0.1950903220 / C7),
    -F(0.9238795325 / C6), -F(0.5555702330 / C7),
     F(0.9238795325 / C6),  F(0.8314696123 / C7),
    -F(0.3826834324 / C6), -F(0.9807852804 / C7),
    -F(0.3826834324 / C6),  F(0.9807852804 / C7),
     F(0.9238795325 / C6), -F(0.8314696123 / C7),
    -F(0.9238795325 / C6),  F(0.5555702330 / C7),
     F(0.3826834324 / C6), -F(0.1950903220 / C7),
#undef F

#undef C0
#undef C1
#undef C2
#undef C3
#undef C4
#undef C5
#undef C6
#undef C7
};

#undef F_PROTO
#undef F_COS

/* Where the newest sample of a fresh or wrapped filter history goes */
static inline int sbc_x_top(int subbands)
{
    return (SBC_X_BUFFER_SIZE - 9 * subbands) & ~7;
}

/* Input order for the analysis filter, per two blocks: the filter reads
 * X[position + i] = pcm[perm[i]], newest pair of blocks first. */
static const uint8_t sbc_perm4[8] = { 7, 3, 6, 4, 0, 2, 1, 5 };
static const uint8_t sbc_perm8[16] ICONST_ATTR = {
    15, 7, 14, 8, 13, 9, 12, 10, 11, 3, 6, 0, 5, 1, 4, 2
};

/* Deinterleave and reorder one frame of PCM into the filter history. */
static void ICODE_ATTR sbc_input(struct sbc_enc *e, const int16_t *pcm)
{
    const int nch = e->channels;
    const int step = 2 * e->subbands;
    const uint8_t *perm = e->subbands == 4 ? sbc_perm4 : sbc_perm8;
    int nsamples = e->blocks * e->subbands;
    int position = e->position;
    int ch, i;

    /* On wraparound, move the 9 * subbands samples of history the filter
     * still needs to the top of the buffer. */
    if (position < nsamples) {
        const int top = sbc_x_top(e->subbands);
        for (ch = 0; ch < nch; ch++)
            memcpy(&e->X[ch][top], &e->X[ch][position],
                   9 * e->subbands * sizeof(int16_t));
        position = top;
    }

    for (; nsamples > 0; nsamples -= step) {
        position -= step;
        for (ch = 0; ch < nch; ch++) {
            int16_t *x = &e->X[ch][position];
            for (i = 0; i < step; i++)
                x[i] = pcm[perm[i] * nch + ch];
        }
        pcm += step * nch;
    }
    e->position = position;
}

/* Polyphase analysis filter for one block: 10 * sb reordered input samples
 * to sb subband samples. The Q15 cosine table matches SCALE_OUT_BITS, so
 * the transform output needs no final shift. */
static void ICODE_ATTR sbc_analyze_block(const int16_t *in, int32_t *out,
                              const int16_t *consts, int sb)
{
    int32_t t1[8];
    int16_t t2[8];
    int i, j, hop;

    /* low pass polyphase filter, with rounding */
    for (i = 0; i < sb; i++)
        t1[i] = 1 << (PROTO_SCALE - 1);
    for (hop = 0; hop < 10 * sb; hop += 2 * sb)
        for (i = 0; i < 2 * sb; i += 2)
            t1[i / 2] += in[hop + i] * consts[hop + i] +
                         in[hop + i + 1] * consts[hop + i + 1];
    for (i = 0; i < sb; i++)
        t2[i] = t1[i] >> PROTO_SCALE;

    /* cosine transform */
    consts += 10 * sb;
    for (i = 0; i < sb; i++) {
        int32_t s = 0;
        for (j = 0; j < sb; j += 2)
            s += t2[j] * consts[j * sb + 2 * i] +
                 t2[j + 1] * consts[j * sb + 2 * i + 1];
        out[i] = s;
    }
}

/* Subband samples of one frame, from the newest X samples. */
static void ICODE_ATTR sbc_analyze(struct sbc_enc *e)
{
    const int sb = e->subbands;
    const int16_t *even = sb == 4 ? analysis_consts4_even
                                  : analysis_consts8_even;
    const int16_t *odd = sb == 4 ? analysis_consts4_odd
                                 : analysis_consts8_odd;
    int ch, blk;

    for (ch = 0; ch < e->channels; ch++) {
        const int16_t *x = &e->X[ch][e->position + (e->blocks - 4) * sb];
        for (blk = 0; blk < e->blocks; blk += 4) {
            sbc_analyze_block(x + 3 * sb, e->sb_sample[blk][ch], odd, sb);
            sbc_analyze_block(x + 2 * sb, e->sb_sample[blk + 1][ch], even, sb);
            sbc_analyze_block(x + sb, e->sb_sample[blk + 2][ch], odd, sb);
            sbc_analyze_block(x, e->sb_sample[blk + 3][ch], even, sb);
            x -= 4 * sb;
        }
    }
}

/* |v| - 1 (0 for 0); or-ed over the blocks this gives the scale factor */
static inline uint32_t sbc_mag(int32_t v)
{
    uint32_t m = v < 0 ? -(uint32_t)v : (uint32_t)v;
    return m ? m - 1 : 0;
}

/* Scale factor: the smallest sf >= 0 with |s| <= 2^(sf + 1) for all the
 * (Q15) samples whose sbc_mag() values were or-ed into x */
static inline unsigned sbc_scale(uint32_t x)
{
    return (31 - SCALE_OUT_BITS) - __builtin_clz(x | (1u << SCALE_OUT_BITS));
}

/* Scale factors. For joint stereo also decides, for every subband but the
 * last, whether mid/side needs fewer bits than left/right, and converts
 * those subbands. Returns the joint stereo flags as sent in the frame. */
static unsigned ICODE_ATTR sbc_scalefactors(struct sbc_enc *e)
{
    const int nsb = e->subbands;
    unsigned joint = 0;
    int ch, sb, blk;

    for (ch = 0; ch < e->channels; ch++) {
        for (sb = 0; sb < nsb; sb++) {
            uint32_t x = 0;
            for (blk = 0; blk < e->blocks; blk++)
                x |= sbc_mag(e->sb_sample[blk][ch][sb]);
            e->scale_factor[ch][sb] = sbc_scale(x);
        }
    }

    if (e->mode != BT_SBC_JOINT)
        return 0;

    for (sb = 0; sb < nsb - 1; sb++) {
        uint32_t x = 0, y = 0;
        unsigned sf_mid, sf_side;

        for (blk = 0; blk < e->blocks; blk++) {
            int32_t l = e->sb_sample[blk][0][sb] >> 1;
            int32_t r = e->sb_sample[blk][1][sb] >> 1;
            x |= sbc_mag(l + r);
            y |= sbc_mag(l - r);
        }
        sf_mid = sbc_scale(x);
        sf_side = sbc_scale(y);
        if (e->scale_factor[0][sb] + e->scale_factor[1][sb] <=
            sf_mid + sf_side)
            continue;

        joint |= 1u << (nsb - 1 - sb);
        e->scale_factor[0][sb] = sf_mid;
        e->scale_factor[1][sb] = sf_side;
        for (blk = 0; blk < e->blocks; blk++) {
            int32_t l = e->sb_sample[blk][0][sb] >> 1;
            int32_t r = e->sb_sample[blk][1][sb] >> 1;
            e->sb_sample[blk][0][sb] = l + r;
            e->sb_sample[blk][1][sb] = l - r;
        }
    }
    return joint;
}

/* Bit allocation (A2DP specification, 12.6.3/12.6.4) for nch channels from
 * ch0 sharing one bitpool: each channel alone for mono and dual channel,
 * both together for stereo and joint stereo. */
static void ICODE_ATTR sbc_bit_alloc(struct sbc_enc *e, int ch0, int nch)
{
    const int nsb = e->subbands;
    const int bitpool = e->bitpool;
    const int8_t *offset = nsb == 4 ? sbc_offset4[e->freq]
                                    : sbc_offset8[e->freq];
    int8_t bitneed[2][8];
    int max_bitneed = 0, bitcount, slicecount, bitslice;
    int ch, sb;

    for (ch = ch0; ch < ch0 + nch; ch++) {
        for (sb = 0; sb < nsb; sb++) {
            int sf = e->scale_factor[ch][sb], need;
            if (e->alloc == BT_SBC_SNR)
                need = sf;
            else if (sf == 0)
                need = -5;
            else {
                int loudness = sf - offset[sb];
                need = loudness > 0 ? loudness / 2 : loudness;
            }
            bitneed[ch][sb] = need;
            if (need > max_bitneed)
                max_bitneed = need;
        }
    }

    /* Lower the slice until the bitpool is reached. Terminates because
     * bitpool <= SBC_MAX_BITS * nch * nsb (checked in bt_sbc_open). */
    bitcount = 0;
    slicecount = 0;
    bitslice = max_bitneed + 1;
    do {
        bitslice--;
        bitcount += slicecount;
        slicecount = 0;
        for (ch = ch0; ch < ch0 + nch; ch++) {
            for (sb = 0; sb < nsb; sb++) {
                int need = bitneed[ch][sb];
                if (need > bitslice + 1 && need < bitslice + SBC_MAX_BITS)
                    slicecount++;
                else if (need == bitslice + 1)
                    slicecount += 2;
            }
        }
    } while (bitcount + slicecount < bitpool);

    if (bitcount + slicecount == bitpool) {
        bitcount += slicecount;
        bitslice--;
    }

    for (ch = ch0; ch < ch0 + nch; ch++) {
        for (sb = 0; sb < nsb; sb++) {
            int bits = bitneed[ch][sb] - bitslice;
            if (bitneed[ch][sb] < bitslice + 2)
                bits = 0;
            else if (bits > SBC_MAX_BITS)
                bits = SBC_MAX_BITS;
            e->bits[ch][sb] = bits;
        }
    }

    /* Distribute the remaining bits, lowest subband first (channels
     * interleaved). This always uses the whole bitpool, so every frame has
     * the same length. */
    for (sb = 0; sb < nsb && bitcount < bitpool; sb++) {
        for (ch = ch0; ch < ch0 + nch && bitcount < bitpool; ch++) {
            uint8_t *bits = &e->bits[ch][sb];
            if (*bits >= 2 && *bits < SBC_MAX_BITS) {
                (*bits)++;
                bitcount++;
            } else if (bitneed[ch][sb] == bitslice + 1 &&
                       bitpool > bitcount + 1) {
                *bits = 2;
                bitcount += 2;
            }
        }
    }
    for (sb = 0; sb < nsb && bitcount < bitpool; sb++) {
        for (ch = ch0; ch < ch0 + nch && bitcount < bitpool; ch++) {
            if (e->bits[ch][sb] < SBC_MAX_BITS) {
                e->bits[ch][sb]++;
                bitcount++;
            }
        }
    }
}

/* MSB-first bit writer */
struct sbc_bits {
    uint8_t *p;
    uint32_t cache;
    unsigned count;
};

static inline void ICODE_ATTR sbc_put_bits(struct sbc_bits *w, uint32_t v, unsigned n)
{
    w->cache = (w->cache << n) | v;
    w->count += n;
    while (w->count >= 8) {
        w->count -= 8;
        *w->p++ = w->cache >> w->count;
    }
}

/* CRC-8 (SBC_CRC_POLY), MSB first, over the first bits of data */
static uint8_t ICODE_ATTR sbc_crc8(uint8_t crc, const uint8_t *data, unsigned bits)
{
    unsigned i;

    for (i = 0; i < bits; i++) {
        unsigned bit = ((data[i / 8] << (i % 8)) ^ crc) & 0x80;
        crc = (crc << 1) ^ (bit ? SBC_CRC_POLY : 0);
    }
    return crc;
}

/* Frame header, scale factors and quantized samples (A2DP specification,
 * 12.6.5). Returns the bytes written. */
static size_t ICODE_ATTR sbc_pack(const struct sbc_enc *e, unsigned joint, uint8_t *out)
{
    const int nsb = e->subbands;
    const int nch = e->channels;
    struct sbc_bits w = { out + SBC_FRAME_HDR, 0, 0 };
    unsigned crc_bits;
    uint8_t crc;
    int ch, sb, blk;

    out[0] = SBC_SYNCWORD;
    out[1] = (e->freq << SBC_FREQ_SHIFT) |
             ((e->blocks / 4 - 1) << SBC_BLOCKS_SHIFT) |
             (e->mode << SBC_MODE_SHIFT) | (e->alloc << SBC_ALLOC_SHIFT) |
             (nsb == 8);
    out[2] = e->bitpool;

    crc_bits = SBC_SF_BITS * nch * nsb;
    if (e->mode == BT_SBC_JOINT) {
        sbc_put_bits(&w, joint, nsb);
        crc_bits += nsb;
    }
    for (ch = 0; ch < nch; ch++)
        for (sb = 0; sb < nsb; sb++)
            sbc_put_bits(&w, e->scale_factor[ch][sb], SBC_SF_BITS);

    /* Quantize: q = (s / 2^(sf + 1) + 1) * (2^bits - 1) / 2, in fixed
     * point with s offset to be unsigned */
    for (blk = 0; blk < e->blocks; blk++) {
        for (ch = 0; ch < nch; ch++) {
            for (sb = 0; sb < nsb; sb++) {
                unsigned bits = e->bits[ch][sb];
                unsigned sf = e->scale_factor[ch][sb];
                uint32_t levels, s;

                if (bits == 0)
                    continue;
                levels = ((1u << bits) - 1) <<
                         (32 - (sf + SCALE_OUT_BITS + 2));
                s = (1u << (sf + SCALE_OUT_BITS + 1)) +
                    (uint32_t)e->sb_sample[blk][ch][sb];
                sbc_put_bits(&w, (uint32_t)(((uint64_t)levels * s) >> 32),
                             bits);
            }
        }
    }
    if (w.count > 0)
        *w.p++ = w.cache << (8 - w.count);

    /* The CRC covers the header from byte 1 except itself, then the joint
     * flags and scale factors. */
    crc = sbc_crc8(SBC_CRC_INIT, out + 1, 16);
    out[3] = sbc_crc8(crc, out + SBC_FRAME_HDR, crc_bits);
    return w.p - out;
}

/* At most SBC_MAX_BITS per subband for each channel sharing the bitpool:
 * one (mono, dual channel) or two (stereo, joint stereo) */
static bool sbc_bitpool_valid(int mode, int nsb, int bitpool)
{
    const bool two_ch_alloc = mode == BT_SBC_STEREO || mode == BT_SBC_JOINT;
    return bitpool >= BT_SBC_BITPOOL_MIN && bitpool <= BT_SBC_BITPOOL_MAX &&
           bitpool <= (two_ch_alloc ? 2 * SBC_MAX_BITS : SBC_MAX_BITS) * nsb;
}

/* Encoded frame length: header, scale factors, then the joint stereo
 * flags and samples padded to a byte */
static size_t sbc_frame_len(int mode, int nsb, int nch, int blocks,
                            int bitpool)
{
    size_t len = SBC_FRAME_HDR + SBC_SF_BITS * nsb * nch / 8;
    if (mode == BT_SBC_STEREO || mode == BT_SBC_JOINT)
        len += ((mode == BT_SBC_JOINT ? nsb : 0) + blocks * bitpool + 7) / 8;
    else
        len += (blocks * nch * bitpool + 7) / 8;
    return len;
}

bool bt_sbc_open(const struct bt_sbc_params *params)
{
    struct sbc_enc *e;
    int nch, nsb, blocks, bitpool;

    nsb = params->subbands;
    blocks = params->blocks;
    bitpool = params->bitpool;
    if (params->freq > BT_SBC_48000 || params->mode > BT_SBC_JOINT ||
        params->alloc > BT_SBC_SNR || (nsb != 4 && nsb != 8) ||
        blocks < 4 || blocks > 16 || (blocks & 3) ||
        !sbc_bitpool_valid(params->mode, nsb, bitpool))
        return false;

    if (sbc_handle <= 0)
        return false;
    sbc = NULL;                 /* encoding stops while it is set up */
    e = core_get_data(sbc_handle);
    memset(e, 0, sizeof(*e));

    nch = params->mode == BT_SBC_MONO ? 1 : 2;
    e->freq = params->freq;
    e->mode = params->mode;
    e->alloc = params->alloc;
    e->channels = nch;
    e->subbands = nsb;
    e->blocks = blocks;
    e->bitpool = bitpool;
    e->frame_bytes = sbc_frame_len(e->mode, nsb, nch, blocks, bitpool);
    e->position = sbc_x_top(nsb);
    sbc = e;
    return true;
}

size_t bt_sbc_frame_bytes(void)
{
    return sbc ? sbc->frame_bytes : 0;
}

size_t bt_sbc_frame_samples(void)
{
    return sbc ? (size_t)sbc->blocks * sbc->subbands : 0;
}

size_t ICODE_ATTR bt_sbc_encode(const int16_t *pcm, uint8_t *out)
{
    struct sbc_enc *e = sbc;
    unsigned joint;

    if (!e)
        return 0;

    sbc_input(e, pcm);
    sbc_analyze(e);
    joint = sbc_scalefactors(e);
    if (e->mode == BT_SBC_STEREO || e->mode == BT_SBC_JOINT) {
        sbc_bit_alloc(e, 0, 2);
    } else {
        sbc_bit_alloc(e, 0, 1);
        if (e->mode == BT_SBC_DUAL)
            sbc_bit_alloc(e, 1, 1);
    }
    return sbc_pack(e, joint, out);
}

bool bt_sbc_set_bitpool(unsigned bitpool)
{
    struct sbc_enc *e = sbc;
    if (!e || !sbc_bitpool_valid(e->mode, e->subbands, bitpool))
        return false;
    e->bitpool = bitpool;
    e->frame_bytes = sbc_frame_len(e->mode, e->subbands, e->channels,
                                   e->blocks, bitpool);
    return true;
}

void bt_sbc_close(void)
{
    sbc = NULL;
}

bool bt_sbc_reserve(void)
{
    if (sbc_handle > 0)
        return true;
    /* Not first-fit: made while USB storage held its buffer, it split the
     * heap once that was freed and playback did not fit */
    sbc_handle = core_alloc_top(sizeof(struct sbc_enc));
    if (sbc_handle <= 0) {
        sbc_handle = 0;
        return false;
    }
    core_pin(sbc_handle);
    return true;
}

void bt_sbc_release(void)
{
    sbc = NULL;
    if (sbc_handle > 0)
        sbc_handle = core_free(sbc_handle);
}
