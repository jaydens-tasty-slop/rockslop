/***************************************************************************
 * Tamagotchi P1 emulator for Rockbox (ArduinoGotchi / TamaLIB core).
 *
 * Core (tama/): TamaLIB (C) 2021 Jean-Christophe Rona, via ArduinoGotchi
 * (C) 2022 Gary Kwok. GPL-2.0-or-later, see ../LICENSE. This file is the
 * Rockbox front end, same license.
 *
 * The Tamagotchi ROM is not part of this plugin. Put the P1 ROM (rom.bin,
 * 12288 bytes) in /.rockbox/rocks/games/tamagotchi/.
 *
 * Keys: LEFT, SELECT, RIGHT = the three Tamagotchi buttons A, B, C;
 * holding SELECT+UP for 500 ms opens the menu (SELECT does not count as B
 * while UP is held). SELECT is a game button, so holding it does not
 * power the player off (use the menu: Quit saves the pet).
 * The pet is saved on quit, every 10 minutes and on USB. When it is started
 * again the time that passed is fast-forwarded, up to a limit that can be
 * set in the menu (the original device runs at 32768 Hz, so this is
 * quick but not free).
 ***************************************************************************/
#include "plugin.h"
#include "lib/helper.h"
#include "lib/display_text.h"
#include "lib/playback_control.h"
#include "lib/hint_strip.h"
#include "tama/cpu.h"
#include "tama/hw.h"
#include "gotchi_view.h"

#if LCD_WIDTH < 128 || LCD_HEIGHT < 128 || !defined(HAVE_LCD_COLOR)
#error "gotchi needs a colour LCD of at least 128x128"
#endif

#define GOTCHI_DIR  PLUGIN_GAMES_DIR "/tamagotchi"
#define ROM_PATH    GOTCHI_DIR "/rom.bin"
#define SAVE_PATH   GOTCHI_DIR "/state.sav"
#define ROM_BYTES   (ROM_WORDS * 2)

#define SAVE_MAGIC   0x414d4154u /* "TAMA" */
#define SAVE_VERSION 1
#define AUTOSAVE_SECS 600

/* Emulated seconds fast-forwarded per chunk while catching up. */
#define CATCHUP_CHUNK 4

static u12_t rom[ROM_WORDS];
static uint32_t rom_sum;
static bool emu_ready;

static struct tama_screen screen;
static bool dirty;

struct settings {
    int sound;   /* 0 off, 1 on */
    int catchup; /* index into catchup_secs */
};
static struct settings cfg = { 1, 2 };

static const uint32_t catchup_secs[] = { 0, 600, 3600, 3 * 3600, 8 * 3600 };
static const struct opt_items catchup_names[] = {
    { "Off", -1 }, { "10 minutes", -1 }, { "1 hour", -1 }, { "3 hours", -1 },
    { "8 hours", -1 },
};
static const struct opt_items onoff_names[] = { { "Off", -1 }, { "On", -1 } };

struct save_file {
    uint32_t magic, version, rom_sum;
    uint32_t rtc;               /* time of the save, seconds */
    struct settings cfg;
    cpu_state_t cpu;
};

/* ---------------------------------------------------------------- sound */

#define BLOCK_FRAMES 256
#define BEEP_AMP 2500

static int16_t pcm[BLOCK_FRAMES * 2];
static volatile uint32_t beep_step; /* phase increment per sample (2^32 = 1 cycle) */
static uint32_t beep_phase;
static uint32_t beep_freq;
static bool beep_open, muted;

static void audio_more(const void **start, size_t *size)
{
    uint32_t phase = beep_phase, step = beep_step;
    for (int i = 0; i < BLOCK_FRAMES; i++) {
        phase += step;
        pcm[2 * i] = pcm[2 * i + 1] = (phase & 0x80000000u) ? BEEP_AMP : -BEEP_AMP;
    }
    beep_phase = phase;
    *start = pcm;
    *size = sizeof(pcm);
}

static void beep_stop(void)
{
    if (beep_open)
        rb->mixer_channel_stop(PCM_MIXER_CHAN_BEEP);
    beep_open = false;
}

/* The P1 buzzer is a square wave on the mixer's beep channel. */
static void beep_start(void)
{
    static const struct mixer_play_cbs cbs = { .get_more = audio_more };
    if (beep_open || !cfg.sound || muted || !beep_freq)
        return;
    beep_step = (uint32_t)(((uint64_t)beep_freq << 32) / rb->mixer_get_frequency());
    rb->mixer_channel_set_amplitude(PCM_MIXER_CHAN_BEEP, MIX_AMP_UNITY);
    rb->mixer_channel_play_data(PCM_MIXER_CHAN_BEEP, &cbs, NULL, 0);
    beep_open = true;
}

