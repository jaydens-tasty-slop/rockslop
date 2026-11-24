/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Tiny llama2-style transformer inference, see llm.h. The model follows
 * karpathy/llama2.c (RMSNorm, RoPE on adjacent pairs, GQA, SwiGLU).
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 ****************************************************************************/
#include "llm.h"

#if defined(__ARM_FEATURE_SIMD32)
#include <arm_acle.h>
#define LLM_SIMD 1
#endif

#define W_PER_LAYER 7

/* ---- small maths (no libm) ---- */

static float llm_expf(float x)
{
    if (x < -87.0f)
        return 0.0f;
    if (x > 88.0f)
        x = 88.0f;
    float t = x * 1.44269504f;               /* x / ln 2 */
    int n = (int)(t < 0.0f ? t - 0.5f : t + 0.5f);
    float r = x - (float)n * 0.693147181f;    /* |r| <= ln2 / 2 */
    float p = 1.0f + r * (1.0f + r * (0.5f + r * (0.166666667f +
              r * (0.0416666667f + r * (0.00833333333f + r * 0.00138888889f)))));
    union { float f; uint32_t i; } u = { p };
    u.i += (uint32_t)n << 23;
    return u.f;
}

static float llm_rsqrtf(float x)
{
    union { float f; uint32_t i; } u = { x };
    u.i = 0x5f3759df - (u.i >> 1);
    float y = u.f;
    for (int i = 0; i < 3; i++)
        y = y * (1.5f - 0.5f * x * y * y);
    return y;
}

/* sin(a) for any a: reduce to [-pi/2, pi/2], odd Taylor series to x^13. */
static float llm_sinf(float a)
{
    const float pi = 3.14159265f, twopi = 6.28318531f;
    int k = (int)(a / twopi + (a < 0.0f ? -0.5f : 0.5f));
    a -= (float)k * twopi;                    /* [-pi, pi] */
    if (a > pi / 2)
        a = pi - a;
    else if (a < -pi / 2)
        a = -pi - a;
    float a2 = a * a, term = a, sum = a;
    for (int i = 1; i <= 6; i++) {
        term *= -a2 / (float)((2 * i) * (2 * i + 1));
        sum += term;
    }
    return sum;
}

static void zero(void *p, size_t n)
{
    uint8_t *b = p;
    while (n--)
        *b++ = 0;
}

/* Soft-float helpers: on this target every float compare or conversion is a
 * libgcc call, so the hot loops use the bit patterns instead. */
typedef uint32_t __attribute__((may_alias)) u32a;

static inline uint32_t fbits(float f)
{
    union { float f; uint32_t u; } x = { f };
    return x.u;
}

static inline float bitsf(uint32_t u)
{
    union { uint32_t u; float f; } x = { u };
    return x.f;
}

/* round to nearest for |x| < 2^22 */
static inline int32_t fround(float x)
{
    return (int32_t)(fbits(x + 12582912.0f) - 0x4b400000);
}

/* max |x[i]|, at least 1e-8 */
static float absmax(const float *x, int n)
{
    const u32a *u = (const u32a *)x;
    uint32_t m = 0x322bcc77;
    for (int i = 0; i < n; i++) {
        uint32_t a = u[i] & 0x7fffffff;
        if (a > m)
            m = a;
    }
    return bitsf(m);
}

/* ---- memory ---- */

static void *take_try(struct llm *m, size_t n)
{
    n = (n + 3) & ~(size_t)3;
    for (int i = 0; i < 2; i++) {
        if (m->mem[i].size >= n) {
            void *p = m->mem[i].p;
            m->mem[i].p += n;
            m->mem[i].size -= n;
            return p;
        }
    }
    return NULL;
}

/* a later, smaller take can still succeed: check m->oom */
static void *take(struct llm *m, size_t n)
{
    void *p = take_try(m, n);
    if (!p)
        m->oom = true;
    return p;
}

/* rows of cols int8 values, each followed by its float scale */
static size_t qt_bytes(const struct llm_qt *t)
{
    return (size_t)t->rows * (t->cols + 4);
}

static bool io_read(struct llm *m, uint32_t off, void *buf, uint32_t n)
{
    if (m->io.start)
        return m->io.wait(m->io.ctx, m->io.start(m->io.ctx, off, buf, n));
    return m->io.read(m->io.ctx, off, buf, n);
}

/* Loads tensor t into RAM if it fits; returns false on a read error. */
static bool place(struct llm *m, struct llm_qt *t)
{
    size_t n = qt_bytes(t);
    uint8_t *p = take_try(m, n);
    if (!p) {
        t->q = NULL;
        m->st.streamed += n;
        return true;
    }
    t->q = (int8_t *)p;
    m->st.resident += n;
    return io_read(m, t->off, p, n);
}

