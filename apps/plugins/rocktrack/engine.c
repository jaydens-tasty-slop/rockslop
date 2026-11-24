/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * RockTrack: XM replay engine and mixer.
 *
 * Integer only: it runs in the mixer's get_more callback (interrupt
 * context), and never allocates. The UI changes song structure only with
 * the PCM lock held (eng_lock), and calls eng_drop() before it frees
 * anything a voice may point at.
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

#define BLOCK       256     /* frames per callback */
#define RAMP        64      /* frames for volume ramps */
#define FADERS      4       /* voices that fade out a cut note */
#define NCHAN       (MAX_CH + PREVIEW_VOICES)

/* 2^(i/12) and 2^(i/768), Q16 */
const uint32_t semi_q16[13] = {
    65536, 69433, 73562, 77936, 82570, 87480,
    92682, 98193, 104032, 110218, 116772, 123715, 131072,
};
static const uint32_t fine_q16[64] = {
    65536, 65595, 65654, 65714, 65773, 65832, 65892, 65951,
    66011, 66071, 66130, 66190, 66250, 66309, 66369, 66429,
    66489, 66549, 66609, 66670, 66730, 66790, 66850, 66911,
    66971, 67032, 67092, 67153, 67213, 67274, 67335, 67395,
    67456, 67517, 67578, 67639, 67700, 67761, 67823, 67884,
    67945, 68007, 68068, 68129, 68191, 68252, 68314, 68376,
    68438, 68499, 68561, 68623, 68685, 68747, 68809, 68871,
    68933, 68996, 69058, 69120, 69183, 69245, 69308, 69370,
};
/* vibrato/tremolo sine, one half period, 0..255 */
static const uint8_t vib_sine[32] = {
    0, 24, 49, 74, 97, 120, 141, 161, 180, 197, 212, 224, 235, 244, 250,
    253, 255, 253, 250, 244, 235, 224, 212, 197, 180, 161, 141, 120, 97,
    74, 49, 24,
};

struct voice {
    const struct sample *smp;
    const void *data;
    int64_t pos;            /* Q16 frames */
    int32_t inc;            /* Q16 frames per output frame */
    bool back;
    bool on;
    int32_t lv, rv;         /* Q12 */
    int32_t tlv, trv;
    int32_t dl, dr;
    int ramp;
};

struct chan {
    struct voice v;
    const struct instr *ins;
    const struct sample *smp;
    int insnum;
    int note;               /* 0-based incl. relative note */
    int fine;
    int period, tperiod;
    int vol, pan;
    bool keyon;
    int fade;               /* 0..65536 */
    int venv_pos, penv_pos;
    int venv, penv;         /* 0..64 */
    int avib_pos, avib_tick;
    /* row state */
    uint8_t fx, param, volc;
    int arp;                /* period offset this tick */
    int vib_delta, trem_delta;
    /* effect memories */
    uint8_t m_up, m_down, m_tporta, m_vslide, m_fup, m_fdown, m_xup, m_xdown;
    uint8_t m_offset, m_retrig, m_pslide, m_tremor, m_fvup, m_fvdown, m_gslide;
    int vib_pos, vib_speed, vib_depth, vib_wave;
    int trem_pos, trem_speed, trem_depth, trem_wave;
    int loop_row, loop_count;
    int delay_tick;
    struct cell delayed;
    int cut_tick, keyoff_tick;
    int retrig_cnt;
    int tremor_cnt;
    bool tremor_off;
};

static struct chan ch[NCHAN];
static struct voice faders[FADERS];
static int next_fader;

bool eng_mute[MAX_CH];
bool eng_smooth = true;
volatile uint8_t eng_peak[MAX_CH];

static unsigned rate = 44100;
static int mode = PLAY_STOP;
static int speed, bpm, gvol;
static int cur_ord, cur_row, tick, row_ticks;
static int pat_delay;
static int jump_ord, break_row;     /* -1: none */
static bool loop_jump;
static int tick_left, tick_rem;
static bool once, ended;
static volatile int shown_ord, shown_row;

static int16_t outbuf[2][BLOCK * 2];
static int32_t acc[BLOCK * 2];
static int outcur;
static bool opened;

/* ---- frequencies ---- */

