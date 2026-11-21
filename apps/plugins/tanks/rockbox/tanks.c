/* Tanks! CE for Rockbox: menus, input and the frame loop.
 * Tanks! CE by commandblockguy (https://github.com/commandblockguy/tanks-ce),
 * MIT, see ../LICENSE. The physics, shell and mine code and the AI in
 * ../engine are the original's; this file, the renderer and the missions
 * are new for this port.
 *
 * Keys: the D-pad drives the tank. SELECT is the aim modifier: SELECT alone
 * (tap) fires, while it is held LEFT/RIGHT turn the barrel, UP aims at the
 * nearest enemy and DOWN lays a mine. Holding SELECT+UP (the aim chord) for
 * a second opens the pause menu. SELECT is also the power key, so
 * SYS_POWEROFF is dropped while playing. */
#include "plugin.h"
#include "lib/helper.h"
#include "lib/display_text.h"
#include "lib/playback_control.h"
#include "../engine/tanks_core.h"

#define FRAME_TICKS 3
#define PAUSE_TICKS HZ /* SELECT+UP is the aim chord: hold 1000 ms */
#define MOVE_UP 1
#define MOVE_DOWN 2
#define MOVE_LEFT 4
#define MOVE_RIGHT 8

#if LCD_WIDTH < 128 || LCD_HEIGHT < 160 || !defined(HAVE_LCD_COLOR)
#error "Tanks needs a colour LCD of at least 128x160"
#endif

static int start_level;     /* menu setting */
static bool playing;        /* a game is in progress (Resume) */
static int tick;

static unsigned last_rgb = 0xffffffffu;

static void pen_color(unsigned rgb)
{
    if (rgb != last_rgb) {
        rb->lcd_set_foreground(LCD_RGBPACK(rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff));
        last_rgb = rgb;
    }
}

void tv_fill(int x, int y, int w, int h, unsigned rgb)
{
    pen_color(rgb);
    rb->lcd_fillrect(x, y, w, h);
}

void tv_line(int x1, int y1, int x2, int y2, unsigned rgb)
{
    pen_color(rgb);
    rb->lcd_drawline(x1, y1, x2, y2);
}

static void show_text(const char *text)
{
    static char buf[520];
    static char *words[110];
    int n = 0;
    char *p;
    rb->strlcpy(buf, text, sizeof(buf));
    for (p = buf; *p && n < (int)ARRAYLEN(words) - 1;) {
        words[n++] = p;
        while (*p && *p != ' ' && *p != '\n')
            p++;
        if (*p == '\n') {
            *p++ = '\0';
            words[n++] = (char *)"";
        } else if (*p) {
            *p++ = '\0';
        }
    }
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_clear_display();
    rb->lcd_setfont(FONT_UI);
    display_text(n, words, NULL, NULL, true);
    rb->lcd_setfont(FONT_SYSFIXED);
}

static void hud(void)
{
    char buf[32];
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_fillrect(0, PLAY_H, LCD_WIDTH, LCD_HEIGHT - PLAY_H);
    rb->lcd_set_foreground(LCD_RGBPACK(220, 220, 220));
    rb->snprintf(buf, sizeof(buf), "Mission %d  Foes %d", tanks_level() + 1, tanks_enemies());
    rb->lcd_putsxy(2, PLAY_H + 2, buf);
    rb->snprintf(buf, sizeof(buf), "Lives %d  Kills %d", tanks_lives(), tanks_kills());
    rb->lcd_putsxy(2, PLAY_H + 12, buf);
}

static void present(void)
{
    last_rgb = 0xffffffffu;
    tanks_draw(tick);
    hud();
    rb->lcd_update();
}

static void banner(const char *a, const char *b)
{
    present();
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_fillrect(8, 50, 112, 34);
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_putsxy(14, 56, a);
    if (b)
        rb->lcd_putsxy(14, 68, b);
    rb->lcd_update();
}

enum play_result { PLAY_MENU, PLAY_LOST_LIFE, PLAY_CLEARED, PLAY_OVER, PLAY_USB };

static enum play_result play(void)
{
    long next = *rb->current_tick;
    bool sel_down = false, chord = false;
    long hold_start = 0;

