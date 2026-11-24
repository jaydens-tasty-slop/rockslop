/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * RockTrack: the chip synth. Every sound is rendered into an ordinary XM
 * sample; the controls ride in the sample name ("rt1:pulse,4"), so any
 * tracker plays the song and RockTrack can edit the sound again.
 * Formulas: DESIGN.md "Synth".
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
#include "rt.h"

#define AMP         100
#define CYCLE       64
#define DRUM_RATE   16726
#define NOISE_LEN   4096

static const char *const src_names[SRC_COUNT] = {
    "sample", "pulse", "tri", "saw", "sine", "noise", "wave",
    "kick", "snare", "hat", "tom",
};

/* round(100 * sin(2 pi i / 64)) */
static const int8_t sine64[64] = {
    0, 10, 20, 29, 38, 47, 56, 63, 71, 77, 83, 88, 92, 96, 98, 100,
    100, 100, 98, 96, 92, 88, 83, 77, 71, 63, 56, 47, 38, 29, 20, 10,
    0, -10, -20, -29, -38, -47, -56, -63, -71, -77, -83, -88, -92, -96,
    -98, -100, -100, -100, -98, -96, -92, -88, -83, -77, -71, -63, -56,
    -47, -38, -29, -20, -10,
};

const char *synth_src_name(int src)
{
    return src >= 0 && src < SRC_COUNT ? src_names[src] : "?";
}

bool synth_is_drum(int src)
{
    return src >= SRC_KICK && src <= SRC_TOM;
}

void synth_defaults(int src, int *a, int *b)
{
    *a = 0;
    *b = 0;
    if (src == SRC_PULSE)
        *a = 8;
    else if (synth_is_drum(src)) {
        *a = 12;
        *b = src == SRC_HAT ? 3 : 6;
    }
}

static int parse_int(const char **p)
{
    int v = 0, neg = 0;
    if (**p == '-') {
        neg = 1;
        (*p)++;
    }
    while (**p >= '0' && **p <= '9')
        v = v * 10 + *(*p)++ - '0';
    return neg ? -v : v;
}

bool synth_parse(const char *name, struct synth *s)
{
    if (rb->strncmp(name, "rt1:", 4))
        return false;
    const char *p = name + 4;
    for (int i = 1; i < SRC_COUNT; i++) {
        int n = rb->strlen(src_names[i]);
        if (rb->strncmp(p, src_names[i], n) || (p[n] && p[n] != ','))
            continue;
        s->src = i;
        synth_defaults(i, &s->a, &s->b);
        p += n;
        if (*p == ',') {
            p++;
            s->a = parse_int(&p);
            if (*p == ',') {
                p++;
                s->b = parse_int(&p);
            }
        }
        return true;
    }
    return false;
}

void synth_name(const struct synth *s, char *name)
{
    if (s->src == SRC_PULSE || s->src == SRC_NOISE)
        rb->snprintf(name, 23, "rt1:%s,%d", src_names[s->src], s->a);
    else if (synth_is_drum(s->src))
        rb->snprintf(name, 23, "rt1:%s,%d,%d", src_names[s->src], s->a,
                     s->b);
    else
        rb->snprintf(name, 23, "rt1:%s", src_names[s->src]);
}

/* 2^(x/12) in Q16 for x in -12..12 semitones (drum tuning) */
static uint32_t tune_q16(int semis)
{
    semis = CLAMP(semis, -12, 12);
    return semis >= 0 ? semi_q16[semis]
                      : (uint32_t)(((uint64_t)1 << 32) / semi_q16[-semis]);
}

static uint32_t lfsr_state;

static int lfsr_step(int tap)
{
    uint32_t r = lfsr_state;
    uint32_t fb = (r ^ (r >> tap)) & 1;
    lfsr_state = (r >> 1) | (fb << 14);
    return r & 1;
}

/* sine from a 64-step table with linear interpolation; phase Q16 cycles */
static int sine_at(uint32_t phase)
{
    int i = (phase >> 10) & 63, f = phase & 1023;
    int a = sine64[i], b = sine64[(i + 1) & 63];
    return a + ((b - a) * f >> 10);
}

