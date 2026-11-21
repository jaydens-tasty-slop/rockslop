/* Square-wave tone sequencer for the ArduboyTones API.
 * ArduboyTones is (c) 2017 Scott Allen, MIT (see LICENSE.ArduboyTones). */
#include "ArduboyTones.h"

static bool (*outputEnabled)();
static uint8_t volMode = VOLUME_IN_TONE;

/* State shared with the PCM callback. The main thread only posts a request;
 * the callback owns the sequence cursor. */
static const uint16_t *volatile reqSeq;
static volatile bool reqStop;
static uint16_t ramTones[MAX_TONES * 2 + 1];

static const uint16_t *seqStart, *seqPos;
static uint32_t phase, step;
static uint32_t samplesLeft;
static bool timed;
static bool silent;
static int16_t amp;
static volatile bool active;

ArduboyTones::ArduboyTones(bool (*outEn)())
{
  outputEnabled = outEn;
}

static void post(const uint16_t *seq)
{
  if (outputEnabled && !outputEnabled())
    return;
  reqSeq = seq;
  active = true;
  ab_audio_start();
}

void ArduboyTones::tone(uint16_t freq, uint16_t dur)
{
  ramTones[0] = freq;
  ramTones[1] = dur;
  ramTones[2] = TONES_END;
  post(ramTones);
}

void ArduboyTones::tone(uint16_t f1, uint16_t d1, uint16_t f2, uint16_t d2)
{
  ramTones[0] = f1; ramTones[1] = d1;
  ramTones[2] = f2; ramTones[3] = d2;
  ramTones[4] = TONES_END;
  post(ramTones);
}

void ArduboyTones::tone(uint16_t f1, uint16_t d1, uint16_t f2, uint16_t d2,
                        uint16_t f3, uint16_t d3)
{
  ramTones[0] = f1; ramTones[1] = d1;
  ramTones[2] = f2; ramTones[3] = d2;
  ramTones[4] = f3; ramTones[5] = d3;
  ramTones[6] = TONES_END;
  post(ramTones);
}

void ArduboyTones::tones(const uint16_t *t)
{
  post(t);
}

void ArduboyTones::tonesInRAM(uint16_t *t)
{
  post(t);
}

void ArduboyTones::noTone()
{
  reqStop = true;
  reqSeq = 0;
}

void ArduboyTones::volumeMode(uint8_t mode)
{
  volMode = mode;
}

bool ArduboyTones::playing()
{
  return active;
}

/* Load the next note; returns false at the end of the sequence. */
static bool next_note(int rate)
{
  for (;;) {
    uint16_t f = pgm_read_word(seqPos);
    if (f == TONES_END)
      return false;
    if (f == TONES_REPEAT) {
      seqPos = seqStart;
      continue;
    }
    uint16_t d = pgm_read_word(seqPos + 1);
    seqPos += 2;
    bool high = (f & TONE_HIGH_VOLUME) != 0;
    f &= ~TONE_HIGH_VOLUME;
    if (volMode == VOLUME_ALWAYS_HIGH) high = true;
    else if (volMode == VOLUME_ALWAYS_NORMAL) high = false;
    amp = high ? 9000 : 4500;
    silent = f < SILENT_FREQ;
    step = (uint32_t)(((uint64_t)f << 32) / (uint32_t)rate);
    timed = d != 0;
    samplesLeft = (uint32_t)((uint64_t)d * rate / 1000);
    if (timed && samplesLeft == 0)
      continue;
    return true;
  }
}

extern "C" int ab_tones_fill(int16_t *buf, int frames, int rate)
{
  if (reqStop) {
    reqStop = false;
    seqPos = 0;
  }
  if (reqSeq) {
    seqStart = seqPos = reqSeq;
    reqSeq = 0;
    samplesLeft = 0;
    timed = true;
    if (!next_note(rate))
      seqPos = 0;
  }
  if (!seqPos) {
    memset(buf, 0, frames * sizeof(int16_t));
    active = false;
    return 0;
  }
  int i = 0;
  while (i < frames) {
    if (timed && samplesLeft == 0) {
      if (!next_note(rate)) {
        seqPos = 0;
        break;
      }
    }
    int n = frames - i;
    if (timed && (uint32_t)n > samplesLeft)
      n = samplesLeft;
    if (silent) {
      memset(buf + i, 0, n * sizeof(int16_t));
    } else {
      for (int k = 0; k < n; k++) {
        phase += step;
        buf[i + k] = (phase & 0x80000000u) ? amp : -amp;
      }
    }
    i += n;
    if (timed)
      samplesLeft -= n;
  }
  if (i < frames)
    memset(buf + i, 0, (frames - i) * sizeof(int16_t));
  if (!seqPos)
    active = false;
  return seqPos != 0 || i > 0;
}
