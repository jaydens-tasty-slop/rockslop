/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Rockbox side of the Gamebuino META compatibility layer: plugin entry,
 * display, keys, start/pause menu, save file and sound output. See
 * ../README. */
#include "plugin.h"
#include "lib/helper.h"
#include "lib/display_text.h"
#include "lib/playback_control.h"
#include "lib/hint_strip.h"
#include "lib/plugin_fpu.h"
#include "gb_glue.h"

extern void exit(int status) NORETURN_ATTR;
extern int rb_atexit(void (*fn)(void));

#define GB_DIR PLUGIN_GAMES_DATA_DIR "/gamebuino"

/* The plugin runtime links only the mem* functions; the games use these. */
size_t strlen(const char *s)
{
    return rb->strlen(s);
}

int strcmp(const char *a, const char *b)
{
    return rb->strcmp(a, b);
}

char *strcpy(char *d, const char *s)
{
    return rb->strcpy(d, s);
}

/* ---- settings ---------------------------------------------------------- */

static struct {
    int sound;
} cfg = {1};

static void file_name(char *buf, size_t n, const char *ext)
{
    rb->snprintf(buf, n, GB_DIR "/%s.%s", gb_game.name, ext);
}

static void make_dirs(void)
{
    rb->mkdir(PLUGIN_GAMES_DATA_DIR);
    rb->mkdir(GB_DIR);
}

static void cfg_load(void)
{
    char path[MAX_PATH];
    unsigned char b;
    file_name(path, sizeof(path), "cfg");
    int fd = rb->open(path, O_RDONLY);
    if (fd >= 0) {
        if (rb->read(fd, &b, 1) == 1)
            cfg.sound = b != 0;
        rb->close(fd);
    }
}

static void cfg_save(void)
{
    char path[MAX_PATH];
    unsigned char b = cfg.sound;
    make_dirs();
    file_name(path, sizeof(path), "cfg");
    int fd = rb->open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0) {
        rb->write(fd, &b, 1);
        rb->close(fd);
    }
}

/* ---- save data --------------------------------------------------------- */

uint8_t gb_save_data[GB_SAVE_MAX];
static size_t save_len;
static bool save_loaded, save_dirty;
static long save_changed;

void gb_save_load(size_t len)
{
    char path[MAX_PATH];
    save_len = len;
    save_loaded = false;
    file_name(path, sizeof(path), "sav");
    int fd = rb->open(path, O_RDONLY);
    if (fd >= 0) {
        save_loaded = rb->ffilesize(fd) == (off_t)len
                      && rb->read(fd, gb_save_data, len) == (ssize_t)len;
        rb->close(fd);
    }
}

int gb_save_loaded(void)
{
    return save_loaded;
}

static void save_flush(void)
{
    char path[MAX_PATH];
    if (!save_dirty)
        return;
    save_dirty = false;
    make_dirs();
    file_name(path, sizeof(path), "sav");
    int fd = rb->open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0) {
        rb->write(fd, gb_save_data, save_len);
        rb->close(fd);
    }
}

void gb_save_dirty(void)
{
    save_dirty = true;
    save_changed = *rb->current_tick;
}

int gb_data_read(const char *name, uint32_t offset, void *dst, size_t len)
{
    char path[MAX_PATH];
    rb->snprintf(path, sizeof(path), GB_DIR "/%s/%s", gb_game.name, name);
    int fd = rb->open(path, O_RDONLY);
    if (fd < 0)
        return -1;
    ssize_t n = -1;
    if (rb->lseek(fd, offset, SEEK_SET) == (off_t)offset)
        n = rb->read(fd, dst, len);
    rb->close(fd);
    return n;
}

/* Games keep files open for good (picture banks, gb_pics.h) and never close
 * them: a bit per fd handed out, and cleanup() closes what is left. */
static uint32_t open_fds;

int gb_file_open(const char *name)
{
    char path[MAX_PATH];
    rb->snprintf(path, sizeof(path), GB_DIR "/%s/%s", gb_game.name, name);
    int fd = rb->open(path, O_RDONLY);
    if (fd >= 0 && fd < 32)
        open_fds |= 1u << fd;
    return fd;
}

