/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Video player: AVI, Matroska and MP4 files with MJPEG, MPEG-1/2 or raw
 * video and PCM, IMA ADPCM or MPEG audio; captions; seeking, scanning,
 * resume. Pictures follow the sound's clock.
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
#include "lib/helper.h"
#include "lib/configfile.h"
#include "videoplayer.h"
#include "demux.h"
#include "video.h"
#include "sound.h"
#include "subs.h"
#include "osd.h"
#include "stats.h"

#define CFG_FILE        "videoplayer.cfg"
#define CFG_VERSION     1
#define RESUME_FILE     PLUGIN_DATA_DIR "/videoplayer.resume"
#define RESUME_ENTRIES  8
#define RESUME_MIN_MS   10000   /* nearer either end: start from the top */

#define SEEK_STEP_MS    10000
#define SEEK_DELAY      (HZ * 2 / 5)    /* seek once the taps rest */
#define SCAN_STEP       (HZ / 6)        /* a new picture while scanning */
#define SCAN_RAMP       (HZ * 3 / 2)    /* then the speed doubles */
#define SCAN_MAX_SHIFT  4               /* up to 32x */
#define MAX_LATE        5       /* show at least every 6th picture */
#define SEEK_DECODE_MAX 600     /* pictures decoded to reach a seek's */
#define DELAY_STEP      20      /* ms */
#define DELAY_MAX       500
#define RING_SIZE       (32 * 1024)
#define RING_MIN        (8 * 1024)
#define MPEG_PIECE      (4 * 1024)     /* read at a time (libmpeg2 copies) */
#define VBUF_MIN        (32 * 1024)
#define VBUF_MAX        (128 * 1024)

enum then { THEN_STOP, THEN_NEXT };

static int rotation = ROTATE_CW;
/* ms the sound plays after its picture, by output: the player's own,
 * Bluetooth (on top of the latency the sink reports) */
static int audio_delay[2];
static int then = THEN_NEXT;
static bool captions = true;
static const char *rotation_names[] = { "clockwise", "anticlockwise", "never" };
static const char *then_names[] = { "stop", "next" };
static struct configdata config[] = {
    { TYPE_ENUM, 0, 3, { .int_p = &rotation }, "rotation", (char **)rotation_names },
    { TYPE_INT, -DELAY_MAX, DELAY_MAX, { .int_p = &audio_delay[0] }, "audio delay", NULL },
#ifdef HAVE_BLUETOOTH
    { TYPE_INT, -DELAY_MAX, DELAY_MAX, { .int_p = &audio_delay[1] }, "bt audio delay", NULL },
#endif
    { TYPE_ENUM, 0, 2, { .int_p = &then }, "then", (char **)then_names },
    { TYPE_BOOL, 0, 1, { .bool_p = &captions }, "captions", NULL },
};

static struct movie movie;
static struct stream vs, as;
static int atrack;              /* the audio track playing, -1: none */
static void *audio_ring, *audio_work;
static size_t audio_ring_size, audio_work_max;
static char title[64];
static uint8_t *vbuf;
static uint32_t vbuf_size;

static struct {
    uint32_t pos;               /* the picture on screen */
    uint32_t frame_ms;
    bool pending;               /* a decoded picture waits in the frame buffer */
    uint32_t pending_ms;
    bool video_end;
    bool fed_end;               /* MPEG: the end has been fed */
    uint32_t feed_ms;           /* the next piece's time */
    bool need_time;             /* the next packet's time goes with it */
    bool paused;
    int late_run;               /* pictures dropped in a row */
    bool osd_drawn;             /* the frame buffer holds the OSD too */
    /* the clock without audio: tick_ms at tick_base */
    bool audio_clock;
    uint32_t tick_ms;
    long tick_base;
    /* taps: a seek waiting for them to rest */
    bool seeking;
    uint32_t seek_ms;
    uint32_t seek_from;         /* where the taps started */
    long seek_tick;
    /* holds: scanning */
    int scan_dir;
    uint32_t scan_ms;
    long scan_start, scan_tick, scan_next;
    uint32_t stop_ms;           /* where the user stopped; 0 at the end */
    uint32_t shown, dropped, bad;
} p;

/* ---- memory: the plugin arena's rest, then the audio buffer ---- */

static uint8_t *mem[2];
static size_t mem_left[2];

void *player_alloc(size_t size)
{
    size = ALIGN_UP(size, 4);
    for (int i = 0; i < 2; i++)
    {
        if (mem_left[i] >= size)
        {
            void *ptr = mem[i];
            mem[i] += size;
            mem_left[i] -= size;
            return ptr;
        }
    }
    return NULL;
}

static size_t largest_free(void)
{
    return MAX(mem_left[0], mem_left[1]) & ~3u;
}

static void memory_reset(void)
{
    mem[0] = rb->plugin_get_buffer(&mem_left[0]);
    mem[1] = rb->plugin_get_audio_buffer(&mem_left[1]);
    for (int i = 0; i < 2; i++)
    {
        uint8_t *aligned = (uint8_t *)ALIGN_UP((uintptr_t)mem[i], 4);
        size_t pad = aligned - mem[i];
        mem[i] = aligned;
        mem_left[i] = mem_left[i] > pad ? mem_left[i] - pad : 0;
    }
}

/* ---- clock ---- */

/* How late the output plays what the mixer takes */
static int sink_latency_ms(void)
{
#ifdef HAVE_BLUETOOTH
    return rb->pcm_sink_latency_ms();
#else
    return 0;
#endif
}

/* Which output: one with a latency of its own is Bluetooth */
static int *delay_now(void)
{
    return &audio_delay[sink_latency_ms() > 0];
}

