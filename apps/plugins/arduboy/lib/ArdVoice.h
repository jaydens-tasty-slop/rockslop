/* SPDX-License-Identifier: GPL-2.0-or-later */
/* ArdVoice stand-in: the real library plays 8 kHz PCM samples through the
 * AVR speaker timer. The Rockbox layer has no PCM path, so the voice samples
 * are accepted and dropped; the games' tone and score audio is unaffected. */
#ifndef ARDVOICE_H
#define ARDVOICE_H

#include <Arduino.h>

class ArdVoice
{
 public:
  void playVoice(const uint8_t *, bool = false) {}
  void playVoice(const uint8_t *, uint32_t, bool = false) {}
  void stopVoice() {}
  bool isVoicePlaying() { return false; }
  void setVolume(uint8_t) {}
};

#endif
