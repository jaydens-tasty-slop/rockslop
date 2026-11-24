/* PokittoLib compatibility layer: the subset of Pokitto::Core, Display,
 * Buttons, Cookie and the audio sink that the ported games use. Written
 * against the PokittoLib API (BSD-3-Clause, see LICENSE.PokittoLib). */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "pk_glue.h"

/* The game's main() becomes the plugin's pk_main(). */
extern "C" int pk_main(void);
#define main pk_main

#define MODE_FAST_16COLOR 1     /* 110x88, 16 colours (4 bits per pixel) */
#define MODE15 15               /* 220x176, 16 colours (4 bits per pixel); shown cropped or scaled */
#define MODE13 13               /* 110x88, 256 colours (8 bits per pixel) */
#define HIGH_RAM_OFF 0
#define HIGH_RAM_ON 1
#define HIGH_RAM_MUSIC 2

#include "My_settings.h"

#if PROJ_SCREENMODE == MODE_FAST_16COLOR
#define PK_SCREEN_W 110
#define PK_SCREEN_H 88
#define PK_DEPTH 4
#elif PROJ_SCREENMODE == MODE15
#define PK_SCREEN_W 220
#define PK_SCREEN_H 176
#define PK_DEPTH 4
#elif PROJ_SCREENMODE == MODE13
#define PK_SCREEN_W 110
#define PK_SCREEN_H 88
#define PK_DEPTH 8
#else
#error "unsupported PROJ_SCREENMODE"
#endif

#define BTN_LEFT  0
#define BTN_UP    1
#define BTN_RIGHT 2
#define BTN_DOWN  3
#define BTN_A     4
#define BTN_B     5
#define BTN_C     6
#define NUM_BTN   7
#define PK_LCD_W 128
#define PK_LCD_H 160

#define WOFF    0
#define WSQUARE 1
#define WSAW    2
#define WTRI    3
#define WNOISE  4

#define NOROT  0
#define NOFLIP 0
#define FLIPH  1
#define FLIPV  2
#define FLIPVH 3

#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef pgm_read_byte
#define pgm_read_byte(p) (*(const uint8_t *)(p))
#endif

extern const unsigned char fontTiny[];
extern const unsigned char fontKoubit[];
extern const unsigned char font3x5[];
extern const unsigned char font5x7[];
extern const unsigned char fontC64[];
extern const unsigned char fontTight[];
extern const unsigned char fontAdventurer[];
extern const unsigned char palettePico[48];

namespace Pokitto {

inline void setBacklight(float) {}
#define POK_BACKLIGHT_INITIALVALUE 1.0f

class Buttons {
public:
    static void pollButtons() {}
    static void update();
    static bool pressed(uint8_t b) { return states[b] == 1; }
    static bool released(uint8_t b);
    static bool held(uint8_t b, uint8_t time) { return states[b] == time + 1; }
    static bool repeat(uint8_t b, uint8_t period);
    static uint8_t timeHeld(uint8_t b) { return states[b] == 0xff ? 0 : states[b]; }
    static bool leftBtn()  { return states[BTN_LEFT] && states[BTN_LEFT] != 0xff; }
    static bool rightBtn() { return states[BTN_RIGHT] && states[BTN_RIGHT] != 0xff; }
    static bool upBtn()    { return states[BTN_UP] && states[BTN_UP] != 0xff; }
    static bool downBtn()  { return states[BTN_DOWN] && states[BTN_DOWN] != 0xff; }
    static bool aBtn()     { return states[BTN_A] && states[BTN_A] != 0xff; }
    static bool bBtn()     { return states[BTN_B] && states[BTN_B] != 0xff; }
    static bool cBtn()     { return states[BTN_C] && states[BTN_C] != 0xff; }
    static uint8_t states[NUM_BTN];
};

class Display {
public:
    static const int16_t width = PK_SCREEN_W;
    static const int16_t height = PK_SCREEN_H;
    static uint8_t screenbuffer[PK_SCREEN_W * PK_SCREEN_H * PK_DEPTH / 8];
    static uint8_t *getBuffer() { return screenbuffer; }

    static uint16_t palette[1 << PK_DEPTH];    /* RGB565 */
    static uint8_t color, bgcolor;
    static uint16_t invisiblecolor;
    static bool persistence, textWrap;
    static int8_t adjustCharStep, adjustLineStep;
    static int16_t cursorX, cursorY;
    static int viewX, viewY;
    static bool viewSet;

    static void loadRGBPalette(const unsigned char *p);
    static void load565Palette(const uint16_t *p);
    static void tweenPalette(uint16_t *dst, const uint16_t *a, const uint16_t *b, uint8_t factor);
    static void setColorDepth(uint8_t) {}
    static void setColor(uint8_t c) { color = c & ((1 << PK_DEPTH) - 1); }
    static void setColor(uint8_t c, uint8_t bg) { setColor(c); bgcolor = bg & ((1 << PK_DEPTH) - 1); }
    static void setInvisibleColor(uint16_t c) { invisiblecolor = c; }
    static void setFont(const unsigned char *f) { font = f; fontWidth = f[0]; fontHeight = f[1]; }
    static int16_t getWidth() { return PK_SCREEN_W; }
    static int16_t getHeight() { return PK_SCREEN_H; }
    static uint8_t fontWidth, fontHeight;
    static void setCursor(int16_t x, int16_t y) { cursorX = x; cursorY = y; }

