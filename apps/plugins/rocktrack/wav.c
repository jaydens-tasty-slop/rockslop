/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * RockTrack: WAV export (the song rendered offline) and WAV samples.
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

#define CHUNK 1024              /* frames per render call */
#define MAX_SECONDS (20 * 60)

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = v;
    p[1] = v >> 8;
    p[2] = v >> 16;
    p[3] = v >> 24;
}

static void wav_header(uint8_t *h, uint32_t rate, uint32_t frames)
{
    uint32_t bytes = frames * 4;
    rb->memcpy(h, "RIFF\0\0\0\0WAVEfmt ", 16);
    put32(h + 4, 36 + bytes);
    put32(h + 16, 16);
    h[20] = 1; h[21] = 0;           /* PCM */
    h[22] = 2; h[23] = 0;           /* stereo */
    put32(h + 24, rate);
    put32(h + 28, rate * 4);
    h[32] = 4; h[33] = 0;
    h[34] = 16; h[35] = 0;
    rb->memcpy(h + 36, "data", 4);
    put32(h + 40, bytes);
}

/* 0 done, 1 stopped by a key, -1 error. */
int wav_export(const char *path, void (*progress)(int pct))
{
    int16_t *buf = pool_alloc(CHUNK * 4);
    if (!buf)
        return -1;
    int fd = rb->open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        pool_free(buf);
        return -1;
    }
    uint8_t h[44];
    unsigned rate = eng_rate();
    wav_header(h, rate, 0);
    int ret = rb->write(fd, h, 44) == 44 ? 0 : -1;

    eng_close();                    /* render here, not in the mixer */
    eng_set_once(true);
    eng_play(PLAY_SONG, 0, 0);
    uint32_t frames = 0, tail = 0;
    long next_ui = 0;
    rb->button_clear_queue();
    while (ret == 0 && tail < rate / 4 && frames < MAX_SECONDS * rate) {
        if (eng_ended()) {
            if (!tail)
                eng_stop();         /* let the notes ring out briefly */
            tail += CHUNK;
        }
        eng_render(buf, CHUNK);
        if (rb->write(fd, buf, CHUNK * 4) != CHUNK * 4)
            ret = -1;
        frames += CHUNK;
        if (TIME_AFTER(*rb->current_tick, next_ui)) {
            int o, r;
            eng_pos(&o, &r);
            progress(song.nord ? o * 100 / song.nord : 0);
            next_ui = *rb->current_tick + HZ / 4;
            long b = rb->button_get(false);
            if (b != BUTTON_NONE &&
                !(b & (BUTTON_REL | BUTTON_REPEAT | SYS_EVENT)))
                ret = 1;                    /* a new key press stops */
        }
    }
    eng_set_once(false);
    eng_stop();
    eng_open();
    wav_header(h, rate, frames);
    if (rb->lseek(fd, 0, SEEK_SET) != 0 || rb->write(fd, h, 44) != 44)
        ret = -1;
    if (rb->close(fd) < 0)
        ret = -1;
    pool_free(buf);
    if (ret < 0)
        rb->remove(path);
    return ret;
}

/* ---- WAV samples ---- */

static uint32_t get32(const uint8_t *p)
{
    return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24;
}

/* relative note and finetune that play `rate` at C-4 */
static void tune_for_rate(uint32_t rate, int8_t *rel, int8_t *fine)
{
    uint64_t r = (uint64_t)rate * 65536 / 8363;     /* ratio, Q16 */
    int oct = 0;
    while (r >= 131072 && oct < 8) {
        r >>= 1;
        oct++;
    }
    while (r < 65536 && oct > -8) {
        r <<= 1;
        oct--;
    }
    int s = 0;
    while (s < 11 && semi_q16[s + 1] <= r)
        s++;
    /* the rest of the ratio, linear within the semitone */
    int f = (int)((r - semi_q16[s]) * 128 /
                  (semi_q16[s + 1] - semi_q16[s]));
    int note = oct * 12 + s;
    if (f >= 64) {                  /* finetune is -128..127: round up */
        note++;
        f -= 128;
    }
    *rel = CLAMP(note, -96, 95);
    *fine = CLAMP(f, -128, 127);
}

