/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * RockTrack: song memory and the song model.
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

struct song song;

/* ---- pool: first-fit allocator over up to two regions ----
 * Blocks carry a size word (bytes incl. header, low bit = used) and are
 * walked linearly; free neighbours merge on free. Allocations are few
 * (patterns, instruments, samples), so the walk is cheap. */

#define ALIGN 8u
#define HDR 8u

struct region {
    uint8_t *base;
    size_t size;
};

static struct region regions[2];
static size_t pool_size;

static void region_init(struct region *r, void *p, size_t size)
{
    uintptr_t a = ((uintptr_t)p + ALIGN - 1) & ~(uintptr_t)(ALIGN - 1);
    size -= a - (uintptr_t)p;
    size &= ~(size_t)(ALIGN - 1);
    r->base = (uint8_t *)a;
    r->size = size >= 2 * HDR ? size : 0;
    if (r->size)
        *(size_t *)r->base = r->size;
    pool_size += r->size;
}

void pool_init(void *a, size_t asize, void *b, size_t bsize)
{
    pool_size = 0;
    region_init(&regions[0], a, a ? asize : 0);
    region_init(&regions[1], b, b ? bsize : 0);
}

void *pool_alloc(size_t size)
{
    size = (size + HDR + ALIGN - 1) & ~(size_t)(ALIGN - 1);
    for (int i = 0; i < 2; i++) {
        struct region *r = &regions[i];
        uint8_t *p = r->base, *end = r->base + r->size;
        while (p < end) {
            size_t bs = *(size_t *)p;
            if (!(bs & 1) && bs >= size) {
                if (bs - size >= 2 * HDR) {
                    *(size_t *)(p + size) = bs - size;
                    bs = size;
                }
                *(size_t *)p = bs | 1;
                return p + HDR;
            }
            p += bs & ~(size_t)1;
        }
    }
    return NULL;
}

void pool_free(void *ptr)
{
    if (!ptr)
        return;
    for (int i = 0; i < 2; i++) {
        struct region *r = &regions[i];
        if ((uint8_t *)ptr < r->base || (uint8_t *)ptr >= r->base + r->size)
            continue;
        *(size_t *)((uint8_t *)ptr - HDR) &= ~(size_t)1;
        /* merge every run of free blocks in this region */
        uint8_t *p = r->base, *end = r->base + r->size;
        while (p < end) {
            size_t bs = *(size_t *)p;
            if (!(bs & 1)) {
                uint8_t *q = p + bs;
                while (q < end && !(*(size_t *)q & 1)) {
                    bs += *(size_t *)q;
                    q += *(size_t *)q;
                }
                *(size_t *)p = bs;
            }
            p += bs & ~(size_t)1;
        }
        return;
    }
}

size_t pool_avail(void)
{
    size_t n = 0;
    for (int i = 0; i < 2; i++) {
        uint8_t *p = regions[i].base, *end = p + regions[i].size;
        while (p < end) {
            size_t bs = *(size_t *)p;
            if (!(bs & 1))
                n += bs - HDR;
            p += bs & ~(size_t)1;
        }
    }
    return n;
}

size_t pool_total(void)
{
    return pool_size;
}

/* ---- patterns ---- */

bool cell_empty(const struct cell *c)
{
    return !c || !(c->note | c->ins | c->vol | c->fx | c->param);
}

struct cell *cell_at(int pat, int row, int ch)
{
    struct pattern *p = &song.pat[pat];
    if (!p->c || row < 0 || row >= p->rows || ch < 0 || ch >= song.nch)
        return NULL;
    return &p->c[row * song.nch + ch];
}

bool pattern_alloc(int pat)
{
    struct pattern *p = &song.pat[pat];
    if (p->c)
        return true;
    size_t n = (size_t)p->rows * song.nch * sizeof(struct cell);
    struct cell *c = pool_alloc(n);
    if (!c)
        return false;
    rb->memset(c, 0, n);
    eng_lock();
    p->c = c;
    eng_unlock();
    return true;
}

struct cell *cell_w(int pat, int row, int ch)
{
    if (!pattern_alloc(pat))
        return NULL;
    return cell_at(pat, row, ch);
}

bool pattern_blank(int pat)
{
    struct pattern *p = &song.pat[pat];
    if (!p->c)
        return true;
    for (int i = 0; i < p->rows * song.nch; i++)
        if (!cell_empty(&p->c[i]))
            return false;
    return true;
}

