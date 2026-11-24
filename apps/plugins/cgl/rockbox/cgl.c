/***************************************************************************
 * Crisp Game Lib Portable for Rockbox: machine-dependent backend, native
 * menus and game loop.  Library and games: MIT, (c) 2022 ABA Games
 * (https://github.com/abagames/crisp-game-lib-portable, see ../LICENSE).
 * Rockbox port: MIT-compatible, same terms.
 *
 * The game view (up to 200x150) is drawn into an RGB565 offscreen buffer
 * (from the plugin buffer) and blitted centred on the 128x160 LCD, scaled
 * to fit it; the library draws rectangles in game units, so fractional
 * scales stay clean.
 ***************************************************************************/
#include "plugin.h"
#include "../../lib/playback_control.h"
#include "../../lib/helper.h"
#include "../../lib/hint_strip.h"
#include "../../lib/plugin_fpu.h"
#include "../lib/cglp.h"
#include "../lib/machineDependent.h"
#include "../lib/sound.h"
#include "../lib/menu.h"
#include "cgl_audio.h"

#ifdef YP3_PAD
#if CONFIG_KEYPAD != YP3_PAD
#error "cgl is written for the YP3 keypad"
#endif
#endif

#define BTN_UP     BUTTON_UP
#define BTN_DOWN   BUTTON_DOWN
#define BTN_LEFT   BUTTON_LEFT
#define BTN_RIGHT  BUTTON_RIGHT
#define BTN_ACTION BUTTON_SELECT

#define CGL_LAUNCHER PLUGIN_GAMES_DIR "/cgl.rock"

/* SELECT+UP held this long opens the menu: time, not frames, so a game
 * that runs below 60 fps does not need a longer hold. */
#define PAUSE_TICKS (HZ / 2)


/* ---------------------------------------------------------------- view */

static fb_data *view_fb;
static size_t view_fb_size;
static int view_w, view_h;   /* game units */
static int view_pw, view_ph; /* pixels on the LCD */
static int view_ox, view_oy;
static unsigned int view_scale; /* 16.16 pixels per game unit */

static inline fb_data pack(unsigned char r, unsigned char g, unsigned char b)
{
    return LCD_RGBPACK(r, g, b);
}

/* Fill a rectangle given in game units (clipped to the view). */
static void fill(int x, int y, int w, int h, fb_data c)
{
    int x1 = x + w, y1 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > view_w) x1 = view_w;
    if (y1 > view_h) y1 = view_h;
    if (x >= x1 || y >= y1)
        return;
    int px0 = (x * view_scale) >> 16, px1 = (x1 * view_scale) >> 16;
    int py0 = (y * view_scale) >> 16, py1 = (y1 * view_scale) >> 16;
    if (px1 <= px0) px1 = px0 + 1;
    if (py1 <= py0) py1 = py0 + 1;
    if (px1 > view_pw) px1 = view_pw;
    if (py1 > view_ph) py1 = view_ph;
    for (int py = py0; py < py1; py++) {
        fb_data *row = view_fb + py * view_pw;
        for (int px = px0; px < px1; px++)
            row[px] = c;
    }
}

void md_initView(int w, int h)
{
    view_w = w;
    view_h = h;
    unsigned int sx = (LCD_WIDTH << 16) / w;
    unsigned int sy = (LCD_HEIGHT << 16) / h;
    view_scale = sx < sy ? sx : sy;
    view_pw = (w * view_scale) >> 16;
    view_ph = (h * view_scale) >> 16;
    view_ox = (LCD_WIDTH - view_pw) / 2;
    view_oy = (LCD_HEIGHT - view_ph) / 2;
}

void md_drawRect(float x, float y, float w, float h, unsigned char r,
                 unsigned char g, unsigned char b)
{
    fill((int)x, (int)y, (int)w, (int)h, pack(r, g, b));
}

void md_drawCharacter(unsigned char grid[CHARACTER_HEIGHT][CHARACTER_WIDTH][3],
                      float x, float y, int hash)
{
    (void)hash;
    int ix = (int)floorf(x), iy = (int)floorf(y);
    for (int cy = 0; cy < CHARACTER_HEIGHT; cy++) {
        for (int cx = 0; cx < CHARACTER_WIDTH; cx++) {
            unsigned char *p = grid[cy][cx];
            if (p[0] | p[1] | p[2])
                fill(ix + cx, iy + cy, 1, 1, pack(p[0], p[1], p[2]));
        }
    }
}

void md_clearView(unsigned char r, unsigned char g, unsigned char b)
{
    fb_data c = pack(r, g, b);
    int n = view_pw * view_ph;
    for (int i = 0; i < n; i++)
        view_fb[i] = c;
}

