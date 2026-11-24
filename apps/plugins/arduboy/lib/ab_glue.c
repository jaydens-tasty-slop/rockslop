/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Rockbox side of the Arduboy compatibility layer: plugin entry, display,
 * keys, start/pause menu, EEPROM file, tone output. See ../README.md. */
#include "plugin.h"
#include "lib/helper.h"
#include "lib/display_text.h"
#include "lib/playback_control.h"
#include "lib/hint_strip.h"
#include "ab_glue.h"

#define SCREEN_W 128
#define SCREEN_H 64
#define OFFSET_Y ((LCD_HEIGHT - SCREEN_H) / 2)

extern void exit(int status);
extern int rb_atexit(void (*fn)(void));

#define AB_DIR PLUGIN_GAMES_DATA_DIR "/arduboy"

/* ---- settings ---------------------------------------------------------- */

struct palette {
    const char *name;
    unsigned char fg[3], bg[3];
};

static const struct palette palettes[] = {
    {"White on black",  {255, 255, 255}, {0, 0, 0}},
    {"Green phosphor",  {120, 255, 120}, {0, 24, 0}},
    {"Amber",           {255, 176, 0},   {24, 12, 0}},
    {"Ice blue",        {140, 210, 255}, {0, 8, 40}},
    {"Pocket LCD",      {20, 40, 20},    {170, 190, 150}},
};
#define NUM_PALETTES ARRAYLEN(palettes)

static struct {
    int palette;
    int sound;
} cfg = {0, 1};

static fb_data fg_color, bg_color, mid_color;
static uint8_t prev_buf[SCREEN_W * SCREEN_H / 8];
static fb_data line_buf[SCREEN_W * 8];

static void apply_palette(void)
{
    const struct palette *p = &palettes[cfg.palette];
    fg_color = LCD_RGBPACK(p->fg[0], p->fg[1], p->fg[2]);
    bg_color = LCD_RGBPACK(p->bg[0], p->bg[1], p->bg[2]);
    mid_color = LCD_RGBPACK((p->fg[0] + p->bg[0]) / 2, (p->fg[1] + p->bg[1]) / 2,
                            (p->fg[2] + p->bg[2]) / 2);
}

static void file_name(char *buf, size_t n, const char *ext)
{
    rb->snprintf(buf, n, AB_DIR "/%s.%s", ab_game.name, ext);
}

static void cfg_load(void)
{
    char path[MAX_PATH];
    unsigned char b[2];
    file_name(path, sizeof(path), "cfg");
    int fd = rb->open(path, O_RDONLY);
    if (fd >= 0) {
        if (rb->read(fd, b, 2) == 2 && b[0] < NUM_PALETTES) {
            cfg.palette = b[0];
            cfg.sound = b[1] != 0;
        }
        rb->close(fd);
    }
    apply_palette();
}

static void cfg_save(void)
{
    char path[MAX_PATH];
    unsigned char b[2] = {cfg.palette, cfg.sound};
    rb->mkdir(PLUGIN_GAMES_DATA_DIR);
    rb->mkdir(AB_DIR);
    file_name(path, sizeof(path), "cfg");
    int fd = rb->open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0) {
        rb->write(fd, b, 2);
        rb->close(fd);
    }
}

/* ---- EEPROM ------------------------------------------------------------ */

uint8_t ab_eeprom[AB_EEPROM_SIZE];
static bool eeprom_dirty;
static long eeprom_changed;

static void eeprom_load(void)
{
    char path[MAX_PATH];
    rb->memset(ab_eeprom, 0xff, sizeof(ab_eeprom));
    file_name(path, sizeof(path), "eep");
    int fd = rb->open(path, O_RDONLY);
    if (fd >= 0) {
        rb->read(fd, ab_eeprom, sizeof(ab_eeprom));
        rb->close(fd);
    }
}

void ab_eeprom_flush(void)
{
    char path[MAX_PATH];
    if (!eeprom_dirty)
        return;
    eeprom_dirty = false;
    rb->mkdir(PLUGIN_GAMES_DATA_DIR);
    rb->mkdir(AB_DIR);
    file_name(path, sizeof(path), "eep");
    int fd = rb->open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0) {
        rb->write(fd, ab_eeprom, sizeof(ab_eeprom));
        rb->close(fd);
    }
}

void ab_eeprom_dirty(void)
{
    eeprom_dirty = true;
    eeprom_changed = *rb->current_tick;
}

/* ---- FX data file ------------------------------------------------------ */

#define FX_PAGE 512
#define FX_PAGES 32

