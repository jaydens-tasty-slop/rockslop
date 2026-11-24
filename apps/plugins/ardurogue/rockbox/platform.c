/* ArduRogue for Rockbox: display, input, menu and save file.
 * Game code (../src) is unmodified upstream, MPL-2.0; see ../LICENSE.
 *
 * Display: the game draws two 64x64 monochrome panels (map left, status/menus
 * right). They are kept in a 128x64 shadow and drawn in colour, centred on
 * the LCD with a title above and a key legend below.
 * Keys: SELECT click = A (act / confirm), long SELECT or SELECT+LEFT = B
 * (action menu / cancel), hold SELECT+UP for 0.5 s = pause menu (resume /
 * help / quit).
 * Save: the game's own 1 KiB EEPROM image in games/ardurogue.sav; quitting
 * from the menu saves the run, as does a USB connection. */
#include "plugin.h"
#include "lib/helper.h"
#include "lib/playback_control.h"
#include "lib/hint_strip.h"
#include <setjmp.h>

#if LCD_WIDTH < 128 || LCD_HEIGHT < 64 || !defined(HAVE_LCD_COLOR)
#error "ArduRogue needs a colour LCD of at least 128x64"
#endif

#define SAVE_PATH PLUGIN_GAMES_DATA_DIR "/ardurogue.sav"
#define PW 128
#define PH 64
#define X0 ((LCD_WIDTH - PW) / 2)
#define Y0 ((LCD_HEIGHT - PH) / 2 - 4)

enum { AR_NONE, AR_UP, AR_DOWN, AR_LEFT, AR_RIGHT, AR_A, AR_B, AR_SAVEQUIT,
       AR_ABANDON };

extern void ar_run(void);
extern void ar_load_save(void);

static jmp_buf quit_env;
static bool usb;
static uint8_t shadow[PH / 8][PW]; /* bit y&7 of [y/8][x] */
static uint8_t panel[PW];          /* palette of each column: 0 map, 1 text */
static bool sel_down, sel_used;

static const fb_data *pal(void)
{
    static fb_data p[4];
    p[0] = FB_RGBPACK(12, 20, 14);   /* map background */
    p[1] = FB_RGBPACK(120, 230, 130);/* map ink */
    p[2] = FB_RGBPACK(26, 18, 10);   /* text background */
    p[3] = FB_RGBPACK(250, 200, 90); /* text ink */
    return p;
}

static void draw_cols(int c0, int c1)
{
    const fb_data *p = pal();
    fb_data row[PW];
    for(int y = 0; y < PH; ++y) {
        for(int x = c0; x < c1; ++x)
            row[x] = p[panel[x] * 2 + ((shadow[y >> 3][x] >> (y & 7)) & 1)];
        rb->lcd_bitmap(row + c0, X0 + c0, Y0 + y, c1 - c0, 1);
    }
    rb->lcd_update_rect(X0 + c0, Y0, c1 - c0, PH);
}

static void draw_chrome(void)
{
    rb->lcd_set_foreground(LCD_RGBPACK(250, 200, 90));
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_clear_display();
    rb->lcd_putsxy(X0 + 34, Y0 - 16, "ArduRogue");
    rb->lcd_hline(X0, X0 + PW - 1, Y0 - 1);
    rb->lcd_hline(X0, X0 + PW - 1, Y0 + PH);
    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_set_foreground(HINT_STRIP_FG);
    rb->lcd_putsxy(2, LCD_HEIGHT - 3 * HINT_STRIP_LINE, "Sel=A (act/OK)");
    rb->lcd_putsxy(2, LCD_HEIGHT - 2 * HINT_STRIP_LINE, "Hold Sel or Sel+Left=B");
    rb->lcd_putsxy(2, LCD_HEIGHT - HINT_STRIP_LINE, "Hold Sel+Up=menu");
    rb->lcd_setfont(FONT_UI);
}

static void redraw(void)
{
    draw_chrome();
    draw_cols(0, PW);
    rb->lcd_update();
}

void ar_paint(const uint8_t *b, int offset)
{
    for(int x = 0; x < 64; ++x) {
        int dx = offset + x;
        if(dx >= PW)
            break;
        panel[dx] = offset >= 64 ? 1 : 0;
        for(int page = 0; page < PH / 8; ++page)
            shadow[page][dx] = b[page * 64 + x];
    }
    draw_cols(offset, MIN(offset + 64, PW));
}

#ifdef HAVE_BLUETOOTH_HID
static unsigned pad_prev; /* game controller buttons already down */
#endif

void ar_wait(void)
{
    rb->sleep(HZ / 20);
    rb->button_clear_queue();
#ifdef HAVE_BLUETOOTH_HID
    pad_prev = rb->button_status() & BUTTON_PAD_ALL; /* releases cleared too */
#endif
}

unsigned ar_seed(void)
{
    return (unsigned)(*rb->current_tick * 2654435761u) ^ (unsigned)rb->get_time()->tm_sec;
}

void ar_file_load(uint8_t *p, int n)
{
    int fd = rb->open(SAVE_PATH, O_RDONLY);
    rb->memset(p, 0, n);
    if(fd >= 0) {
        rb->read(fd, p, n);
        rb->close(fd);
    }
}

void ar_file_store(const uint8_t *p, int n)
{
    int fd = rb->open(SAVE_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd < 0) {
        rb->splash(HZ * 2, "Cannot write save file");
        return;
    }
    rb->write(fd, p, n);
    rb->close(fd);
}