static uint32_t clock_ms(void)
{
    if (p.audio_clock)
    {
        int32_t t = sound_clock_ms() + *delay_now() - sink_latency_ms();
        t = MAX(t, 0);
        if (!sound_drained())
            return t;
        /* the sound has ended: on with the ticks from there (the output
           still plays its latency's worth) */
        p.audio_clock = false;
        p.tick_ms = t;
        p.tick_base = *rb->current_tick;
    }
    if (p.paused || p.scan_dir)
        return p.tick_ms;
    return p.tick_ms + (*rb->current_tick - p.tick_base) * (1000 / HZ);
}

static void boost(bool on)
{
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(on);
#else
    (void)on;
#endif
}

/* Hold the clock (paused or scanning) or let it go on */
static void hold_clock(bool hold)
{
    if (hold)
    {
        p.tick_ms = clock_ms();
        p.audio_clock = false;
    }
    else
    {
        p.tick_base = *rb->current_tick;
        p.audio_clock = atrack >= 0 && !sound_drained();
    }
    if (atrack >= 0)
        sound_play(!hold);
}

static void set_paused(bool paused)
{
    if (paused == p.paused)
        return;
    if (!p.scan_dir)
        hold_clock(paused);
    p.paused = paused;
    boost(!paused);
}

/* ---- pictures ---- */

static void draw_osd(void)
{
    struct osd_state s = {
        .title = title,
        .pos = p.pos,
        .duration = movie.duration,
        .paused = p.paused,
        .seeking = p.seeking || p.scan_dir,
        .seek_ms = p.scan_dir ? p.scan_ms : p.seek_ms,
        .seek_from = p.seek_from,
        .chapters = movie.chapters,
        .nchapters = movie.nchapters,
        .captions = subs_count() ? 1 + (subs_selected() >= 0) : 0,
        .caption = p.scan_dir ? NULL : subs_at(p.pos),
    };
    if (p.scan_dir)
    {
        int shift = MIN((*rb->current_tick - p.scan_start) / SCAN_RAMP,
                        SCAN_MAX_SHIFT);
        s.scan = p.scan_dir * (2 << shift);
    }
    p.osd_drawn = osd_draw(&s);
}

/* Feed MPEG data on. Returns false at the end. */
static bool feed(void)
{
    struct packet pk;

    if (vs.left == 0)
    {
        if (p.fed_end)
            return false;
        if (!movie.ops->next(&movie, &vs, &pk))
        {
            video_feed(NULL, 0, 0);
            p.fed_end = true;
            return true;
        }
        p.feed_ms = movie.pts || p.need_time ? pk.ms : NO_TIME;
        p.need_time = false;
        return true;
    }
    int32_t n = demux_read(&vs, vbuf, MIN(vs.left, vbuf_size));
    if (n <= 0)
        demux_skip(&vs);
    else
        video_feed(vbuf, n, p.feed_ms);
    p.feed_ms = NO_TIME;
    return true;
}

/* The next picture into the frame buffer, skipping (up to max_skip of)
 * those that would show before from_ms; *skipped counts them. Returns
 * false at the end. */
static bool decode_next(uint32_t from_ms, int max_skip, uint32_t *ms,
                        int *skipped)
{
    struct packet pk;
    uint32_t half = p.frame_ms / 2;

    *skipped = 0;

    if (p.osd_drawn)
        video_clear();
    for (;;)
    {
        if (video_streamed())
        {
            if (!video_next(false, ms))
            {
                if (!feed())
                    return false;
                continue;
            }
            if (*ms + half < from_ms && max_skip-- > 0)
            {
                ++*skipped;
                continue;
            }
            video_redraw();
            return true;
        }

        if (!movie.ops->next(&movie, &vs, &pk))
            return false;
        if (pk.size == 0 || pk.size > vbuf_size ||
            (pk.ms + half < from_ms && max_skip-- > 0))
        {
            if (pk.size)
                ++*skipped;
            demux_skip(&vs);
            continue;
        }
        /* at the file's word offset: whole sectors then go straight in
           by DMA (unaligned ones take a bounce buffer) */
        uint8_t *buf = vbuf + (rb->lseek(vs.fd, 0, SEEK_CUR) & 3);
        if (demux_read(&vs, buf, pk.size) != (int32_t)pk.size ||
            !video_decode(buf, pk.size))
        {
            p.bad++;
            continue;
        }
        *ms = pk.ms;
        return true;
    }
}

/* Decode the next picture to show, ahead of its time */
static void next_picture(void)
{
    uint32_t now = clock_ms(), from = 0, ms, t0 = stats_us();

    if (p.late_run < MAX_LATE && now > p.frame_ms)
        from = now - p.frame_ms;
    int skipped;
    if (!decode_next(from, MAX_LATE - p.late_run, &ms, &skipped))
    {
        p.video_end = true;
        return;
    }
    p.dropped += skipped;
    p.late_run = skipped ? p.late_run + skipped : 0;
    p.pending = true;
    p.pending_ms = ms;
    uint32_t pos = p.pos;
    p.pos = ms;     /* the OSD as it will be */
    draw_osd();
    p.pos = pos;
    stats_time(ST_VIDEO, t0);
}

static void present(void)
{
    uint32_t t0 = stats_us();
    rb->lcd_update();
    stats_time(ST_LCD, t0);
    p.pos = p.pending_ms;
    p.pending = false;
    p.shown++;
}

/* The picture on screen again, with the OSD as it is now */
static void refresh(void)
{
    if (p.osd_drawn)
        video_clear();
    video_redraw();
    draw_osd();
    rb->lcd_update();
}

/* Pause or play, with the pop that says which */
static void pause_to(bool paused)
{
    if (p.pending)
        present();
    set_paused(paused);
    osd_pop();
    osd_show_bars();
    if (p.paused)
        refresh();
}