void pattern_free(int pat)
{
    struct pattern *p = &song.pat[pat];
    eng_lock();
    struct cell *c = p->c;
    p->c = NULL;
    eng_unlock();
    pool_free(c);
}

bool pattern_resize(int pat, int rows)
{
    struct pattern *p = &song.pat[pat];
    rows = CLAMP(rows, 1, MAX_ROWS);
    if (!p->c) {
        p->rows = rows;
        return true;
    }
    size_t n = (size_t)rows * song.nch * sizeof(struct cell);
    struct cell *c = pool_alloc(n);
    if (!c)
        return false;
    rb->memset(c, 0, n);
    int keep = MIN(rows, p->rows);
    rb->memcpy(c, p->c, (size_t)keep * song.nch * sizeof(struct cell));
    eng_lock();
    struct cell *old = p->c;
    p->c = c;
    p->rows = rows;
    eng_unlock();
    pool_free(old);
    return true;
}

int pattern_unused(void)
{
    bool used[MAX_PAT];
    rb->memset(used, 0, sizeof(used));
    for (int i = 0; i < song.nord; i++)
        used[song.ord[i]] = true;
    for (int i = 0; i < MAX_PAT; i++)
        if (!used[i] && pattern_blank(i))
            return i;
    return -1;
}

int song_used_patterns(void)
{
    int n = 0;
    for (int i = 0; i < song.nord; i++)
        n = MAX(n, song.ord[i] + 1);
    for (int i = n; i < MAX_PAT; i++)
        if (!pattern_blank(i))
            n = i + 1;
    return n;
}

int pattern_rows(int ord)
{
    if (ord < 0 || ord >= song.nord)
        return 0;
    return song.pat[song.ord[ord]].rows;
}

bool order_insert(int pos, int pat)
{
    if (song.nord >= MAX_ORD)
        return false;
    pos = CLAMP(pos, 0, song.nord);
    eng_lock();
    rb->memmove(&song.ord[pos + 1], &song.ord[pos], song.nord - pos);
    song.ord[pos] = pat;
    song.nord++;
    if (song.restart && pos <= song.restart)
        song.restart++;
    eng_unlock();
    return true;
}

void order_delete(int pos)
{
    if (song.nord <= 1 || pos < 0 || pos >= song.nord)
        return;
    eng_lock();
    rb->memmove(&song.ord[pos], &song.ord[pos + 1], song.nord - pos - 1);
    song.nord--;
    song.ord[song.nord] = 0;
    if (song.restart > pos)
        song.restart--;
    if (song.restart >= song.nord)
        song.restart = 0;
    eng_unlock();
}

/* Re-lay every pattern for a new channel count; all or nothing. */
bool song_set_channels(int nch)
{
    nch = CLAMP(nch, 1, MAX_CH);
    if (nch == song.nch)
        return true;
    struct cell *fresh[MAX_PAT];
    rb->memset(fresh, 0, sizeof(fresh));
    for (int i = 0; i < MAX_PAT; i++) {
        struct pattern *p = &song.pat[i];
        if (!p->c)
            continue;
        size_t n = (size_t)p->rows * nch * sizeof(struct cell);
        fresh[i] = pool_alloc(n);
        if (!fresh[i]) {
            for (int j = 0; j < i; j++)
                pool_free(fresh[j]);
            return false;
        }
        rb->memset(fresh[i], 0, n);
        int keep = MIN(nch, song.nch);
        for (int r = 0; r < p->rows; r++)
            rb->memcpy(&fresh[i][r * nch], &p->c[r * song.nch],
                       keep * sizeof(struct cell));
    }
    eng_lock();
    for (int i = 0; i < MAX_PAT; i++) {
        if (!song.pat[i].c)
            continue;
        pool_free(song.pat[i].c);
        song.pat[i].c = fresh[i];
    }
    for (int c = song.nch; c < nch; c++)
        song.chpan[c] = 128;
    song.nch = nch;
    eng_unlock();
    eng_reset();
    return true;
}

/* ---- instruments ---- */

struct instr *instr_alloc(int nsmp)
{
    size_t n = sizeof(struct instr) + nsmp * sizeof(struct sample);
    struct instr *in = pool_alloc(n);
    if (!in)
        return NULL;
    rb->memset(in, 0, n);
    in->nsmp = nsmp;
    for (int i = 0; i < nsmp; i++) {
        in->smp[i].vol = 64;
        in->smp[i].pan = 128;
    }
    return in;
}

