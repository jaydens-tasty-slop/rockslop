/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Rockbox side of the PokittoLib compatibility layer: plugin entry,
 * display, keys, start/pause menu, files, sample streaming. */
#include "plugin.h"
#include "lib/helper.h"
#include "lib/display_text.h"
#include "lib/playback_control.h"
#include "lib/hint_strip.h"
#include "lib/plugin_fpu.h"
#include "pk_glue.h"

extern void exit(int status);
extern int rb_atexit(void (*fn)(void));

#define DATA_DIR PLUGIN_GAMES_DATA_DIR "/%s"

/* ---- settings ---------------------------------------------------------- */

static struct {
    int sound;
} cfg = {1};

static void data_path(char *buf, size_t n, const char *rel)
{
    rb->snprintf(buf, n, DATA_DIR "/%s", pk_game.name, rel);
}

static void cfg_load(void)
{
    uint8_t b;
    if (pk_file_load("pokitto.cfg", &b, 1))
        cfg.sound = b != 0;
}

static void cfg_save(void)
{
    uint8_t b = cfg.sound;
    pk_file_save("pokitto.cfg", &b, 1);
}

/* ---- files ------------------------------------------------------------- */

int pk_file_exists(const char *rel)
{
    char path[MAX_PATH];
    data_path(path, sizeof(path), rel);
    return rb->file_exists(path);
}

static DIR *listing;

int pk_dir_open(const char *rel)
{
    char path[MAX_PATH];
    if (listing)
        rb->closedir(listing);
    data_path(path, sizeof(path), rel);
    listing = rb->opendir(path);
    return listing != NULL;
}

int pk_dir_next(char *name, size_t n)
{
    struct dirent *e;
    while (listing && (e = rb->readdir(listing))) {
        if (!(rb->dir_get_info(listing, e).attribute & ATTR_DIRECTORY) ||
            e->d_name[0] == '.')
            continue;
        rb->strlcpy(name, e->d_name, n);
        return 1;
    }
    if (listing)
        rb->closedir(listing);
    listing = NULL;
    return 0;
}

/* Games keep files open in objects that are never destroyed (Le Word's
 * File members): a bit per fd handed out, and cleanup() closes what is
 * left. */
static uint32_t open_fds;

static int track_open(int fd)
{
    if (fd >= 0 && fd < 32)
        open_fds |= 1u << fd;
    return fd;
}

int pk_file_open(const char *rel)
{
    char path[MAX_PATH];
    data_path(path, sizeof(path), rel);
    return track_open(rb->open(path, O_RDONLY));
}

int pk_file_create(const char *rel, int truncate)
{
    char path[MAX_PATH];
    rb->mkdir(PLUGIN_GAMES_DATA_DIR);
    rb->snprintf(path, sizeof(path), DATA_DIR, pk_game.name);
    rb->mkdir(path);
    data_path(path, sizeof(path), rel);
    return track_open(rb->open(path, O_RDWR | O_CREAT | (truncate ? O_TRUNC : 0), 0666));
}

int pk_file_write(int fd, const void *buf, size_t n)
{
    return rb->write(fd, buf, n);
}

int pk_file_read(int fd, void *buf, size_t n)
{
    return rb->read(fd, buf, n);
}

void pk_file_seek(int fd, long pos)
{
    rb->lseek(fd, pos, SEEK_SET);
}

void pk_file_close(int fd)
{
    if (fd >= 0 && fd < 32)
        open_fds &= ~(1u << fd);
    rb->close(fd);
}

int pk_file_load(const char *rel, void *buf, size_t size)
{
    char path[MAX_PATH];
    data_path(path, sizeof(path), rel);
    int fd = rb->open(path, O_RDONLY);
    if (fd < 0)
        return 0;
    int ok = rb->read(fd, buf, size) == (ssize_t)size;
    rb->close(fd);
    return ok;
}

