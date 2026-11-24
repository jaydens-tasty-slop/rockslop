/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Video player MJPEG frames: baseline JPEG decoded MCU by MCU straight to the
 * screen. The IDCT is the AAN one of the IJG's jidctfst.c; chroma is
 * upsampled by repeating it, as the stock player does.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "videoplayer.h"
#include "mjpeg.h"

#define LUT_BITS    9

struct huff {
    uint16_t lut[1 << LUT_BITS];    /* (length << 8) | symbol; 0: longer */
    int32_t maxcode[17];            /* largest code of each length, or -1 */
    int32_t valoff[17];             /* code + valoff = index into vals */
    uint16_t spec_len;              /* 0: not built */
    uint8_t spec[16 + 256];         /* code counts per length, then vals */
};

struct comp {
    int id;
    int h, v;                       /* sampling factors */
    int dx, dy;                     /* blocks scaled by 1 / 2^dx, 1 / 2^dy */
    int ux, uy;                     /* then upsampled by 2^ux, 2^uy */
    int tq;                         /* quantization table */
    const struct huff *dc, *ac;
    int pred;                       /* the DC prediction */
    uint8_t *plane;                 /* the MCU's samples */
    int stride;
};

static const uint8_t zigzag[64] = {
     0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

/* The AAN IDCT's scale factors, cos(k * pi / 16) * sqrt(2) << 14 */
static const uint16_t aan[8] = {
    16384, 22725, 21407, 19266, 16384, 12873, 8867, 4520
};

/* Annex K.3: the tables of frames without a DHT (as AVI MJPEG has) */
static const uint8_t std_dc_bits[2][16] = {
    { 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 },
};
static const uint8_t std_dc_vals[12] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};
static const uint8_t std_ac_bits[2][16] = {
    { 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d },
    { 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 },
};
static const uint8_t std_ac_vals[2][162] = {
    {
        0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
        0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
        0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
        0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
        0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
        0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
        0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
        0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
        0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
        0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
        0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
        0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
        0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
        0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
        0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
        0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
        0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
        0xf9, 0xfa
    },
    {
        0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
        0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
        0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
        0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
        0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
        0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
        0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
        0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
        0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
        0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
        0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
        0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
        0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
        0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
        0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
        0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
        0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
        0xf9, 0xfa
    },
};

/* The big tables, in the player's memory while MJPEG plays */
static struct tables {
    /* [class][id], class 0 DC, 1 AC; ids 0 and 1, as MJPEG uses */
    struct huff huff[2][2];
    int32_t qt[4][64];              /* dequantization (AAN scaled), zigzag */
    uint16_t qt_raw[4][64];
    /* YCbCr to RGB, a load per chroma sample: Cr's red term and Cb's blue
       term in the low halves, their green terms << 8 in the high halves */
    int32_t cr_tab[256], cb_tab[256];
    uint8_t planes[3][16 * 16];
} *tab;
static bool huff_set[2][2];         /* by this frame's DHT */
static bool qt_set[4];

/* ---- the bit reader ---- */

static const uint8_t *src, *src_end;
static uint32_t bits;               /* the next bits, first at the top */
static int nbits;
static bool at_marker;              /* zeros from here: src is at a marker */
static int padding;                 /* zero bytes given since */

static inline void fill(void)
{
    while (nbits <= 24)
    {
        unsigned b = 0;
        if (!at_marker && src < src_end)
        {
            b = *src++;
            if (b == 0xff)
            {
                if (src < src_end && *src == 0)
                    src++;
                else
                {
                    at_marker = true;
                    src--;
                    b = 0;
                    padding++;
                }
            }
        }
        else
            padding++;
        bits |= b << (24 - nbits);
        nbits += 8;
    }
}

static inline unsigned get_bits(int n)
{
    unsigned v = bits >> (32 - n);
    bits <<= n;
    nbits -= n;
    return v;
}

static inline int extend(unsigned v, int s)
{
    return v < (1u << (s - 1)) ? (int)v + 1 - (1 << s) : (int)v;
}

