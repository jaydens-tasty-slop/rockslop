/**
 * @file ArduboyTones.h
 * \brief An Arduino library for playing tones and tone sequences, 
 * intended for the Arduboy game system.
 */

/*****************************************************************************
  ArduboyTones

An Arduino library to play tones and tone sequences.

Specifically written for use by the Arduboy miniature game system
https://www.arduboy.com/
but could work with other Arduino AVR boards that have 16 bit timer 3
available, by changing the port and bit definintions for the pin(s)
if necessary.

Copyright (c) 2017 Scott Allen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*****************************************************************************/

#ifndef ARDUBOY_TONES_H
#define ARDUBOY_TONES_H

/* Rockbox port: the AVR timer/pin driver is replaced by a square-wave
 * synthesiser feeding a Rockbox mixer channel (ArduboyTones.cpp, ab_glue.c).
 * The public interface is unchanged. */

#include <Arduino.h>

#define TONES_END 0x8000
#define TONES_REPEAT 0x8001
#define TONE_HIGH_VOLUME 0x8000
#define VOLUME_IN_TONE 0
#define VOLUME_ALWAYS_NORMAL 1
#define VOLUME_ALWAYS_HIGH 2
#define MAX_TONES 3
#define SILENT_FREQ 250

class ArduboyTones
{
 public:
  ArduboyTones(bool (*outEn)());
  static void tone(uint16_t freq, uint16_t dur = 0);
  static void tone(uint16_t freq1, uint16_t dur1,
                   uint16_t freq2, uint16_t dur2);
  static void tone(uint16_t freq1, uint16_t dur1,
                   uint16_t freq2, uint16_t dur2,
                   uint16_t freq3, uint16_t dur3);
  static void tones(const uint16_t *tones);
  static void tonesInRAM(uint16_t *tones);
  static void noTone();
  static void volumeMode(uint8_t mode);
  static bool playing();
};

#include "ArduboyTonesPitches.h"
#endif
