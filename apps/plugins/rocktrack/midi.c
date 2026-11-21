/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * RockTrack: Standard MIDI File import and export.
 *
 * Import quantises notes to 16ths (32nds when the file needs them),
 * spreads polyphony over tracker channels, makes one chip instrument per
 * General MIDI family and drum, and shares repeated patterns. Export
 * plays the order list once into a type 0 file.
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

#define LANES       16      /* tracker channels an import may use */
#define PAT_ROWS    64
#define MAX_TEMPOS  32
#define MAX_PROGS   64
#define DRUM_CH     9

/* A note. Once quantised, t0 is its row and t1 the row of its note-off
 * (0: none); ch becomes its tracker channel and prog its instrument. */
struct mnote {
    uint32_t t0, t1;
    uint8_t ch, key, vel, prog;
};

struct mctx {
    struct mnote *n;        /* NULL while counting */
    int count, cap;
    uint16_t open[16 * 128];    /* sounding note index + 1 */
    int ntempo, nprog;
    uint32_t tempo_t[MAX_TEMPOS], tempo_us[MAX_TEMPOS];
    uint32_t prog_t[MAX_PROGS];
    uint8_t prog_ch[MAX_PROGS], prog_v[MAX_PROGS];
};

/* chip instruments: one per General MIDI family, then the drums */
enum { K_KICK = 16, K_SNARE, K_HAT, K_CYMBAL, K_TOM, K_COUNT };

static const struct preset {
    char name[9];
    uint8_t src, a, b, atk, dec, sus, rel;
} presets[K_COUNT] = {
    { "Piano",    SRC_PULSE, 6, 0, 0, 9, 4, 4 },
    { "Bells",    SRC_SINE, 0, 0, 0, 8, 0, 6 },
    { "Organ",    SRC_PULSE, 8, 0, 0, 0, 15, 2 },
    { "Guitar",   SRC_PULSE, 4, 0, 0, 8, 3, 3 },
    { "Bass",     SRC_TRI, 0, 0, 0, 4, 12, 2 },
    { "Strings",  SRC_SAW, 0, 0, 4, 0, 15, 6 },
    { "Ensemble", SRC_SAW, 0, 0, 5, 0, 13, 7 },
    { "Brass",    SRC_SAW, 0, 0, 2, 6, 11, 3 },
    { "Reed",     SRC_PULSE, 3, 0, 1, 0, 14, 3 },
    { "Flute",    SRC_TRI, 0, 0, 2, 0, 15, 3 },
    { "Lead",     SRC_PULSE, 8, 0, 0, 7, 11, 3 },
    { "Pad",      SRC_SINE, 0, 0, 5, 8, 10, 8 },
    { "Synth FX", SRC_SAW, 0, 0, 3, 8, 8, 8 },
    { "Pluck",    SRC_PULSE, 2, 0, 0, 7, 2, 3 },
    { "Perc",     SRC_SINE, 0, 0, 0, 6, 0, 3 },
    { "Noise",    SRC_NOISE, 0, 0, 0, 8, 4, 5 },
    { "Kick",     SRC_KICK, 12, 6, 0, 0, 15, 1 },
    { "Snare",    SRC_SNARE, 12, 6, 0, 0, 15, 1 },
    { "Hat",      SRC_HAT, 12, 3, 0, 0, 15, 1 },
    { "Cymbal",   SRC_HAT, 10, 15, 0, 0, 15, 1 },
    { "Tom",      SRC_TOM, 12, 6, 0, 0, 15, 1 },
};

/* General MIDI drum key: instrument kind and the tracker note to play */
static int drum_kind(int key, int *note)
{
    *note = 49;                                     /* C-4 */
    switch (key) {
    case 35: case 36:
        return K_KICK;
    case 37: case 38: case 39: case 40:
        return K_SNARE;
    case 42: case 44:
        return K_HAT;
    case 46: case 49: case 51: case 52: case 53: case 55: case 57: case 59:
        return K_CYMBAL;
    case 41: case 43: case 45: case 47: case 48: case 50:
        *note = 49 + key - 45;
        return K_TOM;
    }
    if (key >= 60 && key <= 68) {                   /* bongos to timbales */
        *note = 49 + key - 64;
        return K_TOM;
    }
    return K_HAT;
}

/* ---- reading ---- */

static uint32_t be32(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
}

