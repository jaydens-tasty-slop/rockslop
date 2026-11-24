/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Gamebuino META compatibility layer for Rockbox: the subset of the
 * Gamebuino-Meta API that the ported games use (display, Image, buttons,
 * sound FX, save, gui popups). Written against the public API of
 * https://github.com/Gamebuino/Gamebuino-Meta; no code is copied. */
#ifndef GB_GAMEBUINO_META_H
#define GB_GAMEBUINO_META_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
struct __FlashStringHelper;
#include "gb_glue.h"

#define SAVETYPE_INT 0
#define SAVETYPE_BLOB 1

#ifndef PROGMEM
#define PROGMEM
#endif
#define pgm_read_byte(a) (*(const uint8_t *)(a))
typedef const char *PGM_P;
#define pgm_read_word(a) (*(const uint16_t *)(a))

/* The plugin runtime provides only the mem* functions to C++ code. */
static inline size_t gb_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}
static inline int gb_strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}
static inline long gb_atol(const char *s) {
    long v = 0;
    bool neg = false;
    while (*s == ' ') s++;
    if (*s == '-' || *s == '+') neg = *s++ == '-';
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return neg ? -v : v;
}

/* libm is not linked on the player: gb.cpp provides the few functions the
 * games use. */
extern "C" {
double floor(double);
double round(double);
double nearbyint(double);
float sqrtf(float);
float tanf(float);
float atan2f(float, float);
double atan2(double, double);
double sqrt(double);
float sinf(float);
float cosf(float);
double sin(double);
double cos(double);
}
static inline bool isDigit(int c) { return c >= '0' && c <= '9'; }

void *gb_alloc(size_t n);
void gb_release(void *p);

/* The subset of Arduino's String used by the games, on the layer's heap. */
class String {
public:
    String() : buf(0), len(0) {}
    String(const char *s) : buf(0), len(0) { assign(s, s ? gb_strlen(s) : 0); }
    String(const String &o) : buf(0), len(0) { assign(o.buf, o.len); }
    String(char c) : buf(0), len(0) { assign(&c, 1); }
    String(int n, int base = 10) : buf(0), len(0) { number((long)n, base); }
    String(unsigned n, int base = 10) : buf(0), len(0) { number((long)n, base); }
    String(long n, int base = 10) : buf(0), len(0) { number(n, base); }
    String(unsigned long n, int base = 10) : buf(0), len(0) { number((long)n, base); }
    ~String() { gb_release(buf); }
    String &operator=(const String &o) {
        if (this != &o) assign(o.buf, o.len);
        return *this;
    }
    String &operator=(const char *s) {
        assign(s, s ? gb_strlen(s) : 0);
        return *this;
    }
    String &operator+=(const String &o) { append(o.buf, o.len); return *this; }
    String &operator+=(const char *s) { append(s, s ? gb_strlen(s) : 0); return *this; }
    String &operator+=(char c) { append(&c, 1); return *this; }
    unsigned length() const { return len; }
    const char *c_str() const { return buf ? buf : ""; }
    char operator[](unsigned i) const { return i < len ? buf[i] : 0; }
    char charAt(unsigned i) const { return (*this)[i]; }
    String substring(unsigned from, unsigned to) const {
        String r;
        if (to > len) to = len;
        if (from < to) r.assign(buf + from, to - from);
        return r;
    }
    String substring(unsigned from) const { return substring(from, len); }
    long toInt() const { return buf ? gb_atol(buf) : 0; }
    void toCharArray(char *out, unsigned size) const {
        if (!size) return;
        unsigned n = len < size - 1 ? len : size - 1;
        if (n) memcpy(out, buf, n);
        out[n] = 0;
    }
    bool operator==(const String &o) const {
        return len == o.len && (!len || !memcmp(buf, o.buf, len));
    }
    bool operator==(const char *s) const { return !gb_strcmp(c_str(), s); }
    bool operator!=(const char *s) const { return gb_strcmp(c_str(), s) != 0; }
private:
    char *buf;
    unsigned len;
    void assign(const char *s, unsigned n);
    void append(const char *s, unsigned n);
    void number(long n, int base);
};
inline String operator+(const String &a, const String &b) { String r(a); r += b; return r; }
inline String operator+(const String &a, const char *b) { String r(a); r += b; return r; }
inline String operator+(const char *a, const String &b) { String r(a); r += b; return r; }
inline String operator+(const String &a, char b) { String r(a); r += b; return r; }

