/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Video player: Matroska (and WebM, to name its codecs). Packets come from
 * walking the clusters' blocks; seeks start from a sparse copy of the cues.
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

#define ID_EBML             0x1a45dfa3
#define ID_DOCTYPE          0x4282
#define ID_SEGMENT          0x18538067
#define ID_SEEKHEAD         0x114d9b74
#define ID_SEEK             0x4dbb
#define ID_SEEKID           0x53ab
#define ID_SEEKPOSITION     0x53ac
#define ID_INFO             0x1549a966
#define ID_TIMESTAMPSCALE   0x2ad7b1
#define ID_DURATION         0x4489
#define ID_TITLE            0x7ba9
#define ID_TRACKS           0x1654ae6b
#define ID_TRACKENTRY       0xae
#define ID_TRACKNUMBER      0xd7
#define ID_TRACKTYPE        0x83
#define ID_FLAGENABLED      0xb9
#define ID_FLAGDEFAULT      0x88
#define ID_DEFAULTDURATION  0x23e383
#define ID_NAME             0x536e
#define ID_LANGUAGE         0x22b59c
#define ID_CODECID          0x86
#define ID_CODECPRIVATE     0x63a2
#define ID_VIDEO            0xe0
#define ID_PIXELWIDTH       0xb0
#define ID_PIXELHEIGHT      0xba
#define ID_AUDIO            0xe1
#define ID_SAMPLINGFREQ     0xb5
#define ID_CHANNELS         0x9f
#define ID_BITDEPTH         0x6264
#define ID_CONTENTENCODINGS 0x6d80
#define ID_CONTENTENCODING  0x6240
#define ID_COMPRESSION      0x5034
#define ID_COMPALGO         0x4254
#define ID_COMPSETTINGS     0x4255
#define ID_ENCRYPTION       0x5035
#define ID_CUES             0x1c53bb6b
#define ID_CUEPOINT         0xbb
#define ID_CUETIME          0xb3
#define ID_CUETRACKPOSITIONS 0xb7
#define ID_CUETRACK         0xf7
#define ID_CUECLUSTERPOSITION 0xf1
#define ID_CHAPTERS         0x1043a770
#define ID_EDITIONENTRY     0x45b9
#define ID_CHAPTERATOM      0xb6
#define ID_CHAPTERTIMESTART 0x91
#define ID_CLUSTER          0x1f43b675
#define ID_TIMESTAMP        0xe7
#define ID_SIMPLEBLOCK      0xa3
#define ID_BLOCKGROUP       0xa0
#define ID_BLOCK            0xa1
#define ID_BLOCKDURATION    0x9b
#define ID_REFERENCEBLOCK   0xfb

#define MKV_CUES            256
#define TRACK_VIDEO_TYPE    1
#define TRACK_AUDIO_TYPE    2
#define TRACK_TEXT_TYPE     17

/* the file handle peek() reads with */
static int rd_fd;

static inline const uint8_t *peek(uint32_t pos, uint32_t n)
{
    return demux_peek(rd_fd, pos, n);
}

struct element {
    uint32_t id;
    uint32_t data, end;         /* end: clipped to the parent's */
};

static int vint_len(uint8_t b)
{
    int len = 1;
    while (len <= 8 && !(b & (0x80 >> (len - 1))))
        len++;
    return len;
}

/* A variable-size integer, the length marker taken off; 0 if bad */
static int vint(const uint8_t *p, int avail, uint64_t *v)
{
    int len = vint_len(p[0]);
    if (len > 8 || len > avail)
        return 0;
    uint64_t x = p[0] & (0xff >> len);
    for (int i = 1; i < len; i++)
        x = (x << 8) | p[i];
    *v = x;
    return len;
}

/* The element at pos inside a parent ending at limit */
static bool element(uint32_t pos, uint32_t limit, struct element *e)
{
    if (pos + 2 > limit)
        return false;
    uint32_t n = MIN(12u, limit - pos);
    const uint8_t *h = peek(pos, n);
    if (!h)
        return false;
    int il = vint_len(h[0]);
    if (il > 4 || (uint32_t)il >= n)
        return false;
    uint32_t id = 0;
    for (int i = 0; i < il; i++)
        id = (id << 8) | h[i];
    uint64_t size;
    int sl = vint(h + il, n - il, &size);
    if (!sl)
        return false;
    /* all ones: the size is unknown (it goes on to the parent's end) */
    bool unknown = size == (((uint64_t)1 << (7 * sl)) - 1);
    e->id = id;
    e->data = pos + il + sl;
    if (e->data > limit)
        return false;
    e->end = unknown || size > limit - e->data ? limit : e->data + size;
    return true;
}