static uint32_t vlq(const uint8_t **p, const uint8_t *end)
{
    uint32_t v = 0;
    for (int i = 0; i < 4 && *p < end; i++) {
        uint8_t b = *(*p)++;
        v = v << 7 | (b & 0x7f);
        if (!(b & 0x80))
            break;
    }
    return v;
}

static void note_off(struct mctx *x, uint32_t t, int ch, int key)
{
    uint16_t *o = &x->open[ch * 128 + key];
    if (x->n && *o)
        x->n[*o - 1].t1 = t;
    *o = 0;
}

static void note_on(struct mctx *x, uint32_t t, int ch, int key, int vel)
{
    if (!x->n) {
        x->count += x->count < 65535;
        return;
    }
    if (x->count >= x->cap)
        return;
    note_off(x, t, ch, key);                        /* retriggered */
    struct mnote *n = &x->n[x->count];
    n->t0 = n->t1 = t;
    n->ch = ch;
    n->key = key;
    n->vel = vel;
    x->open[ch * 128 + key] = ++x->count;
}

static void parse_track(struct mctx *x, const uint8_t *p, const uint8_t *end)
{
    uint32_t t = 0;
    int status = 0;
    rb->memset(x->open, 0, sizeof(x->open));
    while (p < end) {
        t += vlq(&p, end);
        if (p >= end)
            break;
        if (*p == 0xff) {                           /* meta event */
            if (end - p < 2)
                break;
            int type = p[1];
            p += 2;
            uint32_t len = vlq(&p, end);
            if (len > (uint32_t)(end - p) || type == 0x2f)
                break;
            if (type == 0x51 && len == 3 && x->n && x->ntempo < MAX_TEMPOS) {
                x->tempo_t[x->ntempo] = t;
                x->tempo_us[x->ntempo++] = p[0] << 16 | p[1] << 8 | p[2];
            }
            p += len;
            continue;
        }
        if (*p == 0xf0 || *p == 0xf7) {             /* sysex */
            p++;
            uint32_t len = vlq(&p, end);
            if (len > (uint32_t)(end - p))
                break;
            p += len;
            status = 0;
            continue;
        }
        if (*p & 0x80)
            status = *p++;
        if (!status)
            break;
        int kind = status & 0xf0, ch = status & 15;
        int nd = kind == 0xc0 || kind == 0xd0 ? 1 : 2;
        if (end - p < nd)
            break;
        int d1 = p[0] & 0x7f, d2 = nd == 2 ? p[1] & 0x7f : 0;
        p += nd;
        if (kind == 0x90 && d2)
            note_on(x, t, ch, d1, d2);
        else if (kind == 0x80 || kind == 0x90)
            note_off(x, t, ch, d1);
        else if (kind == 0xc0 && x->n && x->nprog < MAX_PROGS) {
            x->prog_t[x->nprog] = t;
            x->prog_ch[x->nprog] = ch;
            x->prog_v[x->nprog++] = d1;
        }
    }
    if (x->n)                                       /* close what sounds */
        for (int i = 0; i < 16 * 128; i++)
            if (x->open[i])
                x->n[x->open[i] - 1].t1 = t;
}

static void parse_file(struct mctx *x, const uint8_t *f, long size)
{
    long pos = 14;
    while (size - pos >= 8) {
        uint32_t len = be32(f + pos + 4);
        const uint8_t *body = f + pos + 8;
        if (len > (uint32_t)(size - pos - 8))
            len = size - pos - 8;
        if (!rb->memcmp(f + pos, "MTrk", 4))
            parse_track(x, body, body + len);
        pos += 8 + len;
    }
}

static int prog_at(const struct mctx *x, int ch, uint32_t t)
{
    int prog = 0;
    uint32_t best = 0;
    for (int i = 0; i < x->nprog; i++)
        if (x->prog_ch[i] == ch && x->prog_t[i] <= t && x->prog_t[i] >= best) {
            best = x->prog_t[i];
            prog = x->prog_v[i];
        }
    return prog;
}

static int note_cmp(const void *a, const void *b)
{
    const struct mnote *p = a, *q = b;
    if (p->t0 != q->t0)
        return p->t0 < q->t0 ? -1 : 1;
    if (p->ch != q->ch)
        return p->ch - q->ch;
    return q->key - p->key;                         /* high notes first */
}

static int bpm_of(uint32_t us)
{
    return us ? CLAMP((int)(60000000 / us), 32, 255) : 120;
}