int gb_file_read(int fd, void *dst, size_t len)
{
    return rb->read(fd, dst, len);
}

long gb_file_seek(int fd, long offset, int whence)
{
    return rb->lseek(fd, offset, whence);
}

void gb_file_close(int fd)
{
    if (fd >= 0 && fd < 32)
        open_fds &= ~(1u << fd);
    if (fd >= 0)
        rb->close(fd);
}

void *gb_big_buffer(size_t *size)
{
    return rb->plugin_get_audio_buffer(size);
}

void gb_fatal(const char *msg)
{
    rb->splash(3 * HZ, msg);
    exit(PLUGIN_ERROR);
}

int gb_vformat(char *buf, size_t len, const char *fmt, va_list ap)
{
    return rb->vsnprintf(buf, len, fmt, ap);
}

/* ---- time and random --------------------------------------------------- */

static long tick_base;      /* tick at plugin start */
static long paused_ticks;   /* time spent in the menu */

unsigned long gb_millis(void)
{
    return (unsigned long)(*rb->current_tick - tick_base - paused_ticks) * 10;
}

static uint32_t rng_state = 0x2545f491;

void gb_srand(uint32_t seed)
{
    rng_state = seed ? seed : 0x2545f491;
}

uint32_t gb_rand(void)
{
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return rng_state = x;
}

void gb_idle(void)
{
    rb->sleep(1);
    gb_pump();
}

void *gb_heap_area(size_t *size)
{
    return rb->plugin_get_buffer(size);
}

/* ---- keys -------------------------------------------------------------- */

/* SELECT = A, SELECT+DOWN = B, SELECT+LEFT = MENU. */
static const struct gb_rule default_keys[] = {
    {GB_P_SELECT | GB_P_DOWN, GB_B_B},
    {GB_P_SELECT | GB_P_LEFT, GB_B_MENU},
    {GB_P_SELECT,             GB_B_A},
    {GB_P_UP,                 GB_B_UP},
    {GB_P_DOWN,               GB_B_DOWN},
    {GB_P_LEFT,               GB_B_LEFT},
    {GB_P_RIGHT,              GB_B_RIGHT},
    {0, 0}
};

static long select_since = -1;
static long menu_since = -1;
static bool menu_requested, menu_fired;

static unsigned last_set;
static long set_since;

static unsigned phys_keys(void)
{
    unsigned s = rb->button_status() & BUTTON_MAIN;
    long now = *rb->current_tick;
    if (s != last_set) {
        last_set = s;
        set_since = now;
    }
    unsigned long_ms = gb_game.long_ms ? gb_game.long_ms : 400;
    if (s & BUTTON_SELECT) {
        if (select_since < 0)
            select_since = now;
        if ((now - select_since) * 10 >= (long)long_ms)
            s |= GB_P_LONG;
    } else {
        select_since = -1;
    }
    return s;
}

/* The pause gesture: SELECT+UP unless the game says otherwise, held 500 ms
 * (1000 ms for a game where the chord is also a gameplay combo). SELECT is
 * also the power key (held ~1 s powers off), hence a chord and not a hold. */
static unsigned menu_want(void)
{
    return gb_game.menu_keys ? gb_game.menu_keys : GB_P_SELECT | GB_P_UP;
}

static unsigned menu_time(void)
{
    return gb_game.menu_ms ? gb_game.menu_ms : 500;
}

static void menu_check(unsigned s)
{
    unsigned want = menu_want();
    unsigned ms = menu_time();
    if ((s & want) == want) {
        long now = *rb->current_tick;
        if (menu_since < 0)
            menu_since = now;
        else if (!menu_fired && (now - menu_since) * 10 >= (long)ms) {
            menu_fired = true;
            menu_requested = true;
        }
    } else {
        menu_since = -1;
        menu_fired = false;
    }
}