static void hal_set_frequency(u32_t freq)
{
    beep_freq = freq;
    if (beep_open)
        beep_step = (uint32_t)(((uint64_t)freq << 32) / rb->mixer_get_frequency());
}

static void hal_play_frequency(bool_t en)
{
    if (en)
        beep_start();
    else
        beep_stop();
}

/* -------------------------------------------------------------- display */

static void hal_set_lcd_matrix(u8_t x, u8_t y, bool_t val)
{
    uint32_t bit = 1u << x;
    uint32_t old = screen.row[y];
    screen.row[y] = val ? (old | bit) : (old & ~bit);
    if (screen.row[y] != old)
        dirty = true;
}

static void hal_set_lcd_icon(u8_t icon, bool_t val)
{
    uint8_t old = screen.icon;
    screen.icon = val ? (old | (1u << icon)) : (old & ~(1u << icon));
    if (screen.icon != old)
        dirty = true;
}

static hal_t hal = {
    .set_lcd_matrix = hal_set_lcd_matrix,
    .set_lcd_icon = hal_set_lcd_icon,
    .set_frequency = hal_set_frequency,
    .play_frequency = hal_play_frequency,
};

static uint32_t last_rgb = 0xffffffffu;

void gotchi_fill(int x, int y, int w, int h, uint32_t rgb)
{
    if (rgb != last_rgb) {
        rb->lcd_set_foreground(LCD_RGBPACK(rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff));
        last_rgb = rgb;
    }
    rb->lcd_fillrect(x, y, w, h);
}

static void redraw(void)
{
    int y = VIEW_PANEL_Y + VIEW_PANEL_H + 4;
    last_rgb = 0xffffffffu;
    rb->lcd_set_drawmode(DRMODE_SOLID);
    gotchi_draw(&screen);
    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_set_foreground(HINT_STRIP_FG);
    rb->lcd_set_background(LCD_BLACK);
    last_rgb = 0xffffffffu;
    rb->lcd_putsxy(2, y, "Left=A  select");
    rb->lcd_putsxy(2, y + HINT_STRIP_LINE, "Sel=B  confirm");
    rb->lcd_putsxy(2, y + 2 * HINT_STRIP_LINE, "Right=C  cancel");
    rb->lcd_putsxy(2, y + 3 * HINT_STRIP_LINE, "Hold Sel+Up=menu");
    rb->lcd_update();
    dirty = false;
}

/* ------------------------------------------------------------ ROM, saves */

static bool load_rom(void)
{
    uint8_t buf[256];
    int fd = rb->open(ROM_PATH, O_RDONLY);
    if (fd < 0)
        return false;
    if (rb->lseek(fd, 0, SEEK_END) != ROM_BYTES) {
        rb->close(fd);
        return false;
    }
    rb->lseek(fd, 0, SEEK_SET);
    rom_sum = 0;
    for (int w = 0; w < ROM_WORDS; w += 128) {
        if (rb->read(fd, buf, sizeof(buf)) != (ssize_t)sizeof(buf)) {
            rb->close(fd);
            return false;
        }
        for (int i = 0; i < 128; i++) {
            /* 12-bit words, big endian in 16 bits */
            rom[w + i] = ((buf[2 * i] << 8) | buf[2 * i + 1]) & 0xfff;
            rom_sum = rom_sum * 31 + rom[w + i];
        }
    }
    rb->close(fd);
    return true;
}

static uint32_t rtc_now(void)
{
    return (uint32_t)rb->mktime(rb->get_time());
}

static void save_state(void)
{
    static struct save_file sv; /* too big for the stack of some callers */
    if (!emu_ready)
        return;
    sv.magic = SAVE_MAGIC;
    sv.version = SAVE_VERSION;
    sv.rom_sum = rom_sum;
    sv.rtc = rtc_now();
    sv.cfg = cfg;
    cpu_get_state(&sv.cpu);
    int fd = rb->open(SAVE_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        rb->splash(HZ * 2, "Cannot write save file");
        return;
    }
    rb->write(fd, &sv, sizeof(sv));
    rb->close(fd);
}

/* Returns the seconds since the save, or -1 if there is no usable save. */
static long load_state(void)
{
    static struct save_file sv;
    long elapsed;
    int fd = rb->open(SAVE_PATH, O_RDONLY);
    if (fd < 0)
        return -1;
    ssize_t n = rb->read(fd, &sv, sizeof(sv));
    rb->close(fd);
    if (n != (ssize_t)sizeof(sv) || sv.magic != SAVE_MAGIC ||
        sv.version != SAVE_VERSION || sv.rom_sum != rom_sum)
        return -1;
    cfg = sv.cfg;
    if (cfg.catchup < 0 || cfg.catchup >= (int)ARRAYLEN(catchup_secs))
        cfg.catchup = 0;
    cpu_set_state(&sv.cpu);
    elapsed = (long)(rtc_now() - sv.rtc);
    return elapsed < 0 ? 0 : elapsed;
}