/* Spread the notes over monophonic tracker channels: a free channel last
 * used by the same MIDI channel, else a fresh one, else any free one;
 * when all are busy, cut the note that would sound longest. */
static int assign_lanes(struct mctx *x, int *dropped)
{
    uint32_t busy[LANES], start[LANES];
    int owner[LANES], last[LANES], nch = 1;
    for (int c = 0; c < LANES; c++) {
        busy[c] = start[c] = 0;
        owner[c] = last[c] = -1;
    }
    *dropped = 0;
    for (int i = 0; i < x->count; i++) {
        struct mnote *n = &x->n[i];
        int best = -1, score = 3;
        for (int c = 0; c < LANES && score; c++) {
            if (busy[c] > n->t0)
                continue;
            int s = owner[c] == n->ch ? 0 : owner[c] < 0 ? 1 : 2;
            if (s < score) {
                score = s;
                best = c;
            }
        }
        for (int c = 0; best < 0 && c < LANES; c++)
            if (start[c] < n->t0 && (best < 0 || busy[c] > busy[best]))
                best = c;
        if (best < 0) {
            n->prog = 0;                            /* dropped */
            (*dropped)++;
            continue;
        }
        if (last[best] >= 0 && x->n[last[best]].t1 >= n->t0)
            x->n[last[best]].t1 = 0;                /* the new note cuts it */
        owner[best] = n->ch;
        start[best] = n->t0;
        busy[best] = n->t1 ? n->t1 : n->t0 + 1;
        last[best] = i;
        n->ch = best;
        nch = MAX(nch, best + 1);
    }
    return nch;
}

static void build_pattern(const struct mctx *x, struct cell *c, int base,
                          int rows)
{
    rb->memset(c, 0, (size_t)rows * song.nch * sizeof(*c));
    for (int i = 0; i < x->count; i++) {
        const struct mnote *n = &x->n[i];
        if (!n->prog)
            continue;
        int r = (int)n->t1 - base;
        if (n->t1 && r >= 0 && r < rows && !c[r * song.nch + n->ch].note)
            c[r * song.nch + n->ch].note = NOTE_OFF;
        r = (int)n->t0 - base;
        if (r < 0 || r >= rows)
            continue;
        struct cell *d = &c[r * song.nch + n->ch];
        int vol = (n->vel * 64 + 63) / 127;
        d->note = n->key;
        d->ins = n->prog;
        d->vol = vol < 64 ? 0x10 + vol : 0;
    }
    for (int i = 0; i < x->ntempo; i++) {           /* later tempo changes */
        int r = (int)x->tempo_t[i] - base;
        if (!x->tempo_t[i] || r < 0 || r >= rows)
            continue;
        for (int ch = 0; ch < song.nch; ch++) {
            struct cell *d = &c[r * song.nch + ch];
            if (!d->fx && !d->param) {
                d->fx = 15;
                d->param = bpm_of(x->tempo_us[i]);
                break;
            }
        }
    }
}

static bool cells_blank(const struct cell *c, int n)
{
    for (int i = 0; i < n; i++)
        if (!cell_empty(&c[i]))
            return false;
    return true;
}

/* the pattern number holding these cells: an earlier copy, or a new one */
static int store_pattern(int npat, const struct cell *c, int rows)
{
    size_t size = (size_t)rows * song.nch * sizeof(*c);
    bool blank = cells_blank(c, rows * song.nch);
    for (int q = 0; q < npat; q++) {
        const struct pattern *p = &song.pat[q];
        if (p->rows == rows && (p->c ? !rb->memcmp(p->c, c, size) : blank))
            return q;
    }
    song.pat[npat].rows = rows;
    if (!blank) {
        if (!pattern_alloc(npat))
            return -1;
        rb->memcpy(song.pat[npat].c, c, size);
    }
    return npat;
}

#define FAIL(s) do { rb->strlcpy(err, s, errlen); goto out; } while (0)

