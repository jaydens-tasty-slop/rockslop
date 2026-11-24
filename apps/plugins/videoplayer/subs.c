/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Video player captions. Text files are read a cue at a time and sought
 * by bisection, so they take no memory however long they are; a file's
 * own text track is read along with the video.
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
#include "subs.h"

#define MAX_FILES       4
#define FILE_NAME_LEN   80
#define CUE_TEXT        320
#define LINE_LEN        200
#define CUE_MS          3000    /* a track's cue that does not say */
#define READ_AHEAD      (64 * 1024)     /* past the video, in the file */

struct cue {
    bool valid;
    uint32_t start, end;
    char text[CUE_TEXT];
};

static struct movie *mov;
static const struct stream *video;
static int track_fd;
static int ntracks;
static int8_t tracks[MAX_TRACKS];       /* choice -> movie track */
static char dir[MAX_PATH];
static int nfiles;
static char files[MAX_FILES][FILE_NAME_LEN];
static int base_len;                    /* of the movie's name, w/o suffix */
static int sel = -1;
static int file_fd = -1;
static struct stream ts;
static struct cue cues[2];
static struct cue *cur = &cues[0], *nxt = &cues[1];

/* ---- cue text ---- */

static bool valid_utf8(const unsigned char *s)
{
    while (*s)
    {
        int n = *s < 0x80 ? 0 : (*s & 0xe0) == 0xc0 ? 1 :
                (*s & 0xf0) == 0xe0 ? 2 : (*s & 0xf8) == 0xf0 ? 3 : -1;
        if (n < 0)
            return false;
        s++;
        while (n--)
            if ((*s++ & 0xc0) != 0x80)
                return false;
    }
    return true;
}

static const struct { const char *name; char c; } entities[] = {
    { "amp;", '&' }, { "lt;", '<' }, { "gt;", '>' }, { "quot;", '"' },
    { "apos;", '\'' }, { "#39;", '\'' }, { "nbsp;", ' ' },
};

/* Add a line of text to the cue: markup dropped (SRT/VTT tags, ASS
 * overrides), entities and ASS escapes decoded, and Windows-1252 made
 * UTF-8 if it is not UTF-8 already */
static void cue_add(struct cue *c, const char *line)
{
    bool latin = !valid_utf8((const unsigned char *)line);
    int len = rb->strlen(c->text);
    const unsigned char *s = (const unsigned char *)line;

    if (len && len < CUE_TEXT - 1)
        c->text[len++] = '\n';
    while (*s && len < CUE_TEXT - 4)
    {
        if (*s == '<' || *s == '{')
        {
            const unsigned char *e = (const unsigned char *)
                rb->strchr((const char *)s, *s == '<' ? '>' : '}');
            if (e)
            {
                s = e + 1;
                continue;
            }
        }
        else if (*s == '&')
        {
            unsigned i;
            for (i = 0; i < ARRAYLEN(entities); i++)
            {
                int n = rb->strlen(entities[i].name);
                if (!rb->strncmp((const char *)s + 1, entities[i].name, n))
                {
                    c->text[len++] = entities[i].c;
                    s += n + 1;
                    break;
                }
            }
            if (i < ARRAYLEN(entities))
                continue;
        }
        else if (*s == '\\' && (s[1] == 'N' || s[1] == 'n' || s[1] == 'h'))
        {
            c->text[len++] = s[1] == 'h' ? ' ' : '\n';
            s += 2;
            continue;
        }
        else if (latin && *s >= 0x80)
        {
            static const char cp1252[] = "\x91'\x92'\x93\"\x94\"\x96-\x97-";
            const char *m = rb->strchr(cp1252, *s);
            if (m)
                c->text[len++] = m[1];
            else if (*s >= 0xa0)
                len = (char *)rb->utf8encode(*s, (unsigned char *)c->text + len)
                      - c->text;
            s++;
            continue;
        }
        c->text[len++] = *s++;
    }
    c->text[len] = '\0';
}

/* ---- text files ---- */

