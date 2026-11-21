/* Playtune score player and two square-wave voices (Rockbox mixer).
 * Behaviour follows the Arduboy v1 library's Arduboy_audio.cpp. */
#include "CustomArduboy.h"
#include "Arduboy_audio.h"

#define LOWEST_NOTE 52
#define FIRST_NOTE_IN_INT_ARRAY 52
#define HIGHEST_NOTE 82

/* midi note frequencies * 2, as in the original */
static const unsigned int PROGMEM _midi_word_note_frequencies[] = {
330,349,370,392,415,440,466,494,523,
554,587,622,659,698,740,784,831,880,932,
988,1047,1109,1175,1245,1319,1397,1480,1568,1661,
1760,1865
};

struct Voice {
  volatile uint32_t freq2;   /* frequency * 2, 0 = silent */
  uint32_t phase;
};

static Voice voices[AVAILABLE_TIMERS];
static volatile bool tune_playing;
static const byte *volatile score_start;
static const byte *score_cursor;
static const byte *volatile request;
static volatile bool request_stop;
static uint32_t wait_samples;
static uint32_t tone_samples;            /* ArduboyTunes::tone(), in samples */
static volatile unsigned long tone_req_ms;

void ArduboyAudio::on()
{
  ab_audio_set(1);
}

void ArduboyAudio::off()
{
  ab_audio_set(0);
}

void ArduboyAudio::begin()
{
  tune_playing = false;
}

void ArduboyTunes::initChannel(byte, byte) {}

void ArduboyTunes::playNote(byte chan, byte note)
{
  if (chan >= AVAILABLE_TIMERS)
    return;
  if (note < LOWEST_NOTE || note > HIGHEST_NOTE)
    return;
  voices[chan].freq2 = pgm_read_word(_midi_word_note_frequencies +
                                      note - FIRST_NOTE_IN_INT_ARRAY);
  ab_audio_start();
}

void ArduboyTunes::stopNote(byte chan)
{
  if (chan < AVAILABLE_TIMERS)
    voices[chan].freq2 = 0;
}

void ArduboyTunes::playScore(const byte *score)
{
  request = score;
  tune_playing = true;
  ab_audio_start();
}

void ArduboyTunes::stopScore()
{
  request_stop = true;
  request = 0;
  tune_playing = false;
}

bool ArduboyTunes::playing()
{
  return tune_playing;
}

void ArduboyTunes::delay(unsigned msec)
{
  ab_delay(msec);
}

void ArduboyTunes::closeChannels()
{
  stopScore();
  for (byte i = 0; i < AVAILABLE_TIMERS; i++)
    voices[i].freq2 = 0;
}

void ArduboyTunes::tone(unsigned int frequency, unsigned long duration)
{
  voices[1].freq2 = (uint32_t)frequency * 2;
  tone_req_ms = duration ? duration : 0xffffffffu;
  ab_audio_start();
}

/* Run score commands until a wait is found (called from the PCM callback). */
static void score_step(int rate)
{
  for (;;) {
    byte command = pgm_read_byte(score_cursor++);
    byte opcode = command & 0xf0;
    byte chan = command & 0x0f;
    if (opcode == TUNE_OP_STOPNOTE) {
      ArduboyTunes::stopNote(chan);
    } else if (opcode == TUNE_OP_PLAYNOTE) {
      byte note = pgm_read_byte(score_cursor++);
      if (chan < AVAILABLE_TIMERS && note >= LOWEST_NOTE && note <= HIGHEST_NOTE)
        voices[chan].freq2 = pgm_read_word(_midi_word_note_frequencies +
                                           note - FIRST_NOTE_IN_INT_ARRAY);
    } else if (opcode == TUNE_OP_RESTART) {
      score_cursor = score_start;
    } else if (opcode == TUNE_OP_STOP) {
      tune_playing = false;
      break;
    } else if (opcode < 0x80) {
      unsigned duration = ((unsigned)command << 8) | pgm_read_byte(score_cursor++);
      wait_samples = (uint32_t)((uint64_t)duration * rate / 1000);
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
    for (int i = 0; i < AVAILABLE_TIMERS; i++)
      voices[i].freq2 = 0;
  }
  if (tone_req_ms) {
    tone_samples = tone_req_ms == 0xffffffffu ? tone_req_ms
                 : (uint32_t)((uint64_t)tone_req_ms * rate / 1000);
    tone_req_ms = 0;
  }
  if (request) {
    score_start = score_cursor = request;
    request = 0;
    wait_samples = 0;
  }
  bool active = false;
  for (int i = 0; i < frames; i++) {
    if (tune_playing && wait_samples == 0)
      score_step(rate);
    if (tune_playing && wait_samples)
      wait_samples--;
    if (tone_samples != 0 && tone_samples != 0xffffffffu) {
      if (--tone_samples == 0)
        voices[1].freq2 = 0;
    }
    int sample = 0;
    for (int v = 0; v < AVAILABLE_TIMERS; v++) {
      uint32_t f2 = voices[v].freq2;
      if (!f2)
        continue;
      active = true;
      voices[v].phase += (uint32_t)(((uint64_t)f2 << 31) / (uint32_t)rate);
      sample += (voices[v].phase & 0x80000000u) ? 3000 : -3000;
    }
    buf[i] = ab_audio_enabled() ? sample : 0;
  }
  return active || tune_playing;
}