int midi_load(const char *path, char *err, size_t errlen)
{
    int ret = -1;
    uint8_t *file = NULL;
    struct cell *tmp = NULL;
    int fd = rb->open(path, O_RDONLY);
    if (fd < 0) {
        rb->strlcpy(err, "Cannot open file", errlen);
        return -1;
    }
    long size = rb->ffilesize(fd);
    song_clear();
    struct mctx *x = pool_alloc(sizeof(*x));
    if (x && size >= 14 && (size_t)size < pool_avail() / 2)
        file = pool_alloc(size);
    if (!x || !file) {
        rb->close(fd);
        FAIL("MIDI file too big");
    }
    rb->memset(x, 0, sizeof(*x));
    bool ok = rb->read(fd, file, size) == size;
    rb->close(fd);
    if (!ok || rb->memcmp(file, "MThd", 4) || be32(file + 4) < 6)
        FAIL("Not a MIDI file");
    if (file[9] > 1)
        FAIL("MIDI type 2 is not supported");
    unsigned ppq = file[12] << 8 | file[13];
    if (ppq & 0x8000)               /* SMPTE time: ticks per second / 2 */
        ppq = (unsigned)(-(int8_t)(ppq >> 8)) * (ppq & 0xff) / 2;
    if (!ppq)
        FAIL("Not a MIDI file");

    parse_file(x, file, size);                      /* count the notes */
    if (!x->count)
        FAIL("No notes in this file");
    x->n = pool_alloc(x->count * sizeof(struct mnote));
    if (!x->n)
        FAIL("MIDI file too big");
    x->cap = x->count;
    x->count = 0;
    parse_file(x, file, size);
    pool_free(file);
    file = NULL;

    /* 16ths, or 32nds when many notes fall between 16ths */
    int off = 0;
    for (int i = 0; i < x->count; i++) {
        unsigned f = (uint64_t)x->n[i].t0 * 4 % ppq;
        off += f * 5 > ppq && f * 5 < 4 * ppq;
    }
    int rpb = off * 8 > x->count ? 8 : 4;
#define ROW(t) ((uint32_t)(((uint64_t)(t) * rpb + ppq / 2) / ppq))

    for (int i = 0; i < x->count; i++) {
        struct mnote *n = &x->n[i];
        int note, kind;
        if (n->ch == DRUM_CH) {
            kind = drum_kind(n->key, &note);
        } else {
            kind = prog_at(x, n->ch, n->t0) / 8;
            note = n->key - 11;                     /* MIDI 60 is C-4 */
        }
        n->key = CLAMP(note, 1, 96);
        n->prog = kind + 1;                         /* kind until lanes */
        uint32_t r0 = ROW(n->t0), r1 = ROW(n->t1);
        n->t0 = r0;
        n->t1 = kind >= K_KICK ? 0 : MAX(r1, r0 + 1);
    }
    uint32_t tempo_us = 500000;                     /* 120 BPM */
    for (int i = 0; i < x->ntempo; i++) {
        x->tempo_t[i] = ROW(x->tempo_t[i]);
        if (!x->tempo_t[i])
            tempo_us = x->tempo_us[i];
    }
#undef ROW
    rb->qsort(x->n, x->count, sizeof(struct mnote), note_cmp);

    int dropped;
    int nch = assign_lanes(x, &dropped);
    uint8_t ins_of[K_COUNT];
    rb->memset(ins_of, 0, sizeof(ins_of));
    uint32_t rows_total = 1;
    for (int i = 0; i < x->count; i++) {
        struct mnote *n = &x->n[i];
        if (!n->prog)
            continue;
        int k = n->prog - 1;
        if (!ins_of[k]) {
            const struct preset *p = &presets[k];
            if (song.nins >= MAX_INS ||
                !instr_new_synth(song.nins, p->name, p->src, p->a, p->b,
                                 p->atk, p->dec, p->sus, p->rel))
                FAIL("Out of memory");
            ins_of[k] = song.nins;
        }
        n->prog = ins_of[k];
        rows_total = MAX(rows_total, MAX(n->t0 + 1, n->t1 + 1));
    }

    const char *name = rb->strrchr(path, '/');
    rb->strlcpy(song.name, name ? name + 1 : path, sizeof(song.name));
    char *dot = rb->strrchr(song.name, '.');
    if (dot)
        *dot = 0;
    song.speed = 24 / rpb;
    song.bpm = bpm_of(tempo_us);
    if (!song_set_channels(nch))
        FAIL("Out of memory");

    int npos = (rows_total + PAT_ROWS - 1) / PAT_ROWS;
    bool cut = npos > MAX_ORD;
    npos = MIN(npos, MAX_ORD);
    tmp = pool_alloc(PAT_ROWS * nch * sizeof(struct cell));
    if (!tmp)
        FAIL("Out of memory");
    int npat = 0;
    for (int p = 0; p < npos; p++) {
        int base = p * PAT_ROWS;
        int rows = MIN(PAT_ROWS, ((int)rows_total - base + 15) & ~15);
        build_pattern(x, tmp, base, rows);
        int q = store_pattern(npat, tmp, rows);
        if (q < 0) {
            cut = true;
            break;
        }
        npat += q == npat;
        song.ord[p] = q;
        song.nord = p + 1;
    }
    if (cut)
        app_message("Out of room: the song is cut short");
    else if (dropped)
        app_message("%d notes dropped: over %d at once", dropped, LANES);
    ret = 0;
out:
    pool_free(tmp);
    pool_free(file);
    if (x)
        pool_free(x->n);
    pool_free(x);
    eng_reset();
    return ret;
}

