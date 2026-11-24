/* SPDX-License-Identifier: GPL-2.0-or-later */
/* PokittoLib compatibility layer: core loop, buttons, display (16-colour
 * indexed framebuffer, expanded to RGB565 on present), cookie, audio. */
#include "Pokitto.h"
#include <LibAudio>

static uint32_t rng_state = 0x2545f491;

uint32_t pk_random32()
{
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return rng_state = x;
}

void pk_seed(uint32_t seed)
{
    rng_state = seed ? seed : 0x2545f491;
}

/* libc's generator, which the games seed and use for reproducible maps. */
extern "C" int rand(void)
{
    return (int)(pk_random32() >> 1);
}

extern "C" void srand(unsigned seed)
{
    pk_seed(seed);
}

namespace Pokitto {

/* ---- buttons ----------------------------------------------------------- */

uint8_t Buttons::states[NUM_BTN];
Buttons Core::buttons;
Display Core::display;
Sound Core::sound;
int Sound::volume = 255;

/* states[]: 0 idle, 1 just pressed, 2.. frames held, 0xff just released. */
void Buttons::update()
{
    uint8_t held = pk_buttons();
    for (int b = 0; b < NUM_BTN; b++) {
        if (held & (1 << b)) {
            if (++states[b] == 0xff)
                states[b] = 2;
        } else if (states[b]) {
            states[b] = states[b] == 0xff ? 0 : 0xff;
        }
    }
}

bool Buttons::released(uint8_t b)
{
    if (states[b] != 0xff)
        return false;
    states[b] = 0;
    return true;
}

bool Buttons::repeat(uint8_t b, uint8_t period)
{
    if (!states[b] || states[b] == 0xff)
        return false;
    return period <= 1 || states[b] % period == 1;
}

/* ---- core -------------------------------------------------------------- */

uint32_t Core::frameCount;

static uint8_t frame_rate = 60;
static long frame_t0;
static uint32_t frame_idx;
static uint8_t skipped;

void Core::begin()
{
#if PK_SCREEN_W <= PK_LCD_W
    pk_viewport(PK_SCREEN_W, PK_SCREEN_H);
#endif
    Display::setFont(fontTiny);
    frame_t0 = pk_ticks();
    rng_state ^= (uint32_t)(Core::getTime() * 2654435761u) | 1;
}

void Core::setFrameRate(uint8_t fps)
{
    frame_rate = fps;
}

/* Present the finished frame, then wait for the next frame boundary. When
 * the game runs behind, frames are simulated but not drawn (at most two in
 * a row). */
bool Core::update()
{
    long deadline = frame_t0 + (long)((frame_idx + 1) * PK_HZ / frame_rate);

    if (pk_ticks() <= deadline || skipped >= 2) {
        Display::present();
        skipped = 0;
    } else {
        skipped++;
    }
    while (pk_ticks() < deadline)
        pk_idle();
    if (pk_pump() || pk_ticks() > deadline + PK_HZ / 4) {
        frame_t0 = pk_ticks();      /* after a menu or a long stall */
        frame_idx = 0;
    } else if (++frame_idx >= 600u) {
        frame_t0 = deadline;
        frame_idx = 0;
    }
    pk_audio_poll();
    if (!Display::persistence)
        Display::clear();
    frameCount++;
    Buttons::update();
    return true;
}

void Core::wait(uint32_t ms)
{
    long end = pk_ticks() + (long)(ms / (1000 / PK_HZ));
    while (pk_ticks() < end)
        pk_idle();
}

/* ---- display ----------------------------------------------------------- */

uint8_t Display::screenbuffer[PK_SCREEN_W * PK_SCREEN_H * PK_DEPTH / 8];
uint8_t Display::color = 1;
uint8_t Display::bgcolor = 0;
uint16_t Display::invisiblecolor = 255;
bool Display::persistence = false;
bool Display::textWrap = true;
int8_t Display::adjustCharStep = 1;
int8_t Display::adjustLineStep = 1;
int16_t Display::cursorX, Display::cursorY;
const unsigned char *Display::font = fontTiny;
uint8_t Display::fontWidth = 4, Display::fontHeight = 6;
const int16_t Display::width;
const int16_t Display::height;

uint16_t Display::palette[1 << PK_DEPTH];

void Display::loadRGBPalette(const unsigned char *p)
{
    for (int i = 0; i < (1 << PK_DEPTH); i++, p += 3)   /* the game's RGB table: one entry per colour */
        palette[i] = (p[0] >> 3) << 11 | (p[1] >> 2) << 5 | p[2] >> 3;
}

void Display::load565Palette(const uint16_t *p)
{
    memcpy(palette, p, sizeof(palette));   /* palette-sized table */
}

void Display::tweenPalette(uint16_t *dst, const uint16_t *a, const uint16_t *b, uint8_t factor)
{
    for (int i = 0; i < 16; i++) {   /* the games tween 16-colour palettes */
        int r = a[i] >> 11, g = (a[i] >> 5) & 63, bl = a[i] & 31;
        r += (((b[i] >> 11) - r) * factor) >> 8;
        g += ((((b[i] >> 5) & 63) - g) * factor) >> 8;
        bl += (((b[i] & 31) - bl) * factor) >> 8;
        dst[i] = r << 11 | g << 5 | bl;
    }
}

int Display::viewX, Display::viewY;
bool Display::viewSet;

/* Screen pixel as a palette index. */
static inline uint8_t pixel_at(const uint8_t *buf, int x, int y)
{
#if PK_DEPTH == 4
    uint8_t b = buf[y * (PK_SCREEN_W / 2) + x / 2];
    return (x & 1) ? b & 15 : b >> 4;
#else
    return buf[y * PK_SCREEN_W + x];
#endif
}

void Display::present()
{
    uint16_t pal565[1 << PK_DEPTH];
    for (int i = 0; i < (1 << PK_DEPTH); i++) {
        uint16_t c = palette[i];
        pal565[i] = pk_rgb((c >> 11) << 3, ((c >> 5) & 63) << 2, (c & 31) << 3);
    }
#if PK_SCREEN_W > PK_LCD_W
    /* Output size: a window of the LCD size, or the whole screen scaled. */
    static bool was_follow = true;      /* forces the first viewport call */
    bool follow = viewSet;
    int ow = PK_LCD_W, oh = follow ? PK_LCD_H : PK_SCREEN_H * PK_LCD_W / PK_SCREEN_W;
    if (follow != was_follow) {
        pk_viewport(ow, oh);
        was_follow = follow;
    }
    static uint16_t line[PK_LCD_W * 8];
    int sx0 = 0, sy0 = 0;
    if (follow) {
        sx0 = viewX - ow / 2;
        sy0 = viewY - oh / 2;
        sx0 = sx0 < 0 ? 0 : sx0 > width - ow ? width - ow : sx0;
        sy0 = sy0 < 0 ? 0 : sy0 > height - oh ? height - oh : sy0;
    }
    viewSet = false;
    for (int y = 0; y < oh; y += 8) {
        int rows = oh - y < 8 ? oh - y : 8;
        uint16_t *d = line;
        for (int r = 0; r < rows; r++) {
            if (follow) {
                for (int x = 0; x < ow; x++)
                    *d++ = pal565[pixel_at(screenbuffer, sx0 + x, sy0 + y + r)];
                continue;
            }
            /* Scaled down: average the source pixels under each output pixel. */
            int ya = (y + r) * height / oh, yb = (y + r + 1) * height / oh;
            for (int x = 0; x < ow; x++) {
                int xa = x * width / ow, xb = (x + 1) * width / ow;
                int n = 0, rs = 0, gs = 0, bs = 0;
                for (int sy = ya; sy < yb; sy++)
                    for (int sx = xa; sx < xb; sx++) {
                        uint16_t c = palette[pixel_at(screenbuffer, sx, sy)];
                        rs += c >> 11;
                        gs += (c >> 5) & 63;
                        bs += c & 31;
                        n++;
                    }
                *d++ = pk_rgb((rs / n) << 3, (gs / n) << 2, (bs / n) << 3);
            }
        }
        pk_blit(line, y, ow, rows);
    }
    pk_flush();
#else
    static uint16_t line[PK_SCREEN_W * 8];
    const uint8_t *src = screenbuffer;
    for (int y = 0; y < height; y += 8) {
        int rows = height - y < 8 ? height - y : 8;
        uint16_t *d = line;
        for (int r = 0; r < rows; r++)
#if PK_DEPTH == 4
            for (int x = 0; x < width; x += 2, src++) {
                *d++ = pal565[*src >> 4];
                *d++ = pal565[*src & 15];
            }
#else
            for (int x = 0; x < width; x++)
                *d++ = pal565[*src++];
#endif
        pk_blit(line, y, width, rows);
    }
    pk_flush();
#endif
}

/* The screen byte that fills a whole byte with colour c. */
static uint8_t fillbyte(uint8_t c)
{
#if PK_DEPTH == 4
    return (c & 15) * 0x11;
#else
    return c;
#endif
}

void Display::clear()
{
    memset(screenbuffer, fillbyte(bgcolor), sizeof(screenbuffer));
    cursorX = cursorY = 0;
}

void Display::fillScreen(uint16_t c)
{
    memset(screenbuffer, fillbyte(c), sizeof(screenbuffer));
}

void Display::put(int16_t x, int16_t y, uint8_t c)
{
#if PK_DEPTH == 4
    uint8_t *p = &screenbuffer[y * (width / 2) + x / 2];
    *p = (x & 1) ? (*p & 0xf0) | c : (*p & 0x0f) | (c << 4);
#else
    screenbuffer[y * width + x] = c;
#endif
}

void Display::drawPixel(int16_t x, int16_t y, uint8_t c)
{
    if ((uint16_t)x < width && (uint16_t)y < height)
        put(x, y, c & ((1 << PK_DEPTH) - 1));
}

void Display::drawPixel(int16_t x, int16_t y)
{
    drawPixel(x, y, color);
}

void Display::span(int16_t x0, int16_t x1, int16_t y, uint8_t c)
{
    if ((uint16_t)y >= height)
        return;
    if (x0 < 0)
        x0 = 0;
    if (x1 >= width)
        x1 = width - 1;
    for (int16_t x = x0; x <= x1; x++)
        put(x, y, c);
}

void Display::drawFastHLine(int16_t x, int16_t y, int16_t w)
{
    if (w < 0) {
        x += w;
        w = -w;
    }
    span(x, x + w - 1, y, color);
}

void Display::drawFastVLine(int16_t x, int16_t y, int16_t h)
{
    if (h < 0) {
        y += h;
        h = -h;
    }
    for (int16_t i = 0; i < h; i++)
        drawPixel(x, y + i, color);
}

void Display::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void Display::fillRect(int16_t x, int16_t y, int16_t w, int16_t h)
{
    for (int16_t j = 0; j < h; j++)
        span(x, x + w - 1, y + j, color);
}

void Display::drawRect(int16_t x, int16_t y, int16_t w, int16_t h)
{
    drawFastHLine(x, y, w);
    drawFastHLine(x, y + h - 1, w);
    drawFastVLine(x, y, h);
    drawFastVLine(x + w - 1, y, h);
}

void Display::drawCircle(int16_t x0, int16_t y0, int16_t r)
{
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        drawPixel(x0 + x, y0 + y); drawPixel(x0 - x, y0 + y);
        drawPixel(x0 + x, y0 - y); drawPixel(x0 - x, y0 - y);
        drawPixel(x0 + y, y0 + x); drawPixel(x0 - y, y0 + x);
        drawPixel(x0 + y, y0 - x); drawPixel(x0 - y, y0 - x);
        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

void Display::fillCircle(int16_t x0, int16_t y0, int16_t r)
{
    for (int dy = -r; dy <= r; dy++) {
        int dx = 0;
        while ((dx + 1) * (dx + 1) + dy * dy <= r * r + r)
            dx++;
        span(x0 - dx, x0 + dx, y0 + dy, color);
    }
}

/* 4 bits per pixel, high nibble left; width byte, height byte, then rows. */
void Display::blit(int16_t x, int16_t y, const uint8_t *bmp, bool flipx, bool flipy)
{
    int w = bmp[0], h = bmp[1], stride = PK_DEPTH == 4 ? (w + 1) / 2 : w;
    if (x >= width || y >= height || x + w <= 0 || y + h <= 0)
        return;
    int x0 = x < 0 ? -x : 0, x1 = x + w > width ? width - x : w;
    int y0 = y < 0 ? -y : 0, y1 = y + h > height ? height - y : h;
    for (int j = y0; j < y1; j++) {
        const uint8_t *row = bmp + 2 + (flipy ? h - 1 - j : j) * stride;
        for (int i = x0; i < x1; i++) {
            int s = flipx ? w - 1 - i : i;
#if PK_DEPTH == 4
            uint8_t c = (s & 1) ? row[s >> 1] & 15 : row[s >> 1] >> 4;
#else
            uint8_t c = row[s];
#endif
            if (c != invisiblecolor)
                put(x + i, y + j, c);
        }
    }
}

void Display::drawBitmap(int16_t x, int16_t y, const uint8_t *bmp)
{
    blit(x, y, bmp, false, false);
}

void Display::drawBitmapXFlipped(int16_t x, int16_t y, const uint8_t *bmp)
{
    blit(x, y, bmp, true, false);
}

void Display::drawBitmapYFlipped(int16_t x, int16_t y, const uint8_t *bmp)
{
    blit(x, y, bmp, false, true);
}

/* Rotation is not supported; flip is NOFLIP, FLIPH, FLIPV or FLIPVH. */
void Display::drawBitmap(int16_t x, int16_t y, const uint8_t *bmp, uint8_t, uint8_t flip)
{
    blit(x, y, bmp, flip & FLIPH, flip & FLIPV);
}

/* ---- text -------------------------------------------------------------- */

/* Font: width, height, first char, caps-only flag, then per glyph a width
 * byte and `width` column bytes (bit 0 = top row). Returns the advance. */
int Display::drawChar(int16_t x, int16_t y, unsigned char c)
{
    int fw = font[0], fh = font[1], hbytes = (fh + 7) >> 3;
    const uint8_t *g = font + 4 + (c - font[2]) * (1 + fw * hbytes);
    int cw = *g++;
    for (int i = 0; i < cw; i++, x++)
        for (int hb = 0; hb < hbytes; hb++) {
            uint8_t col = *g++;
            for (int j = hb * 8; j < fh && j < hb * 8 + 8; j++, col >>= 1) {
                uint8_t px = (col & 1) ? color : bgcolor;
                if (px != invisiblecolor)
                    drawPixel(x, y + j, px);
            }
        }
    return cw + adjustCharStep;
}

void Display::write(char c)
{
    if (font[3] && c >= 'a' && c <= 'z')
        c -= 32;
    if (c == '\n') {
        cursorX = 0;
        cursorY += font[1] + adjustLineStep;
    } else if (c == '\r') {
        cursorX = 0;
    } else if (c) {
        if (cursorX >= width - font[0]) {
            if (!textWrap)
                return;
            cursorX = 0;
            cursorY += font[1] + adjustLineStep;
        }
        int step = drawChar(cursorX, cursorY, c);
        if (c == ' ' && adjustCharStep)
            step = (step >> 1) + 1;
        cursorX += step;
    }
}

void Display::print(char c)
{
    write(c);
}

void Display::print(const char *s)
{
    while (*s)
        write(*s++);
}

void Display::print(unsigned long n, int base)
{
    char buf[34];
    int i = sizeof(buf);
    buf[--i] = 0;
    do {
        buf[--i] = "0123456789abcdef"[n % base];
        n /= base;
    } while (n);
    print(&buf[i]);
}

void Display::print(long n, int base)
{
    if (n < 0 && base == 10) {
        write('-');
        n = -n;
    }
    print((unsigned long)n, base);
}

/* ---- cookie ------------------------------------------------------------ */

void Cookie::begin(const char *name, uint32_t size, char *data)
{
    fname = name;
    len = size;
    ptr = data;
    char file[40];
    size_t n = 0;
    for (; name[n] && n < sizeof(file) - 5; n++)
        file[n] = name[n];
    memcpy(file + n, ".sav", 5);
    pk_file_load(file, data, size);
    /* the games pass the whole derived object, which includes these members:
     * a saved copy holds stale pointers */
    fname = name;
    len = size;
    ptr = data;
}

void Cookie::loadCookie()
{
    char file[40];
    size_t n = 0;
    for (; fname[n] && n < sizeof(file) - 5; n++)
        file[n] = fname[n];
    memcpy(file + n, ".sav", 5);
    const char *f = fname;
    uint32_t l = len;
    char *p = ptr;
    pk_file_load(file, ptr, len);
    fname = f;
    len = l;
    ptr = p;
}

void Cookie::saveCookie()
{
    char file[40];
    size_t n = 0;
    for (; fname[n] && n < sizeof(file) - 5; n++)
        file[n] = fname[n];
    memcpy(file + n, ".sav", 5);
    pk_file_save(file, ptr, len);
}

}

/* ---- audio ------------------------------------------------------------- */

bool File::openRO(const char *p)
{
    close();
    size_t n = 0;
    for (; p[n] && n < sizeof(name) - 1; n++)
        name[n] = p[n];
    name[n] = 0;
    return pk_file_exists(name);
}

/* The file is opened by the first read or seek and closed with the object. */
uint32_t File::read(void *buf, uint32_t n)
{
    if (fd < 0)
        fd = pk_file_open(name);
    if (fd < 0)
        return 0;
    int got = pk_file_read(fd, buf, n);
    return got < 0 ? 0 : got;
}

void File::seek(uint32_t pos)
{
    if (fd < 0)
        fd = pk_file_open(name);
    if (fd >= 0)
        pk_file_seek(fd, pos);
}

void File::close()
{
    if (fd >= 0)
        pk_file_close(fd);
    fd = -1;
}

namespace Audio {

static Handle handles[PK_VOICES];

Handle &playVoice(int voice, const char *path, int volume, int slowdown)
{
    handles[voice].voice = voice;
    pk_voice_play(voice, path, PROJ_AUD_FREQ / slowdown, 0, volume + (volume >> 7));
    return handles[voice];
}

void playNote(uint32_t note, uint8_t wave, uint16_t volume, uint32_t ms)
{
    /* Pokitto volumes run to 255; cap the pulse so notes stay quiet. */
    pk_tone(note, wave, volume > 255 ? 255 : volume, ms > 600 ? 600 : ms);
}

void stopVoice(int voice)
{
    pk_voice_stop(voice);
}

bool voiceActive(int voice)
{
    return pk_voice_active(voice);
}

bool Handle::ended() const
{
    return !pk_voice_active(voice);
}

void Handle::setLoop(bool loop)
{
    pk_voice_loop(voice, loop);
}

}

namespace Pokitto {

void Sound::playMusic(const char *path)
{
    pk_voice_play(0, path, PROJ_AUD_FREQ, 1, 256);
}

void Sound::playMusicStream()
{
    pk_voice_pause(0, 0);
}

void Sound::pauseMusicStream()
{
    pk_voice_pause(0, 1);
}

}

OSC osc1, osc2, osc3;
float samplespertick;
bool track1on, track2on, track3on;

/* Only the first oscillator is audible, as a tone refreshed every frame. */
void setOSC(OSC *osc, uint8_t on, uint8_t wave, uint8_t, uint8_t, uint8_t,
            uint8_t note, uint16_t volume, uint16_t, uint16_t, uint16_t,
            uint16_t, int16_t, int16_t, uint8_t, uint8_t, uint8_t)
{
    if (osc != &osc1)
        return;
    if (on && volume)
        pk_tone(note, wave, volume > 255 ? 255 : volume, 100);
    else
        pk_tone_stop();
}

bool playing;

static int disk_fd = -1;

uint8_t fileOpen(const char *path, int)
{
    fileClose();
    disk_fd = pk_file_open(path);
    return disk_fd < 0;
}

uint16_t fileReadBytes(uint8_t *buf, uint16_t n)
{
    int got = disk_fd < 0 ? 0 : pk_file_read(disk_fd, buf, n);
    return got < 0 ? 0 : got;
}

void fileClose()
{
    if (disk_fd >= 0)
        pk_file_close(disk_fd);
    disk_fd = -1;
}

static char dir_entry[40];

char *getFirstDirEntry(const char *path)
{
    if (!pk_dir_open(path))
        return NULL;
    return pk_dir_next(dir_entry, sizeof(dir_entry)) ? dir_entry : NULL;
}

char *getNextDirEntry()
{
    char name[32];
    if (!pk_dir_next(name, sizeof(name)))
        return NULL;
    dir_entry[0] = '/';
    strncpy(dir_entry + 1, name, sizeof(dir_entry) - 3);
    dir_entry[sizeof(dir_entry) - 2] = 0;
    size_t n = strlen(dir_entry);
    dir_entry[n] = '/';
    dir_entry[n + 1] = 0;
    return dir_entry;
}