/* Play (again) from ms: every stream there, the picture at ms on screen */
static bool restart_at(uint32_t ms)
{
    uint32_t at, pic, t0 = stats_us();
    int skipped;

    if (!movie.ops->seek(&movie, &vs, atrack >= 0 ? &as : NULL, subs_stream(),
                         ms, &at))
        return false;
    video_reset();
    p.video_end = p.fed_end = p.pending = false;
    p.need_time = true;
    p.late_run = 0;
    subs_seek(ms);
    if (atrack >= 0)
    {
        sound_reset(ms);
        sound_fill(&movie, &as);
    }
    if (decode_next(ms, SEEK_DECODE_MAX, &pic, &skipped))
    {
        p.pos = MAX(pic, ms);
        draw_osd();
        rb->lcd_update();
    }
    else
        p.video_end = true;
    stats_seek(ms, p.video_end ? 0 : pic, skipped, t0);
    p.tick_ms = p.pos;
    p.tick_base = *rb->current_tick;
    p.audio_clock = atrack >= 0 && !p.paused && !p.scan_dir;
    if (p.audio_clock)
        sound_play(true);
    return true;
}

/* ---- seeking ---- */

static uint32_t clamp_ms(int64_t ms)
{
    if (ms < 0)
        return 0;
    if (movie.duration && ms >= movie.duration)
        return movie.duration > p.frame_ms ? movie.duration - p.frame_ms : 0;
    return ms;
}

static void seek_tap(int dir)
{
    if (!p.seeking)
    {
        p.seeking = true;
        p.seek_ms = p.seek_from = p.pos;
    }
    p.seek_ms = clamp_ms((int64_t)p.seek_ms + dir * SEEK_STEP_MS);
    p.seek_tick = *rb->current_tick;
    osd_seek_hint(dir);
    osd_show_bars();
}

static void scan_start(int dir)
{
    if (!p.paused)
        hold_clock(true);
    p.scan_dir = dir;
    p.scan_ms = p.seeking ? p.seek_ms : p.pos;
    p.seeking = false;
    p.scan_start = p.scan_tick = *rb->current_tick;
    p.scan_next = p.scan_start;
    osd_seek_hint(dir);
    osd_show_bars();
}

/* On with the scan: a keyframe's picture now and then */
static void scan_step(void)
{
    long now = *rb->current_tick;
    uint32_t at, ms;
    int skipped;

    if (TIME_BEFORE(now, p.scan_next))
        return;
    int shift = MIN((now - p.scan_start) / SCAN_RAMP, SCAN_MAX_SHIFT);
    int32_t step = (2 << shift) * (now - p.scan_tick) * (1000 / HZ);
    p.scan_tick = now;
    p.scan_ms = clamp_ms((int64_t)p.scan_ms + p.scan_dir * step);
    osd_seek_hint(p.scan_dir);
    if (movie.ops->seek(&movie, &vs, NULL, NULL, p.scan_ms, &at))
    {
        video_reset();
        p.fed_end = false;
        p.need_time = true;
        if (decode_next(0, 0, &ms, &skipped))
            p.pos = ms;
    }
    draw_osd();
    rb->lcd_update();
    p.scan_next = *rb->current_tick + SCAN_STEP;
}

static bool scan_end(void)
{
    uint32_t ms = p.scan_ms;
    p.scan_dir = 0;
    return restart_at(ms);
}

/* ---- keys ---- */

enum key {
    K_NONE, K_UP, K_RIGHT, K_DOWN, K_LEFT, K_SELECT,
    K_MENU, K_BARS, K_CAPTIONS, K_AUDIO, K_PREV, K_NEXT, K_INFO, K_QUIT,
};

/* Directions as the picture is held */
static enum key key_of(long button)
{
    static const enum key dirs[] = { K_UP, K_RIGHT, K_DOWN, K_LEFT };
    int dir;

    switch (button & ~(BUTTON_REL | BUTTON_REPEAT))
    {
    case BUTTON_UP:     dir = 0; break;
    case BUTTON_RIGHT:  dir = 1; break;
    case BUTTON_DOWN:   dir = 2; break;
    case BUTTON_LEFT:   dir = 3; break;
    case BUTTON_SELECT: return K_SELECT;
#ifdef HAVE_BLUETOOTH_HID
    case BUTTON_PAD_A:  return K_SELECT;
    case BUTTON_PAD_B:  return K_BARS;
    case BUTTON_PAD_X:  return K_CAPTIONS;
    case BUTTON_PAD_Y:  return K_AUDIO;
    case BUTTON_PAD_L1: return K_PREV;
    case BUTTON_PAD_R1: return K_NEXT;
    case BUTTON_PAD_L2: return K_LEFT;      /* as the picture is held */
    case BUTTON_PAD_R2: return K_RIGHT;
    case BUTTON_PAD_MENU: return K_MENU;
    case BUTTON_PAD_VIEW: return K_INFO;
    case BUTTON_PAD_HOME: return K_QUIT;
#endif
    default:            return K_NONE;
    }
    if (video_turned())
        dir = (dir + (rotation == ROTATE_CW ? 3 : 1)) % 4;
    return dirs[dir];
}

enum key_event { EV_NONE, EV_PRESS, EV_REPEAT, EV_HOLD, EV_TAP, EV_RELEASE };

/* The button whose press closed the menu: the rest of that press (its
 * repeats, its release) was the menu's too. As a tap, Left out of the menu
 * also seeked back, and the long decode after it ate a held Select's
 * repeats: no menu but a pause (TS17 torture 18 step 39) */
static long menu_closer;

/* What a button does: pressed, held (EV_HOLD once, then EV_REPEAT),
 * tapped (released unheld) or released after a hold; EV_NONE: the rest of
 * a press that closed the menu */
