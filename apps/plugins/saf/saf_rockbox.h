/* SAF (Small Abstract Fish) frontend for Rockbox plugins, YP3 port.
 * Included from the SAF_PLATFORM_ROCKBOX branch of saf.h. Released under CC0.
 *
 * The 64x64 SAF screen is kept as one byte per pixel and expanded 2x into
 * the top 128x128 of the LCD; the rest is a status strip (see saf_main.h). */
#include "plugin.h"

#undef SAF_PLATFORM_NAME
#define SAF_PLATFORM_NAME "Rockbox"
#undef SAF_PLATFORM_RAM
#define SAF_PLATFORM_RAM 100000
#undef SAF_PLATFORM_HAS_SOUND
#define SAF_PLATFORM_HAS_SOUND 1

#define _RB_SAVE_FILE PLUGIN_GAMES_DATA_DIR "/" SAF_PROGRAM_NAME ".sav"

static uint8_t _rb_screen[SAF_SCREEN_WIDTH * SAF_SCREEN_HEIGHT];
static uint8_t _rb_buttons[SAF_BUTTONS]; /* set by saf_main.h each frame */
static uint8_t _rb_save[SAF_SAVE_SIZE];
static bool _rb_save_dirty;

void SAF_FE_drawPixel(uint8_t x, uint8_t y, uint8_t color)
{
  _rb_screen[y * SAF_SCREEN_WIDTH + x] = color;
}

/* Sound: one square-wave voice on PCM_MIXER_CHAN_BEEP (the mixer, not
 * the playback stream). The mixer callback runs in interrupt context; this plugin is
 * built without the FPU, so it is integer-only. */
#define _RB_SND_FRAMES 256
#define _RB_SND_AMP    2500

static int16_t _rb_snd_pcm[_RB_SND_FRAMES * 2];
static volatile uint32_t _rb_snd_left;  /* frames left in the current sound */
static volatile uint32_t _rb_snd_step, _rb_snd_slide;  /* phase increments */
static uint32_t _rb_snd_phase, _rb_snd_rate;
static bool _rb_snd_open;

static void _rb_snd_more(const void **start, size_t *size)
{
  uint32_t left = _rb_snd_left, step = _rb_snd_step;

  for (int i = 0; i < _RB_SND_FRAMES; ++i)
  {
    int s = 0;
    if (left)
    {
      _rb_snd_phase += step;
      step += _rb_snd_slide;
      s = (_rb_snd_phase & 0x80000000u) ? _RB_SND_AMP : -_RB_SND_AMP;
      --left;
    }
    _rb_snd_pcm[2 * i] = _rb_snd_pcm[2 * i + 1] = s;
  }
  _rb_snd_left = left;
  _rb_snd_step = step;
  *start = _rb_snd_pcm;
  *size = sizeof(_rb_snd_pcm);
}

static void _rb_sound_open(void)
{
  static const struct mixer_play_cbs cbs = { .get_more = _rb_snd_more };

  _rb_snd_rate = rb->mixer_get_frequency();
  rb->mixer_channel_set_amplitude(PCM_MIXER_CHAN_BEEP, MIX_AMP_UNITY);
  rb->mixer_channel_play_data(PCM_MIXER_CHAN_BEEP, &cbs, NULL, 0);
  _rb_snd_open = true;
}

static void _rb_sound_close(void)
{
  if (_rb_snd_open)
    rb->mixer_channel_stop(PCM_MIXER_CHAN_BEEP);
  _rb_snd_open = false;
}

static void _rb_sound_stop(void)
{
  _rb_snd_left = 0;
}

void SAF_FE_playSound(uint8_t sound)
{
  /* start Hz, end Hz, duration ms (the pitch slides linearly) */
  static const uint16_t snd[SAF_SOUNDS][3] = {
    { 880, 880, 120 },   /* BEEP */
    { 2000, 2000, 12 },  /* CLICK */
    { 220, 40, 300 },    /* BOOM */
    { 160, 100, 90 },    /* BUMP */
  };

  if (!_rb_snd_open || sound >= SAF_SOUNDS)
    return;

  uint32_t frames = (_rb_snd_rate / 1000) * snd[sound][2];
  uint32_t s0 = (uint32_t)(((uint64_t)snd[sound][0] << 32) / _rb_snd_rate);
  uint32_t s1 = (uint32_t)(((uint64_t)snd[sound][1] << 32) / _rb_snd_rate);

  rb->pcm_play_lock();
  _rb_snd_step = s0;
  _rb_snd_slide = (uint32_t)((int32_t)(s1 - s0) / (int32_t)frames);
  _rb_snd_left = frames;
  rb->pcm_play_unlock();
}

void SAF_FE_save(uint8_t index, uint8_t data)
{
  _rb_save[index] = data;
  _rb_save_dirty = true;
}

uint8_t SAF_FE_load(uint8_t index)
{
  return _rb_save[index];
}

uint8_t SAF_FE_buttonPressed(uint8_t button)
{
  return _rb_buttons[button];
}

const char *SAF_FE_extension(const char *string)
{
  _SAF_UNUSED(string);
  return SAF_FE_emptyString;
}