static void render_drum(int8_t *d, int len, int src, int tune)
{
    uint32_t t = tune_q16(tune - 12);
    uint32_t phase = 0;                 /* Q16 cycles */
    /* e: exponential decay in Q16, falling ~8 (kick) or ~3 (tom) e-folds */
    int32_t e = 65536;
    int32_t r = 65536 - (src == SRC_KICK ? 8 : 3) * 65536 / len;
    int prev = 0;
    lfsr_state = 1;
    for (int i = 0; i < len; i++) {
        int env = (len - i) * 256 / len;            /* linear 256..0 */
        int v = 0;
        uint32_t f = 0;                             /* Hz << 8 */
        switch (src) {
        case SRC_KICK:                              /* 180 -> 45 Hz */
            f = (uint32_t)((45 * 256 + 135 * (e >> 8)) * (uint64_t)t >> 16);
            v = sine_at(phase) * env / 256;
            break;
        case SRC_SNARE: {
            int tone_env = MAX(0, 256 - i * 1024 / len);
            int noise = lfsr_step(1) ? -AMP : AMP;
            f = (uint32_t)(190 * 256 * (uint64_t)t >> 16);
            v = sine_at(phase) * tone_env * 6 / 2560 +
                noise * env * 7 / 2560;
            break;
        }
        case SRC_HAT: {
            int noise = lfsr_step(1) ? -AMP : AMP;
            v = (noise - prev) / 2 * (env * env / 256) / 256;
            prev = noise;
            break;
        }
        default:                                    /* tom: 220 -> 110 Hz */
            f = (uint32_t)((110 * 256 + 110 * (e >> 8)) * (uint64_t)t >> 16);
            v = sine_at(phase) * env / 256;
            break;
        }
        phase += f * 256 / DRUM_RATE;
        e = (int32_t)((int64_t)e * r >> 16);
        d[i] = (int8_t)CLAMP(v, -127, 127);
    }
}

static int drum_len(int src, int b)
{
    b = CLAMP(b, 0, 15);
    switch (src) {
    case SRC_KICK: return 1600 + 400 * b;
    case SRC_SNARE: return 1200 + 300 * b;
    case SRC_HAT: return 300 + 150 * b;
    default: return 1600 + 300 * b;
    }
}

/* Render the synth into the instrument's first sample (pool memory). */
bool synth_render(struct instr *in, const struct synth *s)
{
    if (s->src <= SRC_SAMPLE || s->src >= SRC_COUNT || !in->nsmp)
        return false;
    struct sample *sm = &in->smp[0];
    int len = s->src == SRC_NOISE ? NOISE_LEN :
              synth_is_drum(s->src) ? drum_len(s->src, s->b) : CYCLE;
    uint8_t steps[32];
    if (s->src == SRC_WAVE) {
        if (sm->data && sm->len == CYCLE && !sm->bits16)
            synth_wave_get(sm, steps);
        else
            for (int i = 0; i < 32; i++)
                steps[i] = 8 + (sine64[i * 2] * 7 + 50) / 100;
    }
    struct synth was;
    bool keep_tuning = synth_parse(sm->name, &was);
    int8_t *tmp = pool_alloc(len);
    if (!tmp)
        return false;
    switch (s->src) {
    case SRC_PULSE: {
        int d = CLAMP(s->a, 1, 15);
        for (int i = 0; i < CYCLE; i++)
            tmp[i] = i < 4 * d ? AMP : -AMP;
        break;
    }
    case SRC_TRI:
        for (int i = 0; i < CYCLE; i++)
            tmp[i] = AMP - AMP * ABS(2 * i - 64) / 32;
        break;
    case SRC_SAW:
        for (int i = 0; i < CYCLE; i++)
            tmp[i] = 2 * AMP * i / 64 - AMP;
        break;
    case SRC_SINE:
        for (int i = 0; i < CYCLE; i++)
            tmp[i] = sine64[i];
        break;
    case SRC_NOISE:
        lfsr_state = 1;
        for (int i = 0; i < NOISE_LEN; i++)
            tmp[i] = lfsr_step(s->a ? 6 : 1) ? -AMP : AMP;
        break;
    case SRC_WAVE:
        for (int i = 0; i < 32; i++) {
            int v = (steps[i] * 2 - 15) * AMP / 15;
            tmp[2 * i] = tmp[2 * i + 1] = v;
        }
        break;
    default:
        render_drum(tmp, len, s->src, s->a);
        break;
    }
    if (!sample_set_data(sm, len, false)) {
        pool_free(tmp);
        return false;
    }
    eng_lock();
    rb->memcpy(sm->data, tmp, len);
    if (!synth_is_drum(s->src)) {
        sm->loop = 1;
        sm->lstart = 0;
        sm->llen = len;
    }
    if (!keep_tuning) {     /* a synth's own tuning; keep the user's */
        sm->rel = 12;
        sm->fine = 0;
    }
    synth_name(s, sm->name);
    eng_unlock();
    pool_free(tmp);
    return true;
}

