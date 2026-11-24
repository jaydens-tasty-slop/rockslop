/* Plugin entry point shared by all SAF games: start menu, input mapping,
 * display and persistent save memory. Include after the game's header, with
 * SAFRB_HELP (help text words, "" = newline) and SAFRB_HINT (short key hint
 * for the status strip) defined. Released under CC0. */
#include "lib/display_text.h"
#include "lib/helper.h"
#include "lib/playback_control.h"
#include "lib/hint_strip.h"

#define FRAME_TICKS      (HZ / SAF_FPS)           /* 4 ticks at 100 Hz */
#define DIR_MASK (BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT)

static fb_data _rb_palette[256];
static unsigned _rb_latched; /* presses seen since the last frame */
static bool _rb_usb, _rb_ended;

static void _rb_load_save(void)
{
  int fd = rb->open(_RB_SAVE_FILE, O_RDONLY);
  if (fd >= 0)
  {
    rb->read(fd, _rb_save, SAF_SAVE_SIZE);
    rb->close(fd);
  }
}

static void _rb_store_save(void)
{
  if (!_rb_save_dirty)
    return;

  int fd = rb->open(_RB_SAVE_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd >= 0)
  {
    rb->write(fd, _rb_save, SAF_SAVE_SIZE);
    rb->close(fd);
    _rb_save_dirty = false;
  }
}

static void _rb_draw_strip(void)
{
  hint_strip_draw(SAFRB_HINT "|Sel=A  Sel+Left/Right=B|Sel+Down=C|Hold Sel+Up=menu",
                  2 * SAF_SCREEN_HEIGHT);
  rb->lcd_update();
}

static void _rb_present(void)
{
  static fb_data row[2 * SAF_SCREEN_WIDTH];
  const uint8_t *s = _rb_screen;

  for (int y = 0; y < SAF_SCREEN_HEIGHT; ++y)
  {
    for (int x = 0; x < SAF_SCREEN_WIDTH; ++x)
    {
      fb_data c = _rb_palette[*s++];
      row[2 * x] = c;
      row[2 * x + 1] = c;
    }
    rb->lcd_bitmap(row, 0, 2 * y, 2 * SAF_SCREEN_WIDTH, 1);
    rb->lcd_bitmap(row, 0, 2 * y + 1, 2 * SAF_SCREEN_WIDTH, 1);
  }

  rb->lcd_update_rect(0, 0, 2 * SAF_SCREEN_WIDTH, 2 * SAF_SCREEN_HEIGHT);
}

static void _rb_handle_event(int ev)
{
  if (ev == BUTTON_NONE)
    return;

  if (ev == SYS_POWEROFF)
    return; /* a held SELECT must not power off during play */

  if (!(ev & (BUTTON_REL | BUTTON_REPEAT)))
    _rb_latched |= ev & (DIR_MASK | BUTTON_SELECT);

  if (rb->default_event_handler(ev) == SYS_USB_CONNECTED)
    _rb_usb = true;
}

static void _rb_wait_release(void)
{
  while (rb->button_status() & (BUTTON_SELECT | BUTTON_UP))
    rb->sleep(HZ / 50);
  rb->button_clear_queue();
  _rb_latched = 0;
}

/* Start menu, also the pause menu. Returns true to play, false to quit. */
static bool _rb_menu(bool started)
{
  MENUITEM_STRINGLIST(start_menu, SAF_PROGRAM_NAME, NULL,
                      "Start", "Help", "About", "Playback Control", "Quit");
  MENUITEM_STRINGLIST(pause_menu, SAF_PROGRAM_NAME, NULL,
                      "Resume", "Help", "About", "Playback Control", "Restart", "Quit");
  static char *help[] = { SAFRB_HELP };
  static char *about[] = { SAFRB_ABOUT,
    "", "Frontend: SAF for", "Rockbox, YP3 port", "(CC0).",
    "", "SAF by drummyfish,", "CC0. Rockbox is", "GPLv2+." };
  int sel = 0;

  _rb_store_save();
  rb->lcd_set_viewport(NULL);
  rb->lcd_set_background(LCD_BLACK);
  rb->lcd_clear_display();

  while (true)
  {
    int r = rb->do_menu(started ? &pause_menu : &start_menu, &sel, NULL, false);

    if (r == MENU_ATTACHED_USB)
    {
      _rb_usb = true;
      return false;
    }

    switch (r)
    {
      case 0:
        return true;
      case 1:
        if (display_text(ARRAYLEN(help), help, NULL, NULL, true))
        {
          _rb_usb = true;
          return false;
        }
        break;
      case 2:
        if (display_text(ARRAYLEN(about), about, NULL, NULL, true))
        {
          _rb_usb = true;
          return false;
        }
        break;
      case 3:
        playback_control(NULL);
        break;
      case 4:
        if (started)
        {
          SAF_FE_init();
          return true;
        }
        return false;
      default:
        return false;
    }
  }
}