/* [hh:]mm:ss[,.]mmm */
static bool parse_time(const char **sp, uint32_t *ms)
{
    const char *s = *sp;
    uint32_t part[3];
    int n = 0, digits = 0;
    uint32_t frac = 0;

    while (*s == ' ' || *s == '\t')
        s++;
    for (;;)
    {
        if (*s < '0' || *s > '9')
            return false;
        uint32_t v = 0;
        while (*s >= '0' && *s <= '9')
            v = v * 10 + (*s++ - '0');
        part[n++] = v;
        if (*s != ':' || n == 3)
            break;
        s++;
    }
    if (n < 2)
        return false;
    if (*s == ',' || *s == '.')
    {
        for (s++; *s >= '0' && *s <= '9'; s++)
            if (digits < 3)
            {
                frac = frac * 10 + (*s - '0');
                digits++;
            }
    }
    for (; digits < 3; digits++)
        frac *= 10;
    uint32_t sec = n == 3 ? part[0] * 3600 + part[1] * 60 + part[2]
                          : part[0] * 60 + part[1];
    *ms = sec * 1000 + frac;
    *sp = s;
    return true;
}

/* A "start --> end" line */
static bool timing_line(const char *line, uint32_t *start, uint32_t *end)
{
    const char *arrow = rb->strstr(line, "-->");
    const char *s = line;
    if (!arrow || !parse_time(&s, start))
        return false;
    s = arrow + 3;
    return parse_time(&s, end);
}

static bool file_cue(struct cue *c)
{
    char line[LINE_LEN];

    c->valid = false;
    do
        if (rb->read_line(file_fd, line, sizeof(line)) <= 0)
            return false;
    while (!timing_line(line, &c->start, &c->end));
    c->text[0] = '\0';
    while (rb->read_line(file_fd, line, sizeof(line)) > 0)
    {
        const char *s = line;
        while (*s == ' ' || *s == '\t')
            s++;
        if (!*s)
            break;
        cue_add(c, line);
    }
    c->valid = true;
    return true;
}

/* To pos, then the start of the next line */
static void file_to(off_t pos)
{
    char line[LINE_LEN];
    rb->lseek(file_fd, pos, SEEK_SET);
    if (pos)
        rb->read_line(file_fd, line, sizeof(line));
}

/* The start of the first cue after pos */
static bool start_after(off_t pos, uint32_t *ms)
{
    char line[LINE_LEN];
    uint32_t end;

    file_to(pos);
    for (int i = 0; i < 32; i++)
    {
        if (rb->read_line(file_fd, line, sizeof(line)) <= 0)
            return false;
        if (timing_line(line, ms, &end))
            return true;
    }
    return false;
}

static void file_seek(uint32_t ms)
{
    off_t lo = 0, hi = rb->ffilesize(file_fd);

    /* lo: the first cue after it starts no later than ms */
    while (hi - lo > 2048)
    {
        off_t mid = lo + (hi - lo) / 2;
        uint32_t t;
        if (start_after(mid, &t) && t <= ms)
            lo = mid;
        else
            hi = mid;
    }
    file_to(lo);
    cur->valid = false;
    while (file_cue(nxt) && nxt->start <= ms)
    {
        struct cue *c = cur;
        cur = nxt;
        nxt = c;
    }
}

/* ---- the movie's own tracks ---- */

static bool track_cue(struct cue *c)
{
    char data[CUE_TEXT];
    struct packet p;

    c->valid = false;
    ts.limit = mov->ops->tell(video) + READ_AHEAD;
    while (mov->ops->next(mov, &ts, &p))
    {
        int32_t n = demux_read(&ts, data, MIN(p.size, sizeof(data) - 1));
        demux_skip(&ts);
        if (n <= 0)
            continue;
        data[n] = '\0';
        char *s = data;
        switch (mov->tracks[ts.track].u.s)
        {
        case SC_TX3G:   /* a 16-bit length first */
            if (n < 2)
                continue;
            s[MIN(n, 2 + demux_rd16be((uint8_t *)s))] = '\0';
            s += 2;
            break;
        case SC_ASS:    /* ReadOrder, Layer, Style, Name, MarginL, MarginR,
                           MarginV, Effect, Text */
            for (int i = 0; i < 8 && s; i++)
            {
                s = rb->strchr(s, ',');
                if (s)
                    s++;
            }
            if (!s)
                continue;
            break;
        default:
            break;
        }
        c->start = p.ms;
        c->end = p.ms + (p.dur_ms ? p.dur_ms : CUE_MS);
        c->text[0] = '\0';
        /* a line at a time, for the encoding check */
        for (char *line = s, *e; line; line = e)
        {
            e = rb->strchr(line, '\n');
            if (e)
                *e++ = '\0';
            if (*line && *line != '\r')
                cue_add(c, line);
        }
        c->valid = true;
        return true;
    }
    return false;
}

/* ---- choices ---- */

