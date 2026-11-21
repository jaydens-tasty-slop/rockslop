/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Compatibility shim for sketches written against the classic Gamebuino
 * library (<Gamebuino.h>, 84x48 monochrome LCD) on top of the layer's
 * Arduboy2. The 84x48 playfield is centred in the 128x64 buffer.
 *
 * What exists: gb.begin/update/titleScreen/menu/keyboard/pickRandomSeed,
 * gb.display (primitives, Gamebuino row-major bitmaps, 3x5 and 5x7 text),
 * gb.buttons (pressed/held/released/repeat), gb.sound (mapped to
 * ArduboyTones), collision helpers. Not supported: the sound tracker
 * (patterns/commands play a short beep), the backlight, battery and popups.
 *
 * Colours follow Gamebuino: BLACK is the drawn (dark) pixel, WHITE the
 * background. The macros are redefined here, so include this header after
 * every other header that uses Arduboy2's BLACK/WHITE.
 * BTN_C is read as A and B pressed together. */
#ifndef GAMEBUINO_H
#define GAMEBUINO_H

#include <Arduino.h>
#include <Arduboy2.h>
#include <ArduboyTones.h>
#include <Print.h>
#include <Tinyfont.h>
#include <SPI.h>
#include <string.h>
#include <new>

#undef BLACK
#undef WHITE
#undef INVERT
#define BLACK 1
#define WHITE 0
#define GRAY 2
#define INVERT 3

#define LCDWIDTH 84
#define LCDHEIGHT 48

#define BTN_UP 0
#define BTN_RIGHT 1
#define BTN_DOWN 2
#define BTN_LEFT 3
#define BTN_A 4
#define BTN_B 5
#define BTN_C 6

#define NOROT 0
#define ROTCCW 1
#define ROT180 2
#define ROTCW 3
#define NOFLIP 0
#define FLIPH 1
#define FLIPV 2
#define FLIPVH 3

#define CMD_VOLUME 0
#define CMD_INSTRUMENT 1
#define CMD_SLIDE 2
#define CMD_ARPEGGIO 3
#define CMD_TREMOLO 4

static inline int16_t gbMin(int16_t a, int16_t b) { return a < b ? a : b; }
static inline int16_t gbMax(int16_t a, int16_t b) { return a > b ? a : b; }

/* Arduino.h macros that the target build of the layer does not provide */
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

/* font descriptors: first byte is the glyph width */
extern const uint8_t font3x5[];
extern const uint8_t font5x7[];
extern const uint8_t font3x5[] __attribute__((weak)) = {3, 5, 0x20, 0x80};
extern const uint8_t font5x7[] __attribute__((weak)) = {5, 7, 0x20, 0x80};

/* an Arduboy2 object for the non-static calls; built in place so no
 * destructor is registered with atexit */
static inline Arduboy2 &gbArduboy()
{
  alignas(Arduboy2) static char buf[sizeof(Arduboy2)];
  static Arduboy2 *a = new (buf) Arduboy2;
  return *a;
}

class GbDisplay : public Print
{
public:
  static const int16_t OX = 22, OY = 8; /* playfield origin in the buffer */

  int16_t cursorX = 0, cursorY = 0;
  uint8_t fontSize = 1;
  bool persistence = false;
  bool textWrap = true;

  GbDisplay() : tiny(Arduboy2::getBuffer(), WIDTH, HEIGHT) { bigFont = false; color = 1; }

  uint8_t width() { return LCDWIDTH; }
  uint8_t height() { return LCDHEIGHT; }

  void setColor(int8_t c, int8_t = -1) { color = c; }
  void setFont(const uint8_t *f) { bigFont = pgm_read_byte(f) >= 5; }
  void setTextWrap(bool w) { textWrap = w; }

  void clear()
  {
    Arduboy2::clear();
    cursorX = cursorY = 0;
  }
  void fillScreen(int8_t c)
  {
    int8_t old = color;
    color = c;
    fillRect(0, 0, LCDWIDTH, LCDHEIGHT);
    color = old;
  }

  void drawPixel(int16_t x, int16_t y)
  {
    if (x < 0 || y < 0 || x >= LCDWIDTH || y >= LCDHEIGHT)
      return;
    uint8_t c = 0;
    if (color == 1) c = 1;
    else if (color == 2) c = (x + y) & 1;
    else if (color == 3) {
      c = Arduboy2::getPixel(x + OX, y + OY) ? 0 : 1;
    }
    Arduboy2::drawPixel(x + OX, y + OY, c);
  }
  uint8_t getPixel(int16_t x, int16_t y)
  {
    return Arduboy2::getPixel(x + OX, y + OY);
  }