    rb->lcd_clear_display();
#ifdef HAVE_BLUETOOTH_HID
    long pad_held = 0;
#endif
    for (;;) {
        int fire = 0, mine = 0, aim = 0, turn = 0, move = 0;
        long ev;

        while ((ev = rb->button_get(false)) != BUTTON_NONE) {
            if (ev == SYS_POWEROFF)
                continue; /* SELECT is the fire/aim key */
            if (ev == BUTTON_SELECT) {
                sel_down = true;
                chord = false;
            } else if (ev == (BUTTON_SELECT | BUTTON_REL)) {
                if (sel_down && !chord)
                    fire = 1;
                sel_down = false;
            } else if (ev == (BUTTON_SELECT | BUTTON_REPEAT)) {
                /* long SELECT alone: nothing */
            } else if (sel_down && ev == BUTTON_DOWN) {
                mine = 1;
                chord = true;
#ifdef HAVE_BLUETOOTH_HID
            } else if (ev & BUTTON_PAD_ALL) {
                /* a press carries every held button: act on new ones */
                long pad = ev & BUTTON_PAD_ALL, pressed = 0;
                if (ev & BUTTON_REL) {
                    pad_held &= ~pad;
                } else if (!(ev & BUTTON_REPEAT)) {
                    pressed = pad & ~pad_held;
                    pad_held = pad;
                }
                if (pressed & BUTTON_PAD_A)
                    fire = 1;
                if (pressed & BUTTON_PAD_B)
                    mine = 1;
                if (pressed & (BUTTON_PAD_MENU | BUTTON_PAD_HOME))
                    return PLAY_MENU;
#endif
            } else if (rb->default_event_handler(ev) == SYS_USB_CONNECTED) {
                return PLAY_USB;
            }
        }

        int st = rb->button_status();
        /* From the live state: SELECT and UP pressed in the same tick
         * arrive as one event, so sel_down alone would miss the chord. */
        if ((st & (BUTTON_SELECT | BUTTON_UP)) == (BUTTON_SELECT | BUTTON_UP)) {
            sel_down = true;
            chord = true;
            if (!hold_start)
                hold_start = *rb->current_tick;
            else if (TIME_AFTER(*rb->current_tick, hold_start + PAUSE_TICKS))
                return PLAY_MENU;
        } else {
            hold_start = 0;
        }
        if (sel_down) {
            if (st & BUTTON_LEFT) { turn = -1; chord = true; }
            if (st & BUTTON_RIGHT) { turn = 1; chord = true; }
            if (st & BUTTON_UP) { aim = 1; chord = true; }
        } else {
            if (st & BUTTON_UP) move |= MOVE_UP;
            if (st & BUTTON_DOWN) move |= MOVE_DOWN;
            if (st & BUTTON_LEFT) move |= MOVE_LEFT;
            if (st & BUTTON_RIGHT) move |= MOVE_RIGHT;
            if ((move & (MOVE_UP | MOVE_DOWN)) == (MOVE_UP | MOVE_DOWN))
                move &= ~(MOVE_UP | MOVE_DOWN);
            if ((move & (MOVE_LEFT | MOVE_RIGHT)) == (MOVE_LEFT | MOVE_RIGHT))
                move &= ~(MOVE_LEFT | MOVE_RIGHT);
        }

#ifdef HAVE_BLUETOOTH_HID
        if (st & BUTTON_PAD_L1) turn = -1;
        if (st & BUTTON_PAD_R1) turn = 1;
        if (st & BUTTON_PAD_Y) aim = 1;
#endif
        switch (tanks_step(move, turn, fire, mine, aim)) {
        case TS_NEXT_LEVEL: return PLAY_CLEARED;
        case TS_LOST_LIFE: return PLAY_LOST_LIFE;
        case TS_GAME_OVER: return PLAY_OVER;
        default: break;
        }
        tick++;
        present();

        next += FRAME_TICKS;
        long now = *rb->current_tick;
        if (next > now)
            rb->sleep(next - now);
        else
            next = now;
    }
}