static int fx_fd = -1;
typedef uint8_t fx_page_t[FX_PAGE];
static fx_page_t *fx_cache;             /* from the plugin buffer */
static uint32_t fx_tag[FX_PAGES];       /* page number + 1, 0 = empty */
static uint32_t fx_used[FX_PAGES];
static uint32_t fx_clock;

static void fx_open(void)
{
    char path[MAX_PATH];
    if (!ab_game.data_file)
        return;
    rb->snprintf(path, sizeof(path), AB_DIR "/%s", ab_game.data_file);
    size_t avail;
    fx_cache = rb->plugin_get_buffer(&avail);
    if (!fx_cache || avail < sizeof(fx_page_t) * FX_PAGES) {
        rb->splash(HZ * 4, "Out of memory");
        exit(PLUGIN_ERROR);
    }
    fx_fd = rb->open(path, O_RDONLY);
    if (fx_fd < 0) {
        rb->splashf(HZ * 4, "Missing data file: %s", path);
        exit(PLUGIN_ERROR);
    }
}

static const uint8_t *fx_page(uint32_t page)
{
    static int last;
    if (fx_tag[last] == page + 1) {
        fx_used[last] = ++fx_clock;
        return fx_cache[last];
    }
    int victim = 0;
    for (int i = 0; i < FX_PAGES; i++) {
        if (fx_tag[i] == page + 1) {
            fx_used[i] = ++fx_clock;
            last = i;
            return fx_cache[i];
        }
        if (fx_used[i] < fx_used[victim])
            victim = i;
    }
    rb->memset(fx_cache[victim], 0, FX_PAGE);
    if (rb->lseek(fx_fd, (off_t)page * FX_PAGE, SEEK_SET) >= 0)
        rb->read(fx_fd, fx_cache[victim], FX_PAGE);
    fx_tag[victim] = page + 1;
    fx_used[victim] = ++fx_clock;
    last = victim;
    return fx_cache[victim];
}

void ab_fx_read(uint32_t offset, void *dst, uint32_t len)
{
    uint8_t *d = dst;
    while (len) {
        uint32_t in_page = offset % FX_PAGE;
        uint32_t n = FX_PAGE - in_page;
        if (n > len)
            n = len;
        if (fx_fd < 0)
            rb->memset(d, 0, n);
        else
            rb->memcpy(d, fx_page(offset / FX_PAGE) + in_page, n);
        d += n;
        offset += n;
        len -= n;
    }
}

/* Game state saved by FX::saveGameState(): [uint16 size][bytes]. */
uint8_t ab_fx_load_state(uint8_t *state, size_t size)
{
    char path[MAX_PATH];
    uint16_t stored;
    uint8_t ok = 0;
    file_name(path, sizeof(path), "sav");
    int fd = rb->open(path, O_RDONLY);
    if (fd >= 0) {
        if (rb->read(fd, &stored, 2) == 2 && stored == size
                && rb->read(fd, state, size) == (ssize_t)size)
            ok = 1;
        rb->close(fd);
    }
    return ok;
}

void ab_fx_save_state(const uint8_t *state, size_t size)
{
    char path[MAX_PATH];
    uint16_t stored = size;
    rb->mkdir(PLUGIN_GAMES_DATA_DIR);
    rb->mkdir(AB_DIR);
    file_name(path, sizeof(path), "sav");
    int fd = rb->open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0) {
        rb->write(fd, &stored, 2);
        rb->write(fd, state, size);
        rb->close(fd);
    }
}

/* ---- time -------------------------------------------------------------- */

static long tick_base;      /* tick at plugin start */
static long paused_ticks;   /* time spent in the menu */

unsigned long ab_millis(void)
{
    return (unsigned long)(*rb->current_tick - tick_base - paused_ticks) * 10;
}

void ab_delay(unsigned long ms)
{
    unsigned long end = ab_millis() + ms;
    while ((long)(end - ab_millis()) > 0)
        ab_idle();
}

static uint32_t rng_state = 0x2545f491;

void ab_srand(unsigned long seed)
{
    rng_state = seed ? seed : 0x2545f491;
}

unsigned long ab_rand(void)
{
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return rng_state = x;
}

static unsigned idle_calls;
static long idle_tick;

void ab_idle(void)
{
    idle_calls++;
    idle_tick = *rb->current_tick;
    rb->sleep(0);
    ab_pump();
}

void ab_fatal(const char *msg)
{
    rb->splash(HZ * 3, msg);
    exit(PLUGIN_ERROR);
}

void ab_set_led(uint8_t r, uint8_t g, uint8_t b)
{
    (void)r; (void)g; (void)b;
}

