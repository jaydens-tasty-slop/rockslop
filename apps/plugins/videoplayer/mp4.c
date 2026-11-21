/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Video player: MP4 and QuickTime. The sample tables stay in the file;
 * each is read through a small window.
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
#include "demux.h"

#define BOX(a, b, c, d) (((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | \
                         ((uint32_t)(c) << 8) | (uint32_t)(d))
#define TABLE_WINDOW    256     /* bytes of a sample table read at once */
#define MAX_CHAP_REFS   4

struct box {
    uint32_t type;
    uint32_t data, end;         /* end: clipped to the parent's */
};

/* the file handle peek() reads with */
static int rd_fd;

static inline const uint8_t *peek(uint32_t pos, uint32_t n)
{
    return demux_peek(rd_fd, pos, n);
}

static uint32_t u32(uint32_t pos)
{
    const uint8_t *p = peek(pos, 4);
    return p ? demux_rd32be(p) : 0;
}

static uint16_t u16(uint32_t pos)
{
    const uint8_t *p = peek(pos, 2);
    return p ? demux_rd16be(p) : 0;
}

static uint8_t u8(uint32_t pos)
{
    const uint8_t *p = peek(pos, 1);
    return p ? p[0] : 0;
}

static void read_string(char *buf, size_t size, uint32_t pos, uint32_t len)
{
    uint32_t n = MIN(len, (uint32_t)size - 1);
    const uint8_t *p = n ? peek(pos, n) : NULL;
    if (!p)
        n = 0;
    rb->memcpy(buf, p, n);
    buf[n] = '\0';
}

/* The box at pos inside a parent ending at limit */
static bool box_at(uint32_t pos, uint32_t limit, struct box *b)
{
    const uint8_t *h = pos + 8 <= limit ? peek(pos, 8) : NULL;
    if (!h)
        return false;
    uint64_t size = demux_rd32be(h);
    b->type = demux_rd32be(h + 4);
    b->data = pos + 8;
    if (size == 1)
    {
        h = pos + 16 <= limit ? peek(pos + 8, 8) : NULL;
        if (!h)
            return false;
        size = ((uint64_t)demux_rd32be(h) << 32) | demux_rd32be(h + 4);
        b->data += 8;
    }
    else if (size == 0)         /* it goes on to the end */
        size = limit - pos;
    if (size < b->data - pos)
        return false;
    b->end = size > limit - pos ? limit : pos + size;
    return true;
}

#define FOR_BOXES(parent, b) \
    for (uint32_t pos_ = (parent)->data; box_at(pos_, (parent)->end, &(b)); \
         pos_ = (b).end)

static bool child(const struct box *parent, uint32_t type, struct box *b)
{
    FOR_BOXES(parent, *b)
        if (b->type == type)
            return true;
    return false;
}

/* ---- sample tables ---- */

static uint32_t entry(int fd, struct mp4_table *t, uint32_t i, int field)
{
    if (i >= t->count || !t->buf)
        return 0;
    if (i < t->first || i >= t->first + t->n)
    {
        uint32_t n = MIN(t->count - i, (uint32_t)t->cap);
        t->n = 0;
        if (!demux_read_at(fd, t->pos + i * t->size, t->buf, n * t->size))
            return 0;
        t->first = i;
        t->n = n;
    }
    return demux_rd32be(t->buf + (i - t->first) * t->size + 4 * field);
}

/* Media time to the movie's ms, and back */
static uint32_t to_ms(const struct mp4_track *tr, uint64_t t)
{
    t = t > tr->start ? t - tr->start : 0;
    return t * 1000 / tr->timescale + tr->delay_ms;
}

static uint64_t to_media(const struct mp4_track *tr, uint32_t ms)
{
    ms = ms > tr->delay_ms ? ms - tr->delay_ms : 0;
    return (uint64_t)ms * tr->timescale / 1000 + tr->start;
}

/* On n samples in a table of runs (stts, ctts): entry *i with *left
 * samples to go, the durations summed into *sum */