static uint64_t read_uint(const struct element *e)
{
    uint32_t n = MIN(e->end - e->data, 8u);
    const uint8_t *p = peek(e->data, n);
    uint64_t v = 0;
    for (uint32_t i = 0; p && i < n; i++)
        v = (v << 8) | p[i];
    return v;
}

/* A float's whole part */
static uint64_t read_float(const struct element *e)
{
    uint64_t bits = read_uint(e);
    int exp, mbits;
    if (e->end - e->data == 4)
    {
        exp = ((bits >> 23) & 0xff) - 127;
        mbits = 23;
    }
    else
    {
        exp = ((bits >> 52) & 0x7ff) - 1023;
        mbits = 52;
    }
    if (exp < 0 || (bits >> 63 && mbits == 52) || (bits >> 31 && mbits == 23))
        return 0;
    uint64_t mant = (bits & (((uint64_t)1 << mbits) - 1)) | ((uint64_t)1 << mbits);
    if (exp >= 63)
        return UINT64_MAX;
    return exp >= mbits ? mant << MIN(exp - mbits, 63 - mbits) : mant >> (mbits - exp);
}

static void read_string(const struct element *e, char *buf, size_t size)
{
    uint32_t n = MIN(e->end - e->data, (uint32_t)size - 1);
    const uint8_t *p = peek(e->data, n);
    if (!p)
        n = 0;
    rb->memcpy(buf, p, n);
    buf[n] = '\0';
}

#define FOR_CHILDREN(parent, e) \
    for (uint32_t pos_ = (parent)->data; element(pos_, (parent)->end, &(e)); \
         pos_ = (e).end)

/* ---- headers ---- */

static const struct { const char *id; const char *name; } other_codecs[] = {
    { "V_MPEG4/ISO/AVC", "H.264" }, { "V_MPEGH/ISO/HEVC", "H.265" },
    { "V_MPEG4/ISO", "MPEG-4" }, { "V_VP8", "VP8" }, { "V_VP9", "VP9" },
    { "V_AV1", "AV1" }, { "V_THEORA", "Theora" }, { "V_MS/VFW", "VfW" },
    { "A_MPEG/L3", "MP3" }, { "A_AAC", "AAC" }, { "A_AC3", "AC-3" },
    { "A_EAC3", "E-AC-3" }, { "A_DTS", "DTS" }, { "A_VORBIS", "Vorbis" },
    { "A_OPUS", "Opus" }, { "A_FLAC", "FLAC" }, { "A_TRUEHD", "TrueHD" },
    { "S_VOBSUB", "VobSub" }, { "S_HDMV/PGS", "PGS" },
};

struct track_info {
    char codec_id[32];
    uint32_t priv, priv_len;    /* CodecPrivate */
    uint32_t width, height;
    uint32_t rate, channels, bits;
    uint64_t default_duration;  /* ns per frame */
    bool enabled;
    bool encoded;               /* compressed or encrypted: no */
};

static void parse_encodings(struct movie *m, int i, const struct element *encs,
                            struct track_info *ti)
{
    struct element enc, x, y;
    FOR_CHILDREN(encs, enc)
    {
        if (enc.id != ID_CONTENTENCODING)
            continue;
        FOR_CHILDREN(&enc, x)
        {
            if (x.id == ID_ENCRYPTION)
                ti->encoded = true;
            if (x.id != ID_COMPRESSION)
                continue;
            uint64_t algo = 0;
            struct element settings = { 0, 0, 0 };
            FOR_CHILDREN(&x, y)
            {
                if (y.id == ID_COMPALGO)
                    algo = read_uint(&y);
                else if (y.id == ID_COMPSETTINGS)
                    settings = y;
            }
            uint32_t n = settings.end - settings.data;
            const uint8_t *p = n ? peek(settings.data, n) : NULL;
            if (algo != 3 || n > MKV_PREFIX_MAX || (n && !p))
                ti->encoded = true;     /* zlib, bzip2, LZO: not here */
            else
            {
                rb->memcpy(m->c.mkv.prefix[i], p, n);
                m->c.mkv.prefix_len[i] = n;
            }
        }
    }
}