uint8_t gb_buttons(void)
{
    unsigned s = phys_keys();
    uint8_t out = 0;
    unsigned used = 0;
    const struct gb_rule *keys = gb_game.keys ? gb_game.keys : default_keys;
    const struct gb_rule *r;
    /* A rule on SELECT alone that is a prefix of a SELECT chord waits a
     * moment, so a chord pressed a few ms apart does not fire it first. */
    long grace = (gb_game.chord_ms ? gb_game.chord_ms : 30) / 10;
    bool menu_chord = (menu_want() & GB_P_SELECT) && menu_want() != GB_P_SELECT;
    for (r = keys; r->phys; r++) {
        if ((s & r->phys) == r->phys && !(used & r->phys & ~GB_P_LONG)) {
            if (r->phys == GB_P_SELECT && s == GB_P_SELECT
                    && *rb->current_tick - set_since < grace) {
                if (menu_chord)
                    goto next;
                for (const struct gb_rule *c = keys; c->phys; c++) {
                    if (c->phys != r->phys && (c->phys & GB_P_SELECT)
                            && !(c->phys & GB_P_LONG))
                        goto next;
                }
            }
            out |= r->out;
            used |= r->phys;
        }
next:   ;
    }
    if (gb_game.view == GB_VIEW_ROT90) {
        /* The player is held turned clockwise: the physical D-pad is
         * rotated against the screen. */
        uint8_t d = out & (GB_B_UP | GB_B_DOWN | GB_B_LEFT | GB_B_RIGHT);
        out &= ~d;
        if (d & GB_B_LEFT)  out |= GB_B_UP;
        if (d & GB_B_UP)    out |= GB_B_RIGHT;
        if (d & GB_B_RIGHT) out |= GB_B_DOWN;
        if (d & GB_B_DOWN)  out |= GB_B_LEFT;
    }
#ifdef HAVE_BLUETOOTH_HID
    /* Game controller: A, B and the META menu key (X) */
    unsigned pad = rb->button_status();
    if (pad & BUTTON_PAD_A) out |= GB_B_A;
    if (pad & BUTTON_PAD_B) out |= GB_B_B;
    if (pad & BUTTON_PAD_X) out |= GB_B_MENU;
#endif
    menu_check(s);
    /* A pause chord that is not a gameplay combo (menu_ms < 1000) presses
     * nothing in the game while it is held. */
    if ((s & menu_want()) == menu_want() && menu_time() < 1000)
        out = 0;
    return out;
}

/* ---- display ----------------------------------------------------------- */

#define BAND 8

static fb_data line_buf[LCD_WIDTH * BAND];
static uint8_t xmap[LCD_WIDTH];
static uint8_t ymap[LCD_HEIGHT];
static int view_w, view_h, view_y;

#if LCD_PIXELFORMAT == RGB565SWAPPED
#define LCD_FROM_565(v) ((fb_data)((((v) & 0xff) << 8) | ((v) >> 8)))
#else
#define LCD_FROM_565(v) ((fb_data)(v))
#endif

static void draw_background(void)
{
    hint_strip_draw(gb_game.hint, view_y + view_h);
    rb->lcd_update();
}

static void view_init(void)
{
    if (gb_game.view == GB_VIEW_ROT90) {
        view_w = LCD_WIDTH;
        view_h = LCD_HEIGHT;
        view_y = 0;
    } else {
        view_w = LCD_WIDTH;
        view_h = gb_game.height * LCD_WIDTH / gb_game.width;
        if (view_h > LCD_HEIGHT)
            view_h = LCD_HEIGHT;
        view_y = (LCD_HEIGHT - view_h) / 2;
        for (int x = 0; x < view_w; x++)
            xmap[x] = x * gb_game.width / view_w;
        for (int y = 0; y < view_h; y++)
            ymap[y] = y * gb_game.height / view_h;
    }
}