static uint32_t pow2_768(int x)     /* 2^(x/768), Q16, x >= 0 */
{
    int oct = x / 768, r = x % 768;
    uint64_t v = (uint64_t)semi_q16[r >> 6] * fine_q16[r & 63] >> 16;
    return oct < 16 ? (uint32_t)(v << oct) : 0xffffffffu;
}

static int note_period(int note, int fine)
{
    if (song.linear)
        return 7680 - note * 64 - fine / 2;
    int x = note * 64 + fine / 2;
    if (x < 0)
        x = 0;
    return (int)((27392u << 16) / pow2_768(x));
}

static uint32_t period_freq_q8(int p)   /* Hz << 8 */
{
    if (song.linear) {
        int x = 4608 - p;               /* 768 per octave above C-4 */
        int oct = 0;
        while (x < 0) {
            x += 768 * 4;
            oct -= 4;
        }
        uint64_t f = (uint64_t)(8363u << 8) * pow2_768(x) >> 16;
        return (uint32_t)(oct < 0 ? f >> -oct : f);
    }
    if (p < 1)
        p = 1;
    return (uint32_t)(3665268736u / (unsigned)p);  /* 8363 * 1712 << 8 */
}

/* ---- voices ---- */

static void voice_target(struct voice *v, int32_t l, int32_t r)
{
    if (l == v->tlv && r == v->trv)
        return;
    v->tlv = l;
    v->trv = r;
    v->dl = (l - v->lv) / RAMP;
    v->dr = (r - v->rv) / RAMP;
    v->ramp = RAMP;
}

/* Hand a sounding voice to a fader so a retrigger does not click. */
static void voice_release_to_fader(struct voice *v)
{
    if (!v->on || (!v->lv && !v->rv))
        return;
    struct voice *f = &faders[next_fader];
    next_fader = (next_fader + 1) % FADERS;
    *f = *v;
    f->tlv = f->trv = -1;
    voice_target(f, 0, 0);
}

static void mix_voice(struct voice *v, int32_t *out, int n, int peak_ch)
{
    const struct sample *s = v->smp;
    if (!v->on || !s || !v->data)
        return;
    uint32_t len = s->len;
    uint32_t lend = s->loop ? s->lstart + s->llen : len;
    if (lend > len)
        lend = len;
    int64_t pos = v->pos;
    int32_t inc = v->inc;
    int32_t lv = v->lv, rv = v->rv;
    int ramp = v->ramp;
    int peak = 0;
    bool smooth = eng_smooth;
    const int8_t *d8 = v->data;
    const int16_t *d16 = v->data;

    for (int i = 0; i < n; i++) {
        int32_t idx = (int32_t)(pos >> 16);
        if (!v->back) {
            if ((uint32_t)idx >= lend) {
                if (!s->loop || s->llen == 0) {
                    v->on = false;
                    break;
                }
                if (s->loop == 1) {
                    pos -= (int64_t)s->llen << 16;
                    while ((pos >> 16) >= lend)
                        pos -= (int64_t)s->llen << 16;
                } else {
                    pos = ((int64_t)lend << 17) - pos - 1;
                    v->back = true;
                    if ((pos >> 16) < (int64_t)s->lstart)
                        pos = (int64_t)s->lstart << 16;
                }
                idx = (int32_t)(pos >> 16);
            }
        } else if (idx < (int32_t)s->lstart) {
            pos = ((int64_t)s->lstart << 17) - pos;
            v->back = false;
            if ((pos >> 16) >= lend)
                pos = ((int64_t)lend << 16) - 1;
            idx = (int32_t)(pos >> 16);
        }
        int32_t a, smp;
        if (s->bits16)
            a = d16[idx];
        else
            a = d8[idx] << 8;
        if (smooth) {
            int32_t ni = idx + 1, b;
            if ((uint32_t)ni >= lend)
                ni = s->loop == 1 ? (int32_t)s->lstart : idx;
            b = s->bits16 ? d16[ni] : d8[ni] << 8;
            smp = a + (((b - a) * (int32_t)((uint32_t)pos & 0xffff)) >> 16);
        } else {
            smp = a;
        }
        if (ramp) {
            lv += v->dl;
            rv += v->dr;
            if (--ramp == 0) {
                lv = v->tlv;
                rv = v->trv;
            }
        }
        int32_t l = (smp * lv) >> 12, r = (smp * rv) >> 12;
        out[2 * i] += l;
        out[2 * i + 1] += r;
        int m = l < 0 ? -l : l;
        if (m > peak)
            peak = m;
        if (v->back)
            pos -= inc;
        else
            pos += inc;
    }
    v->pos = pos;
    v->lv = lv;
    v->rv = rv;
    v->ramp = ramp;
    if (peak_ch >= 0) {
        int p = peak >> 7;
        if (p > 255)
            p = 255;
        if (p > eng_peak[peak_ch])
            eng_peak[peak_ch] = p;
    }
}