static void run_on(int fd, struct mp4_table *t, uint32_t *i, uint32_t *left,
                   uint32_t n, uint64_t *sum)
{
    for (;;)
    {
        while (!*left && *i + 1 < t->count)
            *left = entry(fd, t, ++*i, 0);
        if (!n || !*left)
            return;
        uint32_t k = MIN(n, *left);
        if (sum)
            *sum += (uint64_t)k * entry(fd, t, *i, 1);
        *left -= k;
        n -= k;
    }
}

/* Into chunk c->chunk, its first sample next */
static void enter_chunk(int fd, struct mp4_track *tr, struct mp4_cursor *c)
{
    struct mp4_table *sc = &tr->t[MP4_STSC];

    while (c->stsc + 1 < sc->count && entry(fd, sc, c->stsc + 1, 0) - 1 <= c->chunk)
        c->stsc++;
    c->chunk_left = entry(fd, sc, c->stsc, 1);
    /* co64: files under 4 GB, the low word */
    c->offset = entry(fd, &tr->t[MP4_STCO], c->chunk, tr->co64);
}

static void advance(int fd, struct mp4_track *tr, struct mp4_cursor *c,
                    uint32_t n, uint32_t bytes)
{
    c->sample += n;
    c->chunk_left -= n;
    c->offset += bytes;
    run_on(fd, &tr->t[MP4_STTS], &c->stts, &c->stts_left, n, &c->dts);
    run_on(fd, &tr->t[MP4_CTTS], &c->ctts, &c->ctts_left, n, NULL);
}

static bool is_key(int fd, struct mp4_track *tr, struct mp4_cursor *c)
{
    struct mp4_table *ss = &tr->t[MP4_STSS];

    if (!ss->count)
        return true;    /* no table: every sample is */
    while (c->stss < ss->count)
    {
        uint32_t n = entry(fd, ss, c->stss, 0) - 1;
        if (n >= c->sample)
            return n == c->sample;
        c->stss++;
    }
    return false;
}

/* s on the sample (sound read a chunk at a time: on its chunk's start) */
static void cursor_to(struct movie *m, struct stream *s, uint32_t sample)
{
    struct mp4_track *tr = &m->c.mp4.tracks[s->track];
    struct mp4_cursor *c = &s->c.mp4;
    struct mp4_table *sc = &tr->t[MP4_STSC], *ss = &tr->t[MP4_STSS];
    uint32_t chunks = tr->t[MP4_STCO].count;
    uint64_t first = 0;
    int fd = s->fd;

    rb->memset(c, 0, sizeof(*c));
    s->left = 0;
    c->sample = tr->samples;    /* past the end unless found */
    for (uint32_t j = 0; j < sc->count; j++)
    {
        uint32_t fc = entry(fd, sc, j, 0) - 1, spc = entry(fd, sc, j, 1);
        uint32_t next = j + 1 < sc->count ? entry(fd, sc, j + 1, 0) - 1 : chunks;
        if (next <= fc || next > chunks || !spc)
            continue;
        uint64_t span = (uint64_t)(next - fc) * spc;
        if (sample - first < span)
        {
            c->stsc = j;
            c->chunk = fc + (sample - first) / spc;
            c->sample = sample - (sample - first) % spc;
            enter_chunk(fd, tr, c);
            break;
        }
        first += span;
    }
    c->stts_left = entry(fd, &tr->t[MP4_STTS], 0, 0);
    run_on(fd, &tr->t[MP4_STTS], &c->stts, &c->stts_left, c->sample, &c->dts);
    c->ctts_left = entry(fd, &tr->t[MP4_CTTS], 0, 0);
    run_on(fd, &tr->t[MP4_CTTS], &c->ctts, &c->ctts_left, c->sample, NULL);

    /* on through the chunk */
    if (!tr->chunk_samples && c->sample < sample)
    {
        if (tr->const_size)
            advance(fd, tr, c, sample - c->sample,
                    (sample - c->sample) * tr->const_size);
        while (c->sample < sample)
            advance(fd, tr, c, 1, entry(fd, &tr->t[MP4_STSZ], c->sample, 0));
    }

    /* the first sync sample from it on */
    uint32_t lo = 0, hi = ss->count;
    while (lo < hi)
    {
        uint32_t mid = (lo + hi) / 2;
        if (entry(fd, ss, mid, 0) - 1 < c->sample)
            lo = mid + 1;
        else
            hi = mid;
    }
    c->stss = lo;
}

