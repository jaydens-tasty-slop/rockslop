/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Playtune score player for the Rockbox Arduboy layer (see ArduboyPlaytune.h).
 * Score format: 0x9c nn = note on (voice c, MIDI note nn), 0x8c = note off,
 * 0xe0 = restart, 0xf0 = stop, a byte < 0x80 starts a 15-bit delay in ms. */
#include "ArduboyPlaytune.h"
#include "ab_glue.h"

#define VOICES 2
#define OP_STOPNOTE  0x80
#define OP_PLAYNOTE  0x90
#define OP_RESTART   0xe0
#define OP_STOP      0xf0

/* equal-tempered frequency in Hz of MIDI notes 0..127 */
static const uint16_t note_hz[128] = {
  8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 15, 15,
  16, 17, 18, 19, 21, 22, 23, 24, 26, 28, 29, 31,
  33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62,
  65, 69, 73, 78, 82, 87, 92, 98, 104, 110, 117, 123,
  131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247,
  262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494,
  523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988,
  1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
  2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951,
  4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902,
  8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544,
};

static uint32_t freq[VOICES];
static uint32_t phase[VOICES];
static volatile bool tune_playing;
static const uint8_t *volatile request;
static volatile bool request_stop;
static const uint8_t *score_start;
static const uint8_t *score_cursor;
static uint32_t wait_samples;
static int8_t score_pitch, request_pitch;
static volatile bool score_muted;
static uint32_t tone_freq, tone_phase;
static volatile uint32_t tone_ms;      /* requested; taken by ab_tunes_fill */
static volatile uint16_t tone_req_hz;
static volatile bool tone_stop;
static uint32_t tone_samples;
static volatile bool tone_active;
static const uint8_t *volatile wave_req;
static volatile uint16_t wave_req_hz, wave_req_n;
static volatile bool wave_stop;
static volatile bool wave_active;
static const uint8_t *wave_ptr;
static uint32_t wave_pos, wave_step, wave_n;

ArduboyPlaytune::ArduboyPlaytune(bool (*)()) {}

void ArduboyPlaytune::playScore(const uint8_t *score)
{
  playScore(score, 0);
}

void ArduboyPlaytune::playScore(const uint8_t *score, int8_t pitch)
{
  request_pitch = pitch;
  request = score;
  tune_playing = true;
  ab_audio_start();
}

void ArduboyPlaytune::stopScore()
{
  request = 0;
  request_stop = true;
  tune_playing = false;
}

void ArduboyPlaytune::playTone(uint16_t hz, uint16_t ms)
{
  tone_ms = ms;
  tone_req_hz = hz;
  tone_active = true;
  ab_audio_start();
}

void ArduboyPlaytune::stopTone()
{
  tone_stop = true;
  tone_active = false;
}

bool ArduboyPlaytune::tonePlaying()
{
  return tone_active;
}

void ArduboyPlaytune::muteScore(bool mute)
{
  score_muted = mute;
}

void ArduboyPlaytune::playWave(uint16_t hz, const uint8_t *wave, uint16_t samples)
{
  wave_req_hz = hz;
  wave_req_n = samples;
  wave_req = wave;
  wave_active = true;
  ab_audio_start();
}

void ArduboyPlaytune::stopWave()
{
  wave_stop = true;
  wave_active = false;
}

bool ArduboyPlaytune::wavePlaying()
{
  return wave_active;
}

bool ArduboyPlaytune::playing()
{
  return tune_playing;
}

void ArduboyPlaytune::closeChannels()
{
  stopScore();
}

static void score_step(int rate)
{
  for (;;) {
    uint8_t command = pgm_read_byte(score_cursor++);
    uint8_t opcode = command & 0xf0;
    uint8_t chan = command & 0x0f;
    if (opcode == OP_STOPNOTE) {
      if (chan < VOICES)
        freq[chan] = 0;
    } else if (opcode == OP_PLAYNOTE) {
      uint8_t note = pgm_read_byte(score_cursor++);
      if (chan < VOICES) {
        int n = note + score_pitch;
        if (n >= 0 && n < 128)
          freq[chan] = note_hz[n];
      }
    } else if (opcode == OP_RESTART) {
      score_cursor = score_start;
    } else if (opcode == OP_STOP) {
      tune_playing = false;
      for (int i = 0; i < VOICES; i++)
        freq[i] = 0;
      break;
    } else if (command < 0x80) {
      unsigned ms = ((unsigned)command << 8) | pgm_read_byte(score_cursor++);
      wait_samples = (uint32_t)((uint64_t)ms * rate / 1000);
      if (wait_samples == 0)
        wait_samples = 1;
      break;
    }
  }
}

extern "C" int ab_tunes_fill(int16_t *buf, int frames, int rate)
{
  if (request_stop) {
    request_stop = false;
    for (int i = 0; i < VOICES; i++)
      freq[i] = 0;
  }
  if (request) {
    score_start = score_cursor = request;
    score_pitch = request_pitch;
    request = 0;
    wait_samples = 0;
    for (int i = 0; i < VOICES; i++)
      freq[i] = 0;
  }
  if (tone_stop) {
    tone_stop = false;
    tone_freq = 0;
    tone_samples = 0;
  }
  if (tone_req_hz) {
    tone_freq = tone_req_hz;
    tone_samples = (uint32_t)((uint64_t)tone_ms * rate / 1000);
    if (tone_samples == 0)
      tone_samples = 1;
    tone_req_hz = 0;
  }
  if (wave_stop) {
    wave_stop = false;
    wave_ptr = 0;
  }
  if (wave_req) {
    wave_ptr = wave_req;
    wave_req = 0;
    wave_n = wave_req_n;
    wave_pos = 0;
    wave_step = (uint32_t)(((uint64_t)wave_req_hz << 16) / (uint32_t)rate);
  }
  bool active = false;
  for (int i = 0; i < frames; i++) {
    if (tune_playing && wait_samples == 0)
      score_step(rate);
    if (tune_playing && wait_samples)
      wait_samples--;
    int sample = 0;
    if (wave_ptr) {
      active = true;
      uint32_t idx = wave_pos >> 16;
      if (idx >= wave_n) {
        wave_ptr = 0;
        wave_active = false;
      } else {
        sample += ((int)pgm_read_byte(wave_ptr + idx) - 128) * 24;
        wave_pos += wave_step;
      }
    }
    if (tone_freq) {
      active = true;
      tone_phase += (uint32_t)(((uint64_t)tone_freq << 32) / (uint32_t)rate);
      sample += (tone_phase & 0x80000000u) ? 3000 : -3000;
      if (--tone_samples == 0) {
        tone_freq = 0;
        tone_active = false;
      }
    }
    for (int v = 0; v < VOICES && !(score_muted && tone_freq); v++) {
      if (!freq[v])
        continue;
      active = true;
      phase[v] += (uint32_t)(((uint64_t)freq[v] << 32) / (uint32_t)rate);
      sample += (phase[v] & 0x80000000u) ? 3000 : -3000;
    }
    buf[i] = ab_audio_enabled() ? sample : 0;
  }
  return active || tune_playing;
}
