/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Video player containers: what a file holds and how its packets are read.
 * Each track that plays reads with its own stream (own file handle), in
 * file order: no demuxing buffer, the decoders read straight from the file.
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
#ifndef DEMUX_H
#define DEMUX_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

enum vcodec {
    VC_NONE = 0,
    VC_MJPEG,
    VC_MPEG,        /* MPEG-1 or MPEG-2 video */
    VC_RGB565,      /* BI_BITFIELDS 5-6-5 */
    VC_RGB555,      /* BI_RGB 16 bit */
    VC_BGR24,       /* BI_RGB 24 bit */
};

enum acodec {
    AC_NONE = 0,
    AC_PCM,         /* 8 bit unsigned, 16 bit signed */
    AC_IMA_WAV,     /* Microsoft IMA ADPCM */
    AC_IMA_QT,      /* QuickTime IMA ADPCM ('ima4') */
    AC_MPA,         /* MPEG audio layer I or II */
};

enum scodec {
    SC_NONE = 0,
    SC_TEXT,        /* UTF-8 lines, maybe with SRT style tags */
    SC_ASS,         /* SSA/ASS dialogue fields */
    SC_TX3G,        /* 3GPP timed text: a 16-bit length, then UTF-8 */
};

enum track_type { TRACK_VIDEO, TRACK_AUDIO, TRACK_TEXT };

struct vformat {
    enum vcodec codec;
    int width, height;
    bool bottom_up;             /* raw DIB rows */
};

struct aformat {
    enum acodec codec;
    int channels;
    int bits;                   /* PCM */
    bool big_endian;            /* 16-bit PCM */
    uint32_t rate;
    uint32_t block_align;       /* bytes per block (PCM: per sample frame) */
    uint32_t samples_per_block; /* sample frames per block (PCM: 1) */
};

#define MAX_TRACKS      8
#define CODEC_NAME_LEN  12
#define TRACK_NAME_LEN  24

struct track {
    enum track_type type;
    bool supported;
    bool is_default;
    int id;                     /* the container's number for it */
    char codec[CODEC_NAME_LEN]; /* as the file names it, for messages */
    char lang[4];
    char name[TRACK_NAME_LEN];
    union {
        struct vformat v;
        struct aformat a;
        enum scodec s;
    } u;
};

struct packet {
    uint32_t ms;                /* presentation time */
    uint32_t dur_ms;            /* 0 if the container does not say */
    uint32_t size;
    bool key;
};

/* ---- container state ---- */

struct avi_info {
    uint32_t riff_end;          /* end of the first RIFF ('AVI ') */
    uint32_t movi_start;        /* first chunk of the first 'movi' list */
    uint32_t movi_end;
    uint32_t idx1_pos;          /* first idx1 entry, 0 without an index */
    uint32_t idx1_count;
    uint32_t idx1_base;         /* added to an entry's offset */
    uint32_t frame_scale, frame_rate;   /* video: frame = scale / rate s */
    uint32_t chunk_pad;         /* movi chunks' padding mask (avi.c PAD_*) */
    struct avi_audio_header {
        uint32_t scale, rate, sample_size;
        uint32_t avg_bytes;     /* WAVEFORMATEX nAvgBytesPerSec */
    } audio[MAX_TRACKS];        /* by track index */
};

struct avi_cursor {
    uint16_t id;                /* stream number as two ASCII digits */
    uint32_t pos;               /* next chunk header */
    uint32_t cur;               /* header of the current chunk */
    uint32_t end;               /* end of the current 'movi' list */
    uint32_t riff_end;          /* end of the current RIFF */
    uint32_t count;             /* chunks returned so far */
    uint32_t bytes;             /* stream bytes before the current chunk */
    uint32_t size;              /* size of the current chunk */
};

#define MKV_PREFIX_MAX  16

struct mkv_cue {
    uint32_t ms;
    uint32_t cluster;           /* its file offset */
};

struct mkv_info {
    uint32_t segment;           /* the Segment's data */
    uint32_t segment_end;
    uint32_t clusters;          /* the first Cluster */
    uint32_t timescale;         /* ns per timestamp unit */
    struct mkv_cue *cues;       /* video keyframes, every cue_step-th */
    int ncues, cue_step;
    /* header stripping: what each track's frames start with */
    uint8_t prefix_len[MAX_TRACKS];
    uint8_t prefix[MAX_TRACKS][MKV_PREFIX_MAX];
};

struct mkv_cursor {
    uint16_t number;            /* the track's number in the file */
    uint32_t pos;               /* next element */
    uint32_t cur;               /* the current packet's element */
    uint32_t cluster_end;       /* pos at or past it: between clusters */
    int64_t cluster_time;       /* in timestamp units */
};

struct mp4_table {              /* one sample table, read in a window */
    uint32_t pos;               /* first entry in the file */
    uint32_t count;
    uint8_t size;               /* bytes per entry */
    uint16_t cap;               /* entries the window holds */
    uint32_t first;             /* entries in the window: first.. */
    uint32_t n;
    uint8_t *buf;
};

enum { MP4_STTS, MP4_CTTS, MP4_STSS, MP4_STSC, MP4_STSZ, MP4_STCO, MP4_TABLES };