static void set_codec(struct track *t, struct track_info *ti)
{
    const char *id = ti->codec_id;
    struct aformat *a = &t->u.a;

    t->supported = false;
    if (t->type == TRACK_VIDEO)
    {
        if (!rb->strcmp(id, "V_MJPEG"))
        {
            t->u.v.codec = VC_MJPEG;
            rb->strcpy(t->codec, "MJPEG");
        }
        else if (!rb->strcmp(id, "V_MPEG1") || !rb->strcmp(id, "V_MPEG2"))
        {
            t->u.v.codec = VC_MPEG;
            rb->strcpy(t->codec, id[6] == '1' ? "MPEG-1" : "MPEG-2");
        }
        else if (!rb->strcmp(id, "V_MS/VFW/FOURCC"))
        {
            const uint8_t *bi = ti->priv_len >= 40 ? peek(ti->priv, 40) : NULL;
            if (bi && demux_parse_bitmap(t, bi, 40, 0))
                t->supported = !ti->encoded;
            return;
        }
        t->u.v.width = ti->width;
        t->u.v.height = ti->height;
        t->supported = t->u.v.codec != VC_NONE && ti->width && ti->height &&
                       ti->width <= 4096 && ti->height <= 4096;
    }
    else if (t->type == TRACK_AUDIO)
    {
        a->channels = ti->channels ? ti->channels : 1;
        a->rate = ti->rate ? ti->rate : 8000;
        a->bits = ti->bits;
        a->block_align = 1;
        a->samples_per_block = 1;
        if (!rb->strcmp(id, "A_MPEG/L2") || !rb->strcmp(id, "A_MPEG/L1"))
        {
            a->codec = AC_MPA;
            rb->strcpy(t->codec, id[8] == '2' ? "MP2" : "MP1");
        }
        else if (!rb->strncmp(id, "A_PCM/INT/", 10) &&
                 (ti->bits == 16 || (ti->bits == 8 && id[10] == 'L')))
        {
            a->codec = AC_PCM;
            a->big_endian = id[10] == 'B';
            a->block_align = a->channels * ti->bits / 8;
            rb->strcpy(t->codec, "PCM");
        }
        else if (!rb->strcmp(id, "A_MS/ACM"))
        {
            uint32_t n = MIN(ti->priv_len, 40u);
            const uint8_t *wf = n >= 16 ? peek(ti->priv, n) : NULL;
            if (wf && demux_parse_wave(t, wf, n))
                t->supported = !ti->encoded;
            return;
        }
        t->supported = a->codec != AC_NONE && a->channels <= 2 &&
                       a->rate >= 1000 && a->rate <= 192000;
    }
    else
    {
        if (!rb->strcmp(id, "S_TEXT/UTF8") || !rb->strcmp(id, "S_TEXT/WEBVTT"))
        {
            t->u.s = SC_TEXT;
            rb->strcpy(t->codec, id[7] == 'U' ? "SRT" : "WebVTT");
        }
        else if (!rb->strcmp(id, "S_TEXT/ASS") || !rb->strcmp(id, "S_TEXT/SSA") ||
                 !rb->strcmp(id, "S_ASS") || !rb->strcmp(id, "S_SSA"))
        {
            t->u.s = SC_ASS;
            rb->strcpy(t->codec, "ASS");
        }
        t->supported = t->u.s != SC_NONE;
    }
    if (t->supported)
    {
        t->supported = !ti->encoded && ti->enabled;
        return;
    }
    for (unsigned i = 0; i < ARRAYLEN(other_codecs); i++)
    {
        if (!rb->strncmp(id, other_codecs[i].id, rb->strlen(other_codecs[i].id)))
        {
            rb->strlcpy(t->codec, other_codecs[i].name, sizeof(t->codec));
            return;
        }
    }
    rb->strlcpy(t->codec, id + (id[0] && id[1] == '_' ? 2 : 0), sizeof(t->codec));
}

