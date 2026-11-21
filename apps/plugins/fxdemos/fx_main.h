/* Shared Rockbox glue for the fxdemos plugins: start/pause menu with Help,
 * About and Playback Control, input latching, frame pacing and framebuffer
 * access. Rockbox port code, released under CC0.
 *
 * A demo includes plugin.h and fx_common.h, defines
 *   FX_NAME      menu title
 *   FX_HELP      comma separated string literals for the Help screen
 *   FX_ABOUT     the same for About (credits, URL, licence; the port credit
 *                is appended here)
 * and optionally
 *   FX_SETTINGS  name of a void function(void) shown as "Settings"
 *   FX_PAUSE_MS  how long SELECT+UP must be held to open the menu (default
 *                500; use 1000 where the chord is a normal gameplay input)
 *   FX_STOP      name of a void function(void) called when play ends (menu,
 *                USB, exit), to stop sound started in demo_start
 *   FX_MIN_TICKS minimum ticks per frame (default 3 = ~30 fps)
 * and implements
 *   static bool demo_init(void);   once, false = cannot run
 *   static void demo_start(void);  whenever play starts or resumes
 *   static bool demo_frame(unsigned keys, unsigned edge, int dt_ms);
 * demo_frame draws into fx_fb. The menu opens when SELECT+UP are held
 * together for FX_PAUSE_MS (the demo is not asked). keys are the BUTTON_*
 * bits held, edge the direction presses (including key repeat) since the
 * last frame plus BUTTON_SELECT for a SELECT tap that was not part of a
 * SELECT+direction chord. demo_frame returns true to open the menu anyway.
 * SELECT is also the power key, so a demo must never need it held. Before returning, a demo may narrow fx_y0..fx_y1, the
 * rows pushed to the LCD (default: all), and call fx_boost(false) while it
 * is idle.
 *
 * Not built for the host test harnesses (FX_HOST). */
#ifndef FX_HOST

#include "lib/display_text.h"
#include "lib/helper.h"
#include "lib/playback_control.h"

#ifndef FX_PAUSE_MS
#define FX_PAUSE_MS 500
#endif

#ifndef FX_MIN_TICKS
#define FX_MIN_TICKS 3
#endif

static int fx_y0, fx_y1;
static bool fx_usb;
static bool fx_boosted;

static void fx_boost(bool on)
{
  if (on != fx_boosted)
  {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(on);
#endif
    fx_boosted = on;
  }
}

static bool demo_init(void);
static void demo_start(void);
static bool demo_frame(unsigned keys, unsigned edge, int dt_ms);
#ifdef FX_SETTINGS
static void FX_SETTINGS(void);
#endif

static bool fx_text_screen(char **words, int count)
{
  if (display_text(count, words, NULL, NULL, true))
  {
    fx_usb = true;
    return true;
  }
  return false;
}

/* Start menu, also the pause menu. Returns true to play, false to quit. */
static bool fx_menu(bool started)
{
#ifdef FX_SETTINGS
  MENUITEM_STRINGLIST(start_menu, FX_NAME, NULL,
                      "Start", "Settings", "Help", "About", "Playback Control", "Quit");
  MENUITEM_STRINGLIST(pause_menu, FX_NAME, NULL,
                      "Resume", "Settings", "Help", "About", "Playback Control", "Quit");
  const int skip = 0;
#else
  MENUITEM_STRINGLIST(start_menu, FX_NAME, NULL,
                      "Start", "Help", "About", "Playback Control", "Quit");
  MENUITEM_STRINGLIST(pause_menu, FX_NAME, NULL,
                      "Resume", "Help", "About", "Playback Control", "Quit");
  const int skip = 1; /* no Settings entry: shift to the full layout */
#endif
  static char *help[] = { FX_HELP };
  static char *about[] = { FX_ABOUT, "", "Rockbox port: YP3." };
  int sel = 0;

  fx_boost(false);
  rb->lcd_set_viewport(NULL);
  rb->lcd_set_foreground(LCD_WHITE);
  rb->lcd_set_background(LCD_BLACK);
  rb->lcd_clear_display();

  while (true)
  {
    int r = rb->do_menu(started ? &pause_menu : &start_menu, &sel, NULL, false);

    if (r == MENU_ATTACHED_USB)
    {
      fx_usb = true;
      return false;
    }
    if (r < 0)
      return false;
    if (r > 0)
      r += skip;

    switch (r)
    {
      case 0:
        return true;
      case 1:
#ifdef FX_SETTINGS
        FX_SETTINGS();
#endif
        break;
      case 2:
        if (fx_text_screen(help, ARRAYLEN(help)))
          return false;
        break;
      case 3:
        if (fx_text_screen(about, ARRAYLEN(about)))
          return false;
        break;
      case 4:
        playback_control(NULL);
        break;
      default:
        return false;
    }
  }
}