namespace Gamebuino_Meta {

enum class Color : uint16_t {
    white = 0xFFFF, gray = 0xACD0, darkgray = 0x5268, black = 0x0000,
    purple = 0x9008, pink = 0xCA30, red = 0xD8E4, orange = 0xFD42,
    brown = 0xCC68, beige = 0xFEB2, yellow = 0xF720, lightgreen = 0x8668,
    green = 0x044A, darkblue = 0x0210, blue = 0x4439, lightblue = 0x7DDF,
};

enum class ColorIndex : uint8_t {
    black, darkblue, purple, green, brown, darkgray, gray, white,
    red, orange, yellow, lightgreen, lightblue, blue, pink, beige,
};

enum class ColorMode : uint8_t {
    rgb565 = 0,
    index,
    pal4,   /* layer extension: 4-bit pixels with a 16 entry RGB565 palette
               stored in the image (see tools/img2pal4.py) */
};

const Color WHITE = Color::white;
const Color GRAY = Color::gray;
const Color DARKGRAY = Color::darkgray;
const Color BLACK = Color::black;
const Color PURPLE = Color::purple;
const Color PINK = Color::pink;
const Color RED = Color::red;
const Color ORANGE = Color::orange;
const Color BROWN = Color::brown;
const Color BEIGE = Color::beige;
const Color YELLOW = Color::yellow;
const Color LIGHTGREEN = Color::lightgreen;
const Color GREEN = Color::green;
const Color DARKBLUE = Color::darkblue;
const Color BLUE = Color::blue;
const Color LIGHTBLUE = Color::lightblue;

const ColorIndex INDEX_WHITE = ColorIndex::white;
const ColorIndex INDEX_GRAY = ColorIndex::gray;
const ColorIndex INDEX_DARKGRAY = ColorIndex::darkgray;
const ColorIndex INDEX_BLACK = ColorIndex::black;
const ColorIndex INDEX_PURPLE = ColorIndex::purple;
const ColorIndex INDEX_PINK = ColorIndex::pink;
const ColorIndex INDEX_RED = ColorIndex::red;
const ColorIndex INDEX_ORANGE = ColorIndex::orange;
const ColorIndex INDEX_BROWN = ColorIndex::brown;
const ColorIndex INDEX_BEIGE = ColorIndex::beige;
const ColorIndex INDEX_YELLOW = ColorIndex::yellow;
const ColorIndex INDEX_LIGHTGREEN = ColorIndex::lightgreen;
const ColorIndex INDEX_GREEN = ColorIndex::green;
const ColorIndex INDEX_DARKBLUE = ColorIndex::darkblue;
const ColorIndex INDEX_BLUE = ColorIndex::blue;
const ColorIndex INDEX_LIGHTBLUE = ColorIndex::lightblue;

extern const uint16_t defaultColorPalette[16];

/* ---- Image ---------------------------------------------------------- */

/* An image is read from the same const array formats as the real library.
 * The constructors are constexpr and the header is decoded lazily on first
 * use, so Image objects are constant-initialised (no start-up code, no
 * dependence on the layer being set up). */
class Image {
public:
    constexpr Image() : data(0), wide(true), frame(0), counter(0), last(0), ready(false) {}
    /* 16-bit array: width, height, frames, loop, transparent, mode, pixels */
    constexpr Image(const uint16_t *d)
        : data(d), wide(true), frame(0), counter(0), last(0), ready(false) {}
    /* byte array: width, height, frames (lo, hi), loop, transparent, mode, pixels */
    constexpr Image(const uint8_t *d)
        : data(d), wide(false), frame(0), counter(0), last(0), ready(false) {}
    /* an empty drawable image; its pixels (zeroed) come from the layer's
     * heap on first use */
    constexpr Image(uint16_t w_, uint16_t h_, ColorMode m)
        : data(0), wide(false), frame(0), counter(0), last(0), ready(false),
          w(w_), h(h_), mode(m), blank(true) {}
    /* point at another array (byte format, header included) */
    void init(const uint8_t *d) {
        data = d; wide = false; frame = 0; counter = 0; last = 0;
        ready = false; blank = false; opaque = false; palette = 0;
    }
    void clearTransparentColor() { setup(); opaque = true; }
    /* draw onto this image's current frame */
    void drawImage(int16_t x, int16_t y, Image &src);
    uint16_t width() { setup(); return w; }
    uint16_t height() { setup(); return h; }
    uint16_t frames() { setup(); return nframes; }
    void nextFrame();
    void setFrame(uint16_t f);
    uint16_t getFrame() { return frame; }

