/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * RockTrack: FastTracker 2 XM load/save and ProTracker MOD import.
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

/* ---- buffered file access ---- */

struct io {
    int fd;
    int pos, len;
    bool err;
    uint8_t buf[512];
};

static struct io io;

static int io_u8(void)
{
    if (io.pos >= io.len) {
        io.len = rb->read(io.fd, io.buf, sizeof(io.buf));
        io.pos = 0;
        if (io.len <= 0) {
            io.len = 0;
            io.err = true;
            return 0;
        }
    }
    return io.buf[io.pos++];
}

static int io_u16(void)
{
    int a = io_u8();
    return a | io_u8() << 8;
}

static uint32_t io_u32(void)
{
    uint32_t a = io_u16();
    return a | (uint32_t)io_u16() << 16;
}

static void io_bytes(void *dst, int n)
{
    uint8_t *d = dst;
    while (n--)
        *d++ = io_u8();
}

static long io_tell(void)
{
    return rb->lseek(io.fd, 0, SEEK_CUR) - (io.len - io.pos);
}

static void io_seek(long off)
{
    rb->lseek(io.fd, off, SEEK_SET);
    io.pos = io.len = 0;
}

static void io_put(const void *src, int n)
{
    const uint8_t *s = src;
    while (n--) {
        if (io.pos == (int)sizeof(io.buf)) {
            if (rb->write(io.fd, io.buf, io.pos) != io.pos)
                io.err = true;
            io.pos = 0;
        }
        io.buf[io.pos++] = *s++;
    }
}

static void io_put8(int v)
{
    uint8_t b = v;
    io_put(&b, 1);
}

static void io_put16(int v)
{
    io_put8(v);
    io_put8(v >> 8);
}

static void io_put32(uint32_t v)
{
    io_put16(v);
    io_put16(v >> 16);
}

static void io_zero(int n)
{
    while (n--)
        io_put8(0);
}

static void io_flush(void)
{
    if (io.pos && rb->write(io.fd, io.buf, io.pos) != io.pos)
        io.err = true;
    io.pos = 0;
}

static void name_copy(char *dst, const char *src, int n)
{
    int end = 0;
    for (int i = 0; i < n; i++) {
        char c = src[i];
        if (!c)
            break;
        dst[i] = (c < 32 || c > 126) ? ' ' : c;
        if (c != ' ')
            end = i + 1;
    }
    dst[end] = 0;
}

static void put_name(const char *s, int n)
{
    int l = rb->strlen(s);
    for (int i = 0; i < n; i++)
        io_put8(i < l ? s[i] : (n == 20 && i >= l) ? ' ' : 0);
}

#define FAIL(msg) do { rb->strlcpy(err, msg, errlen); goto fail; } while (0)

/* ---- XM load ---- */

static bool load_samples(struct instr *in, int nsmp, char *err, size_t errlen)
{
    uint8_t flags[MAX_SMP];
    uint32_t bytes[MAX_SMP];
    for (int s = 0; s < nsmp; s++) {
        struct sample *sm = &in->smp[s];
        uint8_t raw[22];
        bytes[s] = io_u32();
        sm->lstart = io_u32();
        sm->llen = io_u32();
        int vol = io_u8();              /* MIN() would read twice */
        sm->vol = MIN(vol, 64);
        sm->fine = (int8_t)io_u8();
        flags[s] = io_u8();
        sm->pan = io_u8();
        sm->rel = (int8_t)io_u8();
        io_u8();
        io_bytes(raw, 22);
        name_copy(sm->name, (char *)raw, 22);
    }
    for (int s = 0; s < nsmp; s++) {
        struct sample *sm = &in->smp[s];
        bool b16 = flags[s] & 0x10;
        uint32_t len = bytes[s] >> b16;
        uint32_t ls = sm->lstart >> b16, ll = sm->llen >> b16;
        uint8_t vol = sm->vol, pan = sm->pan;
        int8_t fine = sm->fine, rel = sm->rel;
        if (!sample_set_data(sm, len, b16)) {
            rb->strlcpy(err, "Out of memory (samples)", errlen);
            return false;
        }
        sm->vol = vol;
        sm->pan = pan;
        sm->fine = fine;
        sm->rel = rel;
        if (b16) {
            int16_t *d = sm->data, v = 0;
            for (uint32_t i = 0; i < len; i++)
                d[i] = v += (int16_t)io_u16();
        } else {
            int8_t *d = sm->data, v = 0;
            for (uint32_t i = 0; i < len; i++)
                d[i] = v += (int8_t)io_u8();
        }
        int lt = flags[s] & 3;
        if (lt && ll && ls < len) {
            sm->loop = lt == 3 ? 1 : lt;
            sm->lstart = ls;
            sm->llen = MIN(ll, len - ls);
        } else {
            sm->loop = 0;
            sm->lstart = sm->llen = 0;
        }
    }
    return true;
}

