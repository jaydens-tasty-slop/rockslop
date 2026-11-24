/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * MPEG-1/2 audio layer I and II decoder (ISO 11172-3, 13818-3 low
 * sampling rates). Fixed point; each granule goes through the synthesis
 * filter as it is read, and a frame comes out a part (384 samples) at a
 * time, so the state is the filter history (4 KB) and the frame's side
 * information.
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
#include "demux.h"
#include "mpa.h"

/* The synthesis window D[] (11172-3 table B.3) as libmad reorders it:
 * D[r + 32 j] in row r, entry j (j < 16; the rest repeat), rows 0..16;
 * Q28 */
#define PRESHIFT(x) (x)
static const int32_t mad_window[17][32] = {
#include "../../../lib/rbcodec/codecs/libmad/D.dat"
};

/* cos((2i + 1) pi / 2n), i < n / 2, for n = 32, 16, 8, 4, 2; Q31 */
static const int32_t dct_cos[31] = {
    2144896910, 2124240380, 2083126254, 2021950484, 1941302225, 1841958164,
    1724875040, 1591180426, 1442161874, 1279254516, 1104027237, 918167572,
    723465451, 521795963, 315101295, 105372028, 2137142927, 2055013723,
    1893911494, 1660027308, 1362349204, 1012316784, 623381598, 210490206,
    2106220352, 1785567396, 1193077991, 418953276, 1984016189, 821806413,
    1518500250,
};
static const uint16_t bitrates[2][2][15] = {    /* [lsf][layer - 1], kbit/s */
    { { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
      { 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384 } },
    { { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256 },
      { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160 } },
};
static const uint16_t sample_rates[3] = { 44100, 48000, 32000 };

/* Layer II quantizers: levels, three samples grouped in one codeword,
 * codeword bits */
static const struct { uint16_t levels; uint8_t grouped, bits; } quant[17] = {
    { 3, 1, 5 }, { 5, 1, 7 }, { 7, 0, 3 }, { 9, 1, 10 }, { 15, 0, 4 },
    { 31, 0, 5 }, { 63, 0, 6 }, { 127, 0, 7 }, { 255, 0, 8 }, { 511, 0, 9 },
    { 1023, 0, 10 }, { 2047, 0, 11 }, { 4095, 0, 12 }, { 8191, 0, 13 },
    { 16383, 0, 14 }, { 32767, 0, 15 }, { 65535, 0, 16 },
};
/* Allocation value -> quantizer + 1 (0: no samples), by table row */
static const uint8_t alloc_quant[6][16] = {
    { 0, 1, 2, 17 },
    { 0, 1, 2, 3, 4, 5, 6, 17 },
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17 },
    { 0, 1, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17 },
    { 0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 },
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
};
/* Per subband: allocation bits << 4 | table row. Tables B.2c/d (low
 * rate), B.2a/b (high rate), 13818-3 B.1 (low sampling rates). */
static const uint8_t sb_alloc[3][30] = {
    { 0x44, 0x44, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34,
      0x34 },
    { 0x43, 0x43, 0x43, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
      0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31,
      0x31, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20 },
    { 0x45, 0x45, 0x45, 0x45, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34,
      0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24,
      0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24 },
};

struct header {
    int layer;
    bool lsf, crc;
    int bitrate;                /* kbit/s */
    uint32_t rate;
    int mode, mode_ext;
    int length;                 /* bytes */
};

struct state {
    /* synthesis history (see synth()): [channel][slot parity][X[k]]
       [slot / 2] */
    int32_t hist[2][2][32][8];
    int slot[2];                /* the newest, counting down */
    int32_t scale[64];          /* 2^(1 - i / 3), Q28 */
    int32_t recip[17];          /* 2^30 / levels */
    /* the frame being decoded */
    struct header h;
    uint32_t pos;               /* its next bit */
    int part, parts;            /* the next part, of how many (0: none) */
    int sblimit, bound;
    uint8_t q[2][32];           /* layer II: quantizer + 1; layer I: bits */
    uint8_t scf[2][32][3];      /* per part (layer I: the first) */
};
static struct state *st;