/* The last sample that starts by ms */
static uint32_t sample_at(struct movie *m, const struct stream *s, uint32_t ms)
{
    struct mp4_track *tr = &m->c.mp4.tracks[s->track];
    struct mp4_table *ts = &tr->t[MP4_STTS];
    uint64_t target = to_media(tr, ms), dts = 0;
    uint32_t sample = 0;

    for (uint32_t i = 0; i < ts->count; i++)
    {
        uint32_t n = entry(s->fd, ts, i, 0), d = entry(s->fd, ts, i, 1);
        if (d && dts + (uint64_t)n * d > target)
            return MIN(sample + (uint32_t)((target - dts) / d), tr->samples - 1);
        dts += (uint64_t)n * d;
        sample += n;
    }
    return tr->samples ? tr->samples - 1 : 0;
}

/* The last sync sample at or before sample, else the first */
static uint32_t key_before(int fd, struct mp4_track *tr, uint32_t sample)
{
    struct mp4_table *ss = &tr->t[MP4_STSS];
    uint32_t lo = 0, hi = ss->count;

    if (!ss->count)
        return sample;
    while (lo < hi)
    {
        uint32_t mid = (lo + hi) / 2;
        if (entry(fd, ss, mid, 0) - 1 <= sample)
            lo = mid + 1;
        else
            hi = mid;
    }
    return entry(fd, ss, lo ? lo - 1 : 0, 0) - 1;
}

/* ---- headers ---- */

/* An MPEG-4 descriptor at *pos: its tag (-1 if none); *pos to its data */
static int descriptor(uint32_t *pos, uint32_t end, uint32_t *len)
{
    uint32_t n = *pos < end ? MIN(end - *pos, 5u) : 0;
    const uint8_t *p = n >= 2 ? peek(*pos, n) : NULL;
    uint32_t i = 1, l = 0;

    if (!p)
        return -1;
    do
        l = (l << 7) | (p[i] & 0x7f);
    while ((p[i++] & 0x80) && i < n);
    *pos += i;
    *len = MIN(l, end - *pos);
    return p[0];
}

/* An esds box: the object type, and where the decoder's setup is */
static int parse_esds(const struct box *b, uint32_t *dsi, uint32_t *dsi_len)
{
    uint32_t pos = b->data + 4, len;    /* after the version and flags */

    if (descriptor(&pos, b->end, &len) != 3)    /* ES_Descriptor */
        return 0;
    uint8_t flags = u8(pos + 2);
    pos += 3;
    if (flags & 0x80)
        pos += 2;
    if (flags & 0x40)
        pos += 1 + u8(pos);
    if (flags & 0x20)
        pos += 2;
    if (descriptor(&pos, b->end, &len) != 4)    /* DecoderConfigDescriptor */
        return 0;
    uint32_t end = pos + len;
    int oti = u8(pos);
    pos += 13;
    if (descriptor(&pos, end, &len) == 5)       /* DecoderSpecificInfo */
    {
        *dsi = pos;
        *dsi_len = len;
    }
    return oti;
}

static void fourcc_name(char *buf, uint32_t fcc)
{
    int n = 4;
    while (n > 1 && (char)(fcc >> (8 * (4 - n))) == ' ')
        n--;    /* trailing spaces */
    for (int i = 0; i < n; i++)
    {
        char c = fcc >> (24 - 8 * i);
        buf[i] = (c >= ' ' && c <= '~') ? c : '?';
    }
    buf[n] = '\0';
}