  void drawFastHLine(int16_t x, int16_t y, int16_t w)
  {
    for (int16_t i = 0; i < w; i++)
      drawPixel(x + i, y);
  }
  void drawFastVLine(int16_t x, int16_t y, int16_t h)
  {
    for (int16_t i = 0; i < h; i++)
      drawPixel(x, y + i);
  }
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
  {
    int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;
    for (;;) {
      drawPixel(x0, y0);
      if (x0 == x1 && y0 == y1)
        break;
      int16_t e2 = 2 * err;
      if (e2 >= dy) { err += dy; x0 += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; }
    }
  }
  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h)
  {
    drawFastHLine(x, y, w);
    drawFastHLine(x, y + h - 1, w);
    drawFastVLine(x, y, h);
    drawFastVLine(x + w - 1, y, h);
  }
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h)
  {
    for (int16_t i = 0; i < h; i++)
      drawFastHLine(x, y + i, w);
  }
  void drawCircle(int16_t x0, int16_t y0, int16_t r)
  {
    int16_t f = 1 - r, ddx = 1, ddy = -2 * r, x = 0, y = r;
    drawPixel(x0, y0 + r); drawPixel(x0, y0 - r);
    drawPixel(x0 + r, y0); drawPixel(x0 - r, y0);
    while (x < y) {
      if (f >= 0) { y--; ddy += 2; f += ddy; }
      x++; ddx += 2; f += ddx;
      drawPixel(x0 + x, y0 + y); drawPixel(x0 - x, y0 + y);
      drawPixel(x0 + x, y0 - y); drawPixel(x0 - x, y0 - y);
      drawPixel(x0 + y, y0 + x); drawPixel(x0 - y, y0 + x);
      drawPixel(x0 + y, y0 - x); drawPixel(x0 - y, y0 - x);
    }
  }
  void fillCircle(int16_t x0, int16_t y0, int16_t r)
  {
    for (int16_t y = -r; y <= r; y++) {
      int16_t w = 0;
      while ((w + 1) * (w + 1) + y * y <= r * r + r)
        w++;
      drawFastHLine(x0 - w, y0 + y, 2 * w + 1);
    }
  }
  void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                    int16_t x2, int16_t y2)
  {
    drawLine(x0, y0, x1, y1);
    drawLine(x1, y1, x2, y2);
    drawLine(x2, y2, x0, y0);
  }
  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                    int16_t x2, int16_t y2)
  {
    int16_t minY = gbMin(y0, gbMin(y1, y2)), maxY = gbMax(y0, gbMax(y1, y2));
    for (int16_t y = minY; y <= maxY; y++) {
      int16_t lo = 1000, hi = -1000;
      edge(x0, y0, x1, y1, y, lo, hi);
      edge(x1, y1, x2, y2, y, lo, hi);
      edge(x2, y2, x0, y0, y, lo, hi);
      if (hi >= lo)
        drawFastHLine(lo, y, hi - lo + 1);
    }
  }

  /* Gamebuino bitmap: width, height, then rows of MSB-first bytes */
  void drawBitmap(int16_t x, int16_t y, const uint8_t *bmp, uint8_t rot = 0,
                  uint8_t flip = 0)
  {
    uint8_t w = pgm_read_byte(bmp), h = pgm_read_byte(bmp + 1);
    uint8_t bpr = (w + 7) / 8;
    for (uint8_t j = 0; j < h; j++)
      for (uint8_t i = 0; i < w; i++) {
        if (!(pgm_read_byte(bmp + 2 + j * bpr + i / 8) & (0x80 >> (i & 7))))
          continue;
        int16_t sx = (flip & 1) ? w - 1 - i : i;
        int16_t sy = (flip & 2) ? h - 1 - j : j;
        int16_t dx, dy;
        switch (rot) {
        case 1: dx = sy; dy = w - 1 - sx; break;
        case 2: dx = w - 1 - sx; dy = h - 1 - sy; break;
        case 3: dx = h - 1 - sy; dy = sx; break;
        default: dx = sx; dy = sy; break;
        }
        drawPixel(x + dx, y + dy);
      }
  }

  void drawChar(int16_t x, int16_t y, char c, uint8_t size = 1)
  {
    /* text is drawn black-on-white or inverted through the same colour */
    if (color == 0) return;
    if (bigFont) {
      gbArduboy().drawChar(x + OX, y + OY, c, 1, 0, size);
    } else {
      tiny.setTextColor(1);
      tiny.printChar(c, x + OX, y + OY);
    }
  }
  size_t write(uint8_t c)
  {
    uint8_t cw = (bigFont ? 6 : 4) * fontSize, ch = (bigFont ? 8 : 6) * fontSize;
    if (c == '\n') {
      cursorX = 0;
      cursorY += ch;
    } else if (c != '\r') {
      if (textWrap && cursorX > LCDWIDTH - cw) {
        cursorX = 0;
        cursorY += ch;
      }
      drawChar(cursorX, cursorY, c, fontSize);
      cursorX += cw;
    }
    return 1;
  }