/* The area around the view stays black, as in the other ported games. */
void md_clearScreen(unsigned char r, unsigned char g, unsigned char b)
{
    (void)r; (void)g; (void)b;
}

void md_consoleLog(char *msg)
{
    (void)msg;
}

unsigned int md_getSeed(void)
{
    return (unsigned int)*rb->current_tick * 2654435761u;
}

static void present(void)
{
    rb->lcd_bitmap(view_fb, view_ox, view_oy, view_pw, view_ph);
    rb->lcd_update_rect(view_ox, view_oy, view_pw, view_ph);
}

/* --------------------------------------------------------------- sound */

/* The synth runs in the mixer callback and lives in cgl_audio.c, which is
 * built without the FPU; this side converts the library's float times. */
float md_getAudioTime(void)
{
    return (float)cgl_audio_clock() / (float)cgl_audio_rate();
}

void md_playTone(float freq, float duration, float when)
{
    uint32_t rate = cgl_audio_rate();
    if (!rate || freq < 20.0f || freq > 8000.0f)
        return;
    uint32_t start = (uint32_t)(when * rate);
    /* phase wraps at 2^32 once per cycle */
    cgl_audio_tone(start, start + (uint32_t)(duration * rate) + 1,
                   (uint32_t)(freq * (4294967296.0f / (float)rate)));
}

void md_stopTone(void)
{
    cgl_audio_stop_tones();
}

/* ---------------------------------------------------------------- loop */

enum run_result { RUN_PAUSE, RUN_QUIT_GAME, RUN_USB };

/* Frames are paced at 60 fps from the 100 Hz tick. */
static enum run_result run_game_loop(void);

static enum run_result run_game(void)
{
    unsigned saved_bg = rb->lcd_get_background();
    enum run_result res = run_game_loop();
    rb->lcd_set_background(saved_bg);
    rb->lcd_setfont(FONT_UI);
    return res;
}

static enum run_result run_game_loop(void)
{
    long start = *rb->current_tick;
    long frame = 0;
    long chord_since = -1;
    hint_strip_draw("Hold Sel+Up=menu", view_oy + view_ph);
    rb->lcd_update();
    for (;;) {
        long due = start + frame * HZ / 60;
        long now = *rb->current_tick;
        if (now < due) {
            rb->sleep(1);
            continue;
        }
        if (now - due > HZ / 4) { /* fell behind: resync instead of racing */
            start = now;
            frame = 0;
        }
        frame++;

        /* SELECT is also the power key: holding it as a game button raises
         * SYS_POWEROFF, which must not shut the player down mid-game. */
        int ev = rb->button_get(false);
        if (ev != SYS_POWEROFF &&
            rb->default_event_handler(ev) == SYS_USB_CONNECTED)
            return RUN_USB;
        int b = rb->button_status();
        bool menu = (b & BTN_ACTION) && (b & BTN_UP);
        if (!menu)
            chord_since = -1;
        else if (chord_since < 0)
            chord_since = now;
        else if (now - chord_since >= PAUSE_TICKS)
            return RUN_PAUSE;
        /* the menu chord is not passed to the game */
#ifdef HAVE_BLUETOOTH_HID
        /* Options / PS open the pause menu; A is the game button, B the
         * game's second (virtual B) button */
        if (b & (BUTTON_PAD_MENU | BUTTON_PAD_HOME))
            return RUN_PAUSE;
        setButtonState(b & BTN_LEFT, b & BTN_RIGHT, !menu && (b & BTN_UP),
                       b & BTN_DOWN, b & BUTTON_PAD_B,
                       !menu && (b & (BTN_ACTION | BUTTON_PAD_A)));
#else
        setButtonState(b & BTN_LEFT, b & BTN_RIGHT, !menu && (b & BTN_UP),
                       b & BTN_DOWN, false, !menu && (b & BTN_ACTION));
#endif

        updateFrame();
        present();
        if (isQuitToMenuRequested)
            return RUN_QUIT_GAME;
        /* a game slower than 60 fps is always due and never sleeps above:
         * let the other threads (codec, heartbeat) run once a frame */
        rb->yield();
    }
}

/* --------------------------------------------------------------- menus */

#define MAX_HELP_LINES 16
static char help_text[400];
static const char *help_lines[MAX_HELP_LINES];
static int help_count;

static const char *help_get_name(int item, void *data, char *buf, size_t len)
{
    (void)data; (void)buf; (void)len;
    return help_lines[item];
}

static void show_lines(const char *title)
{
    struct simplelist_info info;
    rb->simplelist_info_init(&info, (char *)title, help_count, NULL);
    info.get_name = help_get_name;
    rb->simplelist_show_list(&info);
}