void pk_file_save(const char *rel, const void *buf, size_t size)
{
    char path[MAX_PATH];
    rb->mkdir(PLUGIN_GAMES_DATA_DIR);
    rb->snprintf(path, sizeof(path), DATA_DIR, pk_game.name);
    rb->mkdir(path);
    data_path(path, sizeof(path), rel);
    int fd = rb->open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0) {
        rb->write(fd, buf, size);
        rb->close(fd);
    }
}

const uint8_t *pk_assets_load(const char *rel)
{
    char path[MAX_PATH];
    size_t size;
    uint8_t *buf = rb->plugin_get_audio_buffer(&size);
    data_path(path, sizeof(path), rel);
    int fd = rb->open(path, O_RDONLY);
    off_t n = fd >= 0 ? rb->lseek(fd, 0, SEEK_END) : -1;
    if (n < 0 || (size_t)n > size || rb->lseek(fd, 0, SEEK_SET) != 0 ||
        rb->read(fd, buf, n) != n) {
        rb->splashf(HZ * 3, "Missing or too large: %s", path);
        if (fd >= 0)
            rb->close(fd);
        exit(PLUGIN_ERROR);
    }
    rb->close(fd);
    return buf;
}

/* ---- time -------------------------------------------------------------- */

static long tick_base;      /* tick at plugin start */
static long paused_ticks;   /* time spent in the menu */

long pk_ticks(void)
{
    if (!rb)                /* a game's global constructor ran first */
        return 0;
    return *rb->current_tick - tick_base - paused_ticks;
}

void pk_idle(void)
{
    pk_audio_poll();
    rb->sleep(1);
    pk_pump();
}

/* ---- keys -------------------------------------------------------------- */

static const struct pk_rule default_rules[] = {
    {PK_P_SELECT | PK_P_DOWN, PK_B_B},
    {PK_P_SELECT,             PK_B_A},
    {PK_P_UP,                 PK_B_UP},
    {PK_P_DOWN,               PK_B_DOWN},
    {PK_P_LEFT,               PK_B_LEFT},
    {PK_P_RIGHT,              PK_B_RIGHT},
    {0, 0}
};
static const struct pk_keys default_keys = {default_rules, 0, 0, 0};

static long menu_since = -1;
static bool menu_requested, menu_fired;

static bool menu_check(unsigned s)
{
    /* The menu gesture is holding SELECT+UP; true while the chord is down
     * (the caller then hides the keys from the game). */
    unsigned want = pk_game.menu_keys ? pk_game.menu_keys
                                      : PK_P_SELECT | PK_P_UP;
    unsigned ms = pk_game.menu_ms ? pk_game.menu_ms : 500;
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
        return false;
    }
    return true;
}

static bool sel_chord, sel_holdfired;
static long sel_since;

uint8_t pk_buttons(void)
{
    unsigned raw = rb->button_status() & BUTTON_MAIN;
    unsigned s = raw;
    uint8_t out = 0;
    unsigned used = 0;
    const struct pk_keys *k = pk_game.keys ? pk_game.keys() : &default_keys;
    const struct pk_rule *r = k->rules;

    if (k->tap || k->hold) {
        long now = *rb->current_tick;
        long hold = (k->hold_ms ? k->hold_ms : 450) / (1000 / PK_HZ);
        if (raw & BUTTON_SELECT) {
            if (!sel_since) {
                sel_since = now ? now : 1;
                sel_chord = sel_holdfired = false;
            }
            if (raw & ~BUTTON_SELECT)
                sel_chord = true;
            if (!sel_chord)
                s &= ~BUTTON_SELECT;
            if (!sel_chord && !sel_holdfired && now - sel_since >= hold) {
                sel_holdfired = true;
                out |= k->hold;
            }
        } else if (sel_since) {
            if (!sel_chord && !sel_holdfired)
                out |= k->tap;
            sel_since = 0;
        }
    }
    for (; r->phys; r++) {
        if ((s & r->phys) == r->phys && !(used & r->phys)) {
            out |= r->out;
            used |= r->phys;
        }
    }
#ifdef HAVE_BLUETOOTH_HID
    /* Game controller: A, B and C (Y) */
    unsigned pad = rb->button_status();
    if (pad & BUTTON_PAD_A) out |= PK_B_A;
    if (pad & BUTTON_PAD_B) out |= PK_B_B;
    if (pad & BUTTON_PAD_Y) out |= PK_B_C;
#endif
    return menu_check(raw) ? 0 : out;
}