/* ---- keys -------------------------------------------------------------- */

static const struct ab_rule default_keys[] = {
    {AB_P_SELECT | AB_P_DOWN, AB_B_B},
    {AB_P_SELECT,             AB_B_A},
    {AB_P_UP,                 AB_B_UP},
    {AB_P_DOWN,               AB_B_DOWN},
    {AB_P_LEFT,               AB_B_LEFT},
    {AB_P_RIGHT,              AB_B_RIGHT},
    {0, 0}
};

static long select_since = -1;
static long menu_since = -1;
static bool menu_requested, menu_fired;

static unsigned last_set;
static long set_since;

static long tap_until;

static unsigned phys_keys(void)
{
    unsigned s = rb->button_status() & BUTTON_MAIN;
    long now = *rb->current_tick;
    if (s != last_set) {
        last_set = s;
        set_since = now;
    }
    unsigned long_ms = ab_game.long_ms ? ab_game.long_ms : 400;
    if (s & BUTTON_SELECT) {
        if (select_since < 0)
            select_since = now;
        if ((now - select_since) * 10 >= (long)long_ms)
            s |= AB_P_LONG;
    } else {
        if (select_since >= 0 && (now - select_since) * 10 < (long)long_ms)
            tap_until = now + (ab_game.tap_ms ? ab_game.tap_ms : 60) / 10 + 1;
        select_since = -1;
    }
    if (now < tap_until)
        s |= AB_P_TAP;
    return s;
}

/* The pause gesture: SELECT+UP unless the game says otherwise, held 500 ms
 * (1000 ms for a game where the chord is also a gameplay combo). SELECT is
 * also the power key (held ~1 s powers off), hence a chord and not a hold. */
static unsigned menu_want(void)
{
    return ab_game.menu_keys ? ab_game.menu_keys : AB_P_SELECT | AB_P_UP;
}