static inline int huff_decode(const struct huff *h)
{
    fill();
    unsigned e = h->lut[bits >> (32 - LUT_BITS)];
    if (e)
    {
        bits <<= e >> 8;
        nbits -= e >> 8;
        return e & 0xff;
    }
    for (int len = LUT_BITS + 1; len <= 16; len++)
    {
        int32_t code = bits >> (32 - len);
        if (code <= h->maxcode[len])
        {
            bits <<= len;
            nbits -= len;
            return h->spec[16 + code + h->valoff[len]];
        }
    }
    return -1;
}

/* ---- tables ---- */

/* Make h the table of these code counts and symbols (unless it is) */
static bool huff_build(struct huff *h, const uint8_t *count, const uint8_t *vals)
{
    int32_t code = 0;
    int k = 0, n = 0;

    for (int len = 0; len < 16; len++)
        n += count[len];
    if (n > 256)
        return false;
    /* MJPEG repeats the same tables in every frame */
    if (h->spec_len == 16 + n && !rb->memcmp(h->spec, count, 16) &&
        !rb->memcmp(h->spec + 16, vals, n))
        return true;
    h->spec_len = 0;
    rb->memcpy(h->spec, count, 16);
    rb->memcpy(h->spec + 16, vals, n);
    rb->memset(h->lut, 0, sizeof(h->lut));
    for (int len = 1; len <= 16; len++)
    {
        int c = count[len - 1];
        h->valoff[len] = k - code;
        h->maxcode[len] = c ? code + c - 1 : -1;
        /* more codes than the length has: a bad table */
        if (code + c > (1 << len))
            return false;
        for (int i = 0; i < c; i++, code++, k++)
        {
            if (len <= LUT_BITS)
            {
                int shift = LUT_BITS - len;
                for (int j = code << shift; j < (code + 1) << shift; j++)
                    h->lut[j] = (len << 8) | vals[k];
            }
        }
        code <<= 1;
    }
    h->spec_len = 16 + n;
    return true;
}

static void set_qt(int t)
{
    for (int k = 0; k < 64; k++)
    {
        int n = zigzag[k];
        uint32_t scale = (aan[n >> 3] * aan[n & 7] + (1 << 13)) >> 14;
        /* scaled by 1 << 2 for the IDCT's first pass */
        tab->qt[t][k] = (tab->qt_raw[t][k] * scale + (1 << 11)) >> 12;
    }
}

bool mjpeg_init(void)
{
    tab = player_alloc(sizeof(*tab));
    if (!tab)
        return false;
    rb->memset(tab, 0, sizeof(*tab));
    rb->memset(huff_set, 0, sizeof(huff_set));
    rb->memset(qt_set, 0, sizeof(qt_set));
    for (int i = 0; i < 256; i++)
    {
        int c = i - 128;
        int r = (91881 * c + 32768) >> 16;              /* 1.402 */
        int b = (116130 * c + 32768) >> 16;             /* 1.772 */
        int rg = (-46802 * c) >> 8;                     /* 0.714136 << 8 */
        int bg = (-22554 * c + 32768) >> 8;             /* 0.344136 << 8 */
        tab->cr_tab[i] = (int32_t)((uint32_t)rg << 16) | (uint16_t)r;
        tab->cb_tab[i] = (int32_t)((uint32_t)bg << 16) | (uint16_t)b;
    }
    return true;
}

/* ---- the IDCT ---- */

#define FIX_1_082392200 277
#define FIX_1_414213562 362
#define FIX_1_847759065 473
#define FIX_2_613125930 669
#define MUL(v, c)       (((v) * (c)) >> 8)

static inline uint8_t clamp8(int v)
{
    return v < 0 ? 0 : v > 255 ? 255 : v;
}

/* Far above what valid data dequantizes to, low enough that the IDCT of
 * any data cannot overflow */
static inline int32_t clamp16(int32_t v)
{
    return v < -32768 ? -32768 : v > 32767 ? 32767 : v;
}