void synth_wave_get(const struct sample *s, uint8_t steps[32])
{
    const int8_t *d = s->data;
    for (int i = 0; i < 32; i++) {
        int v = d ? d[2 * i] : 0;
        steps[i] = CLAMP((v * 15 / AMP + 15 + 1) / 2, 0, 15);
    }
}

bool synth_wave_set(struct instr *in, const uint8_t steps[32])
{
    struct sample *sm = &in->smp[0];
    if (!sm->data || sm->len != CYCLE || sm->bits16)
        return false;
    int8_t *d = sm->data;
    for (int i = 0; i < 32; i++) {
        int v = (steps[i] * 2 - 15) * AMP / 15;
        d[2 * i] = d[2 * i + 1] = v;
    }
    return true;
}

/* ---- ADSR <-> XM volume envelope ---- */

static const uint8_t env_times[16] = {
    0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192,
};

static void env_add(struct env *e, int x, int y)
{
    if (e->n && x <= e->x[e->n - 1]) {
        e->y[e->n - 1] = y;     /* same position: the later value wins */
        return;
    }
    e->x[e->n] = x;
    e->y[e->n] = y;
    e->n++;
}

void adsr_to_env(const struct adsr *a, struct env *e)
{
    int ta = env_times[CLAMP(a->a, 0, 15)];
    int td = env_times[CLAMP(a->d, 0, 15)];
    int tr = MAX(1, env_times[CLAMP(a->r, 0, 15)]);
    int s = CLAMP(a->s, 0, 15) * 64 / 15;
    rb->memset(e, 0, sizeof(*e));
    env_add(e, 0, ta ? 0 : 64);
    env_add(e, ta, 64);
    env_add(e, ta + td, s);
    e->sus = e->n - 1;
    env_add(e, ta + td + tr, 0);
    e->flags = ENV_ON | ENV_SUS;
}

static int nearest_time(int t)
{
    int best = 0;
    for (int i = 1; i < 16; i++)
        if (ABS(env_times[i] - t) < ABS(env_times[best] - t))
            best = i;
    return best;
}

/* Recognise the shapes adsr_to_env() writes; false for anything else. */
bool env_to_adsr(const struct env *e, struct adsr *a)
{
    int n = e->n, sus = e->sus;
    if ((e->flags & (ENV_ON | ENV_SUS | ENV_LOOP)) != (ENV_ON | ENV_SUS) ||
        n < 2 || n > 4 || sus != n - 2 || e->x[0] != 0 || e->y[n - 1])
        return false;
    a->a = a->d = 0;
    if (n == 4) {                       /* attack and decay */
        if (e->y[0] != 0 || e->y[1] != 64)
            return false;
        a->a = nearest_time(e->x[1]);
        a->d = nearest_time(e->x[2] - e->x[1]);
    } else if (n == 3 && e->y[0] == 64) {
        a->d = nearest_time(e->x[1]);   /* decay only */
    } else if (n == 3 && e->y[0] == 0) {
        a->a = nearest_time(e->x[1]);   /* attack only */
    } else if (n != 2) {
        return false;
    }
    a->s = (e->y[sus] * 15 + 32) / 64;
    a->r = MAX(1, nearest_time(e->x[n - 1] - e->x[sus]));
    return true;
}