    static void clear();
    static void fillScreen(uint16_t c);
    static void drawPixel(int16_t x, int16_t y);
    static void drawPixel(int16_t x, int16_t y, uint8_t c);
    static void drawFastHLine(int16_t x, int16_t y, int16_t w);
    static void drawFastVLine(int16_t x, int16_t y, int16_t h);
    static void drawColumn(int16_t x, int16_t y1, int16_t y2) { drawFastVLine(x, y1, y2 - y1 + 1); }
    static void drawRow(int16_t x1, int16_t x2, int16_t y) { drawFastHLine(x1, y, x2 - x1 + 1); }
    static void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
    static void drawRect(int16_t x, int16_t y, int16_t w, int16_t h);
    static void fillRect(int16_t x, int16_t y, int16_t w, int16_t h);
    static void drawCircle(int16_t x, int16_t y, int16_t r);
    static void fillCircle(int16_t x, int16_t y, int16_t r);
    static void drawBitmap(int16_t x, int16_t y, const uint8_t *bmp);
    static void drawBitmapXFlipped(int16_t x, int16_t y, const uint8_t *bmp);
    static void drawBitmapYFlipped(int16_t x, int16_t y, const uint8_t *bmp);
    static void drawBitmap(int16_t x, int16_t y, const uint8_t *bmp, uint8_t rotation, uint8_t flip);
    static void drawBitmap(int16_t x, int16_t y, const uint8_t *bmp, uint8_t rotation) { drawBitmap(x, y, bmp, rotation, 0); }
    static void fillRectangle(int16_t x, int16_t y, int16_t w, int16_t h) { fillRect(x, y, w, h); }
    static void drawRectangle(int16_t x, int16_t y, int16_t w, int16_t h) { drawRect(x, y, w, h); }

    static void print(const char *s);
    static void print(char c);
    static void print(int n, int base = 10) { print((long)n, base); }
    static void print(unsigned n, int base = 10) { print((unsigned long)n, base); }
    static void print(long n, int base = 10);
    static void print(unsigned long n, int base = 10);
    static void print(uint8_t x, uint8_t y, const char *s) { setCursor(x, y); print(s); }
    static void print(uint8_t x, uint8_t y, int n) { setCursor(x, y); print(n); }
    static void println() { write('\n'); }
    static void println(const char *s) { print(s); write('\n'); }
    static void println(int n) { print(n); write('\n'); }

    static void present();
    /* Screens wider than the LCD are shown scaled down to fit, or, after
     * setView(), as a LCD-sized window centred on (x, y) for that frame. */
    static void setView(int x, int y) { viewX = x; viewY = y; viewSet = true; }
    static void drawPixelRaw(int16_t x, int16_t y, uint8_t c) { put(x, y, c); }
    static void update() {}     /* PC::update() presents the frame */

    static const unsigned char *font;

    static void put(int16_t x, int16_t y, uint8_t c);

private:
    static void blit(int16_t x, int16_t y, const uint8_t *bmp, bool flipx, bool flipy);
    static void span(int16_t x0, int16_t x1, int16_t y, uint8_t c);
    static void write(char c);
    static int drawChar(int16_t x, int16_t y, unsigned char c);
};

/* Streamed music (a file below the game's data dir, loops) and volume. */
class Sound {
public:
    static void playMusicStream(const char *path, int = 0) { playMusic(path); }
    static void playMusic(const char *path);
    static void playMusicStream();     /* resume */
    static void pauseMusicStream();
    static void ampEnable(bool) {}
    static void setVolume(int v) { volume = v; }
    static int getVolume() { return volume; }
    static int volume;
};

class Core {
public:
    static void begin();
    static bool isRunning() { return true; }
    static bool update();
    static void setFrameRate(uint8_t fps);
    static void wait(uint32_t ms);
    static uint32_t getTime() { return pk_ticks() * (1000 / PK_HZ); }
    static bool collidePointRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t w, int16_t h)
    {
        return x1 >= x2 && x1 < x2 + w && y1 >= y2 && y1 < y2 + h;
    }
    static bool collideRectRect(int16_t x1, int16_t y1, int16_t w1, int16_t h1, int16_t x2, int16_t y2, int16_t w2, int16_t h2)
    {
        return !(x2 >= x1 + w1 || x2 + w2 <= x1 || y2 >= y1 + h1 || y2 + h2 <= y1);
    }
    static uint32_t frameCount;
    static Buttons buttons;
    static Display display;     /* the old object API: gb.display, gb.sound */
    static Sound sound;
};

/* Persistent game data, stored below the game's data dir. */
class Cookie {
public:
    void begin(const char *name, uint32_t size, char *data);
    template <class T> void begin(const char *name, T &obj) { begin(name, sizeof(T), (char *)&obj); }
    void loadCookie();
    void saveCookie();
private:
    const char *fname;
    uint32_t len;
    char *ptr;
};

}

#define PROJ_AUD_FREQ_DEFAULT 16000

/* Arduino-style random numbers (seeded from the clock in Core::begin). */
uint32_t pk_random32();
void pk_seed(uint32_t seed);
static inline long random(long max) { return max > 0 ? (long)(pk_random32() % (uint32_t)max) : 0; }
static inline long random(long min, long max) { return min + random(max - min); }

static inline uint8_t xorshift8() { return (uint8_t)(pk_random32() >> 8); }
static inline uint16_t xorshift16() { return (uint16_t)(pk_random32() >> 8); }

#include "Synth.h"