static void parse_track(struct movie *m, const struct element *entry)
{
    struct track *t = &m->tracks[m->ntracks];
    struct track_info ti;
    struct element e, x;
    uint64_t type = 0;

    rb->memset(t, 0, sizeof(*t));
    rb->memset(&ti, 0, sizeof(ti));
    t->is_default = true;
    ti.enabled = true;
    m->c.mkv.prefix_len[m->ntracks] = 0;
    FOR_CHILDREN(entry, e)
    {
        switch (e.id)
        {
        case ID_TRACKNUMBER:    t->id = read_uint(&e); break;
        case ID_TRACKTYPE:      type = read_uint(&e); break;
        case ID_FLAGENABLED:    ti.enabled = read_uint(&e); break;
        case ID_FLAGDEFAULT:    t->is_default = read_uint(&e); break;
        case ID_DEFAULTDURATION: ti.default_duration = read_uint(&e); break;
        case ID_NAME:           read_string(&e, t->name, sizeof(t->name)); break;
        case ID_LANGUAGE:       read_string(&e, t->lang, sizeof(t->lang)); break;
        case ID_CODECID:        read_string(&e, ti.codec_id, sizeof(ti.codec_id)); break;
        case ID_CODECPRIVATE:
            ti.priv = e.data;
            ti.priv_len = e.end - e.data;
            break;
        case ID_VIDEO:
            FOR_CHILDREN(&e, x)
            {
                if (x.id == ID_PIXELWIDTH)
                    ti.width = read_uint(&x);
                else if (x.id == ID_PIXELHEIGHT)
                    ti.height = read_uint(&x);
            }
            break;
        case ID_AUDIO:
            FOR_CHILDREN(&e, x)
            {
                if (x.id == ID_SAMPLINGFREQ)
                    ti.rate = read_float(&x);
                else if (x.id == ID_CHANNELS)
                    ti.channels = read_uint(&x);
                else if (x.id == ID_BITDEPTH)
                    ti.bits = read_uint(&x);
            }
            break;
        case ID_CONTENTENCODINGS:
            parse_encodings(m, m->ntracks, &e, &ti);
            break;
        }
    }
    if (!rb->strcmp(t->lang, "und"))
        t->lang[0] = '\0';
    if (type == TRACK_VIDEO_TYPE)
        t->type = TRACK_VIDEO;
    else if (type == TRACK_AUDIO_TYPE)
        t->type = TRACK_AUDIO;
    else if (type == TRACK_TEXT_TYPE)
        t->type = TRACK_TEXT;
    else
        return;     /* buttons, logos and such */
    set_codec(t, &ti);
    if (t->type == TRACK_VIDEO && (m->video < 0 ||
        (t->supported && !m->tracks[m->video].supported)))
    {
        m->video = m->ntracks;
        if (ti.default_duration)
            m->frame_us = ti.default_duration / 1000;
        /* MPEG: its sequence header (after a BITMAPINFOHEADER for VfW) */
        uint32_t skip = !rb->strcmp(ti.codec_id, "V_MS/VFW/FOURCC") ? 40 : 0;
        uint32_t n = ti.priv_len > skip ? MIN(ti.priv_len - skip, VEXTRA_MAX) : 0;
        const uint8_t *p = n ? peek(ti.priv + skip, n) : NULL;
        m->vextra_len = p ? n : 0;
        if (p)
            rb->memcpy(m->vextra, p, n);
    }
    m->ntracks++;
}