/* ---- tokenizer ---- */

static uint32_t str_hash(const char *s, int n)
{
    uint32_t h = 2166136261u;
    while (n--)
        h = (h ^ (uint8_t)*s++) * 16777619u;
    return h;
}

static int tok_lookup(struct llm *m, const char *s, int n)
{
    uint32_t i = str_hash(s, n) & m->hash_mask;
    for (;;) {
        int id = m->tok_hash[i];
        if (!id)
            return -1;
        id--;
        int len = m->tok_off[id + 1] - m->tok_off[id];
        const char *t = m->tok_str + m->tok_off[id];
        if (len == n) {
            int j = 0;
            while (j < n && t[j] == s[j])
                j++;
            if (j == n)
                return id;
        }
        i = (i + 1) & m->hash_mask;
    }
}

/* sentencepiece byte-fallback tokens <0x00>..<0xFF> follow <unk> <s> </s> */
static bool is_byte_token(struct llm *m, int id)
{
    return id >= 3 && id < 259 &&
           m->tok_off[id + 1] - m->tok_off[id] == 6 &&
           m->tok_str[m->tok_off[id]] == '<';
}

int llm_encode(struct llm *m, const char *text, bool bos, int *tokens, int max)
{
    int n = 0;
    if (bos && n < max)
        tokens[n++] = LLM_BOS;
    if (*text && n < max) {
        int sp = tok_lookup(m, " ", 1);
        if (sp >= 0)
            tokens[n++] = sp;
    }
    int first = n;
    for (const char *c = text; *c && n < max; ) {
        int len = 1;                          /* one UTF-8 code point */
        while (len < 4 && (c[len] & 0xc0) == 0x80)
            len++;
        int id = tok_lookup(m, c, len);
        if (id >= 0) {
            tokens[n++] = id;
        } else {
            for (int i = 0; i < len && n < max; i++)
                tokens[n++] = (uint8_t)c[i] + 3;
        }
        c += len;
    }
    /* merge the best scoring adjacent pair until none is in the vocab */
    char buf[64];
    for (;;) {
        float best = -1e30f;
        int bi = -1, bid = -1;
        for (int i = first; i < n - 1; i++) {
            int a = tokens[i], b = tokens[i + 1];
            if (is_byte_token(m, a) || is_byte_token(m, b))
                continue;
            int la = m->tok_off[a + 1] - m->tok_off[a];
            int lb = m->tok_off[b + 1] - m->tok_off[b];
            if (la + lb > (int)sizeof(buf))
                continue;
            for (int j = 0; j < la; j++)
                buf[j] = m->tok_str[m->tok_off[a] + j];
            for (int j = 0; j < lb; j++)
                buf[la + j] = m->tok_str[m->tok_off[b] + j];
            int id = tok_lookup(m, buf, la + lb);
            if (id >= 0 && m->scores[id] > best) {
                best = m->scores[id];
                bi = i;
                bid = id;
            }
        }
        if (bi < 0)
            break;
        tokens[bi] = bid;
        for (int i = bi + 1; i < n - 1; i++)
            tokens[i] = tokens[i + 1];
        n--;
    }
    return n;
}

const char *llm_piece(struct llm *m, int prev, int tok)
{
    static char out[64];
    if (tok < 0 || tok >= (int)m->h.vocab_size)
        return "";
    if (is_byte_token(m, tok)) {
        out[0] = (char)(tok - 3);
        out[1] = '\0';
        return out;
    }
    const char *s = m->tok_str + m->tok_off[tok];
    int len = m->tok_off[tok + 1] - m->tok_off[tok];
    if (prev == LLM_BOS && len && s[0] == ' ') {
        s++;
        len--;
    }
    if (len > (int)sizeof(out) - 1)
        len = sizeof(out) - 1;
    for (int i = 0; i < len; i++)
        out[i] = s[i];
    out[len] = '\0';
    return out;
}

/* ---- kernels ---- */

/* Quantizes rows x[b * stride .. + n) to int8, one scale each (m->xs). */
static void quantize(struct llm *m, const float *x, int stride, int n, int nb)
{
    for (int b = 0; b < nb; b++, x += stride) {
        int8_t *xq = m->xq + b * m->act;
        float amax = absmax(x, n);
        float inv = 127.0f / amax;
        for (int i = 0; i < n; i++)
            xq[i] = (int8_t)fround(x[i] * inv);
        m->xs[b] = amax / 127.0f;
#ifdef LLM_SIMD
        /* SXTB16 order: word i*2 = (x0, x2), i*2+1 = (x1, x3) */
        uint32_t *xp = m->xp + b * m->act / 2;
        for (int i = 0; i < n; i += 4) {
            const uint8_t *q = (const uint8_t *)&xq[i];
            xp[i / 2]     = (uint32_t)(uint16_t)(int16_t)(int8_t)q[0] |
                            (uint32_t)(uint16_t)(int16_t)(int8_t)q[2] << 16;
            xp[i / 2 + 1] = (uint32_t)(uint16_t)(int16_t)(int8_t)q[1] |
                            (uint32_t)(uint16_t)(int16_t)(int8_t)q[3] << 16;
        }
#endif
    }
}

