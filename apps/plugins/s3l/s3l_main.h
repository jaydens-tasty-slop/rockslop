/* Shared Rockbox glue for the small3dlib / tinyphysicsengine demos: a 128x128
 * software framebuffer, status strip, start/pause menu and input. Released
 * under CC0.
 *
 * A demo defines S3LRB_NAME, S3LRB_HINT (status strip hint), S3LRB_HELP and
 * S3LRB_ABOUT (word lists for display_text), optionally S3LRB_SETTINGS (a
 * void function(void) shown as "Settings" in the menu), and implements
 *   static void demo_init(void);
 *   static void demo_frame(unsigned keys, int dt_ms);
 * demo_frame renders into s3lrb_fb (via s3lrb_set_pixel) and handles input;
 * keys holds the BUTTON_* bits held this frame, including BUTTON_SELECT.
 * Holding SELECT+UP for S3LRB_MENU_MS (default 500; demos that use the
 * chord in play define 1000) opens the menu; below that, the chord is hidden
 * from demo_frame. */
#include "plugin.h"
#include "lib/display_text.h"
#include "lib/helper.h"
#include "lib/playback_control.h"
#include "lib/hint_strip.h"

#define S3LRB_W 128
#define S3LRB_H 128

#if LCD_PIXELFORMAT == RGB565SWAPPED
#define S3LRB_FROM565(c) ((fb_data)((((c) >> 8) | ((c) << 8)) & 0xffff))
#elif LCD_PIXELFORMAT == RGB565
#define S3LRB_FROM565(c) ((fb_data)(c))
#else
#define S3LRB_FROM565(c) FB_RGBPACK((((c) >> 11) << 3), ((((c) >> 5) & 63) << 2), (((c) & 31) << 3))
#endif

static fb_data s3lrb_fb[S3LRB_W * S3LRB_H];
#ifndef S3LRB_MENU_MS
#define S3LRB_MENU_MS 500
#endif

static bool s3lrb_usb;
static int s3lrb_fps;

static inline void s3lrb_set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
  s3lrb_fb[y * S3LRB_W + x] = FB_RGBPACK(r, g, b);
}

static void demo_init(void);
static void demo_frame(unsigned keys, int dt_ms);
#ifdef S3LRB_SETTINGS
static void S3LRB_SETTINGS(void);
#endif

#define S3LRB_DIRS (BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT)

static void s3lrb_draw_strip(void)
{
  hint_strip_draw(S3LRB_HINT "|Hold Sel+Up=menu", S3LRB_H);
  rb->lcd_update();
}

static void s3lrb_draw_fps(void)
{
  rb->lcd_set_foreground(HINT_STRIP_FG);
  rb->lcd_set_background(LCD_BLACK);
  rb->lcd_setfont(FONT_SYSFIXED);
  rb->lcd_putsxyf(LCD_WIDTH - 36, LCD_HEIGHT - HINT_STRIP_LINE, "FPS %2d ", s3lrb_fps);
  rb->lcd_update_rect(LCD_WIDTH - 36, LCD_HEIGHT - HINT_STRIP_LINE, 36, HINT_STRIP_LINE);
}

static bool s3lrb_text(char **words, int count)
{
  if (display_text(count, words, NULL, NULL, true))
  {
    s3lrb_usb = true;
    return true;
  }
  return false;
}