/* ---- channel logic ---- */

static const struct cell empty_cell;

static const struct cell *row_cell(int ord, int row, int c)
{
    if (ord < 0 || ord >= song.nord)
        return &empty_cell;
    const struct cell *p = cell_at(song.ord[ord], row, c);
    return p ? p : &empty_cell;
}

static const struct sample *pick_sample(const struct instr *in, int note)
{
    if (!in || !in->nsmp)
        return NULL;
    int k = in->keymap[CLAMP(note - 1, 0, 95)];
    return k < in->nsmp ? &in->smp[k] : &in->smp[0];
}

static void reset_envelopes(struct chan *c)
{
    c->venv_pos = c->penv_pos = 0;
    c->fade = 65536;
    c->keyon = true;
    c->avib_pos = c->avib_tick = 0;
}

static void key_off(struct chan *c)
{
    c->keyon = false;
    if (!c->ins || !(c->ins->venv.flags & ENV_ON))
        c->vol = 0;
}

static void trigger(struct chan *c, int offset)
{
    if (!c->smp || !c->smp->data || !c->smp->len) {
        c->v.on = false;
        return;
    }
    voice_release_to_fader(&c->v);
    c->v.smp = c->smp;
    c->v.data = c->smp->data;
    c->v.back = false;
    c->v.lv = c->v.rv = 0;
    c->v.tlv = c->v.trv = -1;
    if ((uint32_t)offset >= c->smp->len) {
        c->v.on = false;
        return;
    }
    c->v.pos = (int64_t)offset << 16;
    c->v.on = true;
    if (!(c->vib_wave & 4))
        c->vib_pos = 0;
    if (!(c->trem_wave & 4))
        c->trem_pos = 0;
    c->retrig_cnt = 0;
    c->tremor_cnt = 0;
}

/* The note/instrument part of a cell, at tick 0 or a note delay. */
static void start_cell(struct chan *c, const struct cell *e)
{
    int note = e->note;
    bool porta = e->fx == 3 || e->fx == 5 || (e->vol >> 4) == 0xf;

    if (e->ins) {
        const struct instr *in = e->ins <= MAX_INS ? song.ins[e->ins - 1]
                                                   : NULL;
        c->ins = in;
        c->insnum = e->ins;
        if (!in)
            c->smp = NULL;
    }
    if (note >= 1 && note <= 96 && c->ins) {
        const struct sample *s = pick_sample(c->ins, note);
        int fine = s ? s->fine : 0;
        if (e->fx == 0xe && (e->param >> 4) == 5)
            fine = ((e->param & 15) - 8) * 16;
        int real = note - 1 + (s ? s->rel : 0);
        if (porta && c->v.on) {
            c->tperiod = note_period(real, fine);
        } else if (s) {
            c->smp = s;
            c->note = real;
            c->fine = fine;
            c->period = c->tperiod = note_period(real, fine);
            int off = 0;
            if (e->fx == 9) {
                if (e->param)
                    c->m_offset = e->param;
                off = c->m_offset << 8;
            }
            trigger(c, off);
        }
    } else if (note >= 1 && note <= 96 && !c->ins) {
        c->v.on = false;
    }
    if (e->ins && c->smp) {
        c->vol = c->smp->vol;
        if (!song.chan_pan)
            c->pan = c->smp->pan;
        reset_envelopes(c);
    }
    if (note == NOTE_OFF)
        key_off(c);
}

static void vol_slide(struct chan *c, int p)
{
    if (p >> 4)
        c->vol += p >> 4;
    else
        c->vol -= p & 15;
    c->vol = CLAMP(c->vol, 0, 64);
}