struct bits {
    const uint8_t *p;
    uint32_t pos, end;          /* in bits */
};

/* 1 to 16 bits; reads up to 3 bytes past the frame */
static unsigned getbits(struct bits *b, int n)
{
    if (b->pos + n > b->end)
    {
        b->pos = b->end;
        return 0;
    }
    uint32_t w = demux_rd32be(b->p + (b->pos >> 3)) << (b->pos & 7);
    b->pos += n;
    return w >> (32 - n);
}

size_t mpa_state_size(void)
{
    return sizeof(struct state);
}

void mpa_init(void *state)
{
    st = state;
    static const int32_t third[3] = { 536870912, 426114725, 338207482 };
    for (int i = 0; i < 63; i++)
        st->scale[i] = third[i % 3] >> (i / 3);
    st->scale[63] = 0;
    for (int i = 0; i < 17; i++)
        st->recip[i] = (1 << 30) / quant[i].levels;
    mpa_reset();
}

void mpa_reset(void)
{
    rb->memset(st->hist, 0, sizeof(st->hist));
    st->slot[0] = st->slot[1] = 0;
    st->parts = 0;
}

/* X[k] = sum of x[i] cos((2i + 1) k pi / 2n), i < n, in place (B. G. Lee's
 * split: the even outputs are the half size transform of x[i] + x[n-1-i];
 * with d[i] = (x[i] - x[n-1-i]) cos((2i + 1) pi / 2n), D its half size
 * transform, X[1] = D[0] and X[2k+1] = 2 D[k] - X[2k-1]). c: the cosines
 * of this size, then of the halves. Grows the values up to n times. */
static void dct(int32_t *x, int n, const int32_t *c)
{
    int h = n / 2;
    int32_t a[16], d[16];

    for (int i = 0; i < h; i++)
    {
        a[i] = x[i] + x[n - 1 - i];
        d[i] = ((int64_t)(x[i] - x[n - 1 - i]) * c[i]) >> 31;
    }
    if (h > 1)
    {
        dct(a, h, c + h);
        dct(d, h, c + h);
    }
    x[0] = a[0];
    x[1] = d[0];
    for (int k = 1; k < h; k++)
    {
        x[2 * k] = a[k];
        x[2 * k + 1] = 2 * d[k] - x[2 * k - 1];
    }
}

/* acc plus the top half of a times b (Q23 history times Q28 window:
 * Q19) */
static inline int32_t mla_hi(int32_t acc, int32_t a, int32_t b)
{
#if defined(__ARM_FEATURE_DSP) && __ARM_ARCH >= 6
    asm ("smmla %0, %1, %2, %0" : "+r" (acc) : "r" (a), "r" (b));
    return acc;
#else
    return acc + (int32_t)(((int64_t)a * b) >> 32);
#endif
}

/* sum of sign h[m] w[2 m], m < 8 */
static inline int32_t dot8(const int32_t *h, int sign, const int32_t *w)
{
    int32_t acc = 0;
    for (int m = 0; m < 8; m++)
        acc = mla_hi(acc, sign * h[m], w[2 * m]);
    return acc;
}

/* A Q19 sum of 16 products as a sample (each rounded down: half a unit
 * each back) */
static inline int16_t pcm16(int32_t acc)
{
    int32_t v = (acc + 16) >> 4;
    return v > 32767 ? 32767 : v < -32768 ? -32768 : v;
}