/* One game: returns true to quit the plugin (USB). */
static bool run_game(bool resume)
{
    char buf[32];

    if (!resume) {
        tanks_new_game();
        tanks_begin_level(start_level);
        playing = true;
        rb->snprintf(buf, sizeof(buf), "Mission %d", start_level + 1);
        banner(buf, tanks_level_name(start_level));
        rb->sleep(HZ * 3 / 2);
    }
    for (;;) {
        switch (play()) {
        case PLAY_MENU:
            return false;
        case PLAY_USB:
            return true;
        case PLAY_LOST_LIFE:
            rb->snprintf(buf, sizeof(buf), "%d lives left", tanks_lives());
            banner("Tank destroyed", buf);
            rb->sleep(HZ * 3 / 2);
            tanks_retry_level();
            break;
        case PLAY_CLEARED: {
            int next = tanks_level() + 1;
            banner("Mission cleared!", NULL);
            rb->sleep(HZ * 3 / 2);
            if (next >= tanks_level_count()) {
                banner("All missions", "cleared!");
                rb->sleep(HZ * 2);
                playing = false;
                return false;
            }
            tanks_begin_level(next);
            rb->snprintf(buf, sizeof(buf), "Mission %d", next + 1);
            banner(buf, tanks_level_name(next));
            rb->sleep(HZ * 3 / 2);
            break;
        }
        case PLAY_OVER:
            rb->snprintf(buf, sizeof(buf), "%d kills", tanks_kills());
            banner("Game over", buf);
            rb->sleep(HZ * 2);
            playing = false;
            return false;
        }
        rb->button_clear_queue();
    }
}

static void help(void)
{
    show_text("Destroy every enemy tank to clear a mission. Shells bounce "
              "off walls; yellow tanks lay mines and mines blow up "
              "destructible blocks.\n"
              "D-pad: drive.\nSELECT tap: fire.\n"
              "Hold SELECT and press: LEFT/RIGHT turn the barrel, UP aim at "
              "the nearest enemy, DOWN lay a mine.\n"
              "Hold SELECT+UP for a second (1000 ms; it is also the aim chord): pause menu.");
}

static void about(void)
{
    show_text("Tanks! CE\nby commandblockguy\n"
              "github.com/commandblockguy/tanks-ce\n"
              "License: MIT (full text: LICENSE.txt in rocks/games/tanks/). "
              "Based on Tanks! from Wii Play, not affiliated. The missions "
              "and graphics of this version are new.\n"
              "Rockbox port for the YP3 player.");
}

static void settings(void)
{
    static const struct opt_items names[10] = {
        { "1", -1 }, { "2", -1 }, { "3", -1 }, { "4", -1 }, { "5", -1 },
        { "6", -1 }, { "7", -1 }, { "8", -1 }, { "9", -1 }, { "10", -1 } };
    rb->set_option("Start mission", &start_level, RB_INT, names,
                   tanks_level_count() < 10 ? tanks_level_count() : 10, NULL);
}

enum { M_START, M_NEW, M_SETTINGS, M_HELP, M_ABOUT, M_PLAYBACK, M_QUIT };

enum plugin_status plugin_start(const void *parameter)
{
    MENUITEM_STRINGLIST(m, "Tanks!", NULL, "Start / Resume", "New Game",
                        "Settings", "Help", "About", "Playback Control", "Quit");
    int sel = 0;
    bool usb = false;
    (void)parameter;

    rb->lcd_set_viewport(NULL);
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_background(LCD_BLACK); /* text on the black HUD and banners */
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_setfont(FONT_SYSFIXED);
    backlight_ignore_timeout();
    tanks_seed((unsigned)*rb->current_tick * 2654435761u);

    for (;;) {
        switch (rb->do_menu(&m, &sel, NULL, false)) {
        case M_START:
            usb = run_game(playing);
            break;
        case M_NEW:
            usb = run_game(false);
            break;
        case M_SETTINGS: settings(); break;
        case M_HELP: help(); break;
        case M_ABOUT: about(); break;
        case M_PLAYBACK:
            usb = playback_control(NULL);
            break;
        case MENU_ATTACHED_USB:
            usb = true;
            break;
        case M_QUIT:
        case GO_TO_PREVIOUS:
            goto done;
        }
        if (usb)
            break;
        rb->button_clear_queue();
    }
done:
    backlight_use_settings();
    return usb ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}