static void read_env(struct env *e, const uint8_t *pts, int n, int sus,
                     int ls, int le, int flags)
{
    e->n = MIN(n, 12);
    for (int i = 0; i < e->n; i++) {
        e->x[i] = pts[i * 4] | pts[i * 4 + 1] << 8;
        e->y[i] = MIN(pts[i * 4 + 2], 64);
        if (i && e->x[i] < e->x[i - 1])
            e->x[i] = e->x[i - 1];
    }
    e->sus = sus;
    e->lstart = ls;
    e->lend = le;
    e->flags = e->n ? flags & 7 : 0;
}

int xm_load(const char *path, char *err, size_t errlen)
{
    uint8_t hdr[80];
    io.fd = rb->open(path, O_RDONLY);
    io.pos = io.len = 0;
    io.err = false;
    if (io.fd < 0) {
        rb->strlcpy(err, "Cannot open file", errlen);
        return -1;
    }
    io_bytes(hdr, 80);
    if (io.err || rb->memcmp(hdr, "Extended Module:", 16))
        FAIL("Not an XM file");
    song_clear();
    name_copy(song.name, (char *)hdr + 17, 20);
    uint32_t hsize = hdr[60] | hdr[61] << 8 | hdr[62] << 16 |
                     (uint32_t)hdr[63] << 24;
    song.nord = CLAMP(hdr[64] | hdr[65] << 8, 1, MAX_ORD);
    song.restart = hdr[66] | hdr[67] << 8;
    int nch = hdr[68] | hdr[69] << 8;
    int npat = hdr[70] | hdr[71] << 8;
    int nins = hdr[72] | hdr[73] << 8;
    song.linear = hdr[74] & 1;
    song.speed = CLAMP(hdr[76] | hdr[77] << 8, 1, 31);
    song.bpm = CLAMP(hdr[78] | hdr[79] << 8, 32, 255);
    if (nch < 1 || nch > MAX_CH || npat > MAX_PAT || nins > MAX_INS)
        FAIL("Unsupported XM layout");
    io_bytes(song.ord, 256);
    if (song.restart >= song.nord)
        song.restart = 0;
    if (!song_set_channels(nch))
        FAIL("Out of memory");

    long next = 60 + hsize;
    for (int p = 0; p < npat; p++) {
        io_seek(next);
        uint32_t plen = io_u32();
        io_u8();
        int rows = io_u16();
        int psize = io_u16();
        next += plen + psize;
        rows = CLAMP(rows, 1, MAX_ROWS);
        song.pat[p].rows = rows;
        if (!psize)
            continue;
        io_seek(next - psize);
        if (!pattern_alloc(p))
            FAIL("Out of memory (patterns)");
        struct cell *c = song.pat[p].c;
        int left = psize;
        for (int i = 0; i < rows * nch && left > 0; i++) {
            uint8_t f[5] = { 0 };
            int b = io_u8();
            left--;
            if (b & 0x80) {
                for (int k = 0; k < 5; k++)
                    if (b & (1 << k)) {
                        f[k] = io_u8();
                        left--;
                    }
            } else {
                f[0] = b;
                for (int k = 1; k < 5; k++)
                    f[k] = io_u8();
                left -= 4;
            }
            c[i].note = f[0] <= NOTE_OFF ? f[0] : 0;
            c[i].ins = f[1] <= MAX_INS ? f[1] : 0;
            c[i].vol = f[2];
            c[i].fx = f[3] < 36 ? f[3] : 0;
            c[i].param = f[3] < 36 ? f[4] : 0;
        }
        if (pattern_blank(p))
            pattern_free(p);
    }
    for (int p = 0; p < MAX_PAT; p++)
        if (p >= npat)
            song.pat[p].rows = 64;

    for (int i = 0; i < nins; i++) {
        io_seek(next);
        uint32_t isize = io_u32();
        uint8_t raw[22];
        io_bytes(raw, 22);
        io_u8();
        int nsmp = io_u16();
        if (io.err)
            FAIL("Truncated XM");
        if (nsmp > MAX_SMP)
            FAIL("Too many samples in an instrument");
        if (!instr_set_samples(i, nsmp))
            FAIL("Out of memory (instruments)");
        struct instr *in = song.ins[i];
        name_copy(in->name, (char *)raw, 22);
        if (!nsmp) {
            next += isize;
            continue;
        }
        uint32_t shsize = io_u32();
        uint8_t ext[214];
        io_bytes(ext, 214);
        for (int k = 0; k < 96; k++)
            in->keymap[k] = ext[k] < nsmp ? ext[k] : 0;
        read_env(&in->venv, ext + 96, ext[192], ext[194], ext[195], ext[196],
                 ext[200]);
        read_env(&in->penv, ext + 144, ext[193], ext[197], ext[198],
                 ext[199], ext[201]);
        in->vtype = ext[202];
        in->vsweep = ext[203];
        in->vdepth = ext[204];
        in->vrate = ext[205];
        in->fadeout = ext[206] | ext[207] << 8;
        io_seek(next + isize);
        (void)shsize;
        if (!load_samples(in, nsmp, err, errlen))
            goto fail;
        next = io_tell();
    }
    song.nins = nins;
    rb->close(io.fd);
    eng_reset();
    return 0;
fail:
    rb->close(io.fd);
    return -1;
}