static void tone_porta(struct chan *c)
{
    int step = c->m_tporta * 4;
    if (c->period < c->tperiod) {
        c->period += step;
        if (c->period > c->tperiod)
            c->period = c->tperiod;
    } else if (c->period > c->tperiod) {
        c->period -= step;
        if (c->period < c->tperiod)
            c->period = c->tperiod;
    }
}

static int wave_value(int wave, int pos)    /* -255..255 */
{
    pos &= 63;
    switch (wave & 3) {
    case 1:
        return 255 - pos * 8;
    case 2:
        return pos < 32 ? 255 : -255;
    default:
        return pos < 32 ? vib_sine[pos] : -vib_sine[pos - 32];
    }
}

static void vibrato(struct chan *c)
{
    c->vib_delta = (wave_value(c->vib_wave, c->vib_pos) * c->vib_depth >> 7)
                   * 4;
    c->vib_pos += c->vib_speed;
}

static void tremolo(struct chan *c)
{
    c->trem_delta = wave_value(c->trem_wave, c->trem_pos) * c->trem_depth
                    >> 6;
    c->trem_pos += c->trem_speed;
}

static void row_volume_column(struct chan *c, int v)
{
    int hi = v >> 4, lo = v & 15;
    if (v >= 0x10 && v <= 0x50) {
        c->vol = v - 0x10;
        return;
    }
    switch (hi) {
    case 0x8: c->vol = MAX(0, c->vol - lo); break;
    case 0x9: c->vol = MIN(64, c->vol + lo); break;
    case 0xa: if (lo) c->vib_speed = lo; break;
    case 0xb: if (lo) c->vib_depth = lo; break;
    case 0xc: c->pan = lo << 4; break;
    case 0xf: if (lo) c->m_tporta = lo << 4; break;
    }
}

static void tick_volume_column(struct chan *c, int v)
{
    int lo = v & 15;
    switch (v >> 4) {
    case 0x6: c->vol = MAX(0, c->vol - lo); break;
    case 0x7: c->vol = MIN(64, c->vol + lo); break;
    case 0xb: vibrato(c); break;
    case 0xd: c->pan = MAX(0, c->pan - lo); break;
    case 0xe: c->pan = MIN(255, c->pan + lo); break;
    case 0xf: tone_porta(c); break;
    }
}

static void row_effect(struct chan *c, int ci)
{
    int p = c->param, x = p >> 4, y = p & 15;
    (void)ci;
    switch (c->fx) {
    case 0x1: if (p) c->m_up = p; break;
    case 0x2: if (p) c->m_down = p; break;
    case 0x3: if (p) c->m_tporta = p; break;
    case 0x4:
        if (x) c->vib_speed = x;
        if (y) c->vib_depth = y;
        break;
    case 0x5: case 0x6: case 0xa:
        if (p) c->m_vslide = p;
        break;
    case 0x7:
        if (x) c->trem_speed = x;
        if (y) c->trem_depth = y;
        break;
    case 0x8: c->pan = p; break;
    case 0xb:
        if (mode == PLAY_SONG) {
            jump_ord = p;
            if (break_row < 0)
                break_row = 0;
        }
        break;
    case 0xc: c->vol = MIN(p, 64); break;
    case 0xd:
        break_row = x * 10 + y;
        if (jump_ord < 0)
            jump_ord = mode == PLAY_SONG ? cur_ord + 1 : cur_ord;
        break;
    case 0xe:
        switch (x) {
        case 0x1:
            if (y) c->m_fup = y;
            c->period -= c->m_fup * 4;
            break;
        case 0x2:
            if (y) c->m_fdown = y;
            c->period += c->m_fdown * 4;
            break;
        case 0x4: c->vib_wave = y; break;
        case 0x6:
            if (!y) {
                c->loop_row = cur_row;
            } else if (!c->loop_count) {
                c->loop_count = y;
                loop_jump = true;
                break_row = c->loop_row;
            } else if (--c->loop_count) {
                loop_jump = true;
                break_row = c->loop_row;
            }
            break;
        case 0x7: c->trem_wave = y; break;
        case 0xa:
            if (y) c->m_fvup = y;
            c->vol = MIN(64, c->vol + c->m_fvup);
            break;
        case 0xb:
            if (y) c->m_fvdown = y;
            c->vol = MAX(0, c->vol - c->m_fvdown);
            break;
        case 0xc:
            c->cut_tick = y;
            if (!y)
                c->vol = 0;
            break;
        case 0xe:
            if (!pat_delay)
                pat_delay = y;
            break;
        }
        break;
    case 0xf:
        if (p >= 0x20)
            bpm = p;
        else if (p)
            speed = p;
        break;
    case 16: gvol = MIN(p, 64); break;                  /* G */
    case 17: if (p) c->m_gslide = p; break;              /* H */
    case 20:                                             /* K */
        c->keyoff_tick = p;
        if (!p)
            key_off(c);
        break;
    case 21: c->venv_pos = c->penv_pos = p; break;       /* L */
    case 25: if (p) c->m_pslide = p; break;              /* P */
    case 27:                                             /* R */
        if (x) c->m_retrig = (c->m_retrig & 15) | (x << 4);
        if (y) c->m_retrig = (c->m_retrig & 0xf0) | y;
        break;
    case 29: if (p) c->m_tremor = p; break;              /* T */
    case 33:                                             /* X */
        if (x == 1) {
            if (y) c->m_xup = y;
            c->period -= c->m_xup;
        } else if (x == 2) {
            if (y) c->m_xdown = y;
            c->period += c->m_xdown;
        }
        break;
    }
}