/* ------------------------------------------------------------- emulation */

static void show_text(const char *text);

/* Fast-forward emulated time (no display, no sound). Any key stops it. */
static bool catch_up(uint32_t seconds)
{
    uint32_t done = 0;
    long shown = 0;
    bool ok = true;

    muted = true;
    beep_stop();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    while (done < seconds) {
        uint32_t chunk = MIN(seconds - done, (uint32_t)CATCHUP_CHUNK);
        if (cpu_run(cpu_get_ticks() + chunk * CPU_TICK_HZ)) {
            ok = false;
            break;
        }
        done += chunk;
        if (*rb->current_tick - shown >= HZ) {
            shown = *rb->current_tick;
            rb->splashf(0, "Catching up: %lu of %lu s (any key stops)",
                        (unsigned long)done, (unsigned long)seconds);
        }
        if (rb->button_get(false) & ~(BUTTON_REL | BUTTON_REPEAT))
            break;
    }
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    muted = false;
    return ok;
}

static bool emu_start(void)
{
    if (emu_ready)
        return true;
    if (!load_rom()) {
        show_text("The Tamagotchi P1 ROM is missing or wrong. This player "
                  "cannot ship it, so copy your own rom.bin (12288 bytes) "
                  "to\n/.rockbox/rocks/games/tamagotchi/rom.bin\nand try "
                  "again.");
        return false;
    }
    rb->memset(&screen, 0, sizeof(screen));
    g_hal = &hal;
    cpu_init(rom);
    hw_init();
    emu_ready = true;
    long elapsed = load_state();
    if (elapsed >= 0) {
        cpu_refresh_display();
        uint32_t limit = catchup_secs[cfg.catchup];
        if (limit && elapsed > 3) {
            if (!catch_up(MIN((uint32_t)elapsed, limit))) {
                show_text("The ROM stopped on an unknown instruction. It "
                          "is probably not a Tamagotchi P1 ROM.");
                emu_ready = false;
                return false;
            }
        }
    }
    return true;
}

enum run_result { RUN_MENU, RUN_USB };

static const int btn_map[3][2] = {
    { BUTTON_LEFT, BTN_LEFT }, { BUTTON_SELECT, BTN_MIDDLE },
    { BUTTON_RIGHT, BTN_RIGHT },
};

#ifdef HAVE_BLUETOOTH_HID
/* Game controller: X, A and B press the left, middle and right icon button */
static const int pad_map[3] = { BUTTON_PAD_X, BUTTON_PAD_A, BUTTON_PAD_B };
#endif

static enum run_result run(void)
{
    long last = *rb->current_tick, last_draw = last, last_save = last;
    uint32_t target = cpu_get_ticks(), frac = 0;
    unsigned held = 0;
    long hold_start = 0;

    muted = false;
    redraw();
    for (;;) {
        long ev = rb->button_get_w_tmo(1);
        if (ev != SYS_POWEROFF && rb->default_event_handler(ev) == SYS_USB_CONNECTED)
            return RUN_USB;
#ifdef HAVE_BLUETOOTH_HID
        if (ev == BUTTON_PAD_MENU || ev == BUTTON_PAD_HOME)
            return RUN_MENU;
#endif

        int st = rb->button_status();
        if ((st & (BUTTON_SELECT | BUTTON_UP)) == (BUTTON_SELECT | BUTTON_UP)) {
            if (!hold_start)
                hold_start = *rb->current_tick;
            else if (TIME_AFTER(*rb->current_tick, hold_start + HZ / 2))
                return RUN_MENU;
            st &= ~BUTTON_SELECT; /* the menu chord is not a B press */
        } else {
            hold_start = 0;
        }
        for (int i = 0; i < 3; i++) {
#ifdef HAVE_BLUETOOTH_HID
            bool down = st & (btn_map[i][0] | pad_map[i]);
#else
            bool down = st & btn_map[i][0];
#endif
            if (down != !!(held & (1u << i))) {
                held ^= 1u << i;
                hw_set_button(btn_map[i][1],
                              down ? BTN_STATE_PRESSED : BTN_STATE_RELEASED);
            }
        }

        long now = *rb->current_tick;
        frac += (uint32_t)(now - last) * CPU_TICK_HZ;
        target += frac / HZ;
        frac %= HZ;
        last = now;
        if (cpu_run(target)) {
            show_text("The ROM stopped on an unknown instruction. It is "
                      "probably not a Tamagotchi P1 ROM.");
            emu_ready = false;
            return RUN_MENU;
        }

        if (dirty && now - last_draw >= HZ / 12) {
            last_draw = now;
            redraw();
        }
        if (now - last_save >= HZ * AUTOSAVE_SECS) {
            last_save = now;
            save_state();
        }
    }
}