static unsigned menu_time(void)
{
    return ab_game.menu_ms ? ab_game.menu_ms : 500;
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

uint8_t ab_buttons(void)
{
    /* A sketch waiting for a key in a loop of its own (while
     * (!pressed(A_BUTTON)) {}) never gets back to loop()'s end: sleep here
     * too, or it starves every other thread (on the YP3 the recovery
     * watchdog then resets the player). */
    if (*rb->current_tick - idle_tick > HZ / 10)
        ab_idle();
    unsigned s = phys_keys();
    uint8_t out = 0;
    unsigned used = 0;
    const struct ab_rule *keys = ab_game.keys ? ab_game.keys : default_keys;
    const struct ab_rule *r;
    /* A rule on SELECT alone that is a prefix of a SELECT chord waits a
     * moment, so a chord pressed a few ms apart does not fire it first. */
    long grace = (ab_game.chord_ms ? ab_game.chord_ms : 30) / 10;
    bool menu_chord = (menu_want() & AB_P_SELECT) && menu_want() != AB_P_SELECT;
    for (r = keys; r->phys; r++) {
        if ((s & r->phys) == r->phys && !(used & r->phys & ~(AB_P_LONG | AB_P_TAP))) {
            if (r->phys == AB_P_SELECT && s == AB_P_SELECT
                    && *rb->current_tick - set_since < grace) {
                if (menu_chord)
                    goto next;
                for (const struct ab_rule *c = keys; c->phys; c++) {
                    if (c->phys != r->phys && (c->phys & AB_P_SELECT)
                            && !(c->phys & (AB_P_LONG | AB_P_TAP)))
                        goto next;
                }
            }
            out |= r->out;
            used |= r->phys;
        }
next:   ;
    }
#ifdef HAVE_BLUETOOTH_HID
    {
        /* Game pad: A and B are the Arduboy buttons, Options opens the
         * pause menu (once per press), PS leaves. */
        static bool pad_menu_held;
        unsigned long pad = rb->button_status();
        if (pad & BUTTON_PAD_A)
            out |= AB_B_A;
        if (pad & BUTTON_PAD_B)
            out |= AB_B_B;
        if (pad & BUTTON_PAD_HOME)
            exit(PLUGIN_OK);
        if (pad & BUTTON_PAD_MENU) {
            if (!pad_menu_held)
                menu_requested = true;
            pad_menu_held = true;
        } else {
            pad_menu_held = false;
        }
    }
#endif
    menu_check(s);
    /* A pause chord that is not a gameplay combo (menu_ms < 1000) presses
     * nothing in the game while it is held. */
    if ((s & menu_want()) == menu_want() && menu_time() < 1000)
        out = 0;
    return out;
}

/* ---- display ----------------------------------------------------------- */

static void draw_background(void)
{
    hint_strip_draw(ab_game.hint, OFFSET_Y + SCREEN_H);
    rb->lcd_update();
}

/* A game that fakes grey by alternating dither patterns frame to frame sets
 * ab_game.blend: each pixel is then drawn from the last two frames (both
 * lit: lit, both dark: dark, differing: the mid colour). */
void ab_present(const uint8_t *buf, int invert)
{
    fb_data lut[4] = {bg_color, mid_color, mid_color, fg_color};
    if (invert) {
        lut[0] = fg_color;
        lut[3] = bg_color;
    }
    if (!ab_game.blend) {
        lut[1] = lut[0];
        lut[2] = lut[3];
    }
    for (int page = 0; page < SCREEN_H / 8; page++) {
        const uint8_t *src = buf + page * SCREEN_W;
        const uint8_t *old = prev_buf + page * SCREEN_W;
        for (int x = 0; x < SCREEN_W; x++) {
            unsigned b = src[x];
            unsigned p = ab_game.blend ? old[x] : b;
            fb_data *d = line_buf + x;
            for (int k = 0; k < 8; k++, b >>= 1, p >>= 1, d += SCREEN_W)
                *d = lut[((b & 1) << 1) | (p & 1)];
        }
        rb->lcd_bitmap(line_buf, 0, OFFSET_Y + page * 8, SCREEN_W, 8);
    }
    if (ab_game.blend)
        rb->memcpy(prev_buf, buf, sizeof(prev_buf));
    rb->lcd_update_rect(0, OFFSET_Y, SCREEN_W, SCREEN_H);
    ab_pump();
}

/* ArduboyG greyscale: per pixel level hi:lo in 0..levels, drawn as a blend
 * from the palette's background to its foreground. */
void ab_present_gray(const uint8_t *lo, const uint8_t *hi, int levels)
{
    const struct palette *p = &palettes[cfg.palette];
    fb_data lut[4];
    for (int l = 0; l < 4; l++) {
        int n = l < levels ? l : levels;
        lut[l] = LCD_RGBPACK(p->bg[0] + (p->fg[0] - p->bg[0]) * n / levels,
                             p->bg[1] + (p->fg[1] - p->bg[1]) * n / levels,
                             p->bg[2] + (p->fg[2] - p->bg[2]) * n / levels);
    }
    for (int page = 0; page < SCREEN_H / 8; page++) {
        for (int x = 0; x < SCREEN_W; x++) {
            unsigned a = lo[page * SCREEN_W + x], b = hi[page * SCREEN_W + x];
            fb_data *d = line_buf + x;
            for (int k = 0; k < 8; k++, a >>= 1, b >>= 1, d += SCREEN_W)
                *d = lut[((b & 1) << 1) | (a & 1)];
        }
        rb->lcd_bitmap(line_buf, 0, OFFSET_Y + page * 8, SCREEN_W, 8);
    }
    rb->lcd_update_rect(0, OFFSET_Y, SCREEN_W, SCREEN_H);
    ab_pump();
}

/* ---- audio ------------------------------------------------------------- */

#define AUDIO_FRAMES 256
static int16_t mono_buf[AUDIO_FRAMES];
static int16_t stereo_buf[AUDIO_FRAMES * 2];

static int16_t mono_buf2[AUDIO_FRAMES];

/* Games without a Playtune score player link no ab_tunes_fill(). */
__attribute__((weak)) int ab_tunes_fill(int16_t *buf, int frames, int rate)
{
    (void)buf; (void)frames; (void)rate;
    return 0;
}

static void audio_get_more(const void **start, size_t *size)
{
    unsigned rate = rb->mixer_get_frequency();
    int a = ab_tones_fill(mono_buf, AUDIO_FRAMES, rate);
    int b = ab_tunes_fill(mono_buf2, AUDIO_FRAMES, rate);
    if (!a && !b) {
        *start = NULL;
        *size = 0;
        return;
    }
    for (int i = 0; i < AUDIO_FRAMES; i++) {
        int s = (a ? mono_buf[i] : 0) + (b ? mono_buf2[i] : 0);
        if (s > 32767) s = 32767;
        if (s < -32768) s = -32768;
        stereo_buf[2 * i] = stereo_buf[2 * i + 1] = s;
    }
    *start = stereo_buf;
    *size = sizeof(stereo_buf);
}

void ab_audio_start(void)
{
    static const struct mixer_play_cbs cbs = {.get_more = audio_get_more};
    if (rb->mixer_channel_status(PCM_MIXER_CHAN_BEEP) != CHANNEL_PLAYING) {
        rb->mixer_channel_set_amplitude(PCM_MIXER_CHAN_BEEP, MIX_AMP_UNITY);
        rb->mixer_channel_play_data(PCM_MIXER_CHAN_BEEP, &cbs, NULL, 0);
    }
}

int ab_audio_enabled(void)
{
    return cfg.sound;
}

void ab_audio_set(int on)
{
    cfg.sound = on != 0;
}

/* ---- menus ------------------------------------------------------------- */

#define MAX_WORDS 220
#define TEXT_BYTES 2400

static const char *const layer_credits[] = {
    "Arduboy layer for Rockbox", "",
    "Arduboy2 and Sprites: (c) Scott Allen, Kevin Bates and others,",
    "BSD-3-Clause.", "",
    "ArduboyTones: (c) Scott Allen, MIT.", "",
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
    static struct opt_items colours[NUM_PALETTES];
    static const struct opt_items onoff[] = {{"Off", -1}, {"On", -1}};
    for (unsigned i = 0; i < NUM_PALETTES; i++)
        colours[i].string = palettes[i].name;
    MENUITEM_STRINGLIST(m, "Settings", NULL, "Colours", "Sound");
    int sel = 0;
    for (;;) {
        switch (rb->do_menu(&m, &sel, NULL, false)) {
        case 0:
            rb->set_option("Colours", &cfg.palette, RB_INT, colours,
                           NUM_PALETTES, NULL);
            apply_palette();
            break;
        case 1:
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
    ab_eeprom_flush();
    rb->lcd_setfont(FONT_UI);

    rb->strlcpy(menu_title, ab_game.title, sizeof(menu_title));
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
            if (show_text("Help", ab_game.help, NULL))
                exit(PLUGIN_USB_CONNECTED);
            break;
        case 3:
            if (show_text(ab_game.title, ab_game.credits, layer_credits))
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

void ab_pump(void)
{
    long btn;
    while ((btn = rb->button_get(false)) != BUTTON_NONE) {
        /* SELECT is the power key: a hold of about a second during play is
         * a game gesture, not a shutdown, so SYS_POWEROFF is dropped here
         * (the menus still pass it on). */
        if (btn == SYS_POWEROFF)
            continue;
#ifdef HAVE_BLUETOOTH_HID
        /* the pad's Options and PS from the queue too: a tap is not lost
         * while the game reads no buttons */
        if (!IS_SYSEVENT(btn) && !(btn & (BUTTON_REL | BUTTON_REPEAT))) {
            if (btn & BUTTON_PAD_HOME)
                exit(PLUGIN_OK);
            if (btn & BUTTON_PAD_MENU)
                menu_requested = true;
        }
#endif
        if (rb->default_event_handler(btn) == SYS_USB_CONNECTED)
            exit(PLUGIN_USB_CONNECTED);
    }
    /* A game in a long animation or delay() reads no buttons: check the
     * pause chord here too, or one held then is never seen. */
    menu_check(rb->button_status() & BUTTON_MAIN);
    if (menu_requested)
        game_menu(true);
    if (eeprom_dirty && *rb->current_tick - eeprom_changed > 3 * HZ)
        ab_eeprom_flush();
}

/* ---- entry ------------------------------------------------------------- */

static void cleanup(void)
{
    rb->mixer_channel_stop(PCM_MIXER_CHAN_BEEP);
    if (fx_fd >= 0) {
        rb->close(fx_fd);
        fx_fd = -1;
    }
    ab_eeprom_flush();
    cfg_save();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    backlight_use_settings();
}

enum plugin_status plugin_start(const void *param)
{
    unsigned spins = 0;

    (void)param;
    cfg_load();
    eeprom_load();
    fx_open();
    rb_atexit(cleanup);
    tick_base = *rb->current_tick;
    ab_srand(*rb->current_tick * 2654435761u);
    game_menu(false);
    setup();
    for (;;) {
        unsigned calls = idle_calls;
        loop();
        /* sketches that poll millis() and return early would otherwise spin
         * without sleeping (starving audio and the pause chord). Sleep only
         * after eight loop()s without one: nextFrame() games sleep in it,
         * and another tick per frame would cap 60 fps games at 50. */
        if (idle_calls != calls)
            spins = 0;
        else if (++spins == 8) {
            spins = 0;
            ab_idle();
        }
    }
}

/* sprintf for sketches (the plugin API only has snprintf). */
int sprintf(char *buf, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = rb->vsnprintf(buf, 0x7fffffff, fmt, ap);
    va_end(ap);
    return n;
}