#ifdef LLM_SIMD
static inline uint32_t sxtb16_ror8(uint32_t v)
{
    uint32_t r;
    __asm__ ("sxtb16 %0, %1, ror #8" : "=r" (r) : "r" (v));
    return r;
}
#endif

/* row w . quantized activation row b */
static int32_t dot_q8(const int8_t *w, const struct llm *m, int b, int n)
{
#ifdef LLM_SIMD
    const uint32_t *wp = (const uint32_t *)w;
    const uint32_t *xp = m->xp + b * m->act / 2;
    int32_t acc = 0;
    for (int i = 0; i < n / 8; i++) {
        uint32_t v0 = wp[0], v1 = wp[1];
        acc = __smlad(__sxtb16(v0), xp[0], acc);
        acc = __smlad(sxtb16_ror8(v0), xp[1], acc);
        acc = __smlad(__sxtb16(v1), xp[2], acc);
        acc = __smlad(sxtb16_ror8(v1), xp[3], acc);
        wp += 2;
        xp += 4;
    }
    if (n & 4) {
        uint32_t v = wp[0];
        acc = __smlad(__sxtb16(v), xp[0], acc);
        acc = __smlad(sxtb16_ror8(v), xp[1], acc);
    }
    return acc;
#else
    const int8_t *xq = m->xq + b * m->act;
    int32_t acc = 0;
    for (int i = 0; i < n; i++)
        acc += (int32_t)w[i] * xq[i];
    return acc;
#endif
}

/* Streamed tensors are read in chunks of rows, in their fixed order of use
 * (m->sched, wrapping into the next token). Synchronous: chunk k is read
 * into slot 0 when needed. Asynchronous: it is already on its way into
 * slot k & 1, and finishing it queues chunk k + 2. */
static void chunk_start(struct llm *m, unsigned k)
{
    const struct llm_chunk *c = &m->sched[k % m->nsched];
    m->ticket[k & 1] = m->io.start(m->io.ctx, c->start, m->sbuf[k & 1], c->len);
}

static const int8_t *stream_get(struct llm *m)
{
    const struct llm_chunk *c = &m->sched[m->k % m->nsched];
    if (!m->io.start) {
        m->io.read(m->io.ctx, c->start, m->sbuf[0], c->len);
        return (const int8_t *)(m->sbuf[0] + c->skip);
    }
    m->io.wait(m->io.ctx, m->ticket[m->k & 1]);
    return (const int8_t *)(m->sbuf[m->k & 1] + c->skip);
}

static void stream_next(struct llm *m)
{
    if (m->io.start)
        chunk_start(m, m->k + 2);
    m->k++;
}

/* out[b * stride + r] for rows r0.. of one tensor (q at row r0) */
static void mm_rows(struct llm *m, float *out, int stride, const int8_t *q,
                    int r0, int nr, int cols, int nb)
{
    for (int r = r0; r < r0 + nr; r++) {
        float s;
        __builtin_memcpy(&s, q + cols, 4);
        for (int b = 0; b < nb; b++)
            out[b * stride + r] = (float)dot_q8(q, m, b, cols) * (s * m->xs[b]);
        q += cols + 4;
        if (m->io.yield && (r & 15) == 15)
            m->io.yield();
    }
}

/* out[b * stride + r] = (W x_b)[r] for the nb quantized rows: every weight
 * row is read once for the whole batch. */
static void matmul(struct llm *m, float *out, int stride, struct llm_qt *t,
                   int nb)
{
    if (t->q) {
        mm_rows(m, out, stride, t->q, 0, t->rows, t->cols, nb);
        return;
    }
    for (int r = 0; r < t->rows; ) {
        const struct llm_chunk *c = &m->sched[m->k % m->nsched];
        mm_rows(m, out, stride, stream_get(m), c->r0, c->nr, t->cols, nb);
        r += c->nr;
        stream_next(m);
    }
}

static void rmsnorm(float *o, const float *x, const float *w, int n)
{
    float ss = 0.0f;
    for (int i = 0; i < n; i++)
        ss += x[i] * x[i];
    ss = llm_rsqrtf(ss / (float)n + 1e-5f);
    for (int i = 0; i < n; i++)
        o[i] = w[i] * (ss * x[i]);
}