/* ---- XM save ---- */

static int pack_cell(const struct cell *c, uint8_t *out)
{
    const uint8_t f[5] = { c->note, c->ins, c->vol, c->fx, c->param };
    int mask = 0, n = 0;
    for (int k = 0; k < 5; k++)
        if (f[k])
            mask |= 1 << k;
    if (mask == 31) {
        rb->memcpy(out, f, 5);
        return 5;
    }
    out[n++] = 0x80 | mask;
    for (int k = 0; k < 5; k++)
        if (mask & (1 << k))
            out[n++] = f[k];
    return n;
}

static void write_env(const struct env *e)
{
    for (int i = 0; i < 12; i++) {
        io_put16(i < e->n ? e->x[i] : 0);
        io_put16(i < e->n ? e->y[i] : 0);
    }
}

int xm_save(const char *path)
{
    char tmp[MAX_PATH];
    rb->snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    io.fd = rb->open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (io.fd < 0)
        return -1;
    io.pos = 0;
    io.err = false;

    int npat = song_used_patterns();
    int nins = 0;
    for (int i = 0; i < MAX_INS; i++)
        if (song.ins[i])
            nins = i + 1;

    io_put("Extended Module: ", 17);
    put_name(song.name, 20);
    io_put8(0x1a);
    put_name("RockTrack", 20);
    io_put16(0x0104);
    io_put32(276);
    io_put16(song.nord);
    io_put16(song.restart);
    io_put16(song.nch);
    io_put16(npat);
    io_put16(nins);
    io_put16(song.linear ? 1 : 0);
    io_put16(song.speed);
    io_put16(song.bpm);
    io_put(song.ord, 256);

    for (int p = 0; p < npat; p++) {
        const struct pattern *pt = &song.pat[p];
        int size = 0;
        uint8_t b[5];
        if (pt->c)
            for (int i = 0; i < pt->rows * song.nch; i++)
                size += pack_cell(&pt->c[i], b);
        if (size > 0xffff)
            size = 0;       /* cannot happen: 256 rows * 32 ch * 5 */
        io_put32(9);
        io_put8(0);
        io_put16(pt->rows);
        io_put16(pt->c ? size : 0);
        if (pt->c)
            for (int i = 0; i < pt->rows * song.nch; i++)
                io_put(b, pack_cell(&pt->c[i], b));
    }

    for (int i = 0; i < nins; i++) {
        const struct instr *in = song.ins[i];
        int nsmp = in ? in->nsmp : 0;
        io_put32(nsmp ? 263 : 29);
        put_name(in ? in->name : "", 22);
        io_put8(0);
        io_put16(nsmp);
        if (!nsmp)
            continue;
        io_put32(40);
        io_put(in->keymap, 96);
        write_env(&in->venv);
        write_env(&in->penv);
        io_put8(in->venv.n);
        io_put8(in->penv.n);
        io_put8(in->venv.sus);
        io_put8(in->venv.lstart);
        io_put8(in->venv.lend);
        io_put8(in->penv.sus);
        io_put8(in->penv.lstart);
        io_put8(in->penv.lend);
        io_put8(in->venv.flags);
        io_put8(in->penv.flags);
        io_put8(in->vtype);
        io_put8(in->vsweep);
        io_put8(in->vdepth);
        io_put8(in->vrate);
        io_put16(in->fadeout);
        io_zero(22);
        for (int s = 0; s < nsmp; s++) {
            const struct sample *sm = &in->smp[s];
            int b = sm->bits16;
            io_put32(sm->len << b);
            io_put32(sm->loop ? sm->lstart << b : 0);
            io_put32(sm->loop ? sm->llen << b : 0);
            io_put8(sm->vol);
            io_put8(sm->fine);
            io_put8((sm->loop & 3) | (b ? 0x10 : 0));
            io_put8(sm->pan);
            io_put8(sm->rel);
            io_put8(0);
            put_name(sm->name, 22);
        }
        for (int s = 0; s < nsmp; s++) {
            const struct sample *sm = &in->smp[s];
            if (sm->bits16) {
                const int16_t *d = sm->data;
                int16_t prev = 0;
                for (uint32_t k = 0; k < sm->len; k++) {
                    io_put16((uint16_t)(d[k] - prev));
                    prev = d[k];
                }
            } else {
                const int8_t *d = sm->data;
                int8_t prev = 0;
                for (uint32_t k = 0; k < sm->len; k++) {
                    io_put8((uint8_t)(d[k] - prev));
                    prev = d[k];
                }
            }
        }
    }
    io_flush();
    bool ok = !io.err;
    if (rb->close(io.fd) < 0)
        ok = false;
    if (!ok) {
        rb->remove(tmp);
        return -1;
    }
    rb->remove(path);
    return rb->rename(tmp, path) < 0 ? -1 : 0;
}