/* 32 subband samples (Q24) of one channel to 32 output samples.
 *
 * 11172-3's V (64 values a slot, 16 slots) is the transform X of each
 * slot's samples, mirrored: V[i] = X[16 + i], V[16] = 0, V[i] = -X[48 - i]
 * up to 48, -X[i - 48] after. Output j sums V[j] of the slots of even age
 * and V[32 + j] of the odd, weighted by D[j + 32 a] (age a); in X terms
 * X[16 + j] and -X[16 - j] for j <= 16, -X[48 - j] and -X[j - 16] after.
 * So the history keeps X (32 values a slot) by slot parity, each X[k] of
 * the 8 slots in a row: an output reads two rows, and turns the window
 * (whose rows repeat after 16) instead of the history. */
static void synth(int ch, const int32_t *s, int16_t *out, int stride)
{
    int32_t x[32];

    for (int k = 0; k < 32; k++)
        x[k] = s[k] >> 1;       /* room for the transform's growth */
    dct(x, 32, dct_cos);

    int n0 = st->slot[ch] = (st->slot[ch] - 1) & 15;
    int32_t (*even)[8] = st->hist[ch][n0 & 1];     /* ages 0, 2, ... */
    int32_t (*odd)[8] = st->hist[ch][~n0 & 1];     /* ages 1, 3, ... */
    int h0 = n0 >> 1, h1 = ((n0 + 1) & 15) >> 1;   /* their newest */
    for (int k = 0; k < 32; k++)
        even[k][h0] = x[k];

    const int32_t (*w)[32] = mad_window;
    out[0] = pcm16(dot8(even[16], 1, w[0] + 16 - 2 * h0) +
                   dot8(odd[16], -1, w[0] + 17 - 2 * h1));
    for (int j = 1; j < 16; j++)
    {
        /* outputs j and 32 - j: the same rows, the window forwards and
           backwards */
        const int32_t *e = even[16 + j], *o = odd[16 - j];
        const int32_t *fe = w[j] + 16 - 2 * h0, *fo = w[j] + 17 - 2 * h1;
        const int32_t *be = w[j] + 15 + 2 * h0, *bo = w[j] + 14 + 2 * h1;
        int32_t lo = 0, hi = 0;
        for (int m = 0; m < 8; m++)
        {
            lo = mla_hi(mla_hi(lo, e[m], fe[2 * m]), -o[m], fo[2 * m]);
            hi = mla_hi(mla_hi(hi, e[m], be[-2 * m]), o[m], bo[-2 * m]);
        }
        out[j * stride] = pcm16(lo);
        out[(32 - j) * stride] = pcm16(hi);
    }
    out[16 * stride] = pcm16(dot8(odd[0], -1, w[16] + 17 - 2 * h1));
}

static inline int32_t dequant(const struct state *s, int q, unsigned code,
                              int scf)
{
    int32_t f = (2 * (int32_t)code + 1 - quant[q].levels) * s->recip[q];
    return ((int64_t)f * s->scale[scf]) >> 34;
}

static bool parse_header(const uint8_t *p, struct header *h)
{
    uint32_t w = demux_rd32be(p);
    int version = (w >> 19) & 3;
    int sr = (w >> 10) & 3;
    int br = (w >> 12) & 15;

    if ((w >> 21) != 0x7ff || version == 1 || ((w >> 17) & 3) == 0 ||
        br == 0 || br == 15 || sr == 3)
        return false;
    h->layer = 4 - ((w >> 17) & 3);
    h->lsf = version != 3;
    h->crc = !((w >> 16) & 1);
    h->rate = sample_rates[sr] >> (version == 3 ? 0 : version == 2 ? 1 : 2);
    h->mode = (w >> 6) & 3;
    h->mode_ext = (w >> 4) & 3;
    int pad = (w >> 9) & 1;
    if (h->layer == 3)  /* only its length, to skip it */
    {
        h->bitrate = 0;
        static const uint16_t l3[2][15] = {
            { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 },
            { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160 },
        };
        h->length = (h->lsf ? 72 : 144) * 1000 * l3[h->lsf][br] / h->rate + pad;
        return true;
    }
    h->bitrate = bitrates[h->lsf][h->layer - 1][br];
    if (h->layer == 1)
        h->length = (12 * 1000 * h->bitrate / h->rate + pad) * 4;
    else
        h->length = 144 * 1000 * h->bitrate / h->rate + pad;
    return true;
}