static bool is_text_file(const char *name)
{
    const char *dot = rb->strrchr(name, '.');
    return dot && (!rb->strcasecmp(dot, ".srt") || !rb->strcasecmp(dot, ".vtt"));
}

void subs_init(struct movie *m, const struct stream *v, const char *path, int fd)
{
    mov = m;
    video = v;
    track_fd = fd;
    sel = -1;
    ntracks = nfiles = 0;
    for (int i = 0; i < m->ntracks && ntracks < MAX_TRACKS; i++)
        if (m->tracks[i].type == TRACK_TEXT && m->tracks[i].supported)
            tracks[ntracks++] = i;

    /* name.srt, name.en.srt, name.vtt .. */
    const char *slash = rb->strrchr(path, '/');
    const char *name = slash ? slash + 1 : path;
    const char *dot = rb->strrchr(name, '.');
    base_len = dot ? dot - name : (int)rb->strlen(name);
    rb->strlcpy(dir, path, MIN((size_t)(name - path) + 1, sizeof(dir)));
    DIR *d = rb->opendir(*dir ? dir : "/");
    if (!d)
        return;
    struct dirent *e;
    while (nfiles < MAX_FILES && (e = rb->readdir(d)))
    {
        if (!rb->strncasecmp(e->d_name, name, base_len) &&
            e->d_name[base_len] == '.' && is_text_file(e->d_name) &&
            rb->strlen(e->d_name) < FILE_NAME_LEN)
            rb->strlcpy(files[nfiles++], e->d_name, FILE_NAME_LEN);
    }
    rb->closedir(d);
}

void subs_close(void)
{
    if (file_fd >= 0)
        rb->close(file_fd);
    file_fd = -1;
    sel = -1;
}

int subs_count(void)
{
    return ntracks + nfiles;
}

void subs_label(int i, char *buf, size_t size)
{
    if (i < ntracks)
    {
        demux_track_label(&mov->tracks[tracks[i]], buf, size);
        if (!*buf)
            rb->snprintf(buf, size, "Track %d", i + 1);
        return;
    }
    /* name.en.srt: "en"; name.srt: "SRT" */
    const char *f = files[i - ntracks];
    const char *ext = rb->strrchr(f, '.');
    int len = ext - (f + base_len + 1);
    if (len > 0)
        rb->snprintf(buf, size, "%.*s", len, f + base_len + 1);
    else
    {
        rb->snprintf(buf, size, "%s", ext + 1);
        for (char *c = buf; *c; c++)
            if (*c >= 'a' && *c <= 'z')
                *c -= 'a' - 'A';
    }
}

int subs_default(void)
{
    for (int i = 0; i < ntracks; i++)
        if (mov->tracks[tracks[i]].is_default)
            return i;
    /* a file next to the movie, else its first track */
    return nfiles ? ntracks : ntracks ? 0 : -1;
}

int subs_selected(void)
{
    return sel;
}

void subs_select(int i, uint32_t ms)
{
    char path[MAX_PATH];

    subs_close();
    if (i < 0 || i >= subs_count())
        return;
    if (i < ntracks)
        demux_stream_init(mov, &ts, track_fd, tracks[i]);
    else
    {
        rb->snprintf(path, sizeof(path), "%s%s", dir, files[i - ntracks]);
        file_fd = rb->open(path, O_RDONLY);
        if (file_fd < 0)
            return;
    }
    sel = i;
    if (i < ntracks)
    {
        uint32_t at;
        mov->ops->seek(mov, NULL, NULL, &ts, ms, &at);
    }
    subs_seek(ms);
}

struct stream *subs_stream(void)
{
    return sel >= 0 && sel < ntracks ? &ts : NULL;
}

void subs_seek(uint32_t ms)
{
    cur->valid = nxt->valid = false;
    if (sel < 0)
        return;
    if (sel >= ntracks)
        file_seek(ms);
    else
    {
        /* the cues from about here on */
        while (track_cue(nxt) && nxt->end <= ms)
            ;
    }
}

const char *subs_at(uint32_t ms)
{
    if (sel < 0)
        return NULL;
    if (!nxt->valid)
        sel < ntracks ? track_cue(nxt) : file_cue(nxt);
    while (nxt->valid && nxt->start <= ms)
    {
        struct cue *c = cur;
        cur = nxt;
        nxt = c;
        if (!(sel < ntracks ? track_cue(nxt) : file_cue(nxt)))
            nxt->valid = false;
    }
    if (cur->valid && ms >= cur->start && ms < cur->end && cur->text[0])
        return cur->text;
    return NULL;
}