static enum key_event event_of(long button)
{
    static bool held;
    if (menu_closer)
    {
        bool rest = (button & (BUTTON_REL | BUTTON_REPEAT)) &&
                    (button & ~(BUTTON_REL | BUTTON_REPEAT)) == menu_closer;
        if (!rest || (button & BUTTON_REL))
            menu_closer = 0;
        if (rest)
            return EV_NONE;
    }
    if (button & BUTTON_REL)
    {
        bool was = held;
        held = false;
        return was ? EV_RELEASE : EV_TAP;
    }
    if (button & BUTTON_REPEAT)
    {
        if (held)
            return EV_REPEAT;
        held = true;
        return EV_HOLD;
    }
    held = false;
    return EV_PRESS;
}

/* Play or Pause from a headset (AVRCP) or a keyboard: this player's
 * play/pause, as a toggle. A headset picks Play or Pause from the status
 * we report, the music's, which is stopped while a video plays. */
static bool is_play_key(long button)
{
#ifdef HAVE_BLUETOOTH
    if (button == SYS_BT_PLAY)
        return true;
#endif
#ifdef HAVE_MULTIMEDIA_KEYS
    if (button == BUTTON_MULTIMEDIA_PLAYPAUSE)
        return true;
#endif
    return false;
}

/* The core's handling of an event, but not of the play keys: the music's
 * resume would find the audio buffer taken (an OOM splash) */
static long default_event(long button)
{
    if (is_play_key(button))
        return 0;
    return rb->default_event_handler(button);
}

/* Wait for a button with the OSD kept moving */
static long wait_button(void (*draw)(void))
{
    for (;;)
    {
        long next = osd_card_next_change();
        long timeout = next ? MAX(next - *rb->current_tick, 0) : HZ;
        long button = rb->button_get_w_tmo(timeout);
        if (button != BUTTON_NONE)
            return button;
        draw();
    }
}

/* ---- menus ---- */

enum result {
    R_BACK,     /* the menu closed */
    R_PLAY,
    R_START,    /* from the start */
    R_END,      /* played to the end (or could not go on) */
    R_NEXT, R_PREV,
    R_QUIT, R_USB,
};

static void label_audio(char *buf, size_t size)
{
    if (atrack < 0)
        rb->strlcpy(buf, "None", size);
    else
        demux_track_label(&movie.tracks[atrack], buf, size);
    if (!*buf)
        rb->strlcpy(buf, movie.tracks[atrack].codec, size);
}

static int audio_tracks(int *list)
{
    int n = 0;
    for (int i = 0; i < movie.ntracks; i++)
        if (movie.tracks[i].type == TRACK_AUDIO && movie.tracks[i].supported)
            list[n++] = i;
    return n;
}

static bool set_audio_track(int track);

static const char *rotation_labels[] = { "Clockwise", "Anticlockwise", "Never" };

static void fmt_time(char *buf, size_t size, uint32_t ms)
{
    unsigned long s = ms / 1000;
    if (s >= 3600)
        rb->snprintf(buf, size, "%lu:%02lu:%02lu", s / 3600, s / 60 % 60, s % 60);
    else
        rb->snprintf(buf, size, "%lu:%02lu", s / 60, s % 60);
}

static struct {
    const char *title;
    struct osd_item items[12];
    int n, selected;
} card;

static void draw_menu_card(void)
{
    if (p.osd_drawn)
        video_clear();
    video_redraw();
    osd_draw_menu(card.title, card.items, card.n, card.selected);
    rb->lcd_update();
    p.osd_drawn = true;
}

static void draw_info_card(void)
{
    if (p.osd_drawn)
        video_clear();
    video_redraw();
    osd_draw_info(card.title, card.items, card.n);
    rb->lcd_update();
    p.osd_drawn = true;
}

static bool file_info(const char *path)
{
    static char v[48], a[48], s[32], len[16], size[16], rate[16], shown[24];
    const struct track *vt = &movie.tracks[movie.video];
    const char *name = rb->strrchr(path, '/');
    int n = 0;

    char how[24];
    video_describe(how, sizeof(how));
    rb->snprintf(v, sizeof(v), "%s %s", vt->codec, how);
    if (atrack >= 0)
    {
        const struct aformat *af = &movie.tracks[atrack].u.a;
        unsigned long tenths = af->rate / 100 % 10;
        char khz[8];
        /* 44.1 kHz, 22 kHz: short, for the card's width */
        if (tenths)
            rb->snprintf(khz, sizeof(khz), "%lu.%lu", af->rate / 1000UL, tenths);
        else
            rb->snprintf(khz, sizeof(khz), "%lu", af->rate / 1000UL);
        rb->snprintf(a, sizeof(a), "%s %s kHz%s", movie.tracks[atrack].codec,
                     khz, af->channels > 1 ? "" : " mono");
    }
    else
        rb->strlcpy(a, "None", sizeof(a));
    rb->snprintf(s, sizeof(s), "%d", subs_count());
    fmt_time(len, sizeof(len), movie.duration);
    rb->snprintf(size, sizeof(size), "%lu.%lu MB",
                 (unsigned long)movie.file_size / 1000000,
                 (unsigned long)movie.file_size / 100000 % 10);
    rb->snprintf(rate, sizeof(rate), "%lu kbit/s", movie.duration ?
                 (unsigned long)((uint64_t)movie.file_size * 8 / movie.duration) : 0);
    rb->snprintf(shown, sizeof(shown), "%lu (%lu dropped)",
                 (unsigned long)p.shown, (unsigned long)p.dropped);

    card.title = name ? name + 1 : path;
#define ROW(l, val) card.items[n++] = (struct osd_item){ l, val, false }
    ROW("Format", movie.format);
    ROW("Video", v);
    if (movie.frame_us)
    {
        static char fps[16];
        uint32_t f = 100000000 / movie.frame_us;
        rb->snprintf(fps, sizeof(fps), "%lu.%02lu", (unsigned long)f / 100,
                     (unsigned long)f % 100);
        ROW("FPS", fps);
    }
    ROW("Audio", a);
    ROW("Captions", s);
    ROW("Length", len);
    ROW("Size", size);
    ROW("Bit rate", rate);
    ROW("Pictures", shown);
#undef ROW
    card.n = n;
    osd_card_open();
    draw_info_card();
    for (;;)
    {
        long button = wait_button(draw_info_card);
        if (default_event(button) == SYS_USB_CONNECTED)
            return false;
        if (!(button & (BUTTON_REL | BUTTON_REPEAT)))
            return true;
    }
}