static void parse_cues(struct movie *m, uint32_t pos)
{
    struct mkv_info *mk = &m->c.mkv;
    struct element cues, cp, x, y;
    uint32_t count = 0;
    uint16_t vnum = m->tracks[m->video].id;

    mk->cues = player_alloc(MKV_CUES * sizeof(struct mkv_cue));
    mk->ncues = 0;
    mk->cue_step = 1;
    if (!mk->cues || !element(pos, mk->segment_end, &cues) || cues.id != ID_CUES)
        return;
    FOR_CHILDREN(&cues, cp)
    {
        if (cp.id != ID_CUEPOINT)
            continue;
        uint64_t time = 0, cluster = 0;
        bool video = false;
        FOR_CHILDREN(&cp, x)
        {
            if (x.id == ID_CUETIME)
                time = read_uint(&x);
            else if (x.id == ID_CUETRACKPOSITIONS)
            {
                FOR_CHILDREN(&x, y)
                {
                    if (y.id == ID_CUETRACK)
                        video = read_uint(&y) == vnum;
                    else if (y.id == ID_CUECLUSTERPOSITION)
                        cluster = read_uint(&y);
                }
            }
        }
        if (!video || count++ % mk->cue_step)
            continue;
        if (mk->ncues == MKV_CUES)
        {
            /* full: every other one goes */
            for (int i = 0; i < MKV_CUES / 2; i++)
                mk->cues[i] = mk->cues[2 * i];
            mk->ncues = MKV_CUES / 2;
            mk->cue_step *= 2;
            if ((count - 1) % mk->cue_step)
                continue;
        }
        mk->cues[mk->ncues].ms = time * mk->timescale / 1000000;
        mk->cues[mk->ncues].cluster = mk->segment + cluster;
        mk->ncues++;
    }
}

static void parse_chapters(struct movie *m, uint32_t pos)
{
    struct element chapters, edition, atom, x;

    if (!element(pos, m->c.mkv.segment_end, &chapters) || chapters.id != ID_CHAPTERS)
        return;
    FOR_CHILDREN(&chapters, edition)
    {
        if (edition.id != ID_EDITIONENTRY)
            continue;
        FOR_CHILDREN(&edition, atom)
        {
            if (atom.id != ID_CHAPTERATOM || m->nchapters == MAX_CHAPTERS)
                continue;
            FOR_CHILDREN(&atom, x)
                if (x.id == ID_CHAPTERTIMESTART)
                    m->chapters[m->nchapters++] = read_uint(&x) / 1000000;
        }
        break;      /* the first edition only */
    }
}

static int mkv_open(struct movie *m, int fd)
{
    struct mkv_info *mk = &m->c.mkv;
    struct element e, seg, x, y;
    char doctype[12] = "matroska";
    uint32_t at[4] = { 0, 0, 0, 0 };     /* info, tracks, cues, chapters */
    static const uint32_t ids[4] = { ID_INFO, ID_TRACKS, ID_CUES, ID_CHAPTERS };
    uint64_t duration = 0;

    rd_fd = fd;
    if (!element(0, m->file_size, &e) || e.id != ID_EBML)
        return DEMUX_ERR_FORMAT;
    FOR_CHILDREN(&e, x)
        if (x.id == ID_DOCTYPE)
            read_string(&x, doctype, sizeof(doctype));
    m->format = !rb->strcmp(doctype, "webm") ? "WebM" : "Matroska";
    if (!element(e.end, m->file_size, &seg) || seg.id != ID_SEGMENT)
        return DEMUX_ERR_FORMAT;
    mk->segment = seg.data;
    mk->segment_end = seg.end;
    mk->timescale = 1000000;
    mk->clusters = 0;

    /* the top level up to the first cluster; what comes after that, the
       seek head tells */
    for (uint32_t pos = seg.data; !mk->clusters && element(pos, seg.end, &e);
         pos = e.end)
    {
        for (int i = 0; i < 4; i++)
            if (e.id == ids[i])
                at[i] = pos;
        if (e.id == ID_CLUSTER)
            mk->clusters = pos;
        else if (e.id == ID_SEEKHEAD)
        {
            FOR_CHILDREN(&e, x)
            {
                if (x.id != ID_SEEK)
                    continue;
                uint32_t id = 0, where = 0;
                FOR_CHILDREN(&x, y)
                {
                    if (y.id == ID_SEEKID)
                        id = read_uint(&y);
                    else if (y.id == ID_SEEKPOSITION)
                        where = read_uint(&y);
                }
                for (int i = 0; i < 4; i++)
                    if (id == ids[i] && !at[i])
                        at[i] = seg.data + where;
            }
        }
        if (e.end == seg.end)
            break;
    }
    if (!mk->clusters)
        return DEMUX_ERR_FORMAT;

    m->video = -1;
    for (int i = 0; i < 2; i++)
    {
        if (!at[i] || !element(at[i], seg.end, &e) || e.id != ids[i])
            continue;
        FOR_CHILDREN(&e, x)
        {
            if (x.id == ID_TIMESTAMPSCALE)
                mk->timescale = MAX(read_uint(&x), 1);
            else if (x.id == ID_DURATION)
                duration = read_float(&x);
            else if (x.id == ID_TITLE)
                read_string(&x, m->title, sizeof(m->title));
            else if (x.id == ID_TRACKENTRY && m->ntracks < MAX_TRACKS)
                parse_track(m, &x);
        }
    }
    m->duration = duration * mk->timescale / 1000000;
    if (m->video < 0)
        return DEMUX_ERR_NO_VIDEO;
    if (!m->tracks[m->video].supported)
        return DEMUX_ERR_VCODEC;
    if (at[3])
        parse_chapters(m, at[3]);
    if (at[2])
        parse_cues(m, at[2]);
    m->pts = true;
    return DEMUX_OK;
}