/* ---- display ----------------------------------------------------------- */

static int vp_w = LCD_WIDTH, vp_h = LCD_HEIGHT;
static int vp_x, vp_y;

static void draw_background(void)
{
    hint_strip_draw(pk_game.hint, vp_y + vp_h);
    rb->lcd_update();
}

void pk_viewport(int w, int h)
{
    vp_w = w;
    vp_h = h;
    vp_x = (LCD_WIDTH - w) / 2;
    vp_y = (LCD_HEIGHT - h) / 2;
    draw_background();
}

uint16_t pk_rgb(int r, int g, int b)
{
    return LCD_RGBPACK(r, g, b);
}

void pk_blit(const uint16_t *px, int y, int w, int h)
{
    rb->lcd_bitmap((const fb_data *)px, vp_x, vp_y + y, w, h);
}

void pk_flush(void)
{
    rb->lcd_update_rect(vp_x, vp_y, vp_w, vp_h);
}

/* ---- sample streaming -------------------------------------------------- */

/* Each voice streams unsigned 8-bit mono samples from a file into a ring
 * that the mixer callback drains (resampled to the mixer rate). File reads
 * happen only in pk_audio_poll(), never in the callback. */
#define RING 2048
#define AUDIO_FRAMES 256

struct voice {
    int fd;
    bool active, loop, eof, paused;
    uint32_t step;              /* 16.16 source samples per output frame */
    int vol;                    /* 0..256 */
    volatile uint32_t wr;       /* samples written, total */
    volatile uint32_t idx;      /* read position, samples */
    uint32_t frac;              /* 16-bit fraction of idx */
    uint8_t ring[RING];
};

static struct voice voices[PK_VOICES];

/* One synthesised tone (Pokitto's synth notes). */
static struct {
    uint32_t phase, inc;        /* 32-bit phase, per output frame */
    int wave, vol;
    uint32_t left, total;       /* output frames */
    uint32_t noise;
} tone;

static int16_t out_buf[AUDIO_FRAMES * 2];

static void audio_get_more(const void **start, size_t *size)
{
    int active = 0;
    for (int f = 0; f < AUDIO_FRAMES; f++) {
        int sum = 0;
        if (tone.left) {
            int s;
            tone.phase += tone.inc;
            switch (tone.wave) {
            case 2:  s = (int)(tone.phase >> 16) - 32768; break;        /* saw */
            case 3: {                                                   /* triangle */
                int p = tone.phase >> 16;
                s = (p < 32768 ? p : 65535 - p) * 2 - 32768;
                break;
            }
            case 4:  tone.noise = tone.noise * 1664525 + 1013904223;
                     s = (int)(tone.noise >> 16) - 32768; break;
            default: s = tone.phase & 0x80000000u ? 32767 : -32768;     /* square */
            }
            sum += ((s >> 2) * tone.vol >> 8) * (int)tone.left / (int)tone.total;
            tone.left--;
            active++;
        }
        for (int v = 0; v < PK_VOICES; v++) {
            struct voice *vc = &voices[v];
            if (!vc->active || vc->paused)
                continue;
            uint32_t idx = vc->idx;
            if (idx + 1 >= vc->wr) {
                if (vc->eof)
                    vc->active = false;
                active++;
                continue;       /* starved: silence */
            }
            int a = vc->ring[idx & (RING - 1)];
            int b = vc->ring[(idx + 1) & (RING - 1)];
            int fr = vc->frac >> 8;
            sum += (((a << 8) + (b - a) * fr - (128 << 8)) * vc->vol) >> 9;
            vc->frac += vc->step;
            vc->idx += vc->frac >> 16;
            vc->frac &= 0xffff;
            active++;
        }
        if (sum > 32767)
            sum = 32767;
        else if (sum < -32768)
            sum = -32768;
        out_buf[2 * f] = out_buf[2 * f + 1] = sum;
    }
    if (!active) {
        *start = NULL;
        *size = 0;
        return;
    }
    *start = out_buf;
    *size = sizeof(out_buf);
}