/* jidctfst.c's 8x8 IDCT, in its two passes */
#define IDCT_1D(i0, i1, i2, i3, i4, i5, i6, i7, o0, o1, o2, o3, o4, o5, o6, o7) \
    do {                                                                \
        int32_t t10 = (i0) + (i4), t11 = (i0) - (i4);                   \
        int32_t t13 = (i2) + (i6);                                      \
        int32_t t12 = MUL((i2) - (i6), FIX_1_414213562) - t13;          \
        int32_t e0 = t10 + t13, e3 = t10 - t13;                         \
        int32_t e1 = t11 + t12, e2 = t11 - t12;                         \
        int32_t z13 = (i5) + (i3), z10 = (i5) - (i3);                   \
        int32_t z11 = (i1) + (i7), z12 = (i1) - (i7);                   \
        int32_t d7 = z11 + z13;                                         \
        int32_t d11 = MUL(z11 - z13, FIX_1_414213562);                  \
        int32_t z5 = MUL(z10 + z12, FIX_1_847759065);                   \
        int32_t d10 = MUL(z12, FIX_1_082392200) - z5;                   \
        int32_t d12 = MUL(z10, -FIX_2_613125930) + z5;                  \
        int32_t d6 = d12 - d7;                                          \
        int32_t d5 = d11 - d6;                                          \
        int32_t d4 = d10 + d5;                                          \
        o0 = e0 + d7; o7 = e0 - d7;                                     \
        o1 = e1 + d6; o6 = e1 - d6;                                     \
        o2 = e2 + d5; o5 = e2 - d5;                                     \
        o4 = e3 + d4; o3 = e3 - d4;                                     \
    } while (0)

static void idct8(int32_t *in, uint8_t *out, int stride)
{
    for (int c = 0; c < 8; c++)
    {
        int32_t *p = in + c;
        if (!(p[8] | p[16] | p[24] | p[32] | p[40] | p[48] | p[56]))
        {
            p[8] = p[16] = p[24] = p[32] = p[40] = p[48] = p[56] = p[0];
            continue;
        }
        IDCT_1D(p[0], p[8], p[16], p[24], p[32], p[40], p[48], p[56],
                p[0], p[8], p[16], p[24], p[32], p[40], p[48], p[56]);
    }
    for (int r = 0; r < 8; r++, in += 8, out += stride)
    {
        int32_t o0, o1, o2, o3, o4, o5, o6, o7;
        /* +128 and rounding, before the >> 5 */
        int32_t dc = in[0] + (128 << 5) + 16;
        IDCT_1D(dc, in[1], in[2], in[3], in[4], in[5], in[6], in[7],
                o0, o1, o2, o3, o4, o5, o6, o7);
        out[0] = clamp8(o0 >> 5);
        out[1] = clamp8(o1 >> 5);
        out[2] = clamp8(o2 >> 5);
        out[3] = clamp8(o3 >> 5);
        out[4] = clamp8(o4 >> 5);
        out[5] = clamp8(o5 >> 5);
        out[6] = clamp8(o6 >> 5);
        out[7] = clamp8(o7 >> 5);
    }
}

/* A block's samples scaled by 1 / 2^dx, 1 / 2^dy (box averages) */
static void put_block(int32_t *coef, bool ac, int dx, int dy, uint8_t *out,
                      int stride)
{
    int w = 8 >> dx, h = 8 >> dy;

    if (!ac || (dx == 3 && dy == 3))
    {
        /* flat: what the IDCT gives a lone DC (the average of the block) */
        uint8_t v = clamp8((coef[0] + (128 << 5) + 16) >> 5);
        for (int y = 0; y < h; y++, out += stride)
            for (int x = 0; x < w; x++)
                out[x] = v;
        return;
    }
    if (dx == 0 && dy == 0)
    {
        idct8(coef, out, stride);
        return;
    }
    uint8_t tmp[64];
    int bits = dx + dy;
    idct8(coef, tmp, 8);
    for (int y = 0; y < h; y++, out += stride)
        for (int x = 0; x < w; x++)
        {
            int sum = (1 << bits) >> 1;
            const uint8_t *t = tmp + (y << dy) * 8 + (x << dx);
            for (int j = 0; j < 1 << dy; j++, t += 8)
                for (int i = 0; i < 1 << dx; i++)
                    sum += t[i];
            out[x] = sum >> bits;
        }
}