static void mkv_stream_init(struct movie *m, struct stream *s)
{
    struct mkv_cursor *c = &s->c.mkv;

    c->number = m->tracks[s->track].id;
    c->pos = c->cur = m->c.mkv.clusters;
    c->cluster_end = 0;
    c->cluster_time = 0;
}

/* ---- packets ---- */

/* Past a laced block's frame sizes: the frames go on as one packet */
static int lace_header(const uint8_t *h, int avail, int lacing)
{
    int i = 1, frames = h[0] + 1;
    uint64_t v;

    if (lacing == 1)        /* Xiph: sizes in 255s */
    {
        for (int f = 0; f < frames - 1; f++)
        {
            while (i < avail && h[i] == 255)
                i++;
            i++;
        }
    }
    else if (lacing == 3)   /* EBML: a size, then differences */
    {
        for (int f = 0; f < frames - 1 && i < avail; f++)
        {
            int n = vint(h + i, avail - i, &v);
            if (!n)
                return -1;
            i += n;
        }
    }
    return i <= avail ? i : -1;
}

static bool mkv_next(struct movie *m, struct stream *s, struct packet *p)
{
    struct mkv_info *mk = &m->c.mkv;
    struct mkv_cursor *c = &s->c.mkv;
    struct element e, x;

    rd_fd = s->fd;
    s->left = s->prefix_left = 0;
    for (;;)
    {
        if (s->limit && c->pos >= s->limit)
            return false;
        if (c->pos >= c->cluster_end)
        {
            /* between clusters */
            if (!element(c->pos, mk->segment_end, &e))
                return false;
            if (e.id == ID_CLUSTER)
            {
                c->cluster_end = e.end;
                c->cluster_time = 0;
                c->pos = e.data;
            }
            else
                c->pos = e.end;
            continue;
        }
        if (!element(c->pos, c->cluster_end, &e))
        {
            c->pos = c->cluster_end;
            continue;
        }
        if (e.id == ID_CLUSTER)
        {
            /* the one before was of unknown size */
            c->cluster_end = c->pos;
            continue;
        }
        c->cur = c->pos;
        c->pos = e.end;

        struct element block = e;
        uint64_t duration = 0;
        bool key = true;
        if (e.id == ID_TIMESTAMP)
        {
            c->cluster_time = read_uint(&e);
            continue;
        }
        if (e.id == ID_BLOCKGROUP)
        {
            block.id = 0;
            FOR_CHILDREN(&e, x)
            {
                if (x.id == ID_BLOCK)
                    block = x;
                else if (x.id == ID_BLOCKDURATION)
                    duration = read_uint(&x);
                else if (x.id == ID_REFERENCEBLOCK)
                    key = false;
            }
            if (!block.id)
                continue;
        }
        else if (e.id != ID_SIMPLEBLOCK)
            continue;

        uint32_t avail = MIN(block.end - block.data, 64u);
        const uint8_t *h = avail >= 4 ? peek(block.data, avail) : NULL;
        uint64_t number;
        int n = h ? vint(h, avail, &number) : 0;
        if (!n || number != c->number || (uint32_t)n + 3 > avail)
            continue;
        int16_t tc = (h[n] << 8) | h[n + 1];
        uint8_t flags = h[n + 2];
        uint32_t data = block.data + n + 3;
        int lacing = (flags >> 1) & 3;
        if (lacing)
        {
            int len = lace_header(h + n + 3, avail - n - 3, lacing);
            if (len < 0 || data + len > block.end)
                continue;
            data += len;
        }
        if (e.id == ID_SIMPLEBLOCK)
            key = flags & 0x80;
        int64_t t = c->cluster_time + tc;
        p->ms = t > 0 ? (uint64_t)t * mk->timescale / 1000000 : 0;
        p->dur_ms = duration * mk->timescale / 1000000;
        p->key = key;
        s->prefix = mk->prefix[s->track];
        s->prefix_left = mk->prefix_len[s->track];
        p->size = block.end - data + s->prefix_left;
        s->left = p->size;
        rb->lseek(s->fd, data, SEEK_SET);
        return true;
    }
}