private:
  static void edge(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t y,
                   int16_t &lo, int16_t &hi)
  {
    if (y0 == y1) {
      if (y == y0) {
        lo = gbMin(lo, gbMin(x0, x1));
        hi = gbMax(hi, gbMax(x0, x1));
      }
      return;
    }
    if (y < gbMin(y0, y1) || y > gbMax(y0, y1))
      return;
    int16_t x = x0 + (int32_t)(x1 - x0) * (y - y0) / (y1 - y0);
    lo = gbMin(lo, x);
    hi = gbMax(hi, x);
  }

  Tinyfont tiny;
  bool bigFont;
  int8_t color;
};

class GbButtons
{
public:
  void update()
  {
    Arduboy2::pollButtons();
    bool a = Arduboy2::pressed(A_BUTTON), b = Arduboy2::pressed(B_BUTTON);
    bool now[7] = {Arduboy2::pressed(UP_BUTTON), Arduboy2::pressed(RIGHT_BUTTON),
                   Arduboy2::pressed(DOWN_BUTTON), Arduboy2::pressed(LEFT_BUTTON),
                   a, b, a && b};
    for (uint8_t i = 0; i < 7; i++) {
      if (now[i])
        states[i] = states[i] == 0xFF ? 1 : (states[i] < 0xFE ? states[i] + 1 : states[i]);
      else
        states[i] = (states[i] > 0 && states[i] != 0xFF) ? 0xFF : 0;
    }
  }
  bool pressed(uint8_t b) { return states[b] > 0 && states[b] != 0xFF; }
  bool released(uint8_t b) { return states[b] == 0xFF; }
  bool held(uint8_t b, uint8_t time) { return states[b] == (uint8_t)(time + 1); }
  uint8_t timeHeld(uint8_t b) { return pressed(b) ? states[b] : 0; }
  bool repeat(uint8_t b, uint8_t period)
  {
    if (!pressed(b))
      return false;
    if (period <= 1)
      return true;
    return (states[b] % period) == 1;
  }

private:
  uint8_t states[7] = {0, 0, 0, 0, 0, 0, 0};
};

class GbSound
{
public:
  GbSound() : tones(Arduboy2Audio::enabled) {}
  void playTick() { tones.tone(1800, 8); }
  /* pitch 0..63 in semitones, duration in 1/20 s */
  void playNote(uint8_t pitch, uint8_t duration, uint8_t = 0)
  {
    uint16_t f = 262;
    for (uint8_t p = 0; p < pitch && f < 6000; p++)
      f = f + f / 17 + 1; /* about 5.9 % per semitone */
    tones.tone(f < 250 ? 250 : f, duration * 50);
  }
  void playPattern(const uint16_t *, uint8_t = 0) { tones.tone(660, 70, 880, 70); }
  void command(uint8_t, uint8_t, int8_t, uint8_t) {}
  void stopTrack(uint8_t = 0) {}
  void stopPattern(uint8_t = 0) {}
  void setVolume(uint8_t) {}
  void mute() {}

private:
  ArduboyTones tones;
};

class GbBattery
{
public:
  bool show = false;
};
class GbBacklight
{
public:
  void set(uint8_t) {}
  void automatic(bool) {}
};

class Gamebuino
{
public:
  GbButtons buttons;
  GbDisplay display;
  GbSound sound;
  GbBattery battery;
  GbBacklight backlight;
  uint8_t frameCount = 0;

  void begin()
  {
    gbArduboy().begin();
    Arduboy2::setFrameRate(20);
    Arduboy2::clear();
  }
  void setFrameRate(uint8_t fps) { Arduboy2::setFrameRate(fps); }
  void pickRandomSeed() { Arduboy2::initRandomSeed(); }
  uint16_t getFreeRam() { return 2000; }

  /* true once per frame; presents the previous frame first */
  bool update()
  {
    if (!Arduboy2::nextFrame())
      return false;
    Arduboy2::display(!display.persistence);
    display.cursorX = display.cursorY = 0;
    buttons.update();
    frameCount++;
    return true;
  }