/* Plays until the menu is requested (returns true) or the game/user quits
 * (returns false). */
static bool _rb_play(void)
{
  unsigned long next = *rb->current_tick + FRAME_TICKS;
  bool sel_prev = false, combo = false;
  int a_frames = 0;
  long chord_since = -1;

  _rb_draw_strip();
  _rb_latched = 0;

  while (true)
  {
    long wait;
    while ((wait = (long)(next - *rb->current_tick)) > 0)
    {
      _rb_handle_event(rb->button_get_w_tmo(wait));
      if (_rb_usb)
        return false;
    }
    next += FRAME_TICKS;

    unsigned st = rb->button_status() | _rb_latched;
    _rb_latched = 0;
    bool sel = st & BUTTON_SELECT;
    bool up = st & BUTTON_UP, down = st & BUTTON_DOWN;
    bool left = st & BUTTON_LEFT, right = st & BUTTON_RIGHT;

    rb->memset(_rb_buttons, 0, sizeof(_rb_buttons));

#ifdef HAVE_BLUETOOTH_HID
    /* Options opens the pause menu, PS leaves the game */
    if (st & BUTTON_PAD_MENU)
    {
      _rb_sound_stop();
      _rb_wait_release();
      return true;
    }
    if (st & BUTTON_PAD_HOME)
      return false;
#endif

    /* Holding SELECT+UP for half a second opens the menu; the chord itself
     * is not passed to the game. */
    if (sel && up)
    {
      if (chord_since < 0)
        chord_since = *rb->current_tick;
      combo = true;
      sel_prev = true;
      if (*rb->current_tick - chord_since >= HZ / 2)
      {
        _rb_sound_stop();
        _rb_wait_release();
        return true;
      }
      if (!SAF_FE_loop())
      {
        _rb_present();
        _rb_ended = true;
        return true;
      }
      _rb_present();
      continue;
    }
    chord_since = -1;

    if (sel)
    {
      if (!sel_prev)
        combo = false;
      if (down || left || right)
        combo = true;
      /* SELECT+LEFT/RIGHT = B (also steer), SELECT+DOWN = C */
      _rb_buttons[SAF_BUTTON_B] = left || right;
      _rb_buttons[SAF_BUTTON_C] = down;
      _rb_buttons[SAF_BUTTON_LEFT] = left;
      _rb_buttons[SAF_BUTTON_RIGHT] = right;
    }
    else
    {
      if (sel_prev && !combo)
        a_frames = 3; /* a tap acts on release, held long enough to be seen */
      _rb_buttons[SAF_BUTTON_UP] = up;
      _rb_buttons[SAF_BUTTON_DOWN] = down;
      _rb_buttons[SAF_BUTTON_LEFT] = left;
      _rb_buttons[SAF_BUTTON_RIGHT] = right;
    }
    sel_prev = sel;

    if (a_frames > 0)
    {
      _rb_buttons[SAF_BUTTON_A] = 1;
      a_frames--;
    }

#ifdef HAVE_BLUETOOTH_HID
    /* pad: A, B, X are the game's buttons A, B, C */
    if (st & BUTTON_PAD_A)
      _rb_buttons[SAF_BUTTON_A] = 1;
    if (st & BUTTON_PAD_B)
      _rb_buttons[SAF_BUTTON_B] = 1;
    if (st & BUTTON_PAD_X)
      _rb_buttons[SAF_BUTTON_C] = 1;
#endif

    if (!SAF_FE_loop())
    {
      _rb_present();
      _rb_ended = true; /* game over: back to the start menu */
      return true;
    }
    _rb_present();
  }
}

enum plugin_status plugin_start(const void *parameter)
{
  _SAF_UNUSED(parameter);

  for (int i = 0; i < 256; ++i)
  {
    uint8_t r, g, b;
    SAF_colorToRGB(i, &r, &g, &b);
    _rb_palette[i] = FB_RGBPACK(r, g, b);
  }

  _rb_load_save();
  _rb_sound_open();
  backlight_ignore_timeout();
  playback_control_init(NULL);
  SAF_FE_init();

  bool started = false;
  while (_rb_menu(started))
  {
    started = true;
    if (!_rb_play())
      break;
    if (_rb_ended)
    {
      _rb_ended = false;
      started = false;
      SAF_FE_init();
    }
  }

  _rb_store_save();
  _rb_sound_close();
  backlight_use_settings();
  return _rb_usb ? PLUGIN_USB_CONNECTED : PLUGIN_OK;
}
