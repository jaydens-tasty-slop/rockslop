/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Video player containers: the parts they share.
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
#include "stats.h"

#define FCC(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | \
                         ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#define WAVE_FORMAT_PCM        0x0001
#define WAVE_FORMAT_IMA_ADPCM  0x0011
#define WAVE_FORMAT_MPEG       0x0050
#define WAVE_FORMAT_MPEGLAYER3 0x0055
#define WAVE_FORMAT_EXTENSIBLE 0xfffe
#define BI_RGB       0
#define BI_BITFIELDS 3

/* rb->read, timed for the statistics */
static ssize_t file_read(int fd, void *buf, size_t len)
{
    uint32_t t0 = stats_us();
    ssize_t n = rb->read(fd, buf, len);
    stats_time(ST_READ, t0);
    return n;
}

bool demux_read_at(int fd, uint32_t pos, void *buf, uint32_t len)
{
    return rb->lseek(fd, pos, SEEK_SET) == (off_t)pos &&
           file_read(fd, buf, len) == (ssize_t)len;
}

/* A sector-aligned window on the file: every handle reads the same file */
static struct {
    uint32_t pos, len;
    uint8_t buf[1024];
} win;

const uint8_t *demux_peek(int fd, uint32_t pos, uint32_t n)
{
    if (pos < win.pos || pos + n > win.pos + win.len)
    {
        uint32_t base = pos & ~511u;
        ssize_t got = -1;
        if (rb->lseek(fd, base, SEEK_SET) == (off_t)base)
            got = file_read(fd, win.buf, sizeof(win.buf));
        win.pos = base;
        win.len = got > 0 ? got : 0;
        if (pos + n > win.pos + win.len)
            return NULL;
    }
    return win.buf + (pos - win.pos);
}

int demux_open(struct movie *m, int fd)
{
    uint8_t h[12];

    rb->memset(m, 0, sizeof(*m));
    m->video = -1;
    win.len = 0;
    m->file_size = rb->ffilesize(fd);
    if (!demux_read_at(fd, 0, h, sizeof(h)))
        return DEMUX_ERR_READ;
    if (!rb->memcmp(h, "RIFF", 4) && !rb->memcmp(h + 8, "AVI ", 4))
        m->ops = &avi_ops;
    else if (demux_rd32be(h) == 0x1a45dfa3)
        m->ops = &mkv_ops;
    else if (!rb->memcmp(h + 4, "ftyp", 4) || !rb->memcmp(h + 4, "moov", 4) ||
             !rb->memcmp(h + 4, "mdat", 4) || !rb->memcmp(h + 4, "wide", 4) ||
             !rb->memcmp(h + 4, "free", 4) || !rb->memcmp(h + 4, "skip", 4))
        m->ops = &mp4_ops;
    else
        return DEMUX_ERR_FORMAT;
    return m->ops->open(m, fd);
}

const char *demux_strerror(int err)
{
    switch (err)
    {
    case DEMUX_ERR_READ:     return "Read error";
    case DEMUX_ERR_FORMAT:   return "Not a video file this player knows";
    case DEMUX_ERR_NO_VIDEO: return "No video in this file";
    case DEMUX_ERR_VCODEC:   return "Video format not supported";
    case DEMUX_ERR_MEMORY:   return "Not enough memory";
    default:                 return "Error";
    }
}

void demux_stream_init(struct movie *m, struct stream *s, int fd, int track)
{
    rb->memset(s, 0, sizeof(*s));
    s->fd = fd;
    s->track = track;
    if (track >= 0)
        m->ops->stream_init(m, s);
}

int32_t demux_read(struct stream *s, void *buf, uint32_t len)
{
    uint8_t *b = buf;
    uint32_t pre;

    len = MIN(len, s->left);
    pre = MIN(len, s->prefix_left);
    rb->memcpy(b, s->prefix, pre);
    s->prefix += pre;
    s->prefix_left -= pre;
    s->left -= pre;
    if (len == pre)
        return pre;
    ssize_t n = file_read(s->fd, b + pre, len - pre);
    if (n > 0)
    {
        s->left -= n;
        return pre + n;
    }
    s->left = 0;    /* a read error drops the rest of the packet */
    return pre ? (int32_t)pre : n;
}

void demux_skip(struct stream *s)
{
    s->left = 0;
    s->prefix_left = 0;
}

/* ---- codec descriptions shared by AVI and Matroska ---- */

static void fourcc_name(char *buf, uint32_t fcc)
{
    for (int i = 0; i < 4; i++)
    {
        char c = fcc >> (8 * i);
        buf[i] = (c >= ' ' && c <= '~') ? c : '?';
    }
    buf[4] = '\0';
}

static bool fcc_is(uint32_t fcc, const uint32_t *names, unsigned n)
{
    fcc &= 0xdfdfdfdf;  /* upper case */
    for (unsigned i = 0; i < n; i++)
        if (fcc == (names[i] & 0xdfdfdfdf))
            return true;
    return false;
}

static bool is_mjpeg(uint32_t fcc)
{
    static const uint32_t names[] = {
        FCC('M','J','P','G'), FCC('A','V','R','N'), FCC('D','M','B','1'),
        FCC('I','J','P','G'), FCC('J','P','G','L'), FCC('A','V','D','J'),
    };
    return fcc_is(fcc, names, ARRAYLEN(names));
}

