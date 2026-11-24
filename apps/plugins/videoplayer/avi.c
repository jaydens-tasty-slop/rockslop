/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * AVI (RIFF, with OpenDML's 'AVIX' continuation and the idx1 index).
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

#define FCC(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | \
                         ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
#define TWOCC(a, b)     ((uint16_t)((a) | ((b) << 8)))
#define ALIGN2(x)       (((x) + 1) & ~1u)
#define AVIIF_KEYFRAME  0x10

#define rd32 demux_rd32le

/* idx1 checkpoints: the stream counts at every IDX_STEP-th entry, made
 * while seeks scan the index, so that later seeks start close by. They
 * count the audio track of the last seek. */
#define IDX_STEP    2048
#define IDX_CHECKS  256
static struct check {
    uint32_t vcount, acount, abytes;
    uint32_t key, key_off;      /* the last video keyframe before */
} *idx_checks;                  /* in the player's memory */
static uint32_t idx_nchecks, idx_cap;
static uint16_t idx_aid;

/* Chunk padding: none (a list's own end), to an even size as RIFF says,
 * or to 4 bytes (the movi chunks of a player maker's converter) */
#define PAD_NONE    0
#define PAD_EVEN    1
#define PAD_QUAD    3

/* Where the chunk at pos with this size ends (its size padded by pad, a
 * PAD_* mask), no further than limit: no size makes a walk stand still,
 * go back or leave its list */
static inline uint32_t chunk_end(uint32_t pos, uint32_t size, uint32_t pad,
                                 uint32_t limit)
{
    uint64_t e = (uint64_t)pos + 8 + (((uint64_t)size + pad) & ~(uint64_t)pad);
    return e < limit ? (uint32_t)e : limit;
}

static void parse_strl(struct movie *m, int fd, uint32_t pos, uint32_t end,
                       int n)
{
    uint8_t strh[56], strf[64], h[8];
    uint32_t strf_size = 0;
    bool have_strh = false;

    rb->memset(strh, 0, sizeof(strh));
    rb->memset(strf, 0, sizeof(strf));
    while (pos + 8 <= end && demux_read_at(fd, pos, h, 8))
    {
        uint32_t id = rd32(h), size = rd32(h + 4);
        if (id == FCC('s','t','r','h'))
            have_strh = demux_read_at(fd, pos + 8, strh, MIN(size, sizeof(strh)));
        else if (id == FCC('s','t','r','f') &&
                 demux_read_at(fd, pos + 8, strf, MIN(size, sizeof(strf))))
            strf_size = MIN(size, sizeof(strf));
        pos = chunk_end(pos, size, PAD_EVEN, end);
    }
    uint32_t type = rd32(strh);
    if (!have_strh || strf_size == 0 || m->ntracks == MAX_TRACKS ||
        (type != FCC('v','i','d','s') && type != FCC('a','u','d','s')))
        return;

    struct track *t = &m->tracks[m->ntracks];
    rb->memset(t, 0, sizeof(*t));
    t->id = n;
    if (type == FCC('v','i','d','s'))
    {
        demux_parse_bitmap(t, strf, strf_size, rd32(strh + 4));
        if (m->video >= 0 && m->tracks[m->video].supported)
            return;     /* only the first video track plays */
        m->video = m->ntracks;
        m->c.avi.frame_scale = rd32(strh + 20);
        m->c.avi.frame_rate = rd32(strh + 24);
        m->duration = rd32(strh + 32);    /* frames, for now */
    }
    else
    {
        demux_parse_wave(t, strf, strf_size);
        m->c.avi.audio[m->ntracks].scale = rd32(strh + 20);
        m->c.avi.audio[m->ntracks].rate = rd32(strh + 24);
        m->c.avi.audio[m->ntracks].sample_size = rd32(strh + 44);
        m->c.avi.audio[m->ntracks].avg_bytes = rd32(strf + 8);
    }
    m->ntracks++;
}

/* avih: the frame rate, if the stream header has none, and a frame count */
static void parse_hdrl(struct movie *m, int fd, uint32_t pos, uint32_t end,
                       uint8_t *avih)
{
    uint8_t h[12];
    int n = 0;

    while (pos + 8 <= end && demux_read_at(fd, pos, h, 12))
    {
        uint32_t id = rd32(h), size = rd32(h + 4);
        if (id == FCC('a','v','i','h'))
            demux_read_at(fd, pos + 8, avih, MIN(size, 20));
        else if (id == FCC('L','I','S','T') && rd32(h + 8) == FCC('s','t','r','l'))
            parse_strl(m, fd, pos + 12, chunk_end(pos, size, PAD_NONE, end), n++);
        pos = chunk_end(pos, size, PAD_EVEN, end);
    }
}

/* Find what idx1 offsets are relative to: the 'movi' fourcc (as the
 * specification says) or the start of the file (some writers). */
static void check_index(struct avi_info *avi, int fd)
{
    uint8_t e[16], id[4];

    if (avi->idx1_count > 0 && demux_read_at(fd, avi->idx1_pos, e, 16))
    {
        const uint32_t bases[] = { avi->movi_start - 4, 0 };
        for (unsigned i = 0; i < ARRAYLEN(bases); i++)
        {
            if (demux_read_at(fd, bases[i] + rd32(e + 8), id, 4) &&
                rd32(id) == rd32(e))
            {
                avi->idx1_base = bases[i];
                return;
            }
        }
    }
    avi->idx1_pos = 0;
    avi->idx1_count = 0;
}

static bool fourcc_at(int fd, uint32_t pos)
{
    uint8_t id[4];
    if (!demux_read_at(fd, pos, id, 4))
        return false;
    for (int i = 0; i < 4; i++)
        if (id[i] < 0x20 || id[i] > 0x7e)
            return false;
    return true;
}

/* How the movi chunks are padded: to 4 bytes if only that finds a chunk
 * after the first one */
static uint32_t movi_pad(const struct avi_info *avi, int fd)
{
    uint8_t h[8];
    if (!demux_read_at(fd, avi->movi_start, h, 8))
        return PAD_EVEN;
    uint32_t even = chunk_end(avi->movi_start, rd32(h + 4), PAD_EVEN, avi->movi_end);
    uint32_t quad = chunk_end(avi->movi_start, rd32(h + 4), PAD_QUAD, avi->movi_end);
    if (even != quad && quad + 8 <= avi->movi_end && !fourcc_at(fd, even) &&
        fourcc_at(fd, quad))
        return PAD_QUAD;
    return PAD_EVEN;
}

static uint32_t frame_ms(const struct avi_info *avi, uint32_t frame)
{
    return (uint64_t)frame * avi->frame_scale * 1000 / avi->frame_rate;
}

static int avi_open(struct movie *m, int fd)
{
    struct avi_info *avi = &m->c.avi;
    uint8_t h[12], avih[20];

    rb->memset(avih, 0, sizeof(avih));
    idx_nchecks = idx_cap = 0;

    demux_read_at(fd, 0, h, 12);
    /* a file still being written has a zero or short RIFF size */
    avi->riff_end = rd32(h + 4) + 8;
    if (avi->riff_end <= 12 || avi->riff_end > m->file_size)
        avi->riff_end = m->file_size;

    uint32_t pos = 12;
    while (pos + 8 <= avi->riff_end && demux_read_at(fd, pos, h, 12))
    {
        uint32_t id = rd32(h), size = rd32(h + 4);
        uint32_t end = chunk_end(pos, size, PAD_NONE, avi->riff_end);
        if (id == FCC('L','I','S','T') && rd32(h + 8) == FCC('h','d','r','l'))
            parse_hdrl(m, fd, pos + 12, end, avih);
        else if (id == FCC('L','I','S','T') && rd32(h + 8) == FCC('m','o','v','i') &&
                 !avi->movi_start)
        {
            avi->movi_start = pos + 12;
            avi->movi_end = size ? end : avi->riff_end;
            pos = ALIGN2(avi->movi_end);
            continue;
        }
        else if (id == FCC('i','d','x','1') && avi->movi_start)
        {
            avi->idx1_pos = pos + 8;
            avi->idx1_count = (end - pos - 8) / 16;
        }
        pos = chunk_end(pos, size, PAD_EVEN, avi->riff_end);
    }

    if (avi->movi_start == 0)
        return DEMUX_ERR_FORMAT;
    if (m->video < 0)
        return DEMUX_ERR_NO_VIDEO;
    if (!m->tracks[m->video].supported)
        return DEMUX_ERR_VCODEC;

    if (avi->frame_scale == 0 || avi->frame_rate == 0)
    {
        avi->frame_scale = rd32(avih) ? rd32(avih) : 40000;  /* us per frame */
        avi->frame_rate = 1000000;
    }
    m->format = "AVI";
    /* the larger count: OpenDML's avih counts the first RIFF only, and the
     * converter that pads to 4 bytes leaves the stream's at 2 */
    m->duration = frame_ms(avi, MAX(m->duration, rd32(avih + 16)));
    avi->chunk_pad = movi_pad(avi, fd);
    m->frame_us = (uint64_t)avi->frame_scale * 1000000 / avi->frame_rate;
    check_index(avi, fd);
    if (avi->idx1_count)
    {
        /* without memory for them, seeks scan from the start */
        uint32_t n = MIN(avi->idx1_count / IDX_STEP + 1, IDX_CHECKS);
        idx_checks = player_alloc(n * sizeof(*idx_checks));
        idx_cap = idx_checks ? n : 0;
    }
    return DEMUX_OK;
}

static void avi_stream_init(struct movie *m, struct stream *s)
{
    struct avi_cursor *c = &s->c.avi;
    int n = m->tracks[s->track].id;

    rb->memset(c, 0, sizeof(*c));
    c->id = TWOCC('0' + n / 10, '0' + n % 10);
    c->pos = c->cur = m->c.avi.movi_start;
    c->end = m->c.avi.movi_end;
    c->riff_end = m->c.avi.riff_end;
}

static void cursor_set_eof(const struct movie *m, struct stream *s)
{
    s->c.avi.pos = s->c.avi.end = s->c.avi.riff_end = m->file_size;
    s->left = 0;
}

/* OpenDML: the data goes on in the 'movi' list of a following RIFF 'AVIX' */
static bool next_movi(const struct movie *m, struct stream *s)
{
    struct avi_cursor *c = &s->c.avi;
    uint8_t h[12];
    uint32_t pos = ALIGN2(c->riff_end);

    if (pos + 12 > m->file_size || !demux_read_at(s->fd, pos, h, 12) ||
        rd32(h) != FCC('R','I','F','F') || rd32(h + 8) != FCC('A','V','I','X'))
        return false;
    uint32_t riff_end = chunk_end(pos, rd32(h + 4), PAD_NONE, m->file_size);
    for (pos += 12; pos + 12 <= riff_end && demux_read_at(s->fd, pos, h, 12);
         pos = chunk_end(pos, rd32(h + 4), PAD_EVEN, riff_end))
    {
        if (rd32(h) == FCC('L','I','S','T') && rd32(h + 8) == FCC('m','o','v','i'))
        {
            c->pos = pos + 12;
            c->end = chunk_end(pos, rd32(h + 4), PAD_NONE, riff_end);
            c->riff_end = riff_end;
            return true;
        }
    }
    return false;
}

static inline bool is_data_chunk(uint32_t id)
{
    uint16_t type = id >> 16;
    return type == TWOCC('d','c') || type == TWOCC('d','b') ||
           type == TWOCC('w','b');
}

/* The time of an audio stream position */
static uint32_t audio_ms(const struct movie *m, int track, uint32_t count,
                         uint32_t bytes)
{
    const struct aformat *a = &m->tracks[track].u.a;
    const struct avi_audio_header *h = &m->c.avi.audio[track];

    if (a->codec != AC_MPA)     /* fixed-size blocks: the format says */
        return (uint64_t)(bytes / a->block_align) * a->samples_per_block *
               1000 / a->rate;
    if (h->sample_size == 0 && h->scale && h->rate)     /* a chunk a frame */
        return (uint64_t)count * h->scale * 1000 / h->rate;
    return h->avg_bytes ? (uint64_t)bytes * 1000 / h->avg_bytes : 0;
}

/* The stream position of an audio time: the byte (sample_size or
 * fixed blocks) or, for a chunk a frame, the chunk */
static uint32_t audio_pos(const struct movie *m, int track, uint32_t ms,
                          bool *by_chunk)
{
    const struct aformat *a = &m->tracks[track].u.a;
    const struct avi_audio_header *h = &m->c.avi.audio[track];

    *by_chunk = false;
    if (a->codec != AC_MPA)
        return (uint64_t)ms * a->rate / 1000 / a->samples_per_block *
               a->block_align;
    if (h->sample_size == 0 && h->scale && h->rate)
    {
        *by_chunk = true;
        return (uint64_t)ms * h->rate / ((uint64_t)h->scale * 1000);
    }
    return (uint64_t)ms * h->avg_bytes / 1000;
}

static bool avi_next(struct movie *m, struct stream *s, struct packet *p)
{
    struct avi_cursor *c = &s->c.avi;
    uint8_t h[12];

    c->bytes += c->size;
    c->size = s->left = 0;
    for (;;)
    {
        if (c->pos + 8 > c->end)
        {
            if (!next_movi(m, s))
            {
                cursor_set_eof(m, s);
                return false;
            }
            continue;
        }
        if (!demux_read_at(s->fd, c->pos, h, 8))
        {
            cursor_set_eof(m, s);
            return false;
        }
        uint32_t id = rd32(h), size = rd32(h + 4);
        uint32_t data = c->pos + 8;
        if (id == FCC('L','I','S','T'))
        {
            /* chunks grouped in 'rec ' lists: step inside */
            if (demux_read_at(s->fd, data, h + 8, 4) &&
                rd32(h + 8) == FCC('r','e','c',' '))
                c->pos = data + 4;
            else
                c->pos = chunk_end(c->pos, size, m->c.avi.chunk_pad, c->end);
            continue;
        }
        c->cur = c->pos;
        c->pos = chunk_end(c->cur, size, m->c.avi.chunk_pad, c->end);
        if ((id & 0xffff) != c->id || !is_data_chunk(id))
            continue;   /* other streams, 'ix##', JUNK */
        if (size > m->file_size - data)     /* truncated file */
            size = m->file_size - data;
        if (rb->lseek(s->fd, data, SEEK_SET) != (off_t)data)
        {
            cursor_set_eof(m, s);
            return false;
        }
        p->size = c->size = s->left = size;
        p->dur_ms = 0;
        if (s->track == m->video)
        {
            p->ms = frame_ms(&m->c.avi, c->count);
            p->key = m->tracks[m->video].u.v.codec != VC_MPEG;
        }
        else
        {
            p->ms = audio_ms(m, s->track, c->count, c->bytes);
            p->key = true;
        }
        c->count++;
        return true;
    }
}

/* Make the current chunk the next one avi_next() returns */
static void cursor_unread(struct stream *s)
{
    s->c.avi.pos = s->c.avi.cur;
    s->c.avi.count--;
    s->c.avi.size = s->left = 0;
}

static void cursor_set(const struct movie *m, struct stream *s,
                       uint32_t header, uint32_t count, uint32_t bytes)
{
    struct avi_cursor *c = &s->c.avi;
    c->pos = c->cur = header;
    c->end = m->c.avi.movi_end;
    c->riff_end = m->c.avi.riff_end;
    c->count = count;
    c->bytes = bytes;
    c->size = s->left = 0;
}

/* Without an index (or past it): walk the chunks */
static bool walk_video(struct movie *m, struct stream *v, uint32_t frame)
{
    struct packet p;

    if (frame < v->c.avi.count)
        avi_stream_init(m, v);
    while (v->c.avi.count < frame)
        if (!avi_next(m, v, &p))
            return false;
    return true;
}

static void walk_audio(struct movie *m, struct stream *a, uint32_t target,
                       bool by_chunk)
{
    struct packet p;

    if (target < (by_chunk ? a->c.avi.count : a->c.avi.bytes + a->c.avi.size))
        avi_stream_init(m, a);
    while (avi_next(m, a, &p))
    {
        if (by_chunk ? a->c.avi.count > target
                     : target < a->c.avi.bytes + p.size)
        {
            cursor_unread(a);
            return;
        }
    }
    /* the audio ends first: stays at the end */
}

/* Scan idx1 for the last keyframe at or before frame (any frame, for
 * intra-only video) and the audio chunk holding the target, starting
 * at the last checkpoint before both */
static bool seek_index(struct movie *m, struct stream *v, struct stream *a,
                       uint32_t frame, uint32_t atarget, bool by_chunk,
                       uint8_t *tmp, uint32_t tmp_size,
                       uint32_t *vframe, uint32_t *voff,
                       uint32_t *aoff, uint32_t *acount, uint32_t *abytes)
{
    struct avi_info *avi = &m->c.avi;
    struct check cur = { 0, 0, 0, 0, 0 };
    uint32_t e = 0;
    bool vfound = false, afound = (a == NULL), key_found = false;
    bool intra = m->tracks[m->video].u.v.codec != VC_MPEG;
    uint16_t aid = a ? a->c.avi.id : 0;

    if (aid != idx_aid)
    {
        idx_nchecks = 0;
        idx_aid = aid;
    }
    for (uint32_t i = idx_nchecks; i > 0; i--)
    {
        const struct check *c = &idx_checks[i - 1];
        if (c->vcount <= frame &&
            (by_chunk ? c->acount <= atarget : c->abytes <= atarget))
        {
            e = (i - 1) * IDX_STEP;
            cur = *c;
            key_found = c->key_off != 0;
            break;
        }
    }

    uint32_t per_read = tmp_size / 16;
    while (e < avi->idx1_count && !(vfound && afound))
    {
        uint32_t n = MIN(per_read, avi->idx1_count - e);
        if (!demux_read_at(v->fd, avi->idx1_pos + e * 16, tmp, n * 16))
            break;
        for (const uint8_t *p = tmp; n > 0 && !(vfound && afound); n--, e++, p += 16)
        {
            if (e % IDX_STEP == 0 && e / IDX_STEP == idx_nchecks &&
                idx_nchecks < idx_cap)
                idx_checks[idx_nchecks++] = cur;
            uint32_t id = rd32(p);
            if (!is_data_chunk(id))
                continue;
            if ((id & 0xffff) == v->c.avi.id && !vfound)
            {
                if (intra || (rd32(p + 4) & AVIIF_KEYFRAME))
                {
                    cur.key = cur.vcount;
                    cur.key_off = rd32(p + 8);
                    key_found = true;
                }
                if (cur.vcount++ == frame)
                    vfound = true;
            }
            else if ((id & 0xffff) == aid)
            {
                uint32_t size = rd32(p + 12);
                if (!afound && (by_chunk ? cur.acount == atarget
                                         : atarget < cur.abytes + size))
                {
                    afound = true;
                    *aoff = rd32(p + 8);
                    *acount = cur.acount;
                    *abytes = cur.abytes;
                }
                cur.acount++;
                cur.abytes += size;
            }
        }
    }
    if (!vfound || !key_found)
        return false;
    *vframe = cur.key;
    *voff = cur.key_off;
    if (!afound)
        *aoff = 0;
    return true;
}

static bool avi_seek(struct movie *m, struct stream *v, struct stream *a,
                     struct stream *t, uint32_t ms, uint32_t *at)
{
    struct avi_info *avi = &m->c.avi;
    uint32_t frame = (uint64_t)ms * avi->frame_rate /
                     ((uint64_t)avi->frame_scale * 1000);
    bool by_chunk = false;
    uint8_t tmp[512];
    (void)t;

    if (avi->idx1_count)
    {
        uint32_t vframe, voff, aoff = 0, acount = 0, abytes = 0;
        uint32_t atarget = a ? audio_pos(m, a->track, ms, &by_chunk) : 0;
        if (seek_index(m, v, a, frame, atarget, by_chunk, tmp, sizeof(tmp),
                       &vframe, &voff, &aoff, &acount, &abytes))
        {
            cursor_set(m, v, avi->idx1_base + voff, vframe, 0);
            *at = frame_ms(avi, vframe);
            if (vframe != frame && a)
            {
                /* a keyframe before: the audio from there */
                atarget = audio_pos(m, a->track, *at, &by_chunk);
                seek_index(m, v, a, vframe, atarget, by_chunk, tmp,
                           sizeof(tmp), &vframe, &voff, &aoff, &acount,
                           &abytes);
            }
            if (a && aoff)
                cursor_set(m, a, avi->idx1_base + aoff, acount, abytes);
            else if (a)
                walk_audio(m, a, atarget, by_chunk);
            return true;
        }
        /* not in the index: the data goes on in an OpenDML 'AVIX' */
        if (ALIGN2(avi->riff_end) + 12 > m->file_size)
            return false;
    }
    if (!walk_video(m, v, frame))
        return false;
    *at = frame_ms(avi, frame);
    if (a)
        walk_audio(m, a, audio_pos(m, a->track, *at, &by_chunk), by_chunk);
    return true;
}

static uint32_t avi_tell(const struct stream *s)
{
    return s->c.avi.pos;
}

const struct demux_ops avi_ops = {
    .open = avi_open,
    .stream_init = avi_stream_init,
    .next = avi_next,
    .seek = avi_seek,
    .tell = avi_tell,
};