/* Entropy-decode one block into coef (natural order, zeroed before).
 * Returns 1 with AC coefficients, 0 without, -1 on bad data. */
static int decode_block(struct comp *c, int32_t *coef)
{
    const int32_t *q = tab->qt[c->tq];
    int s = huff_decode(c->dc);
    bool ac = false;

    /* 8-bit samples: DC differences take 11 bits at most, DC values and
     * AC coefficients 10 */
    if (s < 0 || s > 11)
        return -1;
    if (s)
    {
        fill();
        c->pred += extend(get_bits(s), s);
        if (c->pred < -2047 || c->pred > 2047)
            return -1;
    }
    coef[0] = clamp16(c->pred * q[0]);
    for (int k = 1; k < 64; k++)
    {
        int rs = huff_decode(c->ac);
        if (rs < 0)
            return -1;
        s = rs & 15;
        k += rs >> 4;
        if (s)
        {
            if (k > 63 || s > 10)
                return -1;
            fill();
            coef[zigzag[k]] = clamp16(extend(get_bits(s), s) * q[k]);
            ac = true;
        }
        else if (rs != 0xf0)
            break;
    }
    return ac;
}

/* ---- a frame ---- */

static struct comp comps[3];
static int ncomps;
static int width, height;
static int restart_interval;

/* One row of an MCU's colour pixels, samples x to end, each component's
 * samples repeated 2^u times. Inlined, so constant u's make fast loops. */
static inline __attribute__((always_inline))
void put_row(fb_data *p, int step, const uint8_t *yr, const uint8_t *cbr,
             const uint8_t *crr, int x, int end, int yu, int cbu, int cru)
{
    const int32_t *cb_tab = tab->cb_tab, *cr_tab = tab->cr_tab;

    for (; x < end; x++, p += step)
    {
        int l = yr[x >> yu];
        int32_t cb = cb_tab[cbr[x >> cbu]], cr = cr_tab[crr[x >> cru]];
        *p = FB_RGBPACK(clamp8(l + (int16_t)cr),
                        clamp8(l + (((cb >> 16) + (cr >> 16)) >> 8)),
                        clamp8(l + (int16_t)cb));
    }
}

/* The MCU's pixels, its frame position (x0, y0) in scaled pixels */
static void NO_INLINE put_mcu(const struct video_target *t, int x0, int y0,
                    int mcu_w, int mcu_h)
{
    int xa = MAX(x0, t->crop_x), xb = MIN(x0 + mcu_w, t->crop_x + t->disp_w);
    int ya = MAX(y0, t->crop_y), yb = MIN(y0 + mcu_h, t->crop_y + t->disp_h);

    for (int y = ya; y < yb; y++)
    {
        int my = y - y0;
        fb_data *p = t->row0 + (xa - t->crop_x) * t->dx + (y - t->crop_y) * t->dy;
        const uint8_t *yr = comps[0].plane + (my >> comps[0].uy) * comps[0].stride;
        if (ncomps == 1)
        {
            for (int x = xa - x0; x < xb - x0; x++, p += t->dx)
            {
                int l = yr[x >> comps[0].ux];
                *p = FB_RGBPACK(l, l, l);
            }
            continue;
        }
        const uint8_t *cbr = comps[1].plane + (my >> comps[1].uy) * comps[1].stride;
        const uint8_t *crr = comps[2].plane + (my >> comps[2].uy) * comps[2].stride;
        int yu = comps[0].ux, cbu = comps[1].ux, cru = comps[2].ux;
        /* 4:4:4 (and scaled 4:2:x), 4:2:x, then anything else */
        if (yu == 0 && cbu == 0 && cru == 0)
            put_row(p, t->dx, yr, cbr, crr, xa - x0, xb - x0, 0, 0, 0);
        else if (yu == 0 && cbu == 1 && cru == 1)
            put_row(p, t->dx, yr, cbr, crr, xa - x0, xb - x0, 0, 1, 1);
        else
            put_row(p, t->dx, yr, cbr, crr, xa - x0, xb - x0, yu, cbu, cru);
    }
}

/* After an interval: past its RSTn marker (the reader stops at or before
 * it), with fresh bits and predictions */