static void tick_effect(struct chan *c, int t)
{
    int p = c->param, x = p >> 4, y = p & 15;
    switch (c->fx) {
    case 0x0:
        if (p) {
            int k = t % 3;
            int semis = k == 1 ? x : k == 2 ? y : 0;
            if (song.linear)
                c->arp = -semis * 64;
            else
                c->arp = note_period(c->note + semis, c->fine) - c->period;
        }
        break;
    case 0x1:
        c->period -= c->m_up * 4;
        break;
    case 0x2:
        c->period += c->m_down * 4;
        break;
    case 0x3:
        tone_porta(c);
        break;
    case 0x4:
        vibrato(c);
        break;
    case 0x5:
        tone_porta(c);
        vol_slide(c, c->m_vslide);
        break;
    case 0x6:
        vibrato(c);
        vol_slide(c, c->m_vslide);
        break;
    case 0x7:
        tremolo(c);
        break;
    case 0xa:
        vol_slide(c, c->m_vslide);
        break;
    case 0xe:
        if (x == 0x9 && y && t % y == 0)
            trigger(c, 0);
        else if (x == 0xc && t == c->cut_tick)
            c->vol = 0;
        else if (x == 0xd && t == c->delay_tick) {
            start_cell(c, &c->delayed);
            if (c->delayed.vol)
                row_volume_column(c, c->delayed.vol);
        }
        break;
    case 17:                                            /* H */
        if (c->m_gslide >> 4)
            gvol = MIN(64, gvol + (c->m_gslide >> 4));
        else
            gvol = MAX(0, gvol - (c->m_gslide & 15));
        break;
    case 20:
        if (t == c->keyoff_tick)
            key_off(c);
        break;
    case 25:                                            /* P */
        if (c->m_pslide >> 4)
            c->pan = MIN(255, c->pan + (c->m_pslide >> 4));
        else
            c->pan = MAX(0, c->pan - (c->m_pslide & 15));
        break;
    case 27: {                                          /* R */
        int iv = c->m_retrig & 15;
        if (iv && ++c->retrig_cnt >= iv) {
            static const int8_t add[16] = {
                0, -1, -2, -4, -8, -16, 0, 0, 0, 1, 2, 4, 8, 16, 0, 0 };
            int k = c->m_retrig >> 4;
            if (k == 6)
                c->vol = c->vol * 2 / 3;
            else if (k == 7)
                c->vol >>= 1;
            else if (k == 14)
                c->vol = c->vol * 3 / 2;
            else if (k == 15)
                c->vol *= 2;
            else
                c->vol += add[k];
            c->vol = CLAMP(c->vol, 0, 64);
            trigger(c, 0);
        }
        break;
    }
    case 29: {                                          /* T */
        int on = (c->m_tremor >> 4) + 1, off = (c->m_tremor & 15) + 1;
        c->tremor_cnt = (c->tremor_cnt + 1) % (on + off);
        c->tremor_off = c->tremor_cnt >= on;
        break;
    }
    }
}