  void titleScreen(const __FlashStringHelper *name, const uint8_t *logo = 0)
  {
    for (uint8_t t = 0;; t++) {
      if (!Arduboy2::nextFrame())
        continue;
      buttons.update();
      Arduboy2::clear();
      display.setColor(BLACK);
      display.persistence = false;
      if (logo)
        display.drawBitmap((LCDWIDTH - pgm_read_byte(logo)) / 2, 2, logo);
      else {
        display.cursorX = 2;
        display.cursorY = 6;
        display.print(name);
      }
      if (t & 16) {
        display.cursorX = 8;
        display.cursorY = 40;
        display.print(F("press A"));
      }
      Arduboy2::display();
      if (buttons.pressed(BTN_A) && buttons.timeHeld(BTN_A) == 1)
        break;
    }
    display.cursorX = display.cursorY = 0;
  }
  void titleScreen(const uint8_t *logo) { titleScreen(F(""), logo); }
  void titleScreen(const char *name) { titleScreen((const __FlashStringHelper *)name); }

  /* items: PROGMEM array of PROGMEM strings. Returns the index or -1 (B). */
  int8_t menu(const char *const *items, uint8_t length)
  {
    int8_t sel = 0;
    for (;;) {
      if (!Arduboy2::nextFrame())
        continue;
      buttons.update();
      Arduboy2::clear();
      display.setColor(BLACK);
      for (uint8_t i = 0; i < length; i++) {
        display.cursorX = 0;
        display.cursorY = i * 6;
        display.print(i == sel ? '>' : ' ');
        display.print((const __FlashStringHelper *)pgm_read_ptr(items + i));
      }
      Arduboy2::display();
      if (buttons.repeat(BTN_UP, 6) && buttons.timeHeld(BTN_UP) <= 1) sel = sel ? sel - 1 : length - 1;
      if (buttons.repeat(BTN_DOWN, 6) && buttons.timeHeld(BTN_DOWN) <= 1) sel = (sel + 1) % length;
      if (buttons.pressed(BTN_A) && buttons.timeHeld(BTN_A) == 1) return sel;
      if (buttons.pressed(BTN_B) && buttons.timeHeld(BTN_B) == 1) return -1;
    }
  }

  void getDefaultName(char *s) { strcpy(s, "PLAYER"); }

  /* name entry: Up/Down change a letter, Right next, A accept, B delete */
  void keyboard(char *text, uint8_t length)
  {
    uint8_t pos = 0;
    uint8_t n = strlen(text);
    for (uint8_t i = n; i < length; i++)
      text[i] = ' ';
    text[length] = 0;
    for (;;) {
      if (!Arduboy2::nextFrame())
        continue;
      buttons.update();
      Arduboy2::clear();
      display.setColor(BLACK);
      display.cursorX = 0;
      display.cursorY = 8;
      display.print(text);
      display.drawFastHLine(pos * 4, 15, 3);
      display.cursorX = 0;
      display.cursorY = 30;
      display.print(F("U/D letter R next"));
      display.cursorX = 0;
      display.cursorY = 38;
      display.print(F("A accept B back"));
      Arduboy2::display();
      char &c = text[pos];
      if (buttons.pressed(BTN_UP) && buttons.timeHeld(BTN_UP) == 1)
        c = c == ' ' ? 'A' : (c == 'Z' ? ' ' : c + 1);
      if (buttons.pressed(BTN_DOWN) && buttons.timeHeld(BTN_DOWN) == 1)
        c = c == ' ' ? 'Z' : (c == 'A' ? ' ' : c - 1);
      if (buttons.pressed(BTN_RIGHT) && buttons.timeHeld(BTN_RIGHT) == 1 && pos + 1 < length) pos++;
      if (buttons.pressed(BTN_LEFT) && buttons.timeHeld(BTN_LEFT) == 1 && pos) pos--;
      if (buttons.pressed(BTN_B) && buttons.timeHeld(BTN_B) == 1 && pos) pos--;
      if (buttons.pressed(BTN_A) && buttons.timeHeld(BTN_A) == 1) break;
    }
    for (int8_t i = length - 1; i >= 0 && text[i] == ' '; i--)
      text[i] = 0;
  }

  void popup(const __FlashStringHelper *, uint8_t) {}

  bool collidePointRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                        int16_t w, int16_t h)
  {
    return x1 >= x2 && x1 < x2 + w && y1 >= y2 && y1 < y2 + h;
  }
  bool collideRectRect(int16_t x1, int16_t y1, int16_t w1, int16_t h1,
                       int16_t x2, int16_t y2, int16_t w2, int16_t h2)
  {
    return !(x2 >= x1 + w1 || x2 + w2 <= x1 || y2 >= y1 + h1 || y2 + h2 <= y1);
  }
};

#endif