/* ---- MOD import ---- */

static int mod_period_note(int period)
{
    /* 65536 / 2^(i/12) */
    static const uint32_t semi_inv[12] = {
        65536, 61858, 58386, 55109, 52016, 49097,
        46341, 43740, 41285, 38968, 36781, 34716
    };
    if (!period)
        return 0;
    /* note n (1-based) has period 6848 / 2^((n - 1) / 12): 428 at C-4 */
    int best = 0, bestd = 1 << 30;
    for (int n = 1; n <= 96; n++) {
        uint32_t q = (6848u << 12) >> ((n - 1) / 12);
        q = (uint32_t)((uint64_t)q * semi_inv[(n - 1) % 12] >> 16);
        int d = ABS((int)(q >> 12) - period);
        if (d < bestd) {
            bestd = d;
            best = n;
        }
    }
    return best;
}

int mod_load(const char *path, char *err, size_t errlen)
{
    uint8_t hdr[1084];
    io.fd = rb->open(path, O_RDONLY);
    io.pos = io.len = 0;
    io.err = false;
    if (io.fd < 0) {
        rb->strlcpy(err, "Cannot open file", errlen);
        return -1;
    }
    io_bytes(hdr, sizeof(hdr));
    if (io.err)
        FAIL("Not a MOD file");
    const char *tag = (const char *)hdr + 1080;
    int nch = 0;
    if (!rb->memcmp(tag, "M.K.", 4) || !rb->memcmp(tag, "M!K!", 4) ||
        !rb->memcmp(tag, "FLT4", 4) || !rb->memcmp(tag, "4CHN", 4))
        nch = 4;
    else if (tag[1] == 'C' && tag[2] == 'H' && tag[3] == 'N' &&
             tag[0] >= '1' && tag[0] <= '9')
        nch = tag[0] - '0';
    else if (tag[2] == 'C' && tag[3] == 'H' && tag[0] >= '1' &&
             tag[0] <= '3' && tag[1] >= '0' && tag[1] <= '9')
        nch = (tag[0] - '0') * 10 + tag[1] - '0';
    if (nch < 1 || nch > MAX_CH)
        FAIL("Unsupported MOD type");
    song_clear();
    name_copy(song.name, (char *)hdr, 20);
    song.linear = false;
    song.nord = CLAMP(hdr[950], 1, 128);
    song.restart = hdr[951] < song.nord ? hdr[951] : 0;
    int npat = 0;
    for (int i = 0; i < 128; i++) {
        song.ord[i] = hdr[952 + i] & 0x7f;
        npat = MAX(npat, song.ord[i] + 1);
    }
    if (!song_set_channels(nch))
        FAIL("Out of memory");
    for (int c = 0; c < nch; c++)    /* Amiga LRRL, narrowed */
        song.chpan[c] = ((c & 3) == 0 || (c & 3) == 3) ? 0x50 : 0xb0;
    song.chan_pan = true;

    for (int p = 0; p < npat; p++) {
        song.pat[p].rows = 64;
        if (!pattern_alloc(p))
            FAIL("Out of memory (patterns)");
        struct cell *c = song.pat[p].c;
        for (int i = 0; i < 64 * nch; i++) {
            uint8_t b[4];
            io_bytes(b, 4);
            int period = (b[0] & 15) << 8 | b[1];
            c[i].note = mod_period_note(period);
            c[i].ins = (b[0] & 0xf0) | b[2] >> 4;
            c[i].fx = b[2] & 15;
            c[i].param = b[3];
            if (c[i].fx == 0xc) {   /* volume into the volume column */
                c[i].vol = 0x10 + MIN(c[i].param, 64);
                c[i].fx = c[i].param = 0;
            }
        }
    }
    for (int s = 0; s < 31; s++) {
        const uint8_t *h = hdr + 20 + s * 30;
        uint32_t len = (h[22] << 8 | h[23]) * 2;
        uint32_t ls = (h[26] << 8 | h[27]) * 2;
        uint32_t ll = (h[28] << 8 | h[29]) * 2;
        if (!len && !h[0])
            continue;
        if (!instr_set_samples(s, 1))
            FAIL("Out of memory (instruments)");
        struct instr *in = song.ins[s];
        name_copy(in->name, (char *)h, 22);
        struct sample *sm = &in->smp[0];
        if (!sample_set_data(sm, len, false))
            FAIL("Out of memory (samples)");
        io_bytes(sm->data, len);
        sm->vol = MIN(h[25], 64);
        sm->fine = (int8_t)((h[24] & 15) << 4);   /* signed nibble * 16 */
        sm->pan = 128;
        if (ll > 2 && ls < len) {
            sm->loop = 1;
            sm->lstart = ls;
            sm->llen = MIN(ll, len - ls);
        }
    }
    rb->close(io.fd);
    eng_reset();
    return 0;
fail:
    rb->close(io.fd);
    return -1;
}