static int env_value(const struct env *e, int *pos, bool keyon)
{
    int n = e->n;
    if (n < 1)
        return 64;
    int p = *pos, i = 0;
    while (i < n - 1 && p >= e->x[i + 1])
        i++;
    int val;
    if (i >= n - 1 || p <= e->x[i]) {
        val = e->y[i];
    } else {
        int dx = e->x[i + 1] - e->x[i];
        val = e->y[i] + (e->y[i + 1] - e->y[i]) * (p - e->x[i]) / dx;
    }
    if (!((e->flags & ENV_SUS) && keyon && e->sus < n && p == e->x[e->sus])) {
        p++;
        if ((e->flags & ENV_LOOP) && e->lend < n && e->lstart < n &&
            p > e->x[e->lend])
            p = e->x[e->lstart];
        if (p > e->x[n - 1])
            p = e->x[n - 1];
    }
    *pos = p;
    return CLAMP(val, 0, 64);
}

static int auto_vibrato(struct chan *c)
{
    const struct instr *in = c->ins;
    if (!in || !in->vdepth || !in->vrate)
        return 0;
    int depth = in->vdepth;
    if (in->vsweep && c->avib_tick < in->vsweep)
        depth = depth * c->avib_tick / in->vsweep;
    c->avib_tick++;
    int w;
    int pos = (c->avib_pos >> 2) & 63;
    switch (in->vtype & 3) {
    case 1: w = pos < 32 ? 255 : -255; break;
    case 2: w = 255 - pos * 8; break;
    case 3: w = -255 + pos * 8; break;
    default: w = pos < 32 ? vib_sine[pos] : -vib_sine[pos - 32]; break;
    }
    c->avib_pos += in->vrate;
    return w * depth >> 6;
}

/* Per tick for every channel: envelopes, fades, final pitch and volume. */
static void update_chan(struct chan *c, int ci)
{
    if (!c->v.on) {
        voice_target(&c->v, 0, 0);
        return;
    }
    const struct instr *in = c->ins;
    int venv = 64, penv = 32;
    if (in && (in->venv.flags & ENV_ON))
        venv = env_value(&in->venv, &c->venv_pos, c->keyon);
    if (in && (in->penv.flags & ENV_ON))
        penv = env_value(&in->penv, &c->penv_pos, c->keyon);
    if (!c->keyon && in) {
        c->fade -= in->fadeout;
        if (c->fade < 0)
            c->fade = 0;
    }
    int period = c->period + c->vib_delta + c->arp + auto_vibrato(c);
    period = CLAMP(period, 1, song.linear ? 11520 : 32000);
    uint64_t inc = ((uint64_t)period_freq_q8(period) << 8) / rate;
    c->v.inc = (int32_t)MIN(inc, 0x7fffffffu);

    int vol = CLAMP(c->vol + c->trem_delta, 0, 64);
    if (c->tremor_off)
        vol = 0;
    /* 64 * 64 * 64 * 64 = 2^24 -> Q12 */
    int32_t v = vol * venv;                         /* 0..4096 */
    v = (int32_t)((int64_t)v * (c->fade >> 4) >> 12); /* fade 0..4096 */
    v = v * gvol >> 6;
    int pan = c->pan + (penv - 32) * (128 - ABS(c->pan - 128)) / 32;
    pan = CLAMP(pan, 0, 255);
    if (ci < MAX_CH && (eng_mute[ci] || ci >= song.nch))
        v = 0;
    /* released and silent: stop mixing it */
    if (!v && !c->keyon && !c->v.lv && !c->v.rv && !c->v.ramp)
        c->v.on = false;
    /* balance law: full level at centre, the far side fades out */
    voice_target(&c->v, v * MIN(256 - pan, 128) >> 7,
                 v * MIN(pan + 1, 128) >> 7);
}