static void restart(void)
{
    nbits = 0;
    bits = 0;
    at_marker = false;
    padding = 0;
    while (src + 1 < src_end && !(src[0] == 0xff && (src[1] & 0xf8) == 0xd0))
        src++;
    src = MIN(src + 2, src_end);
    for (int i = 0; i < ncomps; i++)
        comps[i].pred = 0;
}

static bool decode_scan(const struct video_target *t)
{
    int hmax = 1, vmax = 1;
    int shift = t->shift;
    int32_t coef[64];

    for (int i = 0; i < ncomps; i++)
    {
        hmax = MAX(hmax, comps[i].h);
        vmax = MAX(vmax, comps[i].v);
    }
    if (ncomps == 1)    /* a lone component's MCU is one block */
        hmax = vmax = comps[0].h = comps[0].v = 1;
    for (int i = 0; i < ncomps; i++)
    {
        struct comp *c = &comps[i];
        if (hmax % c->h || vmax % c->v)
            return false;
        /* subsampled 1 or 2 times: the subsampling takes the frame's
         * scaling first (as libjpeg does), then upsampling makes up for
         * what is left */
        int sx = hmax / c->h - 1, sy = vmax / c->v - 1;
        c->dx = shift - MIN(shift, sx);
        c->dy = shift - MIN(shift, sy);
        c->ux = sx - MIN(shift, sx);
        c->uy = sy - MIN(shift, sy);
        c->stride = (c->h * 8) >> c->dx;
        c->plane = tab->planes[i];
        c->pred = 0;
    }

    int mcu_w = (hmax * 8) >> shift, mcu_h = (vmax * 8) >> shift;
    int mcus_x = (width + hmax * 8 - 1) / (hmax * 8);
    int mcus_y = (height + vmax * 8 - 1) / (vmax * 8);
    int left = restart_interval;

    nbits = 0;
    bits = 0;
    at_marker = false;
    padding = 0;
    for (int my = 0; my < mcus_y; my++)
    {
        int y0 = my * mcu_h;
        bool row_shown = y0 < t->crop_y + t->disp_h && y0 + mcu_h > t->crop_y;
        for (int mx = 0; mx < mcus_x; mx++)
        {
            int x0 = mx * mcu_w;
            bool shown = row_shown &&
                x0 < t->crop_x + t->disp_w && x0 + mcu_w > t->crop_x;
            if (restart_interval)
            {
                if (left == 0)
                {
                    restart();
                    left = restart_interval;
                }
                left--;
            }
            for (int i = 0; i < ncomps; i++)
            {
                struct comp *c = &comps[i];
                for (int by = 0; by < c->v; by++)
                    for (int bx = 0; bx < c->h; bx++)
                    {
                        rb->memset(coef, 0, sizeof(coef));
                        int ac = decode_block(c, coef);
                        if (ac < 0)
                            return false;
                        if (shown)
                            put_block(coef, ac, c->dx, c->dy,
                                      c->plane + by * (8 >> c->dy) * c->stride +
                                      bx * (8 >> c->dx),
                                      c->stride);
                    }
            }
            if (shown)
                put_mcu(t, x0, y0, mcu_w, mcu_h);
            /* the data ran out (the reader looks 4 bytes ahead at most):
             * a cut frame, or a header with a huge size */
            if (padding > 8)
                return false;
        }
        if (y0 + mcu_h >= t->crop_y + t->disp_h)
            break;      /* the rest is cropped off */
        rb->yield();
    }
    return true;
}

static inline unsigned be16(const uint8_t *p)
{
    return (p[0] << 8) | p[1];
}

static bool read_dht(const uint8_t *p, const uint8_t *end)
{
    while (p + 17 <= end)
    {
        int tc = p[0] >> 4, th = p[0] & 15;
        int n = 0;
        for (int i = 1; i <= 16; i++)
            n += p[i];
        if (tc > 1 || th > 1 || n > 256 || p + 17 + n > end ||
            !huff_build(&tab->huff[tc][th], p + 1, p + 17))
            return false;
        huff_set[tc][th] = true;
        p += 17 + n;
    }
    return true;
}