static const struct { uint32_t fcc; const char *name; } other_codecs[] = {
    { BOX('a','v','c','1'), "H.264" }, { BOX('a','v','c','3'), "H.264" },
    { BOX('h','v','c','1'), "H.265" }, { BOX('h','e','v','1'), "H.265" },
    { BOX('a','v','0','1'), "AV1" }, { BOX('v','p','0','9'), "VP9" },
    { BOX('v','p','0','8'), "VP8" }, { BOX('m','j','p','b'), "MJPEG-B" },
    { BOX('a','c','-','3'), "AC-3" }, { BOX('e','c','-','3'), "E-AC-3" },
    { BOX('a','l','a','c'), "ALAC" }, { BOX('O','p','u','s'), "Opus" },
    { BOX('f','L','a','C'), "FLAC" }, { BOX('s','a','m','r'), "AMR" },
    { BOX('s','a','w','b'), "AMR-WB" }, { BOX('w','v','t','t'), "WebVTT" },
    { BOX('c','6','0','8'), "CEA-608" }, { BOX('s','t','p','p'), "TTML" },
};

/* MPEG-4 object types, by name */
static const char *object_name(int oti)
{
    switch (oti)
    {
    case 0x20:              return "MPEG-4";
    case 0x21:              return "H.264";
    case 0x40: case 0x66: case 0x67: case 0x68:
                            return "AAC";
    case 0x69: case 0x6b:   return "MP3";
    case 0xa5:              return "AC-3";
    case 0xa6:              return "E-AC-3";
    case 0xa9:              return "DTS";
    case 0xad:              return "Opus";
    case 0xdd:              return "Vorbis";
    default:                return NULL;
    }
}

static bool is_mpeg1(uint32_t fcc)
{
    return fcc == BOX('m','1','v',' ') || fcc == BOX('m','1','v','1') ||
           fcc == BOX('m','p','e','g') || fcc == BOX('m','p','1','v');
}

static bool is_mpeg2(uint32_t fcc)
{
    /* also HDV and XDCAM: 'hdv1'.., 'xdv1'.., 'xd51'.. */
    return fcc == BOX('m','2','v','1') || fcc == BOX('m','p','2','v') ||
           (fcc >> 8) == BOX(0,'h','d','v') || (fcc >> 8) == BOX(0,'x','d','v') ||
           (fcc >> 8) == BOX(0,'x','d','5');
}

/* MPEG audio: the layer of the first frame (1-3), 0 if none */
static int mpa_layer(struct mp4_track *tr)
{
    uint32_t at = u32(tr->t[MP4_STCO].pos + 4 * tr->co64);
    const uint8_t *h = at ? peek(at, 2) : NULL;

    if (!h || h[0] != 0xff || (h[1] & 0xe0) != 0xe0)
        return 0;
    return 4 - ((h[1] >> 1) & 3);
}

/* The track's codec from its first sample description; for MPEG video
 * also where its sequence header is */