    const void *data;
    bool wide;
    uint16_t frame;
    uint16_t counter;
    uint8_t last;
    bool ready;
    uint16_t w = 0, h = 0, nframes = 1, loop = 0, transparent = 0;
    ColorMode mode = ColorMode::rgb565;
    const uint8_t *pixels = 0;   /* first frame */
    const uint16_t *palette = 0; /* pal4 only */
    uint32_t frameBytes = 0;
    bool blank = false;     /* own pixels, allocated by setup() */
    bool opaque = false;    /* no transparent colour */
    void setup();
};

/* ---- Language ------------------------------------------------------- */

enum LangCode : uint8_t { LANG_EN, LANG_DE, LANG_FR };

struct MultiLang {
    LangCode lang;
    const char *text;
};

/* Only English is built in: the first entry of a table is used. */
class Language {
public:
    LangCode getCurrentLang() { return LANG_EN; }
    const char *get(const MultiLang *l) { return l->text; }
};

/* ---- Display -------------------------------------------------------- */

class Display {
public:
    int16_t width() { return w; }
    int16_t height() { return h; }
    void init(uint16_t width, uint16_t height, ColorMode mode);
    /* a .GMV animation from the game's data directory: the display takes
     * its size and mode, shows frame 0; nextFrame() steps (and wraps) */
    void init(const char *gmv);
    void nextFrame();
    uint16_t frames;
    uint16_t *_buffer;     /* the screen buffer, for games that poke it */