/* ---- writing ---- */

struct writer {
    int fd;
    bool err;
    int n;
    uint32_t len, last;
    uint8_t buf[256];
};

static void w_flush(struct writer *w)
{
    if (w->n && rb->write(w->fd, w->buf, w->n) != w->n)
        w->err = true;
    w->n = 0;
}

static void w_byte(struct writer *w, int b)
{
    w->buf[w->n++] = b;
    w->len++;
    if (w->n == sizeof(w->buf))
        w_flush(w);
}

static void w_vlq(struct writer *w, uint32_t v)
{
    uint8_t t[5];
    int n = 0;
    do {
        t[n++] = v & 0x7f;
        v >>= 7;
    } while (v);
    while (n--)
        w_byte(w, t[n] | (n ? 0x80 : 0));
}

static void w_event(struct writer *w, uint32_t t, int a, int b, int c)
{
    w_vlq(w, t - w->last);
    w->last = t;
    w_byte(w, a);
    w_byte(w, b);
    if (c >= 0)
        w_byte(w, c);
}

static void w_meta(struct writer *w, uint32_t t, int type, const void *d,
                   int len)
{
    w_event(w, t, 0xff, type, -1);
    w_vlq(w, len);
    for (int i = 0; i < len; i++)
        w_byte(w, ((const uint8_t *)d)[i]);
}

static void w_tempo(struct writer *w, uint32_t t, int bpm)
{
    uint32_t us = 60000000 / bpm;
    uint8_t d[3] = { us >> 16, us >> 8, us };
    w_meta(w, t, 0x51, d, 3);
}

/* a General MIDI program for an instrument, by name, else by source */
static int gm_program(const struct instr *in, int src)
{
    static const struct {
        char word[7];
        uint8_t prog;
    } words[] = {
        { "bass", 33 }, { "piano", 0 }, { "organ", 16 }, { "guitar", 25 },
        { "string", 48 }, { "brass", 61 }, { "flute", 73 }, { "bell", 14 },
        { "choir", 52 }, { "pad", 89 },
    };
    static const uint8_t by_src[SRC_COUNT] = {
        [SRC_PULSE] = 80, [SRC_TRI] = 74, [SRC_SAW] = 81, [SRC_SINE] = 88,
        [SRC_NOISE] = 122, [SRC_WAVE] = 82,
    };
    for (unsigned i = 0; i < ARRAYLEN(words); i++)
        if (rb->strcasestr(in->name, words[i].word))
            return words[i].prog;
    return by_src[src];
}

struct out_ins {
    uint8_t ch, prog;
    bool drum, tom, synth;
    uint8_t drum_key;
};

static void map_instruments(struct out_ins *o)
{
    int next = 0;
    for (int i = 0; i < song.nins; i++) {
        const struct instr *in = song.ins[i];
        struct synth s = { SRC_SAMPLE, 0, 0 };
        if (in && in->nsmp)
            synth_parse(in->smp[0].name, &s);
        o[i].synth = s.src != SRC_SAMPLE;
        o[i].drum = synth_is_drum(s.src);
        if (o[i].drum) {
            o[i].ch = DRUM_CH;
            o[i].tom = s.src == SRC_TOM;
            o[i].drum_key = s.src == SRC_KICK ? 36 : s.src == SRC_SNARE ? 38
                          : s.src == SRC_TOM ? 45 : s.b >= 6 ? 46 : 42;
            continue;
        }
        o[i].ch = next;
        o[i].prog = in ? gm_program(in, s.src) : 0;
        next = next == 15 ? 0 : next + 1 + (next + 1 == DRUM_CH);
    }
}