/* cos/sin of pos * freq[j], once per token */
static void rope_prepare(struct llm *m, int pos)
{
    for (int j = 0; j < m->head_size / 2; j++) {
        float a = (float)pos * m->freq[j];
        m->rs[j] = llm_sinf(a);
        m->rc[j] = llm_sinf(a + 1.57079633f);
    }
}

static void rope(struct llm *m, float *v, int n)
{
    int hs = m->head_size;
    for (int j = 0; j < hs / 2; j++) {
        float s = m->rs[j], c = m->rc[j];
        for (int i = 2 * j; i < n; i += hs) {
            float x0 = v[i], x1 = v[i + 1];
            v[i] = x0 * c - x1 * s;
            v[i + 1] = x0 * s + x1 * c;
        }
    }
}

/* Stores v[n] as int8 + scale; *fx (if set) gets the scale in Q24. */
static void kv_store(const float *v, int n, int8_t *q, float *s, int32_t *fx)
{
    float amax = absmax(v, n);
    float inv = 127.0f / amax;
    for (int i = 0; i < n; i++)
        q[i] = (int8_t)fround(v[i] * inv);
    float sc = amax / 127.0f;
    if (s)
        *s = sc;
    if (fx) {
        float f = sc * 16777216.0f;
        *fx = f < 2147483520.0f ? (int32_t)f : 0x7fffff80;
    }
}

/* float bits as an int with the same ordering as the floats */
static inline int32_t forder(float f)
{
    int32_t i = (int32_t)fbits(f);
    return i ^ ((i >> 31) & 0x7fffffff);
}

/* Attention with integer softmax: scores in float (one multiply each), then
 * exp2 from a table in Q15, V weighted in fixed point. */
static void attention(struct llm *m, int l, int pos, const float *qv,
                      float *out)
{
    int hs = m->head_size, kvd = m->kv_dim, seq = m->seq;
    int rep = m->h.n_heads / m->h.n_kv_heads;
    const int8_t *kl = m->kq + (size_t)l * seq * kvd;
    const int8_t *vl = m->vq + (size_t)l * seq * kvd;
    const float *ksl = m->ks + l * seq;
    const int32_t *vxl = m->vsx + l * seq;
    int32_t *e = (int32_t *)m->att;
    int8_t qh[64];
    int64_t acc[64];

    for (int h = 0; h < (int)m->h.n_heads; h++) {
        const float *q = qv + h * hs;
        int ko = (h / rep) * hs;
        float amax = absmax(q, hs);
        float inv = 127.0f / amax;
        for (int i = 0; i < hs; i++)
            qh[i] = (int8_t)fround(q[i] * inv);
        int32_t mx = INT32_MIN;
        for (int t = 0; t <= pos; t++) {
            const int8_t *k = kl + t * kvd + ko;
            int32_t d = 0;
            for (int i = 0; i < hs; i++)
                d += (int32_t)qh[i] * k[i];
            float sc = (float)d * ksl[t];
            m->att[t] = sc;
            if (forder(sc) > mx)
                mx = forder(sc);
        }
        /* y = (max - score) * c in Q16 base-2 units; e = 2^-y in Q15 */
        float fmx = bitsf((uint32_t)(mx ^ ((mx >> 31) & 0x7fffffff)));
        float c = (amax / 127.0f) * llm_rsqrtf((float)hs) * 1.44269504f *
                  65536.0f;
        uint32_t lim = fbits(15.0f * 65536.0f / c);   /* e underflows */
        int32_t sum = 0;
        for (int t = 0; t <= pos; t++) {
            float dd = fmx - m->att[t];
            int32_t ev = 0;
            if ((fbits(dd) & 0x7fffffff) < lim) {
                int32_t y = fround(dd * c);
                ev = m->exp2tab[(y >> 8) & 255] >> (y >> 16);
            }
            e[t] = ev;
            sum += ev;
        }
        for (int i = 0; i < hs; i++)
            acc[i] = 0;
        for (int t = 0; t <= pos; t++) {
            if (!e[t])
                continue;
            int32_t w = (int32_t)(((int64_t)e[t] * vxl[t]) >> 15);
            const int8_t *v = vl + t * kvd + ko;
            for (int i = 0; i < hs; i++)
                acc[i] += (int64_t)w * v[i];
        }
        /* o = sum_t e/sum * vs * v = acc / (2^9 sum) */
        float *o = out + h * hs;
        float scale = 1.0f / ((float)sum * 512.0f);
        for (int i = 0; i < hs; i++)
            o[i] = (float)(int32_t)(acc[i] >> 8) * (scale * 256.0f);
    }
}