static bool fx_play(void)
{
  unsigned latched = 0;
  bool sel_prev = false, combo = false;
#ifdef HAVE_BLUETOOTH_HID
  bool pad_quit = false;
#endif
  unsigned long last = *rb->current_tick, hold_start = 0;

  rb->lcd_set_viewport(NULL);
  rb->button_clear_queue();
  fx_boost(true);
  demo_start();

  while (true)
  {
    int ev;
    while ((ev = rb->button_get(false)) != BUTTON_NONE)
    {
      if (!(ev & BUTTON_REL))
        latched |= ev & FX_DIRS;
      if (rb->default_event_handler(ev) == SYS_USB_CONNECTED)
        fx_usb = true;
    }
    if (fx_usb)
      break;

    unsigned keys = rb->button_status() | latched;
#ifdef HAVE_BLUETOOTH_HID
    /* Options opens the menu, PS leaves the plugin */
    if (keys & BUTTON_PAD_HOME)
    {
      pad_quit = true;
      break;
    }
    if (keys & BUTTON_PAD_MENU)
      break;
#endif
    if ((rb->button_status() & (BUTTON_SELECT | BUTTON_UP)) == (BUTTON_SELECT | BUTTON_UP))
    {
      if (!hold_start)
        hold_start = *rb->current_tick;
      else if ((*rb->current_tick - hold_start) * 1000 >= FX_PAUSE_MS * HZ)
        break;
    }
    else
      hold_start = 0;
    bool sel = keys & BUTTON_SELECT;
    bool tap = false;

    if (sel && (keys & FX_DIRS))
      combo = true;
    if (!sel && sel_prev && !combo)
      tap = true;
    if (!sel)
      combo = false;
    sel_prev = sel;

    unsigned long now = *rb->current_tick;
    int dt = (int)(now - last) * (1000 / HZ);
    last = now;

    unsigned edge = latched & FX_DIRS;
    latched = 0;
    if (tap)
      edge |= BUTTON_SELECT;

    fx_y0 = 0;
    fx_y1 = FX_H;
    if (demo_frame(keys, edge, dt < 1 ? 1 : (dt > 200 ? 200 : dt)))
      break;
    if (fx_y1 > fx_y0)
      rb->lcd_update_rect(0, fx_y0, FX_W, fx_y1 - fx_y0);

    while (*rb->current_tick - now < FX_MIN_TICKS)
      rb->sleep(1);
  }

#ifdef FX_STOP
  FX_STOP();
#endif
  fx_boost(false);
#ifdef HAVE_BLUETOOTH_HID
  return !fx_usb && !pad_quit;
#else
  return !fx_usb;
#endif
}

enum plugin_status plugin_start(const void *parameter)
{
  (void)parameter;

  backlight_ignore_timeout();
  playback_control_init(NULL);

  rb->lcd_set_viewport(NULL);
  fx_fb = (*rb->screens[SCREEN_MAIN]->current_viewport)->buffer->fb_ptr;

  if (demo_init())
  {
    bool started = false;
    while (fx_menu(started))
    {
      started = true;
      if (!fx_play())
        break;
    }
  }

  backlight_use_settings();
  return fx_usb ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}

#endif /* FX_HOST */