struct mp4_track {
    uint32_t timescale;
    uint32_t samples;
    uint32_t const_size;        /* stsz sample size, 0 if per sample */
    /* QuickTime sound: read a chunk at a time, chunk_bytes for every
       chunk_samples samples; 0 if sample by sample */
    uint32_t chunk_bytes, chunk_samples;
    uint32_t start;             /* edit list: the media time shown first */
    uint32_t delay_ms;          /* and when */
    bool co64;
    struct mp4_table t[MP4_TABLES];
};

struct mp4_info {
    struct mp4_track tracks[MAX_TRACKS];
};

struct mp4_cursor {
    uint32_t sample;            /* next sample */
    uint32_t chunk;             /* its chunk (0 based) */
    uint32_t chunk_left;        /* samples left in that chunk, it too */
    uint32_t offset;            /* its file offset */
    uint32_t stsc;              /* stsc entry of the chunk */
    uint32_t stts, stts_left;   /* stts entry of the sample, samples left */
    uint64_t dts;               /* its decoding time */
    uint32_t ctts, ctts_left;
    uint32_t stss;              /* next sync sample entry */
};

struct stream {
    int fd;
    int track;                  /* index in movie.tracks, -1 if none */
    uint32_t left;              /* unread bytes of the current packet */
    uint32_t limit;             /* next() stops at this file offset (if
                                   the container would search), 0: none */
    const uint8_t *prefix;      /* bytes the packet starts with that the
                                   file leaves out (Matroska's header
                                   stripping), counted in left */
    uint8_t prefix_left;
    union {
        struct avi_cursor avi;
        struct mkv_cursor mkv;
        struct mp4_cursor mp4;
    } c;
};

#define VEXTRA_MAX  160
#define MAX_CHAPTERS 32

struct movie {
    const struct demux_ops *ops;
    const char *format;         /* the container's name */
    uint32_t file_size;
    uint32_t duration;          /* ms, 0 if unknown */
    uint32_t frame_us;          /* of the video, 0 if unknown */
    bool pts;                   /* packets' times are when they show (not
                                   when they decode) */
    char title[48];             /* from the file's tags, "" if none */
    int ntracks;
    struct track tracks[MAX_TRACKS];
    int video;                  /* index of the video track */
    uint8_t vextra[VEXTRA_MAX]; /* its codec setup (MPEG sequence header) */
    int vextra_len;
    int nchapters;
    uint32_t chapters[MAX_CHAPTERS];    /* their start, ms */
    union {
        struct avi_info avi;
        struct mkv_info mkv;
        struct mp4_info mp4;
    } c;
};

/* Errors of open() (demux_strerror() names them) */
enum {
    DEMUX_OK = 0,
    DEMUX_ERR_READ = -1,
    DEMUX_ERR_FORMAT = -2,      /* not this container, or damaged */
    DEMUX_ERR_NO_VIDEO = -3,
    DEMUX_ERR_VCODEC = -4,      /* tracks[video].codec names it */
    DEMUX_ERR_MEMORY = -5,
};

struct demux_ops {
    int (*open)(struct movie *m, int fd);
    void (*stream_init)(struct movie *m, struct stream *s);
    /* To the next packet: its data is then read with demux_read() */
    bool (*next)(struct movie *m, struct stream *s, struct packet *p);
    /* Put v on the last keyframe at or before ms and the other streams
     * (NULL if unused) on their packets around it; *at = its time */
    bool (*seek)(struct movie *m, struct stream *v, struct stream *a,
                 struct stream *t, uint32_t ms, uint32_t *at);
    /* Where s reads in the file */
    uint32_t (*tell)(const struct stream *s);
};

extern const struct demux_ops avi_ops, mkv_ops, mp4_ops;

/* Find the container and open it */
int demux_open(struct movie *m, int fd);
const char *demux_strerror(int err);
/* A stream on track (-1: none) reading with fd */
void demux_stream_init(struct movie *m, struct stream *s, int fd, int track);
int32_t demux_read(struct stream *s, void *buf, uint32_t len);
/* Skip the rest of the current packet */
void demux_skip(struct stream *s);

/* Helpers for the containers */
bool demux_read_at(int fd, uint32_t pos, void *buf, uint32_t len);
/* n (up to 512) bytes at pos, NULL past the end; moves fd's position */
const uint8_t *demux_peek(int fd, uint32_t pos, uint32_t n);
static inline uint32_t demux_rd32le(const uint8_t *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline uint16_t demux_rd16le(const uint8_t *p)
{
    return p[0] | (p[1] << 8);
}
static inline uint32_t demux_rd32be(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}
static inline uint16_t demux_rd16be(const uint8_t *p)
{
    return (p[0] << 8) | p[1];
}
/* A WAVEFORMATEX (as AVI and Matroska's A_MS/ACM hold it) */
bool demux_parse_wave(struct track *t, const uint8_t *wf, uint32_t size);
/* A BITMAPINFOHEADER (AVI, Matroska's V_MS/VFW/FOURCC) */
bool demux_parse_bitmap(struct track *t, const uint8_t *bi, uint32_t size,
                        uint32_t handler);
/* The track's language and name, for menus */
void demux_track_label(const struct track *t, char *buf, size_t size);

#endif /* DEMUX_H */