    void clear();
    void _fill() { fillRect(0, 0, w, h); }   /* whole screen in the pen colour */
    void fill() { _fill(); }
    void clear(Color c);
    void clear(ColorIndex c);
    void setColor(Color c);
    void setColor(Color c, Color) { setColor(c); }   /* text background ignored */
    void setColor(ColorIndex c);
    void setColor(uint8_t idx) { setColor((ColorIndex)(idx & 15)); }
    void setColor(int idx) { setColor((ColorIndex)(idx & 15)); }
    void setFontSize(uint8_t s) { fontSize = s ? s : 1; fontWidth = 4 * fontSize; fontHeight = 6 * fontSize; }
    /* Classic fonts: the layer has one font, the call is ignored */
    void setFont(const uint8_t *) {}
    uint8_t fontWidth, fontHeight;   /* cell size, set by init / setFontSize */
    void setCursor(int16_t x, int16_t y) { cursorX = x; cursorY = y; }
    void setCursorX(int16_t x) { cursorX = x; }
    void setCursorY(int16_t y) { cursorY = y; }
    void fill(Color c) { clear(c); }
    void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap);
    /* Classic: rotation is not supported (NOROT only); flip is NOFLIP/FLIPH/FLIPV/FLIPVH */
    void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t rot, uint8_t flip);
    uint8_t getBitmapPixel(const uint8_t *bitmap, uint8_t x, uint8_t y) {
        return (bitmap[2 + y * ((bitmap[0] + 7) / 8) + x / 8] >> (7 - (x & 7))) & 1;
    }
    void setTextWrap(bool on) { textWrap = on; }
    int16_t getCursorX() { return cursorX; }
    int16_t getCursorY() { return cursorY; }
    void setPalette(const uint16_t *p) { palette = p; }
    void setPalette(const Color *p) { palette = (const uint16_t *)p; }
    const uint16_t *getPalette() { return palette; }
    Color getPixel(int16_t x, int16_t y);
    uint8_t getPixelIndex(int16_t x, int16_t y);

    void drawPixel(int16_t x, int16_t y);
    void drawPixel(int16_t x, int16_t y, Color c) { put(x, y, native(c)); }
    void drawFastHLine(int16_t x, int16_t y, int16_t len);
    void drawFastVLine(int16_t x, int16_t y, int16_t len);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h);
    void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h);
    void drawCircle(int16_t x, int16_t y, int16_t r);
    void fillCircle(int16_t x, int16_t y, int16_t r);
    void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                      int16_t x2, int16_t y2);

    void drawImage(int16_t x, int16_t y, Image &img);
    void drawImage(int16_t x, int16_t y, Image &img, int16_t w2, int16_t h2);
    void drawImage(int16_t x, int16_t y, Image &img, int16_t sx, int16_t sy,
                   int16_t sw, int16_t sh);
    /* raw image arrays (frame 0) */
    void drawImage(int16_t x, int16_t y, const uint16_t *d) {
        Image i(d);
        drawImage(x, y, i);
    }
    void drawImage(int16_t x, int16_t y, const uint8_t *d) {
        Image i(d);
        drawImage(x, y, i);
    }
    void drawImage(int16_t x, int16_t y, const uint16_t *d, int16_t w2, int16_t h2) {
        Image i(d);
        drawImage(x, y, i, w2, h2);
    }
    void drawImage(int16_t x, int16_t y, const uint16_t *d, int16_t sx,
                   int16_t sy, int16_t sw, int16_t sh) {
        Image i(d);
        drawImage(x, y, i, sx, sy, sw, sh);
    }

    void print(const char *s);
    void print(const __FlashStringHelper *s) { print((const char *)s); }
    void println(const __FlashStringHelper *s) { print((const char *)s); print('\n'); }
    void print(int16_t x, int16_t y, const char *s) { setCursor(x, y); print(s); }
    void print(float v);    /* two decimals, like Arduino's print */
    void print(int16_t x, int16_t y, float v) { setCursor(x, y); print(v); }
    void print(const MultiLang *l);
    void print(const String &s) { print(s.c_str()); }
    void write(char c) { print(c); }
    void print(char c);
    void print(int n) { print((long)n); }
    void print(unsigned n) { print((unsigned long)n); }
    void print(long n);
    void print(unsigned long n);
    void printf(const char *fmt, ...);
    void printf(int16_t x, int16_t y, const char *fmt, ...);
    void println() { print('\n'); }
    template <typename T> void println(T v) { print(v); print('\n'); }
    void drawChar(int16_t x, int16_t y, char c);

    /* internals shared with Gamebuino::update() */
    int16_t w, h;
    ColorMode colorMode;
    uint8_t *fb;
    const uint16_t *palette;
    uint16_t pen;        /* native pixel value: RGB565 or colour index */
    uint16_t bg;
    uint16_t penRGB;
    uint8_t fontSize;
    bool textWrap;
    int16_t cursorX, cursorY;

    uint16_t native(Color c);
    uint16_t native(ColorIndex c);
    inline void put(int x, int y, uint16_t v) {
        if ((unsigned)x >= (unsigned)w || (unsigned)y >= (unsigned)h) return;
        if (colorMode == ColorMode::rgb565) {
            ((uint16_t *)fb)[y * w + x] = v;
        } else {                        /* 4-bit pixels, high nibble first */
            uint8_t *p = fb + ((y * w + x) >> 1);
            if ((y * w + x) & 1) *p = (*p & 0xF0) | (v & 15);
            else *p = (*p & 0x0F) | (v << 4);
        }
    }
    void blit(int16_t x, int16_t y, int16_t dw, int16_t dh, Image &img,
              int16_t sx, int16_t sy, int16_t sw, int16_t sh);
};

/* ---- Buttons -------------------------------------------------------- */

enum class Button : uint8_t {
    down, left, right, up, a, b, menu, home,
};
const Button BUTTON_DOWN = Button::down;
const Button BUTTON_LEFT = Button::left;
const Button BUTTON_RIGHT = Button::right;
const Button BUTTON_UP = Button::up;
const Button BUTTON_A = Button::a;
const Button BUTTON_B = Button::b;
const Button BUTTON_MENU = Button::menu;
const Button BUTTON_HOME = Button::home;
const Button BUTTON_C = Button::menu;

class Buttons {
public:
    void update();
    bool pressed(Button b) { return states[(int)b] == 1; }
    bool released(Button b) { return states[(int)b] == 0xFFFF; }
    bool held(Button b, uint16_t time);
    bool repeat(Button b, uint16_t period);
    uint16_t timeHeld(Button b);
    uint16_t states[8];     /* 0 idle, 1 first frame pressed, n held n frames;
                               0xFFFF: released this frame */
};

/* ---- Sound ---------------------------------------------------------- */

namespace Sound_FX_Wave {
enum type { NOISE = 0, SQUARE, MAX };
}

struct Sound_FX {
    uint8_t type : 7;
    uint8_t continue_flag : 1;
    uint8_t volume_start;
    int8_t volume_sweep;
    int8_t period_sweep;
    uint16_t period_start;
    uint16_t length;
};