/* Layer I: the bits per sample and the scale factors */
static void layer1_side(struct bits *b)
{
    int nch = st->h.mode == 3 ? 1 : 2;
    int bound = st->h.mode == 1 ? 4 + 4 * st->h.mode_ext : 32;

    for (int sb = 0; sb < 32; sb++)
    {
        for (int ch = 0; ch < nch; ch++)
        {
            if (sb >= bound && ch == 1)
                st->q[1][sb] = st->q[0][sb];
            else
            {
                unsigned a = getbits(b, 4);
                st->q[ch][sb] = a == 0 || a == 15 ? 0 : a + 1;
            }
        }
    }
    for (int sb = 0; sb < 32; sb++)
        for (int ch = 0; ch < nch; ch++)
            if (st->q[ch][sb])
                st->scf[ch][sb][0] = getbits(b, 6);
    st->bound = bound;
    st->parts = 1;
}

/* Layer I's 384 samples */
static void layer1_part(struct bits *b, int16_t *pcm)
{
    int nch = st->h.mode == 3 ? 1 : 2;
    int32_t s[2][32];

    for (int i = 0; i < 12; i++)
    {
        for (int sb = 0; sb < 32; sb++)
        {
            unsigned code = 0;
            for (int ch = 0; ch < nch; ch++)
            {
                int n = st->q[ch][sb];
                if (!n)
                {
                    s[ch][sb] = 0;
                    continue;
                }
                if (sb < st->bound || ch == 0)
                    code = getbits(b, n);
                /* 2^n - 1 levels: layer II's quantizer of as many */
                int q = n == 2 ? 0 : n == 3 ? 2 : n == 4 ? 4 : n;
                s[ch][sb] = dequant(st, q, code, st->scf[ch][sb][0]);
            }
        }
        for (int ch = 0; ch < nch; ch++)
            synth(ch, s[ch], pcm + i * 32 * nch + ch, nch);
    }
}

/* Layer II: the quantizers and the scale factors of the three parts */
static void layer2_side(struct bits *b)
{
    const struct header *h = &st->h;
    int nch = h->mode == 3 ? 1 : 2;
    int table, sblimit;
    uint8_t scfsi[2][32];

    if (h->lsf)
    {
        table = 2;
        sblimit = 30;
    }
    else
    {
        int per_ch = h->bitrate / nch;
        if (per_ch <= 48)
        {
            table = 0;
            sblimit = h->rate == 32000 ? 12 : 8;
        }
        else
        {
            table = 1;
            sblimit = per_ch <= 80 || h->rate == 48000 ? 27 : 30;
        }
    }
    int bound = h->mode == 1 ? MIN(4 + 4 * h->mode_ext, sblimit) : sblimit;

    for (int sb = 0; sb < sblimit; sb++)
    {
        int bits = sb_alloc[table][sb] >> 4, row = sb_alloc[table][sb] & 15;
        for (int ch = 0; ch < nch; ch++)
        {
            if (sb >= bound && ch == 1)
                st->q[1][sb] = st->q[0][sb];
            else
                st->q[ch][sb] = alloc_quant[row][getbits(b, bits)];
        }
    }
    for (int sb = 0; sb < sblimit; sb++)
        for (int ch = 0; ch < nch; ch++)
            if (st->q[ch][sb])
                scfsi[ch][sb] = getbits(b, 2);
    for (int sb = 0; sb < sblimit; sb++)
    {
        for (int ch = 0; ch < nch; ch++)
        {
            uint8_t *f = st->scf[ch][sb];
            if (!st->q[ch][sb])
                continue;
            switch (scfsi[ch][sb])
            {
            case 0:
                f[0] = getbits(b, 6); f[1] = getbits(b, 6); f[2] = getbits(b, 6);
                break;
            case 1:
                f[0] = f[1] = getbits(b, 6); f[2] = getbits(b, 6);
                break;
            case 2:
                f[0] = f[1] = f[2] = getbits(b, 6);
                break;
            default:
                f[0] = getbits(b, 6); f[1] = f[2] = getbits(b, 6);
                break;
            }
        }
    }
    st->sblimit = sblimit;
    st->bound = bound;
    st->parts = 3;
}