static void add_line(const char *s)
{
    if (help_count < MAX_HELP_LINES)
        help_lines[help_count++] = s;
}

static void show_help(int game)
{
    const char *d = getGame(game).description;
    help_count = 0;
    rb->strlcpy(help_text, d, sizeof(help_text));
    char *p = help_text;
    while (*p && help_count < MAX_HELP_LINES - 6) {
        help_lines[help_count++] = p;
        char *nl = rb->strchr(p, '\n');
        if (!nl)
            break;
        *nl = '\0';
        p = nl + 1;
    }
    add_line("");
    add_line("SELECT: the game button");
    add_line("Arrows: directions");
    add_line("SELECT+UP (hold): menu");
#ifdef HAVE_BLUETOOTH_HID
    add_line("Pad: A/B, Options=menu");
#endif
    show_lines("Help");
}

static void show_about(void)
{
    help_count = 0;
    add_line("Crisp Game Lib");
    add_line("Portable, games by");
    add_line("ABA Games (Kenta Cho)");
    add_line("github.com/abagames/");
    add_line("crisp-game-lib-portable");
    add_line("License: MIT");
    add_line("(c) 2022 ABA Games");
    add_line("Rockbox port: YP3");
    add_line("Full text: LICENSE.txt");
    add_line("in rocks/games/cgl/");
    show_lines("About");
}

static unsigned char menu_title[28];

enum { M_START, M_HELP, M_SOUND, M_PLAYBACK, M_ABOUT, M_BACK, M_QUIT };

/* Returns true to quit the plugin, false to go back to the launcher. */
static bool game_menu(int game)
{
    int sel = 0;
    bool started = false;
    MENUITEM_STRINGLIST(new_menu, menu_title, NULL,
                        "Start", "Help", "Toggle Sound", "Playback Control",
                        "About", "Back to Game List", "Quit");
    MENUITEM_STRINGLIST(running_menu, menu_title, NULL,
                        "Resume", "Help", "Toggle Sound", "Playback Control",
                        "About", "Back to Game List", "Quit");
    rb->strlcpy((char *)menu_title, getGame(game).title, sizeof(menu_title));
    if (!menu_title[0])
        rb->strlcpy((char *)menu_title, "Crisp Game", sizeof(menu_title));
    playback_control_init(NULL);
    rb->button_clear_queue();

    for (;;) {
        int r = rb->do_menu(started ? &running_menu : &new_menu, &sel, NULL,
                            false);
        switch (r) {
        case M_START: {
            if (!started)
                restartGame(game);
            started = true;
            enum run_result res = run_game();
            md_stopTone();
            if (res == RUN_USB)
                return true;
            if (res == RUN_QUIT_GAME)
                started = false;
            rb->button_clear_queue();
            break;
        }
        case M_HELP:
            show_help(game);
            break;
        case M_SOUND:
            toggleSound();
            rb->splash(HZ, isSoundEnabled ? "Sound on" : "Sound off");
            break;
        case M_PLAYBACK:
            if (playback_control(NULL))
                return true;
            break;
        case M_ABOUT:
            show_about();
            break;
        case GO_TO_PREVIOUS:
        case M_BACK:
            return false;
        case M_QUIT:
        case MENU_ATTACHED_USB:
            return true;
        default:
            break;
        }
    }
}

/* Everything with FP code runs in here: plugin_start must have none, or
 * its prologue saves FP registers before access is on (fpucheck.py). */
static enum plugin_status __attribute__((noinline)) cgl_main(void)
{
    size_t size;
    view_fb = rb->plugin_get_buffer(&size);
    view_fb = (fb_data *)(((uintptr_t)view_fb + 3) & ~(uintptr_t)3);
    view_fb_size = size;
    if (view_fb_size < LCD_WIDTH * LCD_HEIGHT * sizeof(fb_data)) {
        rb->splash(HZ * 2, "Not enough memory");
        return PLUGIN_ERROR;
    }

    rb->lcd_set_backdrop(NULL);
    backlight_ignore_timeout();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    initGame();
    cgl_audio_open();
    if (rb->audio_status()) /* leave music alone unless asked */
        disableSound();

    bool quit = game_menu(1);

    cgl_audio_close();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    backlight_use_settings();
    if (!quit)
        return rb->plugin_open(CGL_LAUNCHER, NULL);
    return PLUGIN_OK;
}

enum plugin_status plugin_start(const void *param)
{
    (void)param;
    plugin_fpu_enable(true);
    enum plugin_status ret = cgl_main();
    plugin_fpu_enable(false);
    return ret;
}