void gb_present(const uint8_t *fb, const uint16_t *palette, int indexed)
{
    fb_data pal[16];
    const int w = gb_game.width;
    const bool idx = indexed;
    if (idx) {
        for (int i = 0; i < 16; i++)
            pal[i] = LCD_FROM_565(palette[i]);
    }
    for (int y0 = 0; y0 < view_h; y0 += BAND) {
        int n = view_h - y0 < BAND ? view_h - y0 : BAND;
        for (int j = 0; j < n; j++) {
            fb_data *d = line_buf + j * view_w;
            int y = y0 + j;
            if (gb_game.view == GB_VIEW_ROT90) {
                /* LCD (x, y) shows game pixel (159 - y, x) */
                int sx = (gb_game.width - 1) - y;
                for (int x = 0; x < view_w; x++) {
                    if (idx) {
                        {
                        int p = x * w + sx;
                        d[x] = pal[(p & 1 ? fb[p >> 1] : fb[p >> 1] >> 4) & 15];
                    }
                    } else {
                        uint16_t v = ((const uint16_t *)fb)[x * w + sx];
                        d[x] = LCD_FROM_565(v);
                    }
                }
            } else if (idx) {
                int row = ymap[y] * w;
                for (int x = 0; x < view_w; x++) {
                    int p = row + xmap[x];
                    d[x] = pal[(p & 1 ? fb[p >> 1] : fb[p >> 1] >> 4) & 15];
                }
            } else {
                const uint16_t *s = (const uint16_t *)fb + ymap[y] * w;
                for (int x = 0; x < view_w; x++)
                    d[x] = LCD_FROM_565(s[xmap[x]]);
            }
        }
        rb->lcd_bitmap(line_buf, 0, view_y + y0, view_w, n);
    }
    rb->lcd_update_rect(0, view_y, view_w, view_h);
    gb_pump();
}

static inline fb_data from_swapped(uint16_t v)
{
    uint16_t native = (uint16_t)((v << 8) | (v >> 8));
    return LCD_FROM_565(native);
}

void gb_blit_rect(int x, int y, int w, int h, const uint16_t *s)
{
    if (gb_game.view != GB_VIEW_ROT90) {
        for (int r = 0; r < h; r++) {
            for (int i = 0; i < w; i++) {
                uint16_t v = s[r * w + i];
                line_buf[i] = from_swapped(v);
            }
            rb->lcd_bitmap(line_buf, x, y + view_y + r, w, 1);
        }
        rb->lcd_update_rect(x, y + view_y, w, h);
        return;
    }
    /* game (sx, sy) shows at LCD (sy, W - 1 - sx): the rect becomes h wide
     * and w tall, sent in bands that fit line_buf */
    int lx = y, ly = gb_game.width - x - w;
    int rows = (int)(sizeof(line_buf) / sizeof(line_buf[0])) / h;
    for (int r0 = 0; r0 < w; r0 += rows) {
        int n = w - r0 < rows ? w - r0 : rows;
        for (int r = 0; r < n; r++) {
            const uint16_t *src = s + (w - 1 - (r0 + r));
            fb_data *d = line_buf + r * h;
            for (int j = 0; j < h; j++) {
                uint16_t v = src[j * w];
                d[j] = from_swapped(v);
            }
        }
        rb->lcd_bitmap(line_buf, lx, ly + r0, h, n);
    }
    rb->lcd_update_rect(lx, ly, h, w);
}

/* ---- audio ------------------------------------------------------------- */

#define AUDIO_FRAMES 256
static int16_t mono_buf[AUDIO_FRAMES];
static int16_t stereo_buf[AUDIO_FRAMES * 2];

/* Music: a ring buffer filled from the game thread (gb_pump) and drained
 * by the mixer callback, which resamples 8-bit mono to the mixer rate. */
static struct {
    uint8_t *ring;
    size_t size;                /* ring size */
    volatile uint32_t rd, wr;   /* free-running byte counters */
    volatile int active;
    volatile int fd;            /* -1 once the whole file is read */
    int loop;
    uint32_t data_start, remain, data_len;
    uint32_t rate, acc;
    int cur;
} mus;