/* Layer II's granule gr (0-11) of 3 x 32 samples; four use each of the
 * three scale factors */
static void layer2_granule(struct bits *b, int gr, int16_t *pcm)
{
    int nch = st->h.mode == 3 ? 1 : 2;
    int sblimit = st->sblimit;
    int32_t s[2][3][32];

    for (int ch = 0; ch < nch; ch++)
        for (int i = 0; i < 3; i++)
            for (int sb = sblimit; sb < 32; sb++)
                s[ch][i][sb] = 0;
    for (int sb = 0; sb < sblimit; sb++)
    {
        unsigned code[3] = { 0, 0, 0 };
        for (int ch = 0; ch < nch; ch++)
        {
            int qi = st->q[ch][sb];
            if (!qi)
            {
                s[ch][0][sb] = s[ch][1][sb] = s[ch][2][sb] = 0;
                continue;
            }
            qi--;
            if (sb < st->bound || ch == 0)
            {
                if (quant[qi].grouped)
                {
                    unsigned n = quant[qi].levels, c = getbits(b, quant[qi].bits);
                    code[0] = c % n; c /= n;
                    code[1] = c % n; c /= n;
                    code[2] = MIN(c, n - 1);
                }
                else
                {
                    for (int i = 0; i < 3; i++)
                        code[i] = getbits(b, quant[qi].bits);
                }
            }
            for (int i = 0; i < 3; i++)
                s[ch][i][sb] = dequant(st, qi, MIN(code[i], quant[qi].levels - 1u),
                                       st->scf[ch][sb][gr / 4]);
        }
    }
    for (int i = 0; i < 3; i++)
        for (int ch = 0; ch < nch; ch++)
            synth(ch, s[ch][i], pcm + i * 32 * nch + ch, nch);
}

int mpa_decode(const uint8_t *buf, int len, int16_t *pcm, struct mpa_frame *f)
{
    f->samples = 0;
    if (st->parts == 0)
    {
        struct header h;
        int i;
        /* the first frame start, checked against the next if that is
           here */
        for (i = 0; i + 4 <= len; i++)
        {
            if (buf[i] != 0xff || !parse_header(buf + i, &h))
                continue;
            if (i + h.length > len)
                return i;   /* the frame is not all here yet */
            struct header next;
            if (i + h.length + 4 <= len &&
                (!parse_header(buf + i + h.length, &next) ||
                 next.layer != h.layer || next.rate != h.rate))
                continue;
            break;
        }
        if (i + 4 > len)
            return MAX(i - 3, 0);   /* keep a possible start for later */
        if (i > 0)
            return i;               /* skip to the frame */
        if (h.layer == 3)
            return h.length;

        struct bits b = { buf, 32 + (h.crc ? 16 : 0), h.length * 8 };
        st->h = h;
        if (h.layer == 1)
            layer1_side(&b);
        else
            layer2_side(&b);
        st->pos = b.pos;
        st->part = 0;
    }

    struct bits b = { buf, st->pos, st->h.length * 8 };
    f->channels = st->h.mode == 3 ? 1 : 2;
    if (st->h.layer == 1)
        layer1_part(&b, pcm);
    else
        for (int gr = 0; gr < 4; gr++)
            layer2_granule(&b, st->part * 4 + gr, pcm + gr * 96 * f->channels);
    st->pos = b.pos;
    f->rate = st->h.rate;
    f->samples = MPA_PART_SAMPLES;
    if (++st->part < st->parts)
        return 0;
    st->parts = 0;
    return st->h.length;
}