static void set_codec(struct track *t, struct mp4_track *tr, const struct box *e,
                      uint32_t *extra, uint32_t *extra_len)
{
    uint32_t fcc = e->type;
    struct box x;
    int oti = 0;
    const char *name = NULL;

    t->supported = false;
    fourcc_name(t->codec, fcc);
    if (t->type == TRACK_VIDEO)
    {
        struct vformat *v = &t->u.v;
        struct box kids = { 0, e->data + 78, e->end };
        if (fcc == BOX('m','p','4','v') && child(&kids, BOX('e','s','d','s'), &x))
            oti = parse_esds(&x, extra, extra_len);
        v->width = u16(e->data + 24);
        v->height = u16(e->data + 26);
        if (fcc == BOX('j','p','e','g') || fcc == BOX('m','j','p','a') ||
            fcc == BOX('A','V','D','J') || fcc == BOX('d','m','b','1') || oti == 0x6c)
        {
            v->codec = VC_MJPEG;
            name = "MJPEG";
        }
        else if (is_mpeg1(fcc) || oti == 0x6a)
        {
            v->codec = VC_MPEG;
            name = "MPEG-1";
        }
        else if (is_mpeg2(fcc) || (oti >= 0x60 && oti <= 0x65))
        {
            v->codec = VC_MPEG;
            name = "MPEG-2";
        }
        else
            name = object_name(oti);
        if (v->codec != VC_MPEG)
            *extra_len = 0;
        t->supported = v->codec != VC_NONE && v->width && v->height &&
                       v->width <= 4096 && v->height <= 4096;
    }
    else if (t->type == TRACK_AUDIO)
    {
        struct aformat *a = &t->u.a;
        int version = u16(e->data + 8);
        uint32_t dsi, dsi_len;
        static const uint8_t kids_at[3] = { 28, 44, 64 };
        struct box kids = { 0, e->data + kids_at[MIN(version, 2)], e->end };
        if (fcc == BOX('m','p','4','a') && child(&kids, BOX('e','s','d','s'), &x))
            oti = parse_esds(&x, &dsi, &dsi_len);
        a->channels = u16(e->data + 16);
        a->bits = u16(e->data + 18);
        a->rate = u32(e->data + 24) >> 16;
        a->block_align = 1;
        a->samples_per_block = 1;
        if (version < 2 && ((fcc == BOX('t','w','o','s') && a->bits == 16) ||
                            fcc == BOX('s','o','w','t') ||
                            (fcc == BOX('r','a','w',' ') && a->bits == 8)))
        {
            a->codec = AC_PCM;
            a->big_endian = fcc == BOX('t','w','o','s');
            a->block_align = a->channels * a->bits / 8;
            name = "PCM";
        }
        else if (version < 2 && fcc == BOX('i','m','a','4'))
        {
            a->codec = AC_IMA_QT;
            a->block_align = 34 * a->channels;
            a->samples_per_block = 64;
            name = "IMA4";
        }
        else if (oti == 0x69 || oti == 0x6b || fcc == BOX('.','m','p','3'))
        {
            int layer = mpa_layer(tr);
            if (layer && layer < 3)
            {
                a->codec = AC_MPA;
                name = layer == 1 ? "MP1" : "MP2";
            }
            else
                name = "MP3";
        }
        else
            name = object_name(oti);
        /* old QuickTime sound: a sample is a sample frame (stts says so),
           read a chunk at a time */
        struct mp4_table *ts = &tr->t[MP4_STTS];
        if (ts->count == 1 && u32(ts->pos + 4) == 1)
        {
            tr->chunk_samples = a->samples_per_block;
            tr->chunk_bytes = a->block_align;
        }
        t->supported = a->codec != AC_NONE && a->channels >= 1 &&
                       a->channels <= 2 && a->rate >= 1000 && a->rate <= 192000 &&
                       a->block_align;
    }
    else if (fcc == BOX('t','x','3','g') || fcc == BOX('t','e','x','t'))
    {
        t->u.s = SC_TX3G;
        name = fcc == BOX('t','x','3','g') ? "TX3G" : "Text";
        t->supported = true;
    }

    for (unsigned i = 0; !name && i < ARRAYLEN(other_codecs); i++)
        if (fcc == other_codecs[i].fcc)
            name = other_codecs[i].name;
    if (name)
        rb->strlcpy(t->codec, name, sizeof(t->codec));
    if (!tr->samples || !tr->timescale || !tr->t[MP4_STCO].count ||
        !tr->t[MP4_STSC].count)
        t->supported = false;
}

static void parse_elst(struct mp4_track *tr, const struct box *b,
                       uint32_t movie_scale)
{
    bool v1 = u8(b->data) == 1;
    uint32_t size = v1 ? 20 : 12, count = u32(b->data + 4), pos = b->data + 8;

    for (uint32_t i = 0; i < count && pos + size <= b->end; i++, pos += size)
    {
        uint64_t dur = v1 ? ((uint64_t)u32(pos) << 32) | u32(pos + 4) : u32(pos);
        int32_t time = v1 ? (int32_t)u32(pos + 12) : (int32_t)u32(pos + 4);
        if (v1 && u32(pos + 8) == 0xffffffff)
            time = -1;
        if (time == -1)     /* empty: nothing shows for a while */
        {
            tr->delay_ms += dur * 1000 / movie_scale;
            continue;
        }
        tr->start = time > 0 ? time : 0;
        break;
    }
}