static void advance_row(void)
{
    int rows = pattern_rows(cur_ord);
    if (loop_jump) {
        cur_row = break_row;
    } else if (jump_ord >= 0) {
        if (once && mode == PLAY_SONG && jump_ord <= cur_ord)
            ended = true;   /* a backward jump loops the song */
        cur_ord = mode == PLAY_SONG ? jump_ord : cur_ord;
        cur_row = break_row >= 0 ? break_row : 0;
        if (cur_ord >= song.nord || cur_row >= pattern_rows(cur_ord)) {
            if (cur_ord >= song.nord) {
                if (once)
                    ended = true;
                cur_ord = song.restart < song.nord ? song.restart : 0;
            }
            if (cur_row >= pattern_rows(cur_ord))
                cur_row = 0;
        }
    } else if (++cur_row >= rows) {
        cur_row = 0;
        if (mode == PLAY_SONG && ++cur_ord >= song.nord) {
            if (once)
                ended = true;
            cur_ord = song.restart < song.nord ? song.restart : 0;
        }
    }
    jump_ord = break_row = -1;
    loop_jump = false;
}

static void do_tick(void)
{
    bool playing = mode != PLAY_STOP && song.nord > 0;
    if (playing && tick == 0) {
        if (cur_ord >= song.nord)
            cur_ord = 0;
        if (cur_row >= pattern_rows(cur_ord))
            cur_row = 0;
        shown_ord = cur_ord;
        shown_row = cur_row;
        for (int i = 0; i < song.nch; i++) {
            struct chan *c = &ch[i];
            const struct cell *e = row_cell(cur_ord, cur_row, i);
            c->fx = e->fx;
            c->param = e->param;
            c->volc = e->vol;
            c->arp = c->vib_delta = c->trem_delta = 0;
            c->tremor_off = false;
            if (e->fx == 0xe && (e->param >> 4) == 0xd && (e->param & 15)) {
                c->delayed = *e;
                c->delay_tick = e->param & 15;
                continue;
            }
            start_cell(c, e);
            if (e->vol)
                row_volume_column(c, e->vol);
            row_effect(c, i);
        }
        row_ticks = speed * (1 + pat_delay);
    } else if (playing) {
        int t = tick % speed;
        for (int i = 0; i < song.nch; i++) {
            struct chan *c = &ch[i];
            c->arp = 0;
            if (c->fx != 4 && c->fx != 6 && (c->volc >> 4) != 0xb)
                c->vib_delta = 0;
            if (c->fx != 7)
                c->trem_delta = 0;
            if (t == 0) {
                if (c->fx == 0xe && ((c->param >> 4) == 0xa ||
                                     (c->param >> 4) == 0xb))
                    row_effect(c, i);   /* fine slides repeat per delay */
                continue;
            }
            if (c->volc)
                tick_volume_column(c, c->volc);
            tick_effect(c, t);
        }
    }
    for (int i = 0; i < NCHAN; i++)
        update_chan(&ch[i], i);
    if (playing && ++tick >= row_ticks) {
        tick = 0;
        pat_delay = 0;
        advance_row();
    }
}

static void render(int16_t *out, int frames)
{
    int done = 0;
    while (done < frames) {
        if (tick_left <= 0) {
            do_tick();
            int num = rate * 5 + tick_rem;
            int den = bpm * 2;
            tick_left = num / den;
            tick_rem = num % den;
        }
        int n = MIN(frames - done, tick_left);
        n = MIN(n, BLOCK);
        rb->memset(acc, 0, n * 2 * sizeof(int32_t));
        for (int i = 0; i < NCHAN; i++)
            mix_voice(&ch[i].v, acc, n, i < MAX_CH ? i : -1);
        for (int i = 0; i < FADERS; i++) {
            if (faders[i].on) {
                mix_voice(&faders[i], acc, n, -1);
                if (!faders[i].ramp)
                    faders[i].on = false;
            }
        }
        /* headroom grows with the channels (the demos peak near -6 dBFS,
         * close to libopenmpt); a soft knee catches the rest */
        int gain = 384 / (MIN(song.nch, 16) + 2);
        int16_t *o = out + done * 2;
        for (int i = 0; i < n * 2; i++) {
            int32_t s = acc[i] * gain >> 8;
            if (s > 24576)
                s = 24576 + ((s - 24576) >> 2);
            else if (s < -24576)
                s = -24576 + ((s + 24576) >> 2);
            o[i] = (int16_t)CLAMP(s, -32768, 32767);
        }
        done += n;
        tick_left -= n;
    }
}