static void release_buttons(void)
{
    for (int i = 0; i < 3; i++)
        hw_set_button(btn_map[i][1], BTN_STATE_RELEASED);
}

/* ----------------------------------------------------------------- menus */

static void show_text(const char *text)
{
    static char buf[640];
    static char *words[128];
    int n = 0;
    char *p;

    rb->strlcpy(buf, text, sizeof(buf));
    for (p = buf; *p && n < (int)ARRAYLEN(words) - 1;) {
        words[n++] = p;
        while (*p && *p != ' ' && *p != '\n')
            p++;
        if (*p == '\n') {
            *p++ = '\0';
            words[n++] = (char *)""; /* newline */
        } else if (*p) {
            *p++ = '\0';
        }
    }
    rb->lcd_clear_display();
    rb->lcd_setfont(FONT_UI);
    display_text(n, words, NULL, NULL, true);
    rb->lcd_setfont(FONT_SYSFIXED);
}

static void help(void)
{
    show_text("LEFT, SELECT and RIGHT are the three buttons of the "
              "Tamagotchi: A selects an icon, B confirms, C cancels. "
              "First set the clock with B, or the pet will not hatch.\n"
              "Hold SELECT and UP together for half a second to open this menu and pause the pet. When you quit "
              "it is saved, and the time away is fast-forwarded next time, "
              "up to the Catch-up limit in Settings.");
}

static void about(void)
{
    show_text("Tamagotchi P1 emulator.\nTamaLIB by Jean-Christophe Rona, "
              "ArduinoGotchi by Gary Kwok.\n"
              "github.com/GaryZ88/ArduinoGotchi\n"
              "License: GPL-2.0 or later (full text: LICENSE.txt next to "
              "the ROM).\nThe Tamagotchi ROM is not included.\n"
              "Rockbox port for the YP3 player.");
}

static void settings(void)
{
    MENUITEM_STRINGLIST(m, "Settings", NULL, "Sound", "Catch-up limit");
    int sel = 0;
    for (;;) {
        switch (rb->do_menu(&m, &sel, NULL, false)) {
        case 0:
            rb->set_option("Sound", &cfg.sound, RB_BOOL, onoff_names, 2, NULL);
            break;
        case 1:
            rb->set_option("Catch-up limit", &cfg.catchup, RB_INT,
                           catchup_names, ARRAYLEN(catchup_names), NULL);
            break;
        default:
            return;
        }
    }
}

enum { M_RESUME, M_SETTINGS, M_HELP, M_ABOUT, M_PLAYBACK, M_NEWPET, M_QUIT };

static int menu(bool *usb)
{
    MENUITEM_STRINGLIST(m, "Tamagotchi", NULL, "Start / Resume", "Settings",
                        "Help", "About", "Playback Control", "New Pet",
                        "Save and Quit");
    int sel = 0;
    for (;;) {
        switch (rb->do_menu(&m, &sel, NULL, false)) {
        case M_RESUME:
            if (!emu_start())
                break;
            release_buttons();
            if (run() == RUN_USB) {
                *usb = true;
                return M_QUIT;
            }
            beep_stop();
            rb->lcd_clear_display();
            rb->button_clear_queue();
            break;
        case M_SETTINGS: settings(); break;
        case M_HELP: help(); break;
        case M_ABOUT: about(); break;
        case M_PLAYBACK:
            if (playback_control(NULL)) {
                *usb = true;
                return M_QUIT;
            }
            break;
        case M_NEWPET:
            if (emu_ready && rb->yesno_pop("Hatch a new pet?")) {
                rb->memset(&screen, 0, sizeof(screen));
                cpu_reset();
                release_buttons();
            }
            break;
        case MENU_ATTACHED_USB:
            *usb = true;
            return M_QUIT;
        case M_QUIT:
        case GO_TO_PREVIOUS:
            return M_QUIT;
        default:
            break;
        }
    }
}

enum plugin_status plugin_start(const void *parameter)
{
    bool usb = false;
    (void)parameter;

    rb->lcd_set_viewport(NULL);
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_setfont(FONT_SYSFIXED);
    backlight_ignore_timeout();
    menu(&usb);
    save_state();
    beep_stop();
    backlight_use_settings();
    return usb ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}