static void parse_trak(struct movie *m, const struct box *trak,
                       uint32_t movie_scale, uint32_t *chap, int *nchap)
{
    int i = m->ntracks;
    struct track *t = &m->tracks[i];
    struct mp4_track *tr = &m->c.mp4.tracks[i];
    struct box b, x, mdia, minf, stbl, stsd = { 0, 0, 0 };
    uint32_t handler = 0, extra = 0, extra_len = 0;

    rb->memset(t, 0, sizeof(*t));
    rb->memset(tr, 0, sizeof(*tr));
    if (!child(trak, BOX('m','d','i','a'), &mdia) ||
        !child(&mdia, BOX('m','i','n','f'), &minf) ||
        !child(&minf, BOX('s','t','b','l'), &stbl))
        return;
    FOR_BOXES(trak, b)
    {
        if (b.type == BOX('t','k','h','d'))
        {
            t->is_default = u32(b.data) & 1;   /* enabled */
            t->id = u32(b.data + (u8(b.data) == 1 ? 20 : 12));
        }
        else if (b.type == BOX('t','r','e','f'))
        {
            FOR_BOXES(&b, x)
                for (uint32_t p = x.data; x.type == BOX('c','h','a','p') &&
                     p + 4 <= x.end && *nchap < MAX_CHAP_REFS; p += 4)
                    chap[(*nchap)++] = u32(p);
        }
        else if (b.type == BOX('e','d','t','s') &&
                 child(&b, BOX('e','l','s','t'), &x))
            parse_elst(tr, &x, movie_scale);
    }
    FOR_BOXES(&mdia, b)
    {
        if (b.type == BOX('m','d','h','d'))
        {
            bool v1 = u8(b.data) == 1;
            uint16_t lang = u16(b.data + (v1 ? 32 : 20));
            tr->timescale = u32(b.data + (v1 ? 20 : 12));
            if (lang >= 0x400)  /* ISO 639-2, else a Macintosh code */
            {
                for (int k = 0; k < 3; k++)
                    t->lang[k] = ((lang >> (10 - 5 * k)) & 31) + 0x60;
                if (!rb->strcmp(t->lang, "und"))
                    t->lang[0] = '\0';
            }
        }
        else if (b.type == BOX('h','d','l','r'))
            handler = u32(b.data + 8);
    }
    switch (handler)
    {
    case BOX('v','i','d','e'): t->type = TRACK_VIDEO; break;
    case BOX('s','o','u','n'): t->type = TRACK_AUDIO; break;
    case BOX('t','e','x','t'): case BOX('s','b','t','l'): case BOX('s','u','b','t'):
        t->type = TRACK_TEXT;
        break;
    default:
        return;     /* timecodes, hints and such */
    }

    FOR_BOXES(&stbl, b)
    {
        static const struct { uint32_t type; uint8_t table, size; } tables[] = {
            { BOX('s','t','t','s'), MP4_STTS, 8 }, { BOX('c','t','t','s'), MP4_CTTS, 8 },
            { BOX('s','t','s','s'), MP4_STSS, 4 }, { BOX('s','t','s','c'), MP4_STSC, 12 },
            { BOX('s','t','s','z'), MP4_STSZ, 4 }, { BOX('s','t','c','o'), MP4_STCO, 4 },
            { BOX('c','o','6','4'), MP4_STCO, 8 },
        };
        uint32_t head = 8;  /* version, flags, count */
        if (b.type == BOX('s','t','s','d'))
            stsd = b;
        if (b.type == BOX('s','t','s','z'))
        {
            tr->const_size = u32(b.data + 4);
            tr->samples = u32(b.data + 8);
            if (tr->const_size)
                continue;
            head = 12;
        }
        for (unsigned k = 0; k < ARRAYLEN(tables); k++)
        {
            if (b.type != tables[k].type)
                continue;
            struct mp4_table *tb = &tr->t[tables[k].table];
            tb->size = tables[k].size;
            tb->pos = b.data + head;
            tb->count = tb->pos <= b.end ?
                        MIN(u32(b.data + head - 4), (b.end - tb->pos) / tb->size) : 0;
            tr->co64 |= b.type == BOX('c','o','6','4');
        }
    }
    if (stsd.type && box_at(stsd.data + 8, stsd.end, &b))
        set_codec(t, tr, &b, &extra, &extra_len);
    if (t->type == TRACK_VIDEO && (m->video < 0 ||
        (t->supported && !m->tracks[m->video].supported)))
    {
        const uint8_t *p = extra_len ? peek(extra, MIN(extra_len, VEXTRA_MAX)) : NULL;
        m->video = i;
        m->vextra_len = p ? MIN(extra_len, VEXTRA_MAX) : 0;
        if (p)
            rb->memcpy(m->vextra, p, m->vextra_len);
        if (tr->timescale && tr->t[MP4_STTS].count)
            m->frame_us = (uint64_t)u32(tr->t[MP4_STTS].pos + 4) * 1000000 /
                          tr->timescale;
    }
    m->ntracks++;
}