static void get_more(const void **start, size_t *size)
{
    int16_t *b = outbuf[outcur];
    outcur ^= 1;
    render(b, BLOCK);
    *start = b;
    *size = sizeof(outbuf[0]);
}

/* ---- API ---- */

void eng_lock(void)
{
    if (opened)
        rb->pcm_play_lock();
}

void eng_unlock(void)
{
    if (opened)
        rb->pcm_play_unlock();
}

void eng_drop(const void *p)
{
    for (int i = 0; i < NCHAN; i++) {
        struct chan *c = &ch[i];
        if (!p || c->ins == p || c->smp == p || c->v.smp == p) {
            c->v.on = false;
            c->ins = NULL;
            c->smp = NULL;
            c->v.smp = NULL;
        }
    }
    for (int i = 0; i < FADERS; i++)
        if (!p || faders[i].smp == p)
            faders[i].on = false;
}

static void reset_locked(void)
{
    rb->memset(ch, 0, sizeof(ch));
    rb->memset(faders, 0, sizeof(faders));
    for (int i = 0; i < NCHAN; i++) {
        ch[i].pan = i < MAX_CH ? song.chpan[i] : 128;
        ch[i].fade = 65536;
        ch[i].v.tlv = ch[i].v.trv = -1;
    }
    speed = song.speed ? song.speed : 6;
    bpm = song.bpm >= 32 ? song.bpm : 125;
    gvol = 64;
    tick = 0;
    row_ticks = speed;
    pat_delay = 0;
    jump_ord = break_row = -1;
    loop_jump = false;
    ended = false;
}

void eng_reset(void)
{
    eng_lock();
    reset_locked();
    mode = PLAY_STOP;
    eng_unlock();
}

void eng_init(void)
{
    rate = rb->mixer_get_frequency();
    reset_locked();
}

unsigned eng_rate(void)
{
    return rate;
}

void eng_set_rate(unsigned r)
{
    rate = r;
}

void eng_open(void)
{
    static const struct mixer_play_cbs cbs = { .get_more = get_more };
    if (opened)
        return;
    rate = rb->mixer_get_frequency();
    tick_left = 0;
    rb->mixer_channel_set_amplitude(PCM_MIXER_CHAN_PLAYBACK, MIX_AMP_UNITY);
    rb->mixer_channel_play_data(PCM_MIXER_CHAN_PLAYBACK, &cbs, NULL, 0);
    opened = true;
}

void eng_close(void)
{
    if (!opened)
        return;
    rb->mixer_channel_stop(PCM_MIXER_CHAN_PLAYBACK);
    opened = false;
}

void eng_play(int m, int ord, int row)
{
    eng_lock();
    reset_locked();
    mode = m;
    cur_ord = CLAMP(ord, 0, song.nord - 1);
    cur_row = CLAMP(row, 0, pattern_rows(cur_ord) - 1);
    shown_ord = cur_ord;
    shown_row = cur_row;
    tick_left = 0;
    eng_unlock();
}

void eng_stop(void)
{
    eng_lock();
    mode = PLAY_STOP;
    for (int i = 0; i < MAX_CH; i++) {
        voice_release_to_fader(&ch[i].v);
        ch[i].v.on = false;
    }
    eng_unlock();
}

int eng_mode(void)
{
    return mode;
}

void eng_pos(int *ord, int *row)
{
    *ord = shown_ord;
    *row = shown_row;
}

void eng_note(int voice, int ins, int note)
{
    struct chan *c = &ch[MAX_CH + CLAMP(voice, 0, PREVIEW_VOICES - 1)];
    struct cell e = { .note = note, .ins = ins };
    eng_lock();
    c->fx = c->param = c->volc = 0;
    c->arp = c->vib_delta = c->trem_delta = 0;
    start_cell(c, &e);
    eng_unlock();
}

void eng_release(int voice)
{
    struct chan *c = &ch[MAX_CH + CLAMP(voice, 0, PREVIEW_VOICES - 1)];
    eng_lock();
    key_off(c);
    eng_unlock();
}

void eng_set_once(bool o)
{
    once = o;
}

bool eng_ended(void)
{
    return ended;
}

/* Offline rendering (WAV export): the mixer channel must be stopped. */
void eng_render(int16_t *out, int frames)
{
    render(out, frames);
}
