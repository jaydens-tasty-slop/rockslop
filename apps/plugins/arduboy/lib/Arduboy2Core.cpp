/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Arduboy2Core on top of the Rockbox glue. */
#include "Arduboy2.h"

bool Arduboy2Core::inverted;
bool Arduboy2Core::allOn;
uint8_t BeepPin1::duration;
uint8_t BeepPin2::duration;

/* The Sound setting of the start menu decides whether tones play, even for
 * sketches that only call boot() and never audio.begin(). */
void Arduboy2Core::boot()
{
  Arduboy2Audio::begin();
}

void Arduboy2Core::idle()
{
  ab_idle();
}

uint8_t Arduboy2Core::buttonsState()
{
  return ab_buttons();
}

void Arduboy2Core::paintScreen(const uint8_t *image)
{
  ab_present(image, inverted);
}

void Arduboy2Core::paintScreen(uint8_t image[], bool clear)
{
  ab_present(image, inverted);
  if (clear) {
    memset(image, 0, WIDTH * HEIGHT / 8);
  }
}

void Arduboy2Core::blank()
{
  static const uint8_t zero[WIDTH * HEIGHT / 8] = {0};
  ab_present(zero, 0);
}

void Arduboy2Core::invert(bool inverse)
{
  inverted = inverse;
}

void Arduboy2Core::allPixelsOn(bool on)
{
  allOn = on;
}

void Arduboy2Core::sendLCDCommand(uint8_t command)
{
  if (command == OLED_PIXELS_INVERTED) inverted = true;
  else if (command == OLED_PIXELS_NORMAL) inverted = false;
}

void Arduboy2Core::setRGBled(uint8_t red, uint8_t green, uint8_t blue)
{
  ab_set_led(red, green, blue);
}

void Arduboy2Core::digitalWriteRGB(uint8_t red, uint8_t green, uint8_t blue)
{
  ab_set_led(red == RGB_ON ? 255 : 0, green == RGB_ON ? 255 : 0,
             blue == RGB_ON ? 255 : 0);
}

unsigned long Arduboy2Core::generateRandomSeed()
{
  return ab_rand() ^ (millis() * 2654435761UL);
}

SerialClass Serial;

/* assert() in the firmware libc ends in panicf(); a plugin has no panic. */
void panicf(const char *fmt, ...)
{
  ab_fatal(fmt);
}