static void parse_udta(struct movie *m, const struct box *udta)
{
    struct box b, x, y, d;

    FOR_BOXES(udta, b)
    {
        if (b.type == BOX(0xa9,'n','a','m') && b.end - b.data > 4)
        {
            /* QuickTime's: a length, a language, the text */
            read_string(m->title, sizeof(m->title), b.data + 4,
                        MIN(u16(b.data), b.end - b.data - 4));
        }
        else if (b.type == BOX('m','e','t','a'))
        {
            struct box meta = b;
            if (!u32(meta.data))
                meta.data += 4;     /* ISO's version and flags */
            if (child(&meta, BOX('i','l','s','t'), &x) &&
                child(&x, BOX(0xa9,'n','a','m'), &y) &&
                child(&y, BOX('d','a','t','a'), &d) && d.end - d.data > 8)
                read_string(m->title, sizeof(m->title), d.data + 8,
                            d.end - d.data - 8);
        }
    }
}

/* A chapter track's samples start the chapters; it goes from the list */
static void take_chapters(struct movie *m, int i)
{
    struct mp4_track *tr = &m->c.mp4.tracks[i];
    struct mp4_table ts = tr->t[MP4_STTS];
    uint8_t one[8];
    uint64_t dts = 0;

    ts.buf = one;
    ts.cap = 1;
    ts.n = 0;
    for (uint32_t e = 0; tr->timescale && e < ts.count && m->nchapters < MAX_CHAPTERS; e++)
    {
        uint32_t n = entry(rd_fd, &ts, e, 0), d = entry(rd_fd, &ts, e, 1);
        for (uint32_t k = 0; k < n && m->nchapters < MAX_CHAPTERS; k++, dts += d)
            m->chapters[m->nchapters++] = to_ms(tr, dts);
    }
    m->ntracks--;
    rb->memmove(&m->tracks[i], &m->tracks[i + 1],
                (m->ntracks - i) * sizeof(m->tracks[0]));
    rb->memmove(&m->c.mp4.tracks[i], &m->c.mp4.tracks[i + 1],
                (m->ntracks - i) * sizeof(m->c.mp4.tracks[0]));
    if (m->video > i)
        m->video--;
}

static int mp4_open(struct movie *m, int fd)
{
    struct box b, moov = { 0, 0, 0 };
    uint32_t chap[MAX_CHAP_REFS], scale = 1000;
    uint64_t duration = 0;
    int nchap = 0;

    rd_fd = fd;
    m->format = "QuickTime";
    for (uint32_t pos = 0; box_at(pos, m->file_size, &b); pos = b.end)
    {
        if (b.type == BOX('f','t','y','p') && u32(b.data) != BOX('q','t',' ',' '))
            m->format = "MP4";
        else if (b.type == BOX('m','o','o','v'))
            moov = b;
    }
    if (!moov.type)
        return DEMUX_ERR_FORMAT;    /* or its index is compressed ('cmov') */

    if (child(&moov, BOX('m','v','h','d'), &b))
    {
        bool v1 = u8(b.data) == 1;
        scale = MAX(u32(b.data + (v1 ? 20 : 12)), 1u);
        duration = v1 ? ((uint64_t)u32(b.data + 24) << 32) | u32(b.data + 28) :
                        u32(b.data + 16);
        m->duration = duration * 1000 / scale;
    }
    m->video = -1;
    FOR_BOXES(&moov, b)
    {
        if (b.type == BOX('t','r','a','k') && m->ntracks < MAX_TRACKS)
            parse_trak(m, &b, scale, chap, &nchap);
        else if (b.type == BOX('u','d','t','a'))
            parse_udta(m, &b);
    }
    for (int i = m->ntracks - 1; i >= 0; i--)
        for (int k = 0; k < nchap; k++)
            if (m->tracks[i].type == TRACK_TEXT && m->tracks[i].id == (int)chap[k] &&
                m->tracks[i].id)
            {
                take_chapters(m, i);
                break;
            }
    if (m->video < 0)
        return DEMUX_ERR_NO_VIDEO;
    if (!m->tracks[m->video].supported)
        return DEMUX_ERR_VCODEC;

    /* windows on the tables of the tracks that can play */
    for (int i = 0; i < m->ntracks; i++)
    {
        for (int k = 0; m->tracks[i].supported && k < MP4_TABLES; k++)
        {
            struct mp4_table *tb = &m->c.mp4.tracks[i].t[k];
            if (!tb->count)
                continue;
            tb->cap = MIN(tb->count, TABLE_WINDOW / tb->size);
            tb->buf = player_alloc(tb->cap * tb->size);
            if (!tb->buf)
                return DEMUX_ERR_MEMORY;
        }
    }
    m->pts = true;
    return DEMUX_OK;
}