float *llm_forward_batch(struct llm *m, const int *tokens, int n, int pos0,
                         bool all)
{
    int dim = m->h.dim, hid = m->h.hidden_dim, kvd = m->kv_dim;
    int L = m->h.n_layers, V = m->h.vocab_size;
    if (n > m->nb)
        n = m->nb;

    for (int b = 0; b < n; b++) {             /* embedding rows */
        struct llm_qt *e = &m->emb;
        int8_t row[512 + 4];
        float s;
        const int8_t *q;
        if (e->q) {
            q = e->q + (size_t)tokens[b] * (dim + 4);
        } else {
            io_read(m, e->off + (uint32_t)tokens[b] * (dim + 4), row, dim + 4);
            q = row;
        }
        __builtin_memcpy(&s, q + dim, 4);
        for (int i = 0; i < dim; i++)
            m->x[b * dim + i] = (float)q[i] * s;
    }

    for (int l = 0; l < L; l++) {
        struct llm_qt *w = m->w + l * W_PER_LAYER;
        for (int b = 0; b < n; b++)
            rmsnorm(m->xb + b * dim, m->x + b * dim, m->norms + 2 * l * dim, dim);
        quantize(m, m->xb, dim, dim, n);
        matmul(m, m->q, dim, &w[0], n);
        matmul(m, m->xb2, dim, &w[1], n);      /* k */
        matmul(m, m->hb2, hid, &w[2], n);      /* v */
        for (int b = 0; b < n; b++) {          /* the batch's K/V first */
            int pos = pos0 + b, so = l * m->seq + pos;
            size_t o = (size_t)so * kvd;
            rope_prepare(m, pos);
            rope(m, m->q + b * dim, dim);
            rope(m, m->xb2 + b * dim, kvd);
            kv_store(m->xb2 + b * dim, kvd, m->kq + o, m->ks + so, NULL);
            kv_store(m->hb2 + b * hid, kvd, m->vq + o, NULL, m->vsx + so);
        }
        for (int b = 0; b < n; b++)            /* then causal attention */
            attention(m, l, pos0 + b, m->q + b * dim, m->xb + b * dim);
        quantize(m, m->xb, dim, dim, n);
        matmul(m, m->xb2, dim, &w[3], n);
        for (int i = 0; i < n * dim; i++)
            m->x[i] += m->xb2[i];

        for (int b = 0; b < n; b++)
            rmsnorm(m->xb + b * dim, m->x + b * dim,
                    m->norms + (2 * l + 1) * dim, dim);
        quantize(m, m->xb, dim, dim, n);
        matmul(m, m->hb, hid, &w[4], n);
        matmul(m, m->hb2, hid, &w[6], n);
        for (int i = 0; i < n * hid; i++) {    /* SwiGLU, sigmoid table */
            float v = m->hb[i];
            int k = fround(v * 32.0f) + 256;
            float sg = k < 0 ? 0.0f : k > 511 ? 1.0f : m->sigtab[k];
            m->hb[i] = v * sg * m->hb2[i];
        }
        quantize(m, m->hb, hid, hid, n);
        matmul(m, m->xb, dim, &w[5], n);
        for (int i = 0; i < n * dim; i++)
            m->x[i] += m->xb[i];
    }

    /* classifier for every position, or just the last one */
    int first = all ? 0 : n - 1, cnt = n - first;
    for (int b = 0; b < cnt; b++)
        rmsnorm(m->xb + b * dim, m->x + (first + b) * dim, m->norms + 2 * L * dim,
                dim);
    quantize(m, m->xb, dim, dim, cnt);
    matmul(m, m->logits, V, &m->cls, cnt);
    return m->logits;
}

float *llm_forward(struct llm *m, int token, int pos)
{
    return llm_forward_batch(m, &token, 1, pos, true);
}

int llm_sample(struct llm *m, float *logits, float temp, int topk,
               uint32_t *rng)
{
    int V = m->h.vocab_size;
    if (temp <= 0.0f || topk <= 1) {
        int best = 0;
        for (int i = 1; i < V; i++)
            if (logits[i] > logits[best])
                best = i;
        return best;
    }
    if (topk > 64)
        topk = 64;
    /* insertion into a descending top-k list */
    int n = 0;
    for (int i = 0; i < V; i++) {
        if (n == topk && logits[i] <= logits[m->ids[n - 1]])
            continue;
        int j = n < topk ? n++ : n - 1;
        while (j > 0 && logits[m->ids[j - 1]] < logits[i]) {
            m->ids[j] = m->ids[j - 1];
            j--;
        }
        m->ids[j] = i;
    }
    float mx = logits[m->ids[0]], sum = 0.0f, p[64];
    for (int i = 0; i < n; i++) {
        p[i] = llm_expf((logits[m->ids[i]] - mx) / temp);
        sum += p[i];
    }
    uint32_t r = *rng;                        /* xorshift32 */
    r ^= r << 13;
    r ^= r >> 17;
    r ^= r << 5;
    *rng = r;
    float u = (float)(r >> 8) * (1.0f / 16777216.0f) * sum;
    for (int i = 0; i < n; i++) {
        u -= p[i];
        if (u <= 0.0f)
            return m->ids[i];
    }
    return m->ids[n - 1];
}