int wav_load_sample(const char *path, struct instr *in)
{
    uint8_t h[12], ck[8], fmt[16];
    int fd = rb->open(path, O_RDONLY);
    if (fd < 0) {
        app_message("Cannot open the file");
        return -1;
    }
    bool have_fmt = false;
    uint32_t data_len = 0;
    if (rb->read(fd, h, 12) != 12 || rb->memcmp(h, "RIFF", 4) ||
        rb->memcmp(h + 8, "WAVE", 4))
        goto bad;
    while (rb->read(fd, ck, 8) == 8) {
        uint32_t len = get32(ck + 4);
        if (!rb->memcmp(ck, "fmt ", 4) && len >= 16) {
            if (rb->read(fd, fmt, 16) != 16)
                goto bad;
            rb->lseek(fd, (len - 16 + 1) & ~1u, SEEK_CUR);
            have_fmt = true;
        } else if (!rb->memcmp(ck, "data", 4)) {
            data_len = len;
            break;
        } else {
            rb->lseek(fd, (len + 1) & ~1u, SEEK_CUR);
        }
    }
    int format = fmt[0] | fmt[1] << 8, nch = fmt[2] | fmt[3] << 8;
    int bits = fmt[14] | fmt[15] << 8;
    uint32_t rate = get32(fmt + 4);
    if (!have_fmt || !data_len || format != 1 || nch < 1 || nch > 2 ||
        (bits != 8 && bits != 16)) {
        app_message("Needs 8/16-bit PCM WAV");
        goto fail;
    }
    int bpf = nch * bits / 8;
    uint32_t frames = data_len / bpf;
    /* keep 16 KB for editing; drop to 8 bits before cutting the end */
    size_t room = pool_avail() + (in->smp[0].len << in->smp[0].bits16);
    room = room > 16384 ? room - 16384 : 0;
    bool b16 = bits == 16 && frames * 2 <= room;
    uint32_t keep = MIN(frames, (uint32_t)(room >> b16));
    if (keep < 16) {
        app_message("Out of memory");
        goto fail;
    }
    if (!sample_set_data(&in->smp[0], keep, b16)) {
        app_message("Out of memory");
        goto fail;
    }
    struct sample *sm = &in->smp[0];
    uint8_t buf[256];
    uint32_t done = 0;
    while (done < keep) {
        int n = MIN((uint32_t)(sizeof(buf) / bpf), keep - done);
        if (rb->read(fd, buf, n * bpf) != n * bpf)
            break;
        for (int i = 0; i < n; i++) {
            int v = 0;
            for (int c = 0; c < nch; c++) {
                const uint8_t *p = buf + i * bpf + c * bits / 8;
                v += bits == 16 ? (int16_t)(p[0] | p[1] << 8)
                                : (p[0] - 128) << 8;
            }
            v /= nch;
            if (b16)
                ((int16_t *)sm->data)[done + i] = v;
            else
                ((int8_t *)sm->data)[done + i] = v >> 8;
        }
        done += n;
    }
    rb->close(fd);
    const char *name = rb->strrchr(path, '/');
    name = name ? name + 1 : path;
    rb->strlcpy(sm->name, name, sizeof(sm->name));
    char *dot = rb->strrchr(sm->name, '.');
    if (dot)
        *dot = 0;
    if (in->name[0] == 0 || !rb->strncmp(in->name, "Instrument", 10))
        rb->strlcpy(in->name, sm->name, sizeof(in->name));
    tune_for_rate(rate, &sm->rel, &sm->fine);
    sm->vol = 64;
    sm->pan = 128;
    rb->memset(in->keymap, 0, sizeof(in->keymap));
    struct adsr a = { 0, 0, 15, 3 };
    adsr_to_env(&a, &in->venv);
    if (keep < frames)
        app_message("Sample cut to %lu%%", (unsigned long)(keep * 100 /
                                                         frames));
    else
        app_message("Loaded %s", sm->name);
    return 0;
bad:
    app_message("Not a WAV file");
fail:
    rb->close(fd);
    return -1;
}