static void music_mix(int16_t *buf, int frames, uint32_t mixrate)
{
    for (int i = 0; i < frames; i++) {
        mus.acc += mus.rate;
        while (mus.acc >= mixrate) {
            mus.acc -= mixrate;
            if (mus.wr != mus.rd) {
                mus.cur = ((int)mus.ring[mus.rd % mus.size] - 128) * 48;
                mus.rd++;
            } else {
                mus.cur = 0;        /* underrun or end of the file */
                if (mus.fd < 0)
                    mus.active = 0;
            }
        }
        int v = buf[i] + mus.cur;
        buf[i] = v > 32767 ? 32767 : v < -32768 ? -32768 : v;
    }
}

static void audio_get_more(const void **start, size_t *size)
{
    int fx = gb_sound_fill(mono_buf, AUDIO_FRAMES, rb->mixer_get_frequency());
    if (!fx && !mus.active) {
        *start = NULL;
        *size = 0;
        return;
    }
    if (mus.active)
        music_mix(mono_buf, AUDIO_FRAMES, rb->mixer_get_frequency());
    for (int i = 0; i < AUDIO_FRAMES; i++)
        stereo_buf[2 * i] = stereo_buf[2 * i + 1] = mono_buf[i];
    *start = stereo_buf;
    *size = sizeof(stereo_buf);
}

void gb_audio_start(void)
{
    static const struct mixer_play_cbs cbs = {.get_more = audio_get_more};
    if (rb->mixer_channel_status(PCM_MIXER_CHAN_BEEP) != CHANNEL_PLAYING) {
        rb->mixer_channel_set_amplitude(PCM_MIXER_CHAN_BEEP, MIX_AMP_UNITY);
        rb->mixer_channel_play_data(PCM_MIXER_CHAN_BEEP, &cbs, NULL, 0);
    }
}

void gb_audio_lock(int lock)
{
    if (lock)
        rb->pcm_play_lock();
    else
        rb->pcm_play_unlock();
}

int gb_audio_enabled(void)
{
    return cfg.sound;
}

void gb_music_buffer(void *buf, size_t size)
{
    mus.ring = buf;
    mus.size = size;
    mus.fd = -1;
}

void gb_music_stop(void)
{
    gb_audio_lock(1);
    mus.active = 0;
    gb_audio_lock(0);
    if (mus.ring && mus.fd >= 0) {
        rb->close(mus.fd);
        mus.fd = -1;
    }
}

static uint32_t le32(const uint8_t *p)
{
    return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24;
}

/* Finds the sample data of an 8-bit mono WAV; false if it is another kind. */
static bool wav_open(int fd)
{
    uint8_t h[12], c[8];
    uint32_t pos = 12, rate = 0;
    if (rb->read(fd, h, 12) != 12 || rb->memcmp(h, "RIFF", 4)
            || rb->memcmp(h + 8, "WAVE", 4))
        return false;
    for (;;) {
        if (rb->lseek(fd, pos, SEEK_SET) != (off_t)pos
                || rb->read(fd, c, 8) != 8)
            return false;
        uint32_t len = le32(c + 4);
        if (!rb->memcmp(c, "fmt ", 4)) {
            uint8_t f[16];
            if (len < 16 || rb->read(fd, f, 16) != 16)
                return false;
            if (f[0] != 1 || f[2] != 1 || f[14] != 8)   /* PCM, mono, 8 bit */
                return false;
            rate = le32(f + 4);
        } else if (!rb->memcmp(c, "data", 4)) {
            if (!rate)
                return false;
            mus.rate = rate;
            mus.data_start = pos + 8;
            mus.data_len = mus.remain = len;
            return true;
        }
        pos += 8 + len + (len & 1);
    }
}

void gb_quit(void)
{
    exit(PLUGIN_OK);
}

int gb_music_playing(void)
{
    return mus.active;
}

int gb_music_play(const char *name, int loop)
{
    char path[MAX_PATH];
    gb_music_stop();
    if (!cfg.sound || !mus.ring)
        return -1;
    rb->snprintf(path, sizeof(path), GB_DIR "/%s/%s", gb_game.name, name);
    int fd = rb->open(path, O_RDONLY);
    if (fd < 0)
        return -1;
    if (!wav_open(fd)) {
        rb->close(fd);
        return -1;
    }
    mus.rd = mus.wr = 0;
    mus.acc = 0;
    mus.cur = 0;
    mus.loop = loop;
    rb->lseek(fd, mus.data_start, SEEK_SET);
    mus.fd = fd;
    mus.active = 1;
    gb_audio_start();
    return 0;
}