class Sound {
public:
    void fx(const Sound_FX &f) { play(&f, 1); }
    template <size_t N> void fx(const Sound_FX (&f)[N]) { play(f, N); }
    void fx(const Sound_FX *f) { play(f, 0); }
    void play(const Sound_FX *pattern, uint8_t length);
    /* a streamed 8-bit mono WAV from the game's data directory */
    int8_t play(const char *file, bool loop = false);
    /* a Gamebuino META note track (words: pitch<<2 | frames<<8, 0 ends);
     * returns track id 1. Played as square tones, one per frame tick. */
    int8_t play(const uint16_t *track, bool loop = false);
    void stop(int8_t id);
    bool isPlaying(int8_t id) { return id >= 0 && gb_music_playing(); }
    void stopFX();
    /* a square tone of freq Hz for ms milliseconds (20 ms steps) */
    void tone(uint32_t freq, uint32_t ms) {
        static Sound_FX t;
        t = {Sound_FX_Wave::SQUARE, 0, 60, 0, 0, (uint16_t)(22050 / (freq ? freq : 1)),
             (uint16_t)(ms / 20 + 1)};
        fx(t);
    }
    /* the stock UI blips (own short square tones) */
    void playTick() { static const Sound_FX t = {Sound_FX_Wave::SQUARE, 0, 60, 0, 0, 20, 3}; fx(t); }
    void playOK() {
        static const Sound_FX t[] = {{Sound_FX_Wave::SQUARE, 1, 60, 0, 0, 24, 3},
                                     {Sound_FX_Wave::SQUARE, 0, 60, 0, 0, 16, 4}};
        fx(t);
    }
    void playCancel() {
        static const Sound_FX t[] = {{Sound_FX_Wave::SQUARE, 1, 60, 0, 0, 16, 3},
                                     {Sound_FX_Wave::SQUARE, 0, 60, 0, 0, 28, 4}};
        fx(t);
    }
};

/* ---- Save ----------------------------------------------------------- */

struct SaveDefault {
    const uint16_t i;
    const uint8_t type;
    const union {
        const int32_t ival;
        const void *ptr;
    } val;
    const uint32_t length;
};

class Save {
public:
    template <uint16_t N> void config(const SaveDefault (&d)[N]) { config(d, N); }
    void config(const SaveDefault *d, uint16_t n);
    int32_t get(uint16_t i);
    bool get(uint16_t i, void *buf, uint32_t size);
    template <typename T> bool get(uint16_t i, T &obj) {
        return get(i, &obj, sizeof(T));
    }
    bool set(uint16_t i, int32_t num);
    bool set(uint16_t i, const char *buf);
    bool set(uint16_t i, char *buf) { return set(i, (const char *)buf); }
    bool set(uint16_t i, const void *buf, uint32_t size);
    void del(uint16_t i);
private:
    const SaveDefault *defaults;
    uint16_t count;
    int find(uint16_t i);
    uint8_t *slot(int d);
    void reset();
};

/* ---- Gui / lights --------------------------------------------------- */

class Gui {
public:
    void popup(const char *text, uint8_t duration);
    /* list menu: UP/DOWN move, A picks (returns the index), B returns -1 */
    int8_t menu(const char *title, const char *const *items, uint8_t length);
    /* edit `text` (at least its current length, upper case letters, digits,
     * space): UP/DOWN change the letter, RIGHT/A next, LEFT back, A on the
     * last one confirms */
    void keyboard(const char *title, char *text);
    void draw();
    const char *text;
    uint8_t left;
};

class MetaMode {
public:
    bool isActive() { return false; }
};

class Lights {
public:
    uint16_t _buffer[8];
    void clear() {}
    void fill(Color) {}
    void fill(ColorIndex) {}
    void drawPixel(uint8_t, uint8_t, Color) {}
    void drawPixel(uint8_t, uint8_t) {}
    void fillRect(uint8_t, uint8_t, uint8_t, uint8_t) {}
    void drawImage(uint8_t, uint8_t, Image &) {}
    void setColor(Color) {}
    void setColor(Color, Color) {}
    void fill() {}
};

/* ---- direct TFT access ---------------------------------------------- */

/* Games that stream rows of pixels (byte-swapped RGB565, as the real TFT
 * takes them) to the panel. A window is filled row by row; each complete
 * row group goes to the LCD through the view. */