void ar_quit(void)
{
    longjmp(quit_env, 1);
}

static void help(void)
{
    rb->splash(0, "Move with the D-pad (bumping attacks). SELECT = A: "
        "repeat last action, confirm. Hold SELECT or SELECT+LEFT = B: "
        "action menu (use, drop, throw, shoot, wait, save), cancel.");
    rb->splash(0, "Hold SELECT+UP for 0.5 s: this menu. Save and Quit keeps your run; it is "
        "deleted when loaded, so save again before leaving. Descend the "
        "stairs, find the Amulet of Yendor, return to the surface.");
}

static void about(void)
{
    rb->splash(0, "ArduRogue by Tiberius Brown. github.com/tiberiusbrown/ardurogue. "
        "License: MPL-2.0 (full text in the plugin source tree, ardurogue/LICENSE). "
        "Rockbox port for the YP3 player.");
}

/* Native start / pause menu. Returns AR_NONE to play, else a quit code. */
static int menu(void)
{
    MENUITEM_STRINGLIST(m, "ArduRogue", NULL, "Play / Resume", "Help",
                        "About", "Playback Control", "Save and Quit",
                        "Quit without Saving");
    int selected = 0, ret = AR_NONE;
    bool done = false;
    while(!done) {
        switch(rb->do_menu(&m, &selected, NULL, false)) {
        case 0: done = true; break;
        case 1: help(); break;
        case 2: about(); break;
        case 3: playback_control(NULL); break;
        case 4: ret = AR_SAVEQUIT; done = true; break;
        case 5: ret = AR_ABANDON; done = true; break;
        case MENU_ATTACHED_USB:
            usb = true;
            ret = AR_SAVEQUIT;
            done = true;
            break;
        default: done = true; break; /* cancel: back to the game */
        }
    }
    redraw();
    sel_down = sel_used = false;
    rb->button_clear_queue();
    return ret;
}

/* The pause gesture: SELECT and UP held together for half a second. */
static bool menu_held(void)
{
    long end = *rb->current_tick + HZ / 2;
    while(TIME_BEFORE(*rb->current_tick, end)) {
        if((rb->button_status() & (BUTTON_SELECT | BUTTON_UP))
                != (BUTTON_SELECT | BUTTON_UP))
            return false;
        rb->sleep(HZ / 50);
    }
    return true;
}


int ar_wait_btn(void)
{
    for(;;) {
        int ev = rb->button_get_w_tmo(HZ);
        unsigned base = ev & ~(BUTTON_REL | BUTTON_REPEAT);
        if(ev == BUTTON_NONE)
            continue;
#ifdef HAVE_BLUETOOTH_HID
        /* Game controller: A is the A button, B the B button (long press
           or chord on the keys), Options or PS opens the menu. An event
           carries every button held: act on new presses only. */
        if(!(ev & SYS_EVENT)) {
            unsigned pad = ev & BUTTON_PAD_ALL, fresh = 0;
            if(ev & BUTTON_REL)
                pad_prev &= ~pad;
            else {
                fresh = pad & ~pad_prev;
                pad_prev = pad;
            }
            if(fresh & BUTTON_PAD_A)
                return AR_A;
            if(fresh & BUTTON_PAD_B)
                return AR_B;
            if(fresh & (BUTTON_PAD_MENU | BUTTON_PAD_HOME)) {
                int r = menu();
                pad_prev = rb->button_status() & BUTTON_PAD_ALL;
                if(r != AR_NONE)
                    return r;
                continue;
            }
            base &= ~BUTTON_PAD_ALL; /* the keys held with it */
        }
#endif
        if(ev & BUTTON_REL) {
            bool act = sel_down && !sel_used;
            if(sel_down && !(rb->button_status() & BUTTON_SELECT))
                sel_down = false;
            if(act && !sel_down)
                return AR_A;
            continue;
        }
        if(base & BUTTON_SELECT) {
            if(!(ev & BUTTON_REPEAT) && base == BUTTON_SELECT) {
                sel_down = true;
                sel_used = false;
            } else if(base == BUTTON_SELECT) { /* long press */
                if(!sel_used) {
                    sel_used = true;
                    return AR_B;
                }
            } else if(!(ev & BUTTON_REPEAT)) { /* chord */
                sel_used = true;
                if(base & BUTTON_LEFT)
                    return AR_B;
                if((base & BUTTON_UP) && menu_held()) {
                    int r = menu();
                    if(r != AR_NONE)
                        return r;
                }
            }
            continue;
        }
        switch(base) {
        case BUTTON_UP: return AR_UP;
        case BUTTON_DOWN: return AR_DOWN;
        case BUTTON_LEFT: return AR_LEFT;
        case BUTTON_RIGHT: return AR_RIGHT;
        default:
            if(rb->default_event_handler(ev) == SYS_USB_CONNECTED) {
                usb = true;
                return AR_SAVEQUIT;
            }
        }
    }
}

enum plugin_status plugin_start(const void *parameter)
{
    (void)parameter;
    rb->lcd_set_viewport(NULL);
    rb->lcd_set_drawmode(DRMODE_SOLID);
    backlight_ignore_timeout();
    rb->memset(panel, 0, sizeof(panel));
    redraw();
    ar_load_save();
    if(menu() == AR_NONE && !usb && !setjmp(quit_env))
        ar_run();
    backlight_use_settings();
    return usb ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}