static bool read_dqt(const uint8_t *p, const uint8_t *end)
{
    while (p < end)
    {
        int pq = p[0] >> 4, tq = p[0] & 15;
        int len = pq ? 128 : 64;
        if (tq > 3 || p + 1 + len > end)
            return false;
        bool changed = !qt_set[tq];
        for (int k = 0; k < 64; k++)
        {
            unsigned v = pq ? be16(p + 1 + 2 * k) : p[1 + k];
            changed |= v != tab->qt_raw[tq][k];
            tab->qt_raw[tq][k] = v;
        }
        if (changed)
            set_qt(tq);
        qt_set[tq] = true;
        p += 1 + len;
    }
    return true;
}

bool mjpeg_decode(const uint8_t *data, uint32_t size, video_layout_fn *layout)
{
    const uint8_t *p = data, *end = data + size;
    const struct video_target *t = NULL;

    if (size < 4 || p[0] != 0xff || p[1] != 0xd8)
        return false;
    p += 2;
    rb->memset(huff_set, 0, sizeof(huff_set));
    restart_interval = 0;
    ncomps = 0;

    while (p + 4 <= end)
    {
        if (p[0] != 0xff)
        {
            p++;
            continue;
        }
        int m = p[1];
        if (m == 0xff || m == 0x01 || (m >= 0xd0 && m <= 0xd7))
        {
            p++;
            continue;
        }
        if (m == 0xd9)
            break;
        unsigned len = be16(p + 2);
        const uint8_t *seg = p + 4, *seg_end = p + 2 + len;
        if (len < 2 || seg_end > end)
            return false;

        switch (m)
        {
        case 0xc0:      /* baseline */
        case 0xc1:      /* extended sequential, Huffman */
            if (len < 8 || seg[0] != 8)
                return false;
            height = be16(seg + 1);
            width = be16(seg + 3);
            ncomps = seg[5];
            if ((ncomps != 1 && ncomps != 3) || len < 8u + 3 * ncomps ||
                !width || !height)
                return false;
            for (int i = 0; i < ncomps; i++)
            {
                const uint8_t *s = seg + 6 + 3 * i;
                comps[i].id = s[0];
                comps[i].h = s[1] >> 4;
                comps[i].v = s[1] & 15;
                comps[i].tq = s[2] & 3;
                if (comps[i].h < 1 || comps[i].h > 2 ||
                    comps[i].v < 1 || comps[i].v > 2)
                    return false;
            }
            t = layout(width, height);
            if (!t)
                return false;
            break;
        case 0xc4:
            if (!read_dht(seg, seg_end))
                return false;
            break;
        case 0xdb:
            if (!read_dqt(seg, seg_end))
                return false;
            break;
        case 0xdd:
            if (len >= 4)
                restart_interval = be16(seg);
            break;
        case 0xda:
        {
            /* one scan with every component (no non-interleaved ones) */
            if (!t || len < 6 || seg[0] != ncomps || len < 6u + 2 * ncomps)
                return false;
            for (int i = 0; i < ncomps; i++)
            {
                const uint8_t *s = seg + 1 + 2 * i;
                struct comp *c = &comps[i];
                int td = s[1] >> 4, ta = s[1] & 15;
                if (s[0] != c->id || !qt_set[c->tq] || td > 1 || ta > 1)
                    return false;
                /* no DHT: the standard tables */
                if (!huff_set[0][td] &&
                    !huff_build(&tab->huff[0][td], std_dc_bits[td], std_dc_vals))
                    return false;
                if (!huff_set[1][ta] &&
                    !huff_build(&tab->huff[1][ta], std_ac_bits[ta], std_ac_vals[ta]))
                    return false;
                c->dc = &tab->huff[0][td];
                c->ac = &tab->huff[1][ta];
            }
            src = seg_end;
            src_end = end;
            return decode_scan(t);
        }
        default:
            if ((m & 0xf0) == 0xc0 && m != 0xc8 && m != 0xcc)
                return false;   /* progressive, lossless, arithmetic */
            break;
        }
        p = seg_end;
    }
    return false;
}