class Tft {
public:
    /* the Classic-style status text on the TFT is dropped */
    void setCursor(int16_t, int16_t) {}
    void setFontSize(uint8_t) {}
    void print(const char *) {}
    void setAddrWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
        wx = x0; wy = y0; ww = x1 - x0 + 1; wh = y1 - y0 + 1; pos = 0;
    }
    void dataMode() {}
    void idleMode() {}
    void sendBuffer(const uint16_t *buf, uint32_t n) {
        while (n >= (uint32_t)ww && ww > 0 && pos < (uint32_t)ww * wh) {
            uint32_t rows = n / ww;
            uint32_t left = wh - pos / ww;
            if (rows > left) rows = left;
            gb_blit_rect(wx, wy + pos / ww, ww, rows, buf);
            buf += rows * ww;
            n -= rows * ww;
            pos += rows * ww;
        }
    }
    int16_t wx, wy, ww, wh;
    uint32_t pos;
};

extern volatile uint32_t dma_desc_free_count;

/* ---- Gamebuino ------------------------------------------------------ */

class Gamebuino {
public:
    void begin();
    bool update();
    void waitForUpdate() { while (!update()) {} }
    uint32_t getFreeRam() { return 0; }
    void pickRandomSeed();
    int8_t menu(const char *const *items, uint8_t length) { return gui.menu("Menu", items, length); }
    void popup(const char *text, uint8_t duration) { gui.popup(text, duration); }
    void getDefaultName(char *name) { strcpy(name, "PLAYER"); }
    void keyboard(char *name, uint8_t) { gui.keyboard("ENTER NAME", name); }
    void changeGame() { gb_quit(); }
    uint8_t getTimePerFrame() { return frameMs ? frameMs : 40; }
    bool collideRectRect(int16_t x1, int16_t y1, int16_t w1, int16_t h1,
                         int16_t x2, int16_t y2, int16_t w2, int16_t h2) {
        return !(x1 >= x2 + w2 || x2 >= x1 + w1 || y1 >= y2 + h2 || y2 >= y1 + h1);
    }
    uint8_t getCpuLoad() { return 0; }
    uint32_t frameCount;
    unsigned long frameStart;
    Display display;
    Buttons buttons;
    Sound sound;
    Save save;
    Gui gui;
    Lights lights;
    MetaMode metaMode;
    Language language;
    Color createColor(uint8_t r, uint8_t g, uint8_t b) {
        return (Color)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    }
    Tft tft;
    /* layer: the next frame is due at this game time (ms) */
    unsigned long nextFrameAt;
    uint8_t frameMs;    /* 0: 40 ms (25 fps) */
    void setFrameRate(uint8_t fps) { frameMs = fps ? 1000 / fps : 0; }
};

extern Gamebuino gb;

} // namespace Gamebuino_Meta

using namespace Gamebuino_Meta;

/* Arduino helpers. */
typedef bool boolean;
enum { NOROT = 0, NOFLIP = 0, FLIPH = 1, FLIPV = 2, FLIPVH = 3 };
#ifndef F
#define F(s) ((const __FlashStringHelper *)(s))
#endif
static const uint8_t font3x5[] = {3, 5};
static const uint8_t font5x7[] = {5, 7};
#define NAMELENGTH 10
#define NUM_HIGHSCORE 5
typedef float float_t;
#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif
typedef uint8_t byte;
template <class T> constexpr T abs(T x) { return x < 0 ? -x : x; }
template <class A, class B> constexpr auto min(A a, B b) -> decltype(a + b) {
    return a < b ? a : b;
}
template <class A, class B> constexpr auto max(A a, B b) -> decltype(a + b) {
    return a > b ? a : b;
}
#ifndef constrain
#define constrain(x, lo, hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))
#endif
int snprintf(char *buf, size_t len, const char *fmt, ...);
unsigned long millis();
long map(long x, long in_min, long in_max, long out_min, long out_max);
void delay(unsigned long ms);
long random(long max);
long random(long min, long max);
void randomSeed(unsigned long seed);

/* Arduino stubs for games that touch the SPI bus and the serial port. */
#define MSBFIRST 1
#define SPI_MODE0 0
struct SPISettings {
    SPISettings(uint32_t, int, int) {}
};
struct SPIClass {
    void beginTransaction(const SPISettings &) {}
    void endTransaction() {}
};
extern SPIClass SPI;
struct SerialClass {
    void begin(unsigned long) {}
    template <typename T> void println(T) {}
    template <typename T> void print(T) {}
};
extern SerialClass SerialUSB;

#endif