static void mp4_stream_init(struct movie *m, struct stream *s)
{
    cursor_to(m, s, 0);
}

/* ---- packets ---- */

static bool mp4_next(struct movie *m, struct stream *s, struct packet *p)
{
    struct mp4_track *tr = &m->c.mp4.tracks[s->track];
    struct mp4_cursor *c = &s->c.mp4;
    int fd = s->fd;

    s->left = 0;
    for (;;)
    {
        if (c->sample >= tr->samples)
            return false;
        if (c->chunk_left)
            break;
        if (++c->chunk >= tr->t[MP4_STCO].count)
            return false;
        enter_chunk(fd, tr, c);
    }
    if (s->limit && c->offset >= s->limit)
        return false;

    uint32_t n = 1, bytes;
    if (tr->chunk_samples)
    {
        n = MIN(c->chunk_left, tr->samples - c->sample);
        bytes = n / tr->chunk_samples * tr->chunk_bytes;
    }
    else
        bytes = tr->const_size ? tr->const_size :
                entry(fd, &tr->t[MP4_STSZ], c->sample, 0);
    int64_t pts = c->dts;
    if (tr->t[MP4_CTTS].count)
        pts += (int32_t)entry(fd, &tr->t[MP4_CTTS], c->ctts, 1);
    p->ms = to_ms(tr, pts > 0 ? pts : 0);
    p->dur_ms = (uint64_t)n * entry(fd, &tr->t[MP4_STTS], c->stts, 1) * 1000 /
                tr->timescale;
    p->key = tr->chunk_samples || is_key(fd, tr, c);
    p->size = s->left = bytes;
    uint32_t at = c->offset;
    advance(fd, tr, c, n, bytes);
    rb->lseek(fd, at, SEEK_SET);
    return true;
}

/* ---- seeking ---- */

static bool mp4_seek(struct movie *m, struct stream *v, struct stream *a,
                     struct stream *t, uint32_t ms, uint32_t *at)
{
    *at = ms;
    if (v)
    {
        struct mp4_track *tr = &m->c.mp4.tracks[v->track];
        uint32_t key = key_before(v->fd, tr, sample_at(m, v, ms));
        /* found by decoding time: one that shows after ms (ctts) goes
           back a keyframe */
        for (int tries = 0; tries < 2; tries++)
        {
            struct stream w;
            struct packet p;
            cursor_to(m, v, key);
            w = *v;
            if (!mp4_next(m, &w, &p))
                break;
            *at = p.ms;
            uint32_t before = key ? key_before(v->fd, tr, key - 1) : key;
            if (p.ms <= ms || before >= key)
                break;
            key = before;
        }
    }
    if (a)
        cursor_to(m, a, sample_at(m, a, ms));
    if (t)
        cursor_to(m, t, sample_at(m, t, ms));
    return true;
}

static uint32_t mp4_tell(const struct stream *s)
{
    return s->c.mp4.offset;
}

const struct demux_ops mp4_ops = {
    .open = mp4_open,
    .stream_init = mp4_stream_init,
    .next = mp4_next,
    .seek = mp4_seek,
    .tell = mp4_tell,
};