/* Start menu, also the pause menu. Returns true to play, false to quit. */
static bool s3lrb_menu(bool started)
{
#ifdef S3LRB_SETTINGS
  MENUITEM_STRINGLIST(start_menu, S3LRB_NAME, NULL,
                      "Start", "Settings", "Help", "About", "Playback Control", "Quit");
  MENUITEM_STRINGLIST(pause_menu, S3LRB_NAME, NULL,
                      "Resume", "Settings", "Help", "About", "Playback Control", "Quit");
  const int skip = 0;
#else
  MENUITEM_STRINGLIST(start_menu, S3LRB_NAME, NULL,
                      "Start", "Help", "About", "Playback Control", "Quit");
  MENUITEM_STRINGLIST(pause_menu, S3LRB_NAME, NULL,
                      "Resume", "Help", "About", "Playback Control", "Quit");
  const int skip = 1; /* no Settings entry: shift to the full layout */
#endif
  static char *help[] = { S3LRB_HELP };
  static char *about[] = { S3LRB_ABOUT,
    "", "Rockbox port: YP3.", "small3dlib and", "tinyphysicsengine by", "drummyfish, CC0." };
  int sel = 0;

  rb->lcd_set_viewport(NULL);
  rb->lcd_set_background(LCD_BLACK);
  rb->lcd_clear_display();

  while (true)
  {
    int r = rb->do_menu(started ? &pause_menu : &start_menu, &sel, NULL, false);

    if (r == MENU_ATTACHED_USB)
    {
      s3lrb_usb = true;
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
#ifdef S3LRB_SETTINGS
        S3LRB_SETTINGS();
#endif
        break;
      case 2:
        if (s3lrb_text(help, ARRAYLEN(help)))
          return false;
        break;
      case 3:
        if (s3lrb_text(about, ARRAYLEN(about)))
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

static bool s3lrb_play(void)
{
  unsigned latched = 0;
  unsigned long chord_since = 0;
  unsigned long last = *rb->current_tick, fps_t = last;
  int frames = 0;

  s3lrb_draw_strip();
  rb->button_clear_queue();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
  rb->cpu_boost(true);
#endif

  while (true)
  {
    int ev;
    while ((ev = rb->button_get(false)) != BUTTON_NONE)
    {
      if (ev == SYS_POWEROFF)
        continue; /* a held SELECT must not power off during play */
      if (!(ev & BUTTON_REL))
        latched |= ev & (S3LRB_DIRS | BUTTON_SELECT);
      if (rb->default_event_handler(ev) == SYS_USB_CONNECTED)
        s3lrb_usb = true;
    }
    if (s3lrb_usb)
      break;

    unsigned keys = rb->button_status() | latched;
    latched = 0;
#ifdef HAVE_BLUETOOTH_HID
    /* Options opens the menu, PS leaves */
    if (keys & BUTTON_PAD_HOME)
      break;
    if (keys & BUTTON_PAD_MENU)
    {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
      rb->cpu_boost(false);
#endif
      rb->button_clear_queue();
      return true;
    }
#endif
    if ((keys & (BUTTON_SELECT | BUTTON_UP)) == (BUTTON_SELECT | BUTTON_UP))
    {
      if (!chord_since)
        chord_since = *rb->current_tick ? *rb->current_tick : 1;
      else if ((*rb->current_tick - chord_since) * (1000 / HZ) >= S3LRB_MENU_MS)
      {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(false);
#endif
        rb->button_clear_queue();
        return true;
      }
#if S3LRB_MENU_MS <= 500
      keys &= ~(BUTTON_SELECT | BUTTON_UP);
#endif
    }
    else
      chord_since = 0;

    unsigned long now = *rb->current_tick;
    int dt = (int)(now - last) * (1000 / HZ);
    last = now;
    demo_frame(keys, dt < 10 ? 10 : (dt > 100 ? 100 : dt));

    rb->lcd_bitmap(s3lrb_fb, 0, 0, S3LRB_W, S3LRB_H);
    rb->lcd_update_rect(0, 0, S3LRB_W, S3LRB_H);

    frames++;
    if (now - fps_t >= HZ)
    {
      s3lrb_fps = frames * HZ / (int)(now - fps_t);
      frames = 0;
      fps_t = now;
      s3lrb_draw_fps();
    }
    rb->yield();
  }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
  rb->cpu_boost(false);
#endif
  return false;
}

enum plugin_status plugin_start(const void *parameter)
{
  (void)parameter;

  backlight_ignore_timeout();
  playback_control_init(NULL);
  demo_init();

  bool started = false;
  while (s3lrb_menu(started))
  {
    started = true;
    if (!s3lrb_play())
      break;
  }

  backlight_use_settings();
  return s3lrb_usb ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}