/* Keep the ring full; called from the game thread. */
static void music_pump(void)
{
    if (!mus.active || mus.fd < 0)
        return;
    gb_audio_start();
    for (;;) {
        uint32_t used = mus.wr - mus.rd;
        size_t free_b = mus.size - used;
        size_t at = mus.wr % mus.size;
        size_t n = mus.size - at;           /* contiguous space */
        if (free_b < 2048)
            return;
        if (n > free_b)
            n = free_b;
        if (n > 4096)
            n = 4096;
        if (n > mus.remain)
            n = mus.remain;
        if (n) {
            ssize_t r = rb->read(mus.fd, mus.ring + at, n);
            if (r <= 0)
                mus.remain = 0;
            else {
                mus.remain -= r;
                mus.wr += r;
            }
        }
        if (!mus.remain) {
            if (mus.loop) {
                rb->lseek(mus.fd, mus.data_start, SEEK_SET);
                mus.remain = mus.data_len;
            } else {
                rb->close(mus.fd);
                mus.fd = -1;
                return;
            }
        }
    }
}

/* ---- menus ------------------------------------------------------------- */

#define MAX_WORDS 220
#define TEXT_BYTES 2400

static const char *const layer_credits[] = {
    "Gamebuino META layer for Rockbox:", "",
    "own implementation of the Gamebuino-Meta API",
    "(github.com/Gamebuino/Gamebuino-Meta, LGPL-3.0).", "",
    "Rockbox port: YP3.",
    NULL
};

static bool show_text(const char *title, const char *const *a,
                      const char *const *b)
{
    static char text[TEXT_BYTES];
    static char *words[MAX_WORDS];
    static struct style_text style[] = {
        {0, TEXT_CENTER | TEXT_UNDERLINE}, LAST_STYLE_ITEM};
    unsigned nw = 0, pos = 0;
    const char *const *lists[2] = {a, b};

    words[nw++] = (char *)title;
    words[nw++] = (char *)"";
    for (int l = 0; l < 2; l++) {
        for (const char *const *ln = lists[l]; ln && *ln; ln++) {
            const char *s = *ln;
            if (!*s && nw < MAX_WORDS)
                words[nw++] = (char *)"";
            while (*s && nw < MAX_WORDS) {
                while (*s == ' ')
                    s++;
                if (!*s)
                    break;
                unsigned n = 0;
                while (s[n] && s[n] != ' ')
                    n++;
                if (pos + n + 1 > TEXT_BYTES)
                    break;
                rb->memcpy(text + pos, s, n);
                text[pos + n] = 0;
                words[nw++] = text + pos;
                pos += n + 1;
                s += n;
            }
            if (nw < MAX_WORDS)
                words[nw++] = (char *)"";
        }
    }
    rb->lcd_setfont(FONT_UI);
    bool usb = display_text(nw, words, style, NULL, true);
    rb->lcd_setfont(FONT_SYSFIXED);
    return usb;
}

static void settings_menu(void)
{
    static const struct opt_items onoff[] = {{"Off", -1}, {"On", -1}};
    MENUITEM_STRINGLIST(m, "Settings", NULL, "Sound");
    int sel = 0;
    for (;;) {
        switch (rb->do_menu(&m, &sel, NULL, false)) {
        case 0:
            rb->set_option("Sound", &cfg.sound, RB_INT, onoff, 2, NULL);
            break;
        case MENU_ATTACHED_USB:
            cfg_save();
            exit(PLUGIN_USB_CONNECTED);
        default:
            cfg_save();
            return;
        }
    }
}

static char menu_title[48];