static const struct mixer_play_cbs mixer_cbs = {.get_more = audio_get_more};
static bool sfx_paused;

static void audio_kick(void)
{
    if (!sfx_paused &&
        rb->mixer_channel_status(PCM_MIXER_CHAN_BEEP) != CHANNEL_PLAYING) {
        rb->mixer_channel_set_amplitude(PCM_MIXER_CHAN_BEEP, MIX_AMP_UNITY);
        rb->mixer_channel_play_data(PCM_MIXER_CHAN_BEEP, &mixer_cbs, NULL, 0);
    }
}

/* MIDI note number (A4 = 69 = 440 Hz), wave 1 square / 2 saw / 3 triangle
 * / 4 noise, volume 0..255, duration in ms, fading out linearly. */
void pk_tone(int note, int wave, int vol, int ms)
{
    static const uint16_t semitone[12] = {  /* 2^(n/12) * 4096 */
        4096, 4340, 4598, 4871, 5161, 5468, 5793, 6137, 6502, 6889, 7298, 7732};
    if (!cfg.sound)
        return;
    int m = note - 69 + 120;            /* keep the semitone count positive */
    uint32_t hz = (uint32_t)(((uint64_t)440 * semitone[m % 12] * (1u << (m / 12))) >> 22);
    unsigned rate = rb->mixer_get_frequency();
    tone.left = 0;
    tone.wave = wave;
    tone.vol = vol;
    tone.inc = (uint32_t)(((uint64_t)hz << 32) / rate);
    tone.total = (uint32_t)((uint64_t)rate * ms / 1000);
    tone.left = tone.total;
    audio_kick();
}

void pk_tone_stop(void)
{
    tone.left = 0;
}

void pk_voice_stop(int v)
{
    struct voice *vc = &voices[v];
    vc->active = false;
    if (vc->fd >= 0) {
        rb->close(vc->fd);
        vc->fd = -1;
    }
}

int pk_voice_play(int v, const char *rel, int rate, int loop, int vol)
{
    char path[MAX_PATH];
    struct voice *vc = &voices[v];
    if (!cfg.sound || v < 0 || v >= PK_VOICES)
        return 0;
    pk_voice_stop(v);
    data_path(path, sizeof(path), rel);
    vc->fd = rb->open(path, O_RDONLY);
    if (vc->fd < 0)
        return 0;
    vc->loop = loop;
    vc->vol = vol;
    vc->eof = false;
    vc->paused = false;
    vc->wr = 0;
    vc->idx = 0;
    vc->frac = 0;
    vc->step = (uint32_t)(((uint64_t)rate << 16) / rb->mixer_get_frequency());
    vc->active = true;
    pk_audio_poll();
    audio_kick();
    return 1;
}

void pk_voice_pause(int v, int paused)
{
    voices[v].paused = paused;
    if (!paused)
        audio_kick();
}

void pk_voice_loop(int v, int loop)
{
    voices[v].loop = loop;
}

int pk_voice_active(int v)
{
    return voices[v].active;
}