/* ---- seeking ---- */

static void cursor_at(struct stream *s, uint32_t cluster)
{
    s->c.mkv.pos = s->c.mkv.cur = cluster;
    s->c.mkv.cluster_end = 0;
    s->left = s->prefix_left = 0;
}

/* The cluster at pos (or the next one after it): its time */
static bool cluster_at(struct movie *m, uint32_t *pos, uint32_t *end,
                       int64_t *time)
{
    struct element e, x;

    while (element(*pos, m->c.mkv.segment_end, &e))
    {
        if (e.id != ID_CLUSTER)
        {
            *pos = e.end;
            continue;
        }
        *end = e.end;
        *time = 0;
        FOR_CHILDREN(&e, x)
        {
            if (x.id == ID_TIMESTAMP)
                *time = read_uint(&x);
            if (x.id == ID_TIMESTAMP || x.id == ID_SIMPLEBLOCK ||
                x.id == ID_BLOCKGROUP)
                break;
        }
        return true;
    }
    return false;
}

/* s on the last packet of its track at or before ms (keyframes only if
 * key) from the cluster at pos on; false if there is none before ms */
static bool walk(struct movie *m, struct stream *s, uint32_t pos, uint32_t ms,
                 bool key)
{
    struct stream w = *s;
    struct mkv_cursor found;
    struct packet p;
    bool any = false;

    w.limit = 0;
    cursor_at(&w, pos);
    while (mkv_next(m, &w, &p) && p.ms <= ms)
    {
        if (key && !p.key)
            continue;
        found = w.c.mkv;
        any = true;
    }
    if (!any)
        return false;
    s->c.mkv = found;
    s->c.mkv.pos = found.cur;
    s->left = s->prefix_left = 0;
    return true;
}

static bool mkv_seek(struct movie *m, struct stream *v, struct stream *a,
                     struct stream *t, uint32_t ms, uint32_t *at)
{
    struct mkv_info *mk = &m->c.mkv;
    uint64_t target = (uint64_t)ms * 1000000 / mk->timescale;
    uint32_t start = mk->clusters, ring[4];
    int n = 0;

    rd_fd = (v ? v : a ? a : t)->fd;
    for (int i = 0; i < mk->ncues && mk->cues[i].ms <= ms; i++)
        start = mk->cues[i].cluster;
    /* the clusters from there that start by ms: the last four kept */
    for (uint32_t pos = start, end; ; pos = end)
    {
        int64_t time;
        if (!cluster_at(m, &pos, &end, &time) || (time > (int64_t)target && n))
            break;
        ring[n++ % 4] = pos;
    }
    if (!n)
        return false;

    uint32_t from = ring[(n - 1) % 4];
    *at = ms;
    if (v)
    {
        /* the keyframe: in the last of those clusters, else one before */
        int back;
        for (back = 0; back < MIN(n, 4); back++)
        {
            from = ring[(n - 1 - back) % 4];
            if (walk(m, v, from, ms, true))
                break;
        }
        if (back == MIN(n, 4))
            cursor_at(v, from);     /* none before: the first one after */
        else
        {
            struct stream w = *v;
            struct packet p;
            if (mkv_next(m, &w, &p))
                *at = p.ms;
        }
    }
    if (a && !walk(m, a, from, ms, false))
        cursor_at(a, from);
    if (t)
        cursor_at(t, from);
    return true;
}

static uint32_t mkv_tell(const struct stream *s)
{
    return s->c.mkv.pos;
}

const struct demux_ops mkv_ops = {
    .open = mkv_open,
    .stream_init = mkv_stream_init,
    .next = mkv_next,
    .seek = mkv_seek,
    .tell = mkv_tell,
};