int midi_save(const char *path)
{
    struct out_ins *oi = pool_alloc(MAX(1, song.nins) * sizeof(*oi));
    if (!oi)
        return -1;
    map_instruments(oi);
    struct writer w;
    rb->memset(&w, 0, sizeof(w));
    w.fd = rb->open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (w.fd < 0) {
        pool_free(oi);
        return -1;
    }
    static const uint8_t head[22] = {
        'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, 0, 24,
        'M', 'T', 'r', 'k', 0, 0, 0, 0,
    };
    for (unsigned i = 0; i < sizeof(head); i++)
        w_byte(&w, head[i]);
    w.len = 0;
    w_meta(&w, 0, 0x03, song.name, rb->strlen(song.name));
    w_tempo(&w, 0, song.bpm);

    /* one MIDI tick per tracker tick: 24 a beat at speed 6 */
    int16_t sounding[MAX_CH];       /* channel << 8 | key, -1 silent */
    uint8_t last_ins[MAX_CH], chprog[16];
    for (int i = 0; i < MAX_CH; i++) {
        sounding[i] = -1;
        last_ins[i] = 0;
    }
    rb->memset(chprog, 0xff, sizeof(chprog));
    bool seen[MAX_ORD];
    rb->memset(seen, 0, sizeof(seen));
    seen[0] = true;
    uint32_t t = 0;
    int speed = song.speed, ord = 0, row = 0;
    while (true) {                  /* the order list once, no loops */
        int pat = song.ord[ord], next_ord = -1, next_row = 0;
        for (int ch = 0; ch < song.nch; ch++) {
            const struct cell *c = cell_at(pat, row, ch);
            if (!c)
                break;
            if (c->fx == 15 && c->param) {
                if (c->param < 0x20)
                    speed = c->param;
                else
                    w_tempo(&w, t, c->param);
            } else if (c->fx == 0xb) {
                next_ord = c->param;
            } else if (c->fx == 0xd && next_ord < 0) {
                next_ord = ord + 1;
                next_row = (c->param >> 4) * 10 + (c->param & 15);
            }
            if (c->ins)
                last_ins[ch] = c->ins;
            if (!c->note)
                continue;
            if (sounding[ch] >= 0) {
                w_event(&w, t, 0x80 | sounding[ch] >> 8, sounding[ch] & 127,
                        0);
                sounding[ch] = -1;
            }
            int i = last_ins[ch] - 1;
            if (c->note > 96 || i < 0 || i >= song.nins || !song.ins[i])
                continue;
            const struct instr *in = song.ins[i];
            const struct out_ins *o = &oi[i];
            int key;
            if (o->drum) {
                key = o->tom ? CLAMP(45 + c->note - 49, 41, 50)
                             : o->drum_key;
            } else {
                key = c->note + 11;
                if (o->synth && in->nsmp)
                    key += in->smp[0].rel - 12;
            }
            int vol = c->vol >= 0x10 && c->vol <= 0x50 ? c->vol - 0x10
                    : c->fx == 0xc ? MIN(c->param, 64)
                    : in->nsmp ? in->smp[0].vol : 64;
            if (!o->drum && chprog[o->ch] != o->prog) {
                chprog[o->ch] = o->prog;
                w_event(&w, t, 0xc0 | o->ch, o->prog, -1);
            }
            key = CLAMP(key, 0, 127);
            w_event(&w, t, 0x90 | o->ch, key, MAX(1, vol * 127 / 64));
            sounding[ch] = o->ch << 8 | key;
        }
        t += speed;
        if (next_ord < 0 && ++row < pattern_rows(ord))
            continue;
        ord = next_ord >= 0 ? next_ord : ord + 1;
        if (ord >= song.nord || seen[ord])
            break;
        seen[ord] = true;
        row = next_ord >= 0 && next_row < pattern_rows(ord) ? next_row : 0;
    }
    for (int ch = 0; ch < MAX_CH; ch++)
        if (sounding[ch] >= 0)
            w_event(&w, t, 0x80 | sounding[ch] >> 8, sounding[ch] & 127, 0);
    w_meta(&w, t, 0x2f, NULL, 0);
    w_flush(&w);
    uint8_t len[4] = { w.len >> 24, w.len >> 16, w.len >> 8, w.len };
    if (rb->lseek(w.fd, 18, SEEK_SET) != 18 || rb->write(w.fd, len, 4) != 4)
        w.err = true;
    if (rb->close(w.fd) < 0)
        w.err = true;
    pool_free(oi);
    if (w.err) {
        rb->remove(path);
        return -1;
    }
    return 0;
}