/* ---- speculative decoding ---- */

static float urand(uint32_t *rng)
{
    uint32_t r = *rng;                        /* xorshift32 */
    r ^= r << 13;
    r ^= r >> 17;
    r ^= r << 5;
    *rng = r;
    return (float)(r >> 8) * (1.0f / 16777216.0f);
}

static int argmax(const float *l, int V)
{
    int best = 0;
    for (int i = 1; i < V; i++)
        if (l[i] > l[best])
            best = i;
    return best;
}

/* p = softmax(l / temp) */
static void softmax_t(float *p, const float *l, int V, float temp)
{
    float mx = l[argmax(l, V)], sum = 0.0f, inv = 1.0f / temp;
    for (int i = 0; i < V; i++)
        sum += p[i] = llm_expf((l[i] - mx) * inv);
    inv = 1.0f / sum;
    for (int i = 0; i < V; i++)
        p[i] *= inv;
}

/* sample from p[] whose entries add up to total */
static int sample_p(const float *p, int V, float total, uint32_t *rng)
{
    float u = urand(rng) * total;
    for (int i = 0; i < V; i++) {
        u -= p[i];
        if (u <= 0.0f)
            return i;
    }
    return V - 1;
}

int llm_speculate(struct llm *t, struct llm *d, int last, int pos, int k,
                  float temp, uint32_t *rng, int *out)
{
    int V = t->h.vocab_size, toks[LLM_MAX_BATCH];
    if (k > t->nb - 1)
        k = t->nb - 1;
    if (k > d->nb - 1)
        k = d->nb - 1;
    if (k > t->seq - 2 - pos)
        k = t->seq - 2 - pos;
    if (k < 0)
        k = 0;
    float *p = d->logits;                     /* row 0: scratch */

    /* draft k tokens; draft probabilities in d->logits rows 1..k */
    toks[0] = last;
    for (int i = 0; i < k; i++) {
        float *lq = llm_forward(d, toks[i], pos + i);
        if (temp > 0.0f) {
            float *q = d->logits + (i + 1) * V;
            softmax_t(q, lq, V, temp);
            toks[i + 1] = sample_p(q, V, 1.0f, rng);
        } else {
            toks[i + 1] = argmax(lq, V);
        }
    }

    /* one pass of the target over last + the drafts */
    float *lp = llm_forward_batch(t, toks, k + 1, pos, true);
    int n = 0;
    for (int i = 0; i < k; i++) {
        const float *pl = lp + i * V;
        int x = toks[i + 1];
        if (temp <= 0.0f) {
            int a = argmax(pl, V);
            out[n++] = a;
            if (a != x)
                return n;
            continue;
        }
        const float *q = d->logits + (i + 1) * V;
        softmax_t(p, pl, V, temp);
        if (urand(rng) * q[x] < p[x]) {       /* accept: min(1, p/q) */
            out[n++] = x;
            continue;
        }
        float z = 0.0f;                       /* resample from (p - q)+ */
        for (int j = 0; j < V; j++)
            z += p[j] = p[j] > q[j] ? p[j] - q[j] : 0.0f;
        out[n++] = z > 0.0f ? sample_p(p, V, z, rng) : x;
        return n;
    }
    /* all accepted: a bonus token from the last row; the draft has not
     * seen its own last token yet */
    const float *pl = lp + k * V;
    if (temp > 0.0f) {
        softmax_t(p, pl, V, temp);
        out[n++] = sample_p(p, V, 1.0f, rng);
    } else {
        out[n++] = argmax(pl, V);
    }
    if (k > 0)
        llm_forward(d, toks[k], pos + k);
    return n;
}

/* ---- loading ---- */

/* Rows per streamed chunk: the sector-aligned read (up to 1022 bytes of
 * rounding) must stay within LLM_CHUNK, one SD command. */
#define CHUNK_ROWS(rs) ((LLM_CHUNK - 1024) / (rs) > 0 ? (LLM_CHUNK - 1024) / (rs) : 1)

static void qt_init(struct llm_qt *t, uint32_t *off, int rows, int cols)
{
    t->q = NULL;
    t->off = *off;
    t->rows = rows;
    t->cols = cols;
    *off += qt_bytes(t);
}