void instr_free(struct instr *in)
{
    if (!in)
        return;
    for (int i = 0; i < in->nsmp; i++)
        pool_free(in->smp[i].data);
    pool_free(in);
}

/* Give instrument idx (0-based) nsmp samples, keeping what fits. */
bool instr_set_samples(int idx, int nsmp)
{
    struct instr *old = song.ins[idx];
    struct instr *in = instr_alloc(nsmp);
    if (!in)
        return false;
    if (old) {
        size_t head = sizeof(struct instr);
        rb->memcpy(in, old, head);
        in->nsmp = nsmp;
        int keep = MIN(nsmp, old->nsmp);
        rb->memcpy(in->smp, old->smp, keep * sizeof(struct sample));
        for (int i = keep; i < old->nsmp; i++)
            pool_free(old->smp[i].data);
    }
    eng_lock();
    if (old)
        eng_drop(old);
    song.ins[idx] = in;
    if (idx >= song.nins)
        song.nins = idx + 1;
    eng_unlock();
    pool_free(old);
    return true;
}

bool sample_set_data(struct sample *s, uint32_t len, bool bits16)
{
    void *d = NULL;
    if ((uint64_t)len << bits16 > pool_total())     /* a corrupt length */
        return false;
    if (len) {
        d = pool_alloc(len << bits16);
        if (!d)
            return false;
        rb->memset(d, 0, len << bits16);
    }
    eng_lock();
    eng_drop(s);
    void *old = s->data;
    s->data = d;
    s->len = len;
    s->bits16 = bits16;
    s->lstart = 0;
    s->llen = 0;
    s->loop = 0;
    eng_unlock();
    pool_free(old);
    return true;
}

struct instr *instr_new_synth(int idx, const char *name, int src, int a,
                              int b, int atk, int dec, int sus, int rel)
{
    if (!song.ins[idx] && !instr_set_samples(idx, 1))
        return NULL;
    struct instr *in = song.ins[idx];
    rb->strlcpy(in->name, name, sizeof(in->name));
    struct synth s = { src, a, b };
    if (!synth_render(in, &s))
        return NULL;
    struct adsr ad = { atk, dec, sus, rel };
    adsr_to_env(&ad, &in->venv);
    return in;
}

/* ---- whole song ---- */

void song_clear(void)
{
    eng_lock();
    eng_drop(NULL);
    for (int i = 0; i < MAX_PAT; i++) {
        pool_free(song.pat[i].c);
        song.pat[i].c = NULL;
    }
    for (int i = 0; i < MAX_INS; i++) {
        instr_free(song.ins[i]);
        song.ins[i] = NULL;
    }
    rb->memset(&song, 0, sizeof(song));
    song.nch = 4;
    song.nord = 1;
    song.speed = 6;
    song.bpm = 125;
    song.linear = true;
    for (int i = 0; i < MAX_PAT; i++)
        song.pat[i].rows = 64;
    for (int i = 0; i < MAX_CH; i++)
        song.chpan[i] = 128;
    eng_unlock();
}

/* Templates: 0 chip 4 channels, 1 chip 8 channels, 2 empty. */
void song_new(int template_id)
{
    song_clear();
    rb->strcpy(song.name, "New song");
    if (template_id == 2) {
        song_set_channels(4);
        instr_new_synth(0, "Pulse", SRC_PULSE, 8, 0, 0, 0, 15, 2);
        return;
    }
    song_set_channels(template_id == 1 ? 8 : 4);
    instr_new_synth(0, "Lead", SRC_PULSE, 8, 0, 0, 7, 11, 3);
    instr_new_synth(1, "Pulse 25%", SRC_PULSE, 4, 0, 0, 6, 9, 3);
    instr_new_synth(2, "Bass", SRC_TRI, 0, 0, 0, 0, 15, 2);
    instr_new_synth(3, "Kick", SRC_KICK, 12, 6, 0, 0, 15, 1);
    instr_new_synth(4, "Snare", SRC_SNARE, 12, 6, 0, 0, 15, 1);
    instr_new_synth(5, "Hat", SRC_HAT, 12, 3, 0, 0, 15, 1);
    instr_new_synth(6, "Arp", SRC_PULSE, 2, 0, 0, 5, 7, 2);
    instr_new_synth(7, "Sine pad", SRC_SINE, 0, 0, 5, 8, 10, 8);
    if (song.ins[0]) {
        song.ins[0]->vdepth = 3;
        song.ins[0]->vrate = 10;
        song.ins[0]->vsweep = 24;
    }
}