static bool is_mpeg(uint32_t fcc)
{
    static const uint32_t names[] = {
        FCC('M','P','G','1'), FCC('M','P','G','2'), FCC('P','I','M','1'),
        FCC('M','P','E','G'), FCC('M','P','2','V'), FCC('M','P','1','V'),
        0x10000001, 0x10000002,     /* DirectShow MPEG-1/2 video */
    };
    return fcc_is(fcc, names, ARRAYLEN(names));
}

bool demux_parse_bitmap(struct track *t, const uint8_t *bi, uint32_t size,
                        uint32_t handler)
{
    if (size < 20)
        return false;
    int32_t width = demux_rd32le(bi + 4), height = demux_rd32le(bi + 8);
    int bits = demux_rd16le(bi + 14);
    uint32_t comp = demux_rd32le(bi + 16);
    enum vcodec codec = VC_NONE;

    t->type = TRACK_VIDEO;
    if (is_mjpeg(comp) || is_mjpeg(handler))
        codec = VC_MJPEG;
    else if (is_mpeg(comp) || is_mpeg(handler))
        codec = VC_MPEG;
    else if (comp == BI_RGB && bits == 16)
        codec = VC_RGB555;
    else if (comp == BI_RGB && bits == 24)
        codec = VC_BGR24;
    else if (comp == BI_BITFIELDS && bits == 16)
        codec = (size >= 44 && demux_rd32le(bi + 40) == 0x7c00) ?
                VC_RGB555 : VC_RGB565;

    if (comp == BI_RGB || comp == BI_BITFIELDS)
        rb->snprintf(t->codec, sizeof(t->codec), "RGB%d", bits);
    else
        fourcc_name(t->codec, comp > 0xff ? comp : handler);
    if (codec == VC_NONE || width <= 0 || width > 4096 ||
        height == 0 || height < -4096 || height > 4096)
        return false;

    t->u.v.codec = codec;
    t->u.v.width = width;
    /* DIBs are stored bottom-up unless the height is negative */
    t->u.v.bottom_up = codec != VC_MJPEG && codec != VC_MPEG && height > 0;
    t->u.v.height = height < 0 ? -height : height;
    t->supported = true;
    return true;
}

bool demux_parse_wave(struct track *t, const uint8_t *wf, uint32_t size)
{
    if (size < 16)
        return false;
    uint16_t tag = demux_rd16le(wf);
    int channels = demux_rd16le(wf + 2);
    uint32_t rate = demux_rd32le(wf + 4);
    uint32_t align = demux_rd16le(wf + 12);
    int bits = demux_rd16le(wf + 14);
    struct aformat *a = &t->u.a;

    t->type = TRACK_AUDIO;
    if (tag == WAVE_FORMAT_EXTENSIBLE && size >= 26)
        tag = demux_rd16le(wf + 24);  /* first bytes of the SubFormat GUID */
    switch (tag)
    {
    case WAVE_FORMAT_PCM:        rb->strcpy(t->codec, "PCM"); break;
    case WAVE_FORMAT_IMA_ADPCM:  rb->strcpy(t->codec, "IMA ADPCM"); break;
    case WAVE_FORMAT_MPEG:       rb->strcpy(t->codec, "MP2"); break;
    case WAVE_FORMAT_MPEGLAYER3: rb->strcpy(t->codec, "MP3"); break;
    case 0x00ff: case 0x1610:    rb->strcpy(t->codec, "AAC"); break;
    case 0x2000:                 rb->strcpy(t->codec, "AC-3"); break;
    case 0x0160: case 0x0161:    rb->strcpy(t->codec, "WMA"); break;
    default:
        rb->snprintf(t->codec, sizeof(t->codec), "0x%04x", tag);
        break;
    }
    if (channels < 1 || channels > 2 || rate < 1000 || rate > 192000)
        return false;

    if (tag == WAVE_FORMAT_PCM && (bits == 8 || bits == 16))
    {
        a->codec = AC_PCM;
        a->block_align = channels * bits / 8;
        a->samples_per_block = 1;
    }
    else if (tag == WAVE_FORMAT_IMA_ADPCM && bits == 4 &&
             align > 4u * channels && (align - 4 * channels) % (4 * channels) == 0)
    {
        a->codec = AC_IMA_WAV;
        a->block_align = align;
        /* the header sample plus two per data byte */
        a->samples_per_block = (align - 4 * channels) * 2 / channels + 1;
    }
    else if (tag == WAVE_FORMAT_MPEG)
    {
        a->codec = AC_MPA;      /* the frame headers say the rest */
        a->block_align = 1;
        a->samples_per_block = 1;
    }
    else
        return false;

    a->channels = channels;
    a->bits = bits;
    a->rate = rate;
    t->supported = true;
    return true;
}

void demux_track_label(const struct track *t, char *buf, size_t size)
{
    const char *sep = t->lang[0] && t->name[0] ? ": " : "";
    if (!t->lang[0] && !t->name[0])
        rb->snprintf(buf, size, "Track %d (%s)", t->id, t->codec);
    else
        rb->snprintf(buf, size, "%s%s%s", t->lang, sep, t->name);
}