const char *llm_open(struct llm *m, const struct llm_io *io,
                     struct llm_mem *mem, int nmem, int max_seq, int max_batch,
                     unsigned flags)
{
    zero(m, sizeof(*m));
    m->io = *io;
    m->io.start = NULL;                       /* synchronous while loading */
    bool (*read)(void *, uint32_t, void *, uint32_t) = io->read;
    void *ctx = io->ctx;
    for (int i = 0; i < nmem && i < 2; i++)
        m->mem[i] = mem[i];
    struct llm_header *h = &m->h;
    if (!read(ctx, 0, h, sizeof(*h)))
        return "Can't read model";
    if (h->magic != LLM_MAGIC || h->version != LLM_VERSION)
        return "Not a model file";
    if (h->dim > 512 || h->dim % 4 || h->hidden_dim > 1024 ||
        h->hidden_dim % 4 || h->n_heads == 0 || h->n_kv_heads == 0 ||
        h->n_heads % h->n_kv_heads || h->dim / h->n_heads > 64 ||
        h->vocab_size > 4096)
        return "Unsupported model shape";
    h->name[sizeof(h->name) - 1] = '\0';
    h->prompt[sizeof(h->prompt) - 1] = '\0';
    h->stop[sizeof(h->stop) - 1] = '\0';

    int dim = h->dim, hid = h->hidden_dim, V = h->vocab_size;
    int L = h->n_layers;
    m->head_size = dim / h->n_heads;
    m->kv_dim = m->head_size * h->n_kv_heads;
    m->seq = max_seq < (int)h->seq_len ? max_seq : (int)h->seq_len;
    int act = hid > dim ? hid : dim;
    m->act = act;
    m->nb = max_batch < 1 ? 1 : max_batch > LLM_MAX_BATCH ? LLM_MAX_BATCH
                                                         : max_batch;
    int nb = m->nb;
    size_t free0 = m->mem[0].size + m->mem[1].size;

    /* tokenizer */
    if (flags & LLM_NO_TOKENIZER)
        goto state;
    m->scores = take(m, V * sizeof(float));
    uint8_t *lens = take(m, V);
    m->tok_off = take(m, (V + 1) * sizeof(uint16_t));
    uint32_t pool = h->tok_size - V * 5;
    m->tok_str = take(m, pool);
    m->hash_mask = 1;
    while (m->hash_mask < 2 * V)
        m->hash_mask <<= 1;
    m->tok_hash = take(m, m->hash_mask * sizeof(uint16_t));
    m->hash_mask--;
    if (m->oom)
        return "Out of memory (tokenizer)";
    if (!read(ctx, h->tok_off, m->scores, V * 4) ||
        !read(ctx, h->tok_off + V * 4, lens, V) ||
        !read(ctx, h->tok_off + V * 5, m->tok_str, pool))
        return "Can't read tokenizer";
    m->tok_off[0] = 0;
    for (int i = 0; i < V; i++)
        m->tok_off[i + 1] = m->tok_off[i] + lens[i];
    zero(m->tok_hash, (m->hash_mask + 1) * sizeof(uint16_t));
    for (int id = 0; id < V; id++) {
        int n = lens[id];
        const char *s = m->tok_str + m->tok_off[id];
        if (!n || is_byte_token(m, id))
            continue;
        uint32_t i = str_hash(s, n) & m->hash_mask;
        while (m->tok_hash[i])
            i = (i + 1) & m->hash_mask;
        m->tok_hash[i] = id + 1;
    }

    /* state, KV cache, norms */
state:
    m->x = take(m, nb * dim * 4);
    m->xb = take(m, nb * dim * 4);
    m->xb2 = take(m, nb * dim * 4);
    m->q = take(m, nb * dim * 4);
    m->hb = take(m, nb * hid * 4);
    m->hb2 = take(m, nb * hid * 4);
    m->att = take(m, m->seq * 4);
    m->logits = take(m, nb * V * 4);
    m->xq = take(m, nb * act);
    m->xp = take(m, nb * act * 2);
    m->ids = take(m, 64 * sizeof(int));
    m->freq = take(m, m->head_size / 2 * 4);
    m->norms = take(m, (2 * L + 1) * dim * 4);
    m->w = take(m, L * W_PER_LAYER * sizeof(struct llm_qt));
    size_t kvb = (size_t)L * m->seq * m->kv_dim;
    m->kq = take(m, kvb);
    m->vq = take(m, kvb);
    m->ks = take(m, (size_t)L * m->seq * 4);
    m->vsx = take(m, (size_t)L * m->seq * 4);
    m->sigtab = take(m, 512 * sizeof(float));
    m->exp2tab = take(m, 256 * sizeof(uint16_t));
    if (m->oom)
        return "Out of memory (KV cache)";
    m->st.kv = 2 * kvb + 2 * (size_t)L * m->seq * 4;
    for (int i = 0; i < 512; i++)             /* sigmoid on [-8, 8) */
        m->sigtab[i] = 1.0f / (1.0f + llm_expf(-(float)(i - 256) / 32.0f));
    for (int i = 0; i < 256; i++)             /* 2^(-i/256) in Q15 */
        m->exp2tab[i] = (uint16_t)fround(32767.0f *
                        llm_expf(-(float)i / 256.0f * 0.693147181f));
    if (!read(ctx, h->norm_off, m->norms, (2 * L + 1) * dim * 4))
        return "Can't read norms";
    for (int j = 0; j < m->head_size / 2; j++)   /* theta^(-2j/hs) */
        m->freq[j] = llm_expf(-(float)(2 * j) / (float)m->head_size *
                              9.21034037f);   /* ln 10000 */
    if (h->rope_theta != 10000.0f)
        return "Unsupported RoPE theta";

    /* tensor table */
    uint32_t off = h->tensor_off;
    qt_init(&m->emb, &off, V, dim);
    for (int l = 0; l < L; l++) {
        struct llm_qt *w = m->w + l * W_PER_LAYER;
        qt_init(&w[0], &off, dim, dim);
        qt_init(&w[1], &off, m->kv_dim, dim);
        qt_init(&w[2], &off, m->kv_dim, dim);
        qt_init(&w[3], &off, dim, dim);
        qt_init(&w[4], &off, hid, dim);
        qt_init(&w[5], &off, dim, hid);
        qt_init(&w[6], &off, hid, dim);
    }
    bool shared = h->flags & LLM_SHARED_CLASSIFIER;
    if (!shared)
        qt_init(&m->cls, &off, V, dim);

    /* Unless everything fits: stream slots and schedule first (a chunk plus sector
     * alignment slack), then the embedding (and classifier), then layer
     * tensors in file order wherever they fit. */
    size_t all = qt_bytes(&m->emb) + (shared ? 0 : qt_bytes(&m->cls));
    for (int i = 0; i < L * W_PER_LAYER; i++)
        all += qt_bytes(&m->w[i]);
    /* streamed tensors in the order llm_forward uses them, in chunks; the
     * schedule is sized for the worst case before the weights take RAM */
    static const uint8_t order[W_PER_LAYER] = { 0, 1, 2, 3, 4, 6, 5 };
    struct llm_qt *use[L * W_PER_LAYER + 1];
    int nuse = 0, nch = 0;
    for (int l = 0; l < L; l++)
        for (int i = 0; i < W_PER_LAYER; i++)
            use[nuse++] = &m->w[l * W_PER_LAYER + order[i]];
    use[nuse++] = &m->cls;                    /* = emb when shared */
    for (int i = 0; i < nuse; i++) {
        int rpc = CHUNK_ROWS(use[i]->cols + 4);
        nch += (use[i]->rows + rpc - 1) / rpc;
    }
    size_t slot = LLM_CHUNK;
    if (all > m->mem[0].size + m->mem[1].size) {
        m->sbuf[0] = take(m, slot);
        m->sbuf[1] = io->start ? take(m, slot) : m->sbuf[0];
        m->sched = take(m, nch * sizeof(*m->sched));
        if (m->oom)
            return "Out of memory (stream buffer)";
    }
    m->st.other = free0 - (m->mem[0].size + m->mem[1].size) - m->st.kv;
    if (!place(m, &m->emb) || (!shared && !place(m, &m->cls)))
        return "Can't read weights";
    for (int i = 0; i < L * W_PER_LAYER; i++)
        if (!place(m, &m->w[i]))
            return "Can't read weights";
    if (shared)
        m->cls = m->emb;
    if (m->st.streamed && !m->sched)          /* split across the regions */
        return "Out of memory (weights)";

    for (int i = 0; i < nuse; i++) {
        struct llm_qt *t = use[i];
        if (t->q)
            continue;
        int rs = t->cols + 4, rpc = CHUNK_ROWS(rs);
        for (int r = 0; r < t->rows; r += rpc) {
            struct llm_chunk *c = &m->sched[m->nsched++];
            uint32_t a = t->off + (uint32_t)r * rs;
            c->r0 = r;
            c->nr = t->rows - r < rpc ? t->rows - r : rpc;
            c->start = a & ~511u;            /* whole sectors */
            c->len = ((a + c->nr * rs + 511) & ~511u) - c->start;
            c->skip = a - c->start;
        }
    }
    if (io->start && m->nsched) {             /* start the pipeline */
        m->io.start = io->start;
        chunk_start(m, 0);
        chunk_start(m, 1);
    }
    m->st.mem_left = m->mem[0].size + m->mem[1].size;
    return NULL;
}
