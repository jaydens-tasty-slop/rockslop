/* RUMBLO for Rockbox: menus, input and the frame loop.
 * RUMBLO by Techcrafter (https://github.com/Techcrafter/RUMBLO), LGPL-3.0,
 * see ../LICENSE. game.c/view.c/assets.c are derived from its source and art.
 *
 * Keys: LEFT/RIGHT move, UP jump, DOWN shield, SELECT attack (hold to
 * repeat). Hold SELECT+UP for a second for the pause menu (the combo is
 * a plausible fighting move, so it is 1000 ms rather than 500). SELECT is also
 * the power key, so SYS_POWEROFF is dropped while fighting. */
#include "plugin.h"
#include "lib/helper.h"
#include "lib/display_text.h"
#include "lib/playback_control.h"
#include "lib/hint_strip.h"
#include "game.h"
#include "view.h"
#include "assets.h"

#define ARENA_X 0
#define ARENA_Y 8
#define FRAME_TICKS (HZ / 20)
#define PAUSE_TICKS HZ     /* Select+Up held 1 s, however slow the frames */

#if LCD_WIDTH < 128 || LCD_HEIGHT < 128 || !defined(HAVE_LCD_COLOR)
#error "RUMBLO needs a colour LCD of at least 128x128"
#endif

struct settings {
    int map;        /* 0, 1, 2 = random */
    int character;  /* 0, 1 */
    int opponent;   /* 0, 1 */
    int difficulty; /* 0..2, 3 = random */
};
static struct settings cfg = { 0, 0, 1, 1 };

static const struct opt_items map_names[] = {
    { "Sky Arena", -1 }, { "Lava Pillars", -1 }, { "Random", -1 } };
static const struct opt_items char_names[] = {
    { "Blue Fighter", -1 }, { "Orange Fighter", -1 } };
static const struct opt_items diff_names[] = {
    { "Easy", -1 }, { "Normal", -1 }, { "Hard", -1 }, { "Random", -1 } };

static struct game game;
static bool fighting; /* a fight is in progress (Resume) */
static pixel_t bg[VIEW_W * VIEW_H], frame[VIEW_W * VIEW_H];
static int bg_screen = -1;

static pixel_t pack(int r, int g, int b)
{
    return LCD_RGBPACK(r, g, b);
}

static void show_text(const char *text)
{
    static char buf[480];
    static char *words[96];
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
    rb->lcd_clear_display();
    rb->lcd_setfont(FONT_UI);
    display_text(n, words, NULL, NULL, true);
    rb->lcd_setfont(FONT_SYSFIXED);
}

static void draw_arena(int screen)
{
    if (screen != bg_screen) {
        view_screen(bg, screen);
        bg_screen = screen;
    }
}

static void hearts(int x, int y, int n, unsigned color)
{
    rb->lcd_set_foreground(color);
    for (int i = 0; i < START_LIVES; i++) {
        if (i < n)
            rb->lcd_fillrect(x + i * 10, y, 8, 8);
        else
            rb->lcd_drawrect(x + i * 10, y, 8, 8);
    }
}

static void hud(void)
{
    rb->lcd_set_foreground(LCD_RGBPACK(230, 230, 230));
    rb->lcd_putsxy(4, ARENA_Y + VIEW_H + 2, "YOU");
    hearts(40, ARENA_Y + VIEW_H + 4, game.f[0].lives, LCD_RGBPACK(60, 140, 255));
    rb->lcd_set_foreground(LCD_RGBPACK(230, 230, 230));
    rb->lcd_putsxy(4, ARENA_Y + VIEW_H + 15, "CPU");
    hearts(40, ARENA_Y + VIEW_H + 17, game.f[1].lives, LCD_RGBPACK(255, 110, 50));
    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_set_foreground(HINT_STRIP_FG);
    rb->lcd_putsxy(2, LCD_HEIGHT - 3 * HINT_STRIP_LINE, "L/R=move  Up=jump");
    rb->lcd_putsxy(2, LCD_HEIGHT - 2 * HINT_STRIP_LINE, "Down=shield  Sel=hit");
    rb->lcd_putsxy(2, LCD_HEIGHT - HINT_STRIP_LINE, "Hold Sel+Up=menu");
    rb->lcd_setfont(FONT_UI);
}