/* The menu closes on button's press (or repeat) */
static enum result close_menu(long button, enum result r)
{
    menu_closer = button & ~(BUTTON_REL | BUTTON_REPEAT);
    return r;
}

static enum result menu(const char *path)
{
    enum { M_RESUME, M_CAPTIONS, M_AUDIO, M_DELAY, M_TURN, M_THEN, M_INFO,
           M_START, M_QUIT, M_COUNT };
    static char cap[24], aud[24], delay[16];
    int tracks[MAX_TRACKS];
    int ntracks = audio_tracks(tracks);
    int what[M_COUNT];

    card.title = "Video";
    card.selected = 0;
    osd_card_open();
    for (;;)
    {
        int n = 0;
        if (subs_selected() >= 0)
            subs_label(subs_selected(), cap, sizeof(cap));
        else
            rb->strlcpy(cap, "Off", sizeof(cap));
        label_audio(aud, sizeof(aud));
        rb->snprintf(delay, sizeof(delay), "%+d ms", *delay_now());
#define ITEM(id, l, val, adj) do { what[n] = id; \
        card.items[n++] = (struct osd_item){ l, val, adj }; } while (0)
        ITEM(M_RESUME, p.paused ? "Play" : "Resume", NULL, false);
        if (subs_count())
            ITEM(M_CAPTIONS, "Captions", cap, true);
        if (ntracks > 1)
            ITEM(M_AUDIO, "Audio", aud, true);
        if (atrack >= 0)
            ITEM(M_DELAY, sink_latency_ms() ? "BT delay" : "Sound delay", delay, true);
        ITEM(M_TURN, "Turn video", rotation_labels[rotation], true);
        ITEM(M_THEN, "At the end", then == THEN_NEXT ? "Next" : "Stop", true);
        ITEM(M_INFO, "File info", NULL, false);
        ITEM(M_START, "Play from start", NULL, false);
        ITEM(M_QUIT, "Quit", NULL, false);
#undef ITEM
        card.n = n;
        draw_menu_card();

        long button = wait_button(draw_menu_card);
        if (default_event(button) == SYS_USB_CONNECTED)
            return R_USB;
        enum key k = key_of(button);
        enum key_event ev = event_of(button);
        /* Select on its release (its hold opened this); the rest on their
           press and repeats */
        if (k == K_SELECT ? ev != EV_TAP :
            ev == EV_TAP || ev == EV_RELEASE)
            continue;
        int id = what[card.selected], dir = k == K_LEFT ? -1 : 1;
        bool adjust = card.items[card.selected].adjust;
        switch (k)
        {
        case K_UP:
        case K_DOWN:
            card.selected = (card.selected + (k == K_UP ? n - 1 : 1)) % n;
            continue;
        case K_LEFT:
        case K_RIGHT:
            if (!adjust)
            {
                if (k == K_LEFT)
                    return close_menu(button, R_BACK);
                continue;
            }
            break;
        case K_SELECT:
            break;
        case K_MENU:
        case K_BARS:
            return close_menu(button, R_BACK);
        case K_QUIT:
            return R_QUIT;
        default:
            continue;
        }
        switch (id)
        {
        case M_RESUME:
            return R_PLAY;
        case M_CAPTIONS:
        {
            /* Off, then each choice */
            int count = subs_count() + 1;
            int c = (subs_selected() + 1 + dir + count) % count;
            subs_select(c - 1, p.pos);
            captions = c > 0;
            break;
        }
        case M_AUDIO:
        {
            int i = 0;
            while (i < ntracks && tracks[i] != atrack)
                i++;
            if (!set_audio_track(tracks[(i + dir + ntracks) % ntracks]))
                return R_END;
            break;
        }
        case M_DELAY:
            *delay_now() = MIN(MAX(*delay_now() + dir * DELAY_STEP, -DELAY_MAX),
                               DELAY_MAX);
            break;
        case M_TURN:
            rotation = (rotation + dir + 3) % 3;
            video_set_rotation(rotation);
            break;
        case M_THEN:
            then = then == THEN_NEXT ? THEN_STOP : THEN_NEXT;
            break;
        case M_INFO:
            if (!file_info(path))
                return R_USB;
            osd_card_open();
            break;
        case M_START:
            return R_START;
        case M_QUIT:
            return R_QUIT;
        }
    }
}

/* ---- playback ---- */

static bool set_audio_track(int track)
{
    const struct aformat *af = &movie.tracks[track].u.a;

    if (track == atrack)
        return true;
    if (atrack >= 0)
        sound_close();
    atrack = -1;
    if (!sound_init(af, audio_ring, audio_ring_size, audio_work, audio_work_max))
    {
        osd_toast("Not enough memory for that sound");
        return true;
    }
    atrack = track;
    demux_stream_init(&movie, &as, as.fd, track);
    return restart_at(p.pos);
}

