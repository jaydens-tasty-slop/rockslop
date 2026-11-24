/* SPDX-License-Identifier: GPL-2.0-or-later */
/* BeepPin1/BeepPin2 (hardware timer square wave) are not provided on
 * Rockbox; the classes exist so sketches compile, and are silent. */
#ifndef ARDUBOY2_BEEP_H
#define ARDUBOY2_BEEP_H
#include <stdint.h>

class BeepPin1
{
 public:
  static uint8_t duration;
  static void begin() {}
  static void tone(uint16_t) {}
  static void tone(uint16_t, uint8_t) {}
  static void timer() {}
  static void noTone() {}
  static constexpr uint16_t freq(const float hz)
  {
    return (uint16_t) (((16000000UL / 8 / 2) + (hz / 2)) / hz) - 1;
  }
};

class BeepPin2
{
 public:
  static uint8_t duration;
  static void begin() {}
  static void tone(uint16_t) {}
  static void tone(uint16_t, uint8_t) {}
  static void timer() {}
  static void noTone() {}
  static constexpr uint16_t freq(const float hz)
  {
    return (uint16_t) (((16000000UL / 128 / 2) + (hz / 2)) / hz) - 1;
  }
};
#endif