static void present(void)
{
    rb->memcpy(frame, bg, sizeof(frame));
    view_fighters(frame, &game);
    rb->lcd_bitmap((const fb_data *)frame, ARENA_X, ARENA_Y, VIEW_W, VIEW_H);
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_fillrect(0, ARENA_Y + VIEW_H + 4, LCD_WIDTH, LCD_HEIGHT - ARENA_Y - VIEW_H - 4);
    hud();
    rb->lcd_update();
}

static void banner(const char *s)
{
    draw_arena(game_screen(&game));
    rb->memcpy(frame, bg, sizeof(frame));
    view_fighters(frame, &game);
    rb->lcd_bitmap((const fb_data *)frame, ARENA_X, ARENA_Y, VIEW_W, VIEW_H);
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_drawmode(DRMODE_FG);
    rb->lcd_putsxy(48, ARENA_Y + 40, s);
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_update();
}

enum fight_result { FIGHT_PAUSE, FIGHT_OVER, FIGHT_USB };

static enum fight_result fight(void)
{
    long pause_since = -1;
    long next = *rb->current_tick;

    draw_arena(game_screen(&game));
    present();
#ifdef HAVE_BLUETOOTH_HID
    long pad_held = 0;
#endif
    for (;;) {
        struct input in = { 0 };
        long ev;

        while ((ev = rb->button_get(false)) != BUTTON_NONE) {
            if (ev == SYS_POWEROFF)
                continue; /* SELECT is the attack key: do not power off */
            if (ev == BUTTON_UP)
                in.jump = true;
            else if (ev == BUTTON_SELECT || ev == (BUTTON_SELECT | BUTTON_REPEAT))
                in.attack = true;
#ifdef HAVE_BLUETOOTH_HID
            /* game controller: A shoots (held: keeps shooting), B jumps,
             * Options or PS pauses; a press carries every held button */
            else if (ev & BUTTON_PAD_ALL) {
                long pad = ev & BUTTON_PAD_ALL, pressed = 0;
                if (ev & BUTTON_REL) {
                    pad_held &= ~pad;
                } else {
                    if (!(ev & BUTTON_REPEAT)) {
                        pressed = pad & ~pad_held;
                        pad_held = pad;
                    }
                    if (pad & BUTTON_PAD_A)
                        in.attack = true;
                }
                if (pressed & BUTTON_PAD_B)
                    in.jump = true;
                if (pressed & (BUTTON_PAD_MENU | BUTTON_PAD_HOME))
                    return FIGHT_PAUSE;
            }
#endif
            else if (rb->default_event_handler(ev) == SYS_USB_CONNECTED)
                return FIGHT_USB;
        }
        int st = rb->button_status();
        in.left = st & BUTTON_LEFT;
        in.right = st & BUTTON_RIGHT;
        in.down = st & BUTTON_DOWN;
        if (!((st & BUTTON_UP) && (st & BUTTON_SELECT)))
            pause_since = -1;
        else if (pause_since < 0)
            pause_since = *rb->current_tick;
        else if (*rb->current_tick - pause_since >= PAUSE_TICKS)
            return FIGHT_PAUSE;

        game_step(&game, &in);
        if (game.winner >= 0)
            return FIGHT_OVER;
        draw_arena(game_screen(&game));
        present();

        next += FRAME_TICKS;
        long now = *rb->current_tick;
        if (next > now)
            rb->sleep(next - now);
        else
            next = now; /* fell behind: do not race to catch up */
    }
}

static void new_fight(void)
{
    static const char *const count[] = { "3", "2", "1", "FIGHT!" };
    static unsigned seed;
    int map = cfg.map, diff = cfg.difficulty;
    seed = seed * 1103515245u + (unsigned)*rb->current_tick + 12345u;
    if (map == 2)
        map = (seed >> 16) & 1;
    if (diff == 3)
        diff = ((seed >> 16) >> 1) % 3;
    game_start(&game, map, cfg.character, cfg.opponent, diff);
    bg_screen = -1;
    fighting = true;
    draw_arena(game_screen(&game));
    for (int i = 0; i < 4; i++) {
        banner(count[i]);
        rb->sleep(HZ * 7 / 10);
    }
}

