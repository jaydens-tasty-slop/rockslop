/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Compatibility shim for games written against the old Arduboy v1 library
 * (<Arduboy.h>) and Team a.r.g.'s Arglib fork of it: an `Arduboy` class on
 * the layer's Arduboy2, SimpleButtons, a Sprites that is built from an
 * Arduboy object, and `arduboy.tunes`. Include via <Arduboy.h> or <Arglib.h>. */
#ifndef ARDUBOY_V1_H
#define ARDUBOY_V1_H

#include <Arduino.h>
#include <Arduboy2.h>
#include <ArduboyTones.h>
#include <ArduboyPlaytune.h>
#include <string.h>

/* the v1 Arduboy.h exposed the short integer names */
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef Arduboy2Audio ArduboyAudio;

#ifndef EEPROM_VERSION
#define EEPROM_VERSION 0
#define EEPROM_BRIGHTNESS 1
#define EEPROM_AUDIO_ON_OFF 2
#endif
#ifndef EEPROM_STORAGE_SPACE_START
#define EEPROM_STORAGE_SPACE_START 16
#endif

/* v1 ArduboyTunes: tone() and Playtune scores, on the layer's mixer sources. */
class ArduboyTunes
{
public:
  void initChannel(uint8_t) {}
  void playScore(const uint8_t *score) { playtune().playScore(score); }
  void stopScore() { playtune().stopScore(); }
  void closeChannels() { playtune().stopScore(); }
  bool playing() { return playtune().playing() || tones().playing(); }
  void tone(unsigned int frequency, unsigned long duration)
  {
    tones().tone(frequency, duration);
  }
  void delay(unsigned msec) { ::delay(msec); }

private:
  static ArduboyPlaytune &playtune()
  {
    static ArduboyPlaytune pt(Arduboy2Audio::enabled);
    return pt;
  }
  static ArduboyTones &tones()
  {
    static ArduboyTones t(Arduboy2Audio::enabled);
    return t;
  }
};

class Arduboy : public Arduboy2
{
public:
  ArduboyTunes tunes;

  void start() { begin(); }
  void beginNoLogo() { begin(); }
  void poll() { pollButtons(); }
  void saveMuchPower() {}
  void idle() {}
  void blank() { clear(); }
  void clearDisplay() { clear(); }
  bool not_pressed(uint8_t buttons) { return notPressed(buttons); }
  uint8_t getInput() { return buttonsState(); }
  uint8_t width() { return WIDTH; }
  uint8_t height() { return HEIGHT; }
  void drawScreen(const uint8_t *image) { memcpy(getBuffer(), image, WIDTH * HEIGHT / 8); }
  void drawScreen(uint8_t image[]) { memcpy(getBuffer(), image, WIDTH * HEIGHT / 8); }
  void swap(int16_t &a, int16_t &b) { int16_t t = a; a = b; b = t; }

  void drawSprite(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w,
                  int16_t h, uint8_t frame, uint8_t color)
  {
    drawBitmap(x, y, bitmap + frame * w * ((h + 7) / 8), w, h, color);
  }
  void drawMaskedSprite(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w,
                        int16_t h, const uint8_t *mask, uint8_t frame, uint8_t color)
  {
    uint16_t off = frame * w * ((h + 7) / 8);
    if (mask)
      drawBitmap(x, y, mask + off, w, h, color ? BLACK : WHITE);
    drawBitmap(x, y, bitmap + off, w, h, color);
  }
};

class SimpleButtons
{
public:
  SimpleButtons(Arduboy &a) : arduboy(&a) {}
  void poll() { previousButtonState = currentButtonState; currentButtonState = arduboy->getInput(); }
  bool pressed(uint8_t buttons) { return (currentButtonState & buttons) == buttons; }
  bool notPressed(uint8_t buttons) { return (currentButtonState & buttons) == 0; }
  bool justPressed(uint8_t button)
  {
    return !(previousButtonState & button) && (currentButtonState & button);
  }

private:
  uint8_t currentButtonState = 0;
  uint8_t previousButtonState = 0;
  Arduboy *arduboy;
};

/* v1 Sprites(Arduboy &): the layer's Sprites draw into the shared buffer. */
class SpritesV1 : public Sprites
{
public:
  SpritesV1(Arduboy &) {}
  SpritesV1() {}
};
#define Sprites SpritesV1

#endif