void pk_audio_poll(void)
{
    bool any = false;
    for (int v = 0; v < PK_VOICES; v++) {
        struct voice *vc = &voices[v];
        if (vc->fd < 0 || !vc->active)
            continue;
        any = true;
        if (vc->eof)
            continue;
        uint32_t used = vc->wr - vc->idx;
        while (used < RING) {
            uint32_t at = vc->wr & (RING - 1);
            uint32_t n = RING - used;
            if (n > RING - at)
                n = RING - at;
            ssize_t got = rb->read(vc->fd, vc->ring + at, n);
            if (got <= 0) {
                if (vc->loop && vc->wr != 0 &&
                    rb->lseek(vc->fd, 0, SEEK_SET) == 0)
                    continue;
                vc->eof = true;
                break;
            }
            vc->wr += got;
            used += got;
        }
    }
    if (any)
        audio_kick();
    for (int v = 0; v < PK_VOICES; v++)
        if (!voices[v].active && voices[v].fd >= 0)
            pk_voice_stop(v);
}

static void sfx_pause(void)
{
    sfx_paused = true;
    rb->mixer_channel_stop(PCM_MIXER_CHAN_BEEP);
}

static void sfx_resume(void)
{
    sfx_paused = false;
    for (int v = 0; v < PK_VOICES; v++)
        if (!cfg.sound)
            pk_voice_stop(v);
    pk_audio_poll();
}

/* ---- menus ------------------------------------------------------------- */

#define MAX_WORDS 260
#define TEXT_BYTES 2800

static const char *const layer_credits[] = {
    "PokittoLib compatibility layer for Rockbox", "",
    "Based on PokittoLib, (c) 2016 Jonne Valola and contributors,",
    "BSD-3-Clause. font3x5 and font5x7: Gamebuino Library, (c) 2014",
    "Aurelien Rodot, LGPL-3.0-or-later.", "",
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

/* Returns when the player chooses Play/Resume; exits the plugin on Quit. */
static char menu_title[48];

static void game_menu(bool resume)
{
    int sel = 0;
    long t0 = *rb->current_tick;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    backlight_use_settings();
    sfx_pause();
    rb->lcd_setfont(FONT_UI);

    rb->strlcpy(menu_title, pk_game.title, sizeof(menu_title));
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
            if (show_text("Help", pk_game.help, NULL))
                exit(PLUGIN_USB_CONNECTED);
            break;
        case 3:
            if (show_text(pk_game.title, pk_game.credits, layer_credits))
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
    menu_since = -1;
    sfx_resume();
}

int pk_pump(void)
{
    long btn;
    while ((btn = rb->button_get(false)) != BUTTON_NONE) {
        if (btn == SYS_POWEROFF && pk_game.hold_select)
            continue;
        if (rb->default_event_handler(btn) == SYS_USB_CONNECTED)
            exit(PLUGIN_USB_CONNECTED);
    }
#ifdef HAVE_BLUETOOTH_HID
    /* Game controller: Options opens the pause menu, PS leaves */
    static bool pad_menu_down;
    unsigned pad = rb->button_status();
    if (pad & BUTTON_PAD_HOME)
        exit(PLUGIN_OK);
    if (pad & BUTTON_PAD_MENU) {
        if (!pad_menu_down)
            menu_requested = true;
        pad_menu_down = true;
    } else {
        pad_menu_down = false;
    }
#endif
    /* A game in a long animation or delay() reads no buttons: check the
     * pause chord here too, or one held then is never seen. */
    menu_check(rb->button_status() & BUTTON_MAIN);
    if (menu_requested) {
        game_menu(true);
        return 1;
    }
    return 0;
}

/* ---- entry ------------------------------------------------------------- */

static void cleanup(void)
{
    rb->mixer_channel_stop(PCM_MIXER_CHAN_BEEP);
    for (int v = 0; v < PK_VOICES; v++)
        pk_voice_stop(v);
    for (int fd = 0; fd < 32; fd++)
        if (open_fds & (1u << fd))
            pk_file_close(fd);
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
    for (int v = 0; v < PK_VOICES; v++)
        voices[v].fd = -1;
    cfg_load();
    rb_atexit(cleanup);
    plugin_fpu_enable(true);
    tick_base = *rb->current_tick;
    game_menu(false);
    pk_main();
    return PLUGIN_OK;
}