/* The video's memory: its packets, the decoder, the sound */
static bool setup_memory(void)
{
    const struct track *vt = &movie.tracks[movie.video];

    if (vt->u.v.codec == VC_MPEG)
        vbuf_size = MPEG_PIECE;
    else if (vt->u.v.codec == VC_MJPEG)
        vbuf_size = MIN(MAX(largest_free() / 4, VBUF_MIN), VBUF_MAX);
    else
        vbuf_size = ((vt->u.v.width * (vt->u.v.codec == VC_BGR24 ? 3 : 2) + 3) & ~3)
                    * vt->u.v.height;
    vbuf = player_alloc(vbuf_size + 3);
    if (!vbuf)
        return false;
    if (!video_init(vt, movie.vextra, movie.vextra_len, rotation))
        return false;
    return true;
}

/* The sound: the default track, else the first that plays */
static void setup_audio(void)
{
    int tracks[MAX_TRACKS];
    int n = audio_tracks(tracks), pick = -1;
    size_t work = 0;

    atrack = -1;
    for (int i = 0; i < n; i++)
    {
        work = MAX(work, sound_work_size(&movie.tracks[tracks[i]].u.a));
        if (pick < 0 || movie.tracks[tracks[i]].is_default)
            pick = tracks[i];
    }
    if (pick < 0)
    {
        for (int i = 0; i < movie.ntracks; i++)
        {
            if (movie.tracks[i].type == TRACK_AUDIO)
            {
                char msg[48];
                rb->snprintf(msg, sizeof(msg), "No sound: %s",
                             movie.tracks[i].codec[0] ? movie.tracks[i].codec
                                                      : "unknown");
                osd_toast(msg);
                break;
            }
        }
        return;
    }
    audio_work_max = work;
    audio_work = player_alloc(work);
    audio_ring_size = MIN(largest_free(), RING_SIZE);
    audio_ring = player_alloc(audio_ring_size);
    if (!audio_work || !audio_ring || audio_ring_size < RING_MIN ||
        !sound_init(&movie.tracks[pick].u.a, audio_ring, audio_ring_size,
                    audio_work, work))
    {
        osd_toast("Not enough memory for the sound");
        return;
    }
    atrack = pick;
}

static enum result play(uint32_t start, const char *path)
{
    if ((start == 0 || !restart_at(start)) && !restart_at(0))
        return R_QUIT;
    if (start)
    {
        char t[16], msg[32];
        fmt_time(t, sizeof(t), start);
        rb->snprintf(msg, sizeof(msg), "Resumed at %s", t);
        osd_toast(msg);
    }
    osd_show_bars();

    for (;;)
    {
        bool running = !p.paused && !p.scan_dir;
        if (running)
            stats_report(false, p.shown, p.dropped, p.bad,
                         atrack >= 0 ? sound_ring_ms() : 0,
                         sink_latency_ms(), *delay_now());
        if (atrack >= 0 && running)
        {
            uint32_t t0 = stats_us();
            sound_fill(&movie, &as);
            stats_time(ST_SOUND, t0);
        }
        if (running && !p.pending && !p.video_end)
            next_picture();

        long timeout = HZ / 10;
        if (running && p.pending)
        {
            int32_t wait = p.pending_ms - clock_ms();
            if (wait <= 5)
            {
                stats_late(-wait, p.frame_ms);
                present();
                timeout = 0;
            }
            else
                timeout = MIN(MAX(wait / (1000 / HZ), 1), timeout);
        }
        else if (running && !p.video_end)
            timeout = 0;
        else if (running && p.video_end && !p.seeking &&
                 (!p.audio_clock || sound_drained()))
            return R_END;
        if (!running)
        {
            /* the OSD's next step, also after one just drawn: a flat wait
               left a pause's pop half open, its icon unseen */
            long next = osd_next_change();
            if (next && !TIME_BEFORE(*rb->current_tick, next))
            {
                refresh();
                next = osd_next_change();
            }
            if (next)
                timeout = MIN(timeout, MAX(next - *rb->current_tick, 0));
        }
        if (p.scan_dir)
        {
            scan_step();
            timeout = MIN(timeout, MAX(p.scan_next - *rb->current_tick, 0));
        }
        if (p.seeking && TIME_AFTER(*rb->current_tick, p.seek_tick + SEEK_DELAY))
        {
            p.seeking = false;
            if (!restart_at(p.seek_ms))
                return R_END;
        }
        rb->reset_poweroff_timer();

        uint32_t t0 = stats_us();
        long button = rb->button_get_w_tmo(timeout);
        if (running)
            stats_time(ST_WAIT, t0);
        if (button == BUTTON_NONE)
            continue;
        if (is_play_key(button))
        {
            pause_to(!p.paused);
            continue;
        }
        enum key k = key_of(button);
        enum key_event ev = event_of(button);
        if (ev == EV_NONE)
            continue;
        switch (k)
        {
        case K_SELECT:
            if (ev == EV_TAP)
                pause_to(!p.paused);
            else if (ev == EV_HOLD)
                k = K_MENU;
            break;
        case K_LEFT:
        case K_RIGHT:
        {
            int dir = k == K_RIGHT ? 1 : -1;
            if (ev == EV_TAP)
                seek_tap(dir);
            else if (ev == EV_HOLD)
                scan_start(dir);
            else if (ev == EV_RELEASE && p.scan_dir && !scan_end())
                return R_END;
            if (p.paused && ev != EV_REPEAT)
                refresh();
            break;
        }
        case K_UP:
        case K_DOWN:
            if (ev == EV_PRESS || ev == EV_HOLD || ev == EV_REPEAT)
            {
                rb->adjust_volume(k == K_UP ? 1 : -1);
                osd_show_volume();
                if (p.paused)
                    refresh();
            }
            break;
        case K_BARS:
            if (ev == EV_PRESS)
            {
                osd_toggle_bars();
                if (p.paused)
                    refresh();
            }
            break;
        case K_CAPTIONS:
            if (ev == EV_PRESS && subs_count())
            {
                int c = (subs_selected() + 2) % (subs_count() + 1);
                char label[32], msg[48];
                subs_select(c - 1, p.pos);
                captions = c > 0;
                if (c)
                    subs_label(c - 1, label, sizeof(label));
                rb->snprintf(msg, sizeof(msg), "Captions: %s", c ? label : "Off");
                osd_toast(msg);
            }
            break;
        case K_AUDIO:
            if (ev == EV_PRESS)
            {
                int tracks[MAX_TRACKS], n = audio_tracks(tracks), i = 0;
                char label[32], msg[48];
                if (n < 2)
                    break;
                while (i < n && tracks[i] != atrack)
                    i++;
                if (!set_audio_track(tracks[(i + 1) % n]))
                    return R_END;
                label_audio(label, sizeof(label));
                rb->snprintf(msg, sizeof(msg), "Audio: %s", label);
                osd_toast(msg);
            }
            break;
        case K_PREV:
        case K_NEXT:
            if (ev == EV_PRESS)
            {
                p.stop_ms = p.pos;
                return k == K_NEXT ? R_NEXT : R_PREV;
            }
            break;
        case K_INFO:
            if (ev == EV_PRESS)
            {
                bool was = p.paused;
                set_paused(true);
                if (!file_info(path))
                    return R_USB;
                set_paused(was);
                refresh();
            }
            break;
        case K_QUIT:
            if (ev == EV_PRESS)
            {
                p.stop_ms = p.pos;
                return R_QUIT;
            }
            break;
        default:
            if (default_event(button) == SYS_USB_CONNECTED)
            {
                p.stop_ms = p.pos;
                return R_USB;
            }
            break;
        }

        if (k == K_MENU && (ev == EV_PRESS || ev == EV_HOLD))
        {
            bool was = p.paused;
            if (p.pending)
                present();
            set_paused(true);
            if (p.seeking)
            {
                p.seeking = false;
                restart_at(p.seek_ms);
            }
            enum result r = menu(path);
            switch (r)
            {
            case R_BACK:
            case R_PLAY:
                set_paused(r == R_PLAY ? false : was);
                refresh();
                break;
            case R_START:
                set_paused(false);
                if (!restart_at(0))
                    return R_END;
                break;
            default:
                p.stop_ms = p.pos;
                return r;
            }
        }
    }
}