/* Returns when the player chooses Play/Resume; exits the plugin on Quit. */
static void game_menu(bool resume)
{
    int sel = 0;
    long t0 = *rb->current_tick;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    backlight_use_settings();
    save_flush();
    rb->lcd_setfont(FONT_UI);

    rb->strlcpy(menu_title, gb_game.title, sizeof(menu_title));
    MENUITEM_STRINGLIST(start_menu, menu_title, NULL, "Play", "Settings",
                        "Help", "About", "Playback Control", "Quit");
    MENUITEM_STRINGLIST(pause_menu, menu_title, NULL, "Resume", "Settings",
                        "Help", "About", "Playback Control", "Quit");
    for (;;) {
        switch (rb->do_menu(resume ? &pause_menu : &start_menu, &sel, NULL,
                            false)) {
        case 0:
            goto done;
        case 1:
            settings_menu();
            break;
        case 2:
            if (show_text("Help", gb_game.help, NULL))
                exit(PLUGIN_USB_CONNECTED);
            break;
        case 3:
            if (show_text(gb_game.title, gb_game.credits, layer_credits))
                exit(PLUGIN_USB_CONNECTED);
            break;
        case 4:
            if (playback_control(NULL))
                exit(PLUGIN_USB_CONNECTED);
            break;
        case MENU_ATTACHED_USB:
            exit(PLUGIN_USB_CONNECTED);
        default: /* Quit or cancel */
            exit(PLUGIN_OK);
        }
    }
done:
    cfg_save();
    rb->lcd_setfont(FONT_SYSFIXED);
    backlight_ignore_timeout();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    draw_background();
    rb->button_clear_queue();
    paused_ticks += *rb->current_tick - t0;
    menu_requested = menu_fired = false;
    menu_since = select_since = -1;
}

void gb_pump(void)
{
    long btn;
    while ((btn = rb->button_get(false)) != BUTTON_NONE) {
        /* SELECT doubles as the power key and may be held in play: the
         * pause menu is the way out. */
        if (btn == SYS_POWEROFF)
            continue;
        if (rb->default_event_handler(btn) == SYS_USB_CONNECTED)
            exit(PLUGIN_USB_CONNECTED);
    }
#ifdef HAVE_BLUETOOTH_HID
    /* Game controller: Options opens the pause menu, PS leaves */
    static bool pad_menu_down;
    unsigned pad = rb->button_status();
    if (pad & BUTTON_PAD_HOME)
        gb_quit();
    if (pad & BUTTON_PAD_MENU) {
        if (!pad_menu_down)
            menu_requested = true;
        pad_menu_down = true;
    } else {
        pad_menu_down = false;
    }
#endif
    music_pump();
    /* A game in a long animation or delay() reads no buttons: check the
     * pause chord here too, or one held then is never seen. */
    menu_check(rb->button_status() & BUTTON_MAIN);
    if (menu_requested)
        game_menu(true);
    if (save_dirty && *rb->current_tick - save_changed > 3 * HZ)
        save_flush();
}

/* ---- entry ------------------------------------------------------------- */

static void cleanup(void)
{
    rb->mixer_channel_stop(PCM_MIXER_CHAN_BEEP);
    gb_music_stop();
    for (int fd = 0; fd < 32; fd++)
        if (open_fds & (1u << fd))
            gb_file_close(fd);
    save_flush();
    cfg_save();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    backlight_use_settings();
    plugin_fpu_enable(false);
}

/* plugin_crt0 runs the game's C++ static constructors before plugin_start,
 * and they may do float maths (Gamebuino Bang Bang's cannons, Apollo's
 * tables): FP access on first, or they fault (NOCP). cleanup() turns it off. */
static void __attribute__((constructor(101))) fpu_before_constructors(void)
{
    plugin_fpu_enable(true);
}

enum plugin_status plugin_start(const void *param)
{
    (void)param;
    cfg_load();
    rb_atexit(cleanup);
    plugin_fpu_enable(true);
    tick_base = *rb->current_tick;
    gb_srand(*rb->current_tick * 2654435761u);
    view_init();
    game_menu(false);
    setup();
    for (;;)
        loop();
}