static void game_over(void)
{
    draw_arena(game.winner == 0 ? SCREEN_WIN : SCREEN_LOSE);
    rb->memcpy(frame, bg, sizeof(frame));
    rb->lcd_clear_display();
    rb->lcd_bitmap((const fb_data *)frame, ARENA_X, ARENA_Y, VIEW_W, VIEW_H);
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_putsxy(8, ARENA_Y + VIEW_H + 10, "Press SELECT");
    rb->lcd_update();
    bg_screen = -1;
    fighting = false;
    rb->sleep(HZ);
    rb->button_clear_queue();
#ifdef HAVE_BLUETOOTH_HID
    while (rb->button_status() & BUTTON_PAD_A)
        rb->sleep(HZ / 20); /* still shooting: wait for the release */
    rb->button_clear_queue();
#endif
    while (rb->button_get(true) != BUTTON_SELECT)
#ifdef HAVE_BLUETOOTH_HID
        if (rb->button_status() & BUTTON_PAD_A)
            break;
#else
        ;
#endif
}

static void help(void)
{
    show_text("A two-fighter brawl on small platforms. Hit the other "
              "fighter with your shots to knock it off; whoever loses all "
              "three lives first is out. LEFT and RIGHT move, UP jumps, DOWN "
              "raises the shield (it blocks knockback but you cannot move), "
              "SELECT shoots, up to three shots at once. Hold SELECT and UP "
              "together for a second (1000 ms) for the pause menu.");
}

static void about(void)
{
    show_text("RUMBLO 0.1\nby Techcrafter\n"
              "github.com/Techcrafter/RUMBLO\n"
              "License: LGPL-3.0 (full text: LICENSE.txt in rocks/games/rumblo/). "
              "Art by the author, same license.\n"
              "Rockbox port for the YP3 player.");
}

static void settings(void)
{
    MENUITEM_STRINGLIST(m, "Settings", NULL, "Arena", "Your fighter",
                        "Opponent", "Difficulty");
    int sel = 0;
    for (;;) {
        switch (rb->do_menu(&m, &sel, NULL, false)) {
        case 0:
            rb->set_option("Arena", &cfg.map, RB_INT, map_names, 3, NULL);
            break;
        case 1:
            rb->set_option("Your fighter", &cfg.character, RB_INT, char_names, 2, NULL);
            break;
        case 2:
            rb->set_option("Opponent", &cfg.opponent, RB_INT, char_names, 2, NULL);
            break;
        case 3:
            rb->set_option("Difficulty", &cfg.difficulty, RB_INT, diff_names, 4, NULL);
            break;
        default:
            return;
        }
    }
}

enum { M_START, M_NEW, M_SETTINGS, M_HELP, M_ABOUT, M_PLAYBACK, M_QUIT };

enum plugin_status plugin_start(const void *parameter)
{
    MENUITEM_STRINGLIST(m, "RUMBLO", NULL, "Start Fight", "New Fight",
                        "Settings", "Help", "About", "Playback Control", "Quit");
    int sel = 0;
    bool usb = false;
    (void)parameter;

    rb->lcd_set_viewport(NULL);
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_setfont(FONT_SYSFIXED);
    view_init(pack);
    backlight_ignore_timeout();

    for (;;) {
        int r = rb->do_menu(&m, &sel, NULL, false);
        bool run = false;
        switch (r) {
        case M_START:
            if (!fighting)
                new_fight();
            run = true;
            break;
        case M_NEW:
            new_fight();
            run = true;
            break;
        case M_SETTINGS: settings(); break;
        case M_HELP: help(); break;
        case M_ABOUT: about(); break;
        case M_PLAYBACK:
            if (playback_control(NULL))
                usb = true;
            break;
        case MENU_ATTACHED_USB:
            usb = true;
            break;
        case M_QUIT:
        case GO_TO_PREVIOUS:
            goto done;
        }
        if (usb)
            goto done;
        if (run) {
            rb->lcd_clear_display();
            switch (fight()) {
            case FIGHT_OVER:
                game_over();
                break;
            case FIGHT_USB:
                usb = true;
                goto done;
            default:
                break;
            }
            rb->button_clear_queue();
        }
    }
done:
    backlight_use_settings();
    return usb ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}