/* ---- resume points: "MS PATH" lines, the latest first ---- */

static uint32_t resume_get(const char *path)
{
    char line[MAX_PATH + 16];
    uint32_t ms = 0;
    int fd = rb->open(RESUME_FILE, O_RDONLY);

    if (fd < 0)
        return 0;
    while (rb->read_line(fd, line, sizeof(line)) > 0)
    {
        char *name = rb->strchr(line, ' ');
        if (name && !rb->strcmp(name + 1, path))
        {
            ms = rb->atoi(line);
            break;
        }
    }
    rb->close(fd);
    return ms;
}

/* Put path's point (ms, 0 for none) first; buf holds the others meanwhile */
static void resume_set(const char *path, uint32_t ms, char *buf, size_t size)
{
    char line[MAX_PATH + 16];
    size_t used = 0;
    int n = 0;
    int fd = rb->open(RESUME_FILE, O_RDONLY);

    if (fd >= 0)
    {
        while (n < RESUME_ENTRIES - 1 && rb->read_line(fd, line, sizeof(line)) > 0)
        {
            char *name = rb->strchr(line, ' ');
            size_t len = rb->strlen(line) + 1;
            if (!name || !rb->strcmp(name + 1, path) || used + len > size)
                continue;
            rb->memcpy(buf + used, line, len);
            used += len;
            n++;
        }
        rb->close(fd);
    }
    else if (ms == 0)
        return;
    fd = rb->open(RESUME_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
        return;
    if (ms)
        rb->fdprintf(fd, "%lu %s\n", (unsigned long)ms, path);
    for (size_t i = 0; i < used; i += rb->strlen(buf + i) + 1)
        rb->fdprintf(fd, "%s\n", buf + i);
    rb->close(fd);
}

/* ---- files ---- */

static bool is_video(const char *name)
{
    static const char *const exts[] = { ".avi", ".mkv", ".mka", ".mp4", ".m4v",
                                        ".mov", ".webm" };
    const char *dot = rb->strrchr(name, '.');
    for (unsigned i = 0; dot && i < ARRAYLEN(exts); i++)
        if (!rb->strcasecmp(dot, exts[i]))
            return true;
    return false;
}

/* The video before (dir < 0) or after path in its folder, by name */
static bool sibling(const char *path, int dir, char *out, size_t size)
{
    const char *slash = rb->strrchr(path, '/');
    char folder[MAX_PATH], best[MAX_PATH] = "";
    if (!slash)
        return false;
    rb->strlcpy(folder, path, MIN((size_t)(slash - path) + 2, sizeof(folder)));
    DIR *d = rb->opendir(folder);
    if (!d)
        return false;
    struct dirent *e;
    while ((e = rb->readdir(d)))
    {
        if (!is_video(e->d_name))
            continue;
        int c = rb->strcasecmp(e->d_name, slash + 1);
        if ((dir > 0 && c > 0 && (!*best || rb->strcasecmp(e->d_name, best) < 0)) ||
            (dir < 0 && c < 0 && (!*best || rb->strcasecmp(e->d_name, best) > 0)))
            rb->strlcpy(best, e->d_name, sizeof(best));
    }
    rb->closedir(d);
    if (!*best)
        return false;
    rb->snprintf(out, size, "%s%s", folder, best);
    return true;
}

/* A card over the screen, until a key */
static void message(const char *heading, const char *text)
{
    osd_draw_message(heading, text);
    rb->lcd_update();
    for (;;)
    {
        long button = rb->button_get(true);
        if (IS_SYSEVENT(button))
        {
            if (default_event(button) == SYS_USB_CONNECTED)
                return;
        }
        else if (key_of(button) != K_NONE &&
                 !(button & (BUTTON_REL | BUTTON_REPEAT)))
            return;
    }
}

/* What the file has against what plays; with its picture size when that
 * is what is wrong */
static void unsupported(const char *heading, bool size)
{
    const struct track *vt = &movie.tracks[movie.video];
    const char *sound = "no";
    char dims[16] = "", text[192];

    for (int i = 0; i < movie.ntracks; i++)
    {
        const struct track *t = &movie.tracks[i];
        if (t->type == TRACK_AUDIO && (!rb->strcmp(sound, "no") || t->is_default))
            sound = t->codec[0] ? t->codec : "unknown";
    }
    if (size)
        rb->snprintf(dims, sizeof(dims), "%dx%d ", vt->u.v.width, vt->u.v.height);
    rb->snprintf(text, sizeof(text), "This file: %s%s video, %s sound.\n"
                 "Plays: MJPEG video, or MPEG-1/2 up to %dx%d; MP2, PCM or "
                 "ADPCM sound.", dims, vt->codec[0] ? vt->codec : "unknown",
                 sound, MAX(LCD_WIDTH, LCD_HEIGHT), MIN(LCD_WIDTH, LCD_HEIGHT));
    message(heading, text);
}

/* Why the file does not play */
static void open_error(int err)
{
    char text[64];

    if (err == DEMUX_ERR_VCODEC)
        unsupported("Unsupported format", false);
    else
    {
        rb->snprintf(text, sizeof(text), "%s.", demux_strerror(err));
        message("Can't play this video", text);
    }
}

static enum result play_file(const char *path)
{
    int fd_v = rb->open(path, O_RDONLY);
    int fd_a = rb->open(path, O_RDONLY);
    int fd_t = rb->open(path, O_RDONLY);
    enum result r = R_QUIT;

    memory_reset();
    osd_init();
    /* a plain screen: under the picture, or a message's backdrop */
    rb->lcd_set_viewport(NULL);
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_clear_display();
    rb->lcd_update();
    if (fd_v < 0 || fd_a < 0 || fd_t < 0)
    {
        message("Can't play this video", "The file does not open.");
        goto out;
    }
    int err = demux_open(&movie, fd_v);
    if (err < 0)
    {
        open_error(err);
        goto out;
    }
    if (!setup_memory())
    {
        unsupported("Video too big", true);
        goto out;
    }

    rb->memset(&p, 0, sizeof(p));
    p.frame_ms = movie.frame_us ? MAX((movie.frame_us + 500) / 1000, 1) : 40;
    demux_stream_init(&movie, &vs, fd_v, movie.video);
    setup_audio();
    if (atrack >= 0)
        demux_stream_init(&movie, &as, fd_a, atrack);
    else
        as.fd = fd_a;
    subs_init(&movie, &vs, path, fd_t);
    if (captions)
        subs_select(subs_default(), 0);

    const char *name = rb->strrchr(path, '/');
    if (movie.title[0])
        rb->strlcpy(title, movie.title, sizeof(title));
    else
    {
        rb->strlcpy(title, name ? name + 1 : path, sizeof(title));
        char *dot = rb->strrchr(title, '.');
        if (dot)
            *dot = '\0';
    }

    uint32_t saved = resume_get(path);
    backlight_ignore_timeout();
    boost(true);

    stats_begin();
    r = play(saved, path);
    stats_report(true, p.shown, p.dropped, p.bad,
                 atrack >= 0 ? sound_ring_ms() : 0,
                 sink_latency_ms(), *delay_now());

    if (!p.paused)
        boost(false);
    if (atrack >= 0)
        sound_close();
    subs_close();
    video_close();
    backlight_use_settings();
    if (r == R_END)
        p.stop_ms = 0;
    if (p.stop_ms < RESUME_MIN_MS ||
        (movie.duration && p.stop_ms + RESUME_MIN_MS > movie.duration))
        p.stop_ms = 0;
    if (p.stop_ms || saved)
        resume_set(path, p.stop_ms, (char *)vbuf, vbuf_size);
out:
    if (fd_t >= 0)
        rb->close(fd_t);
    if (fd_a >= 0)
        rb->close(fd_a);
    if (fd_v >= 0)
        rb->close(fd_v);
    return r;
}

enum plugin_status plugin_start(const void *parameter)
{
    static char path[MAX_PATH];

    if (!parameter)
    {
        rb->splash(HZ * 2, "Play a video from the file browser");
        return PLUGIN_OK;
    }
    configfile_load(CFG_FILE, config, ARRAYLEN(config), CFG_VERSION);
    if (rotation < 0 || rotation > ROTATE_NEVER)
        rotation = ROTATE_CW;
    rb->strlcpy(path, parameter, sizeof(path));

    enum plugin_status status = PLUGIN_OK;
    for (;;)
    {
        enum result r = play_file(path);
        if (r == R_USB)
        {
            status = PLUGIN_USB_CONNECTED;
            break;
        }
        if (r == R_QUIT || (r == R_END && then == THEN_STOP) ||
            !sibling(path, r == R_PREV ? -1 : 1, path, sizeof(path)))
            break;
    }
    configfile_save(CFG_FILE, config, ARRAYLEN(config), CFG_VERSION);
    return status;
}
