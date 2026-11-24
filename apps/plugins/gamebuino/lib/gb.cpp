/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Gamebuino META API on top of gb_glue.c: display, images, buttons, FX
 * sound, save data and popups. See Gamebuino-Meta.h. */
#include "Gamebuino-Meta.h"
#include "gb_font.h"

namespace Gamebuino_Meta {

Gamebuino gb;
volatile uint32_t dma_desc_free_count = 3;

const uint16_t defaultColorPalette[16] = {
    0x0000, 0x0210, 0x9008, 0x044A, 0xCC68, 0x5268, 0xACD0, 0xFFFF,
    0xD8E4, 0xFD42, 0xF720, 0x8668, 0x7DDF, 0x4439, 0xCA30, 0xFEB2,
};

static inline int iabs(int v) { return v < 0 ? -v : v; }
static inline int imin(int a, int b) { return a < b ? a : b; }
static inline int imax(int a, int b) { return a > b ? a : b; }

/* ---- Image ---------------------------------------------------------- */

void Image::setup()
{
    if (ready)
        return;
    if (blank) {
        nframes = 1;
        frameBytes = mode == ColorMode::rgb565 ? (uint32_t)w * h * 2
                                               : (uint32_t)((w + 1) / 2) * h;
        uint8_t *p = (uint8_t *)gb_alloc(frameBytes);
        if (p)
            memset(p, 0, frameBytes);
        pixels = p;
        ready = true;
        return;
    }
    if (wide) {
        const uint16_t *d = (const uint16_t *)data;
        w = d[0];
        h = d[1];
        nframes = d[2];
        loop = d[3];
        transparent = d[4];
        mode = (ColorMode)d[5];
        pixels = (const uint8_t *)(d + 6);
        palette = 0;
        if (mode == ColorMode::pal4) {
            palette = d + 6;
            pixels = (const uint8_t *)(d + 6 + 16);
        }
    } else {
        const uint8_t *d = (const uint8_t *)data;
        w = d[0];
        h = d[1];
        nframes = d[2] | (d[3] << 8);
        loop = d[4];
        transparent = d[5];
        mode = (ColorMode)d[6];
        pixels = d + 7;
        palette = 0;
    }
    if (!nframes)
        nframes = 1;
    frameBytes = mode == ColorMode::rgb565 ? (uint32_t)w * h * 2
                                           : (uint32_t)((w + 1) / 2) * h;
    ready = true;
}

void Image::nextFrame()
{
    setup();
    if (nframes == 1 || !loop || last == (gb.frameCount & 0xFF))
        return;
    last = gb.frameCount & 0xFF;
    if (++counter < loop)
        return;
    counter = 0;
    frame = (frame + 1 >= nframes) ? 0 : frame + 1;
}

void Image::setFrame(uint16_t f)
{
    setup();
    if (nframes == 1)
        return;
    frame = f >= nframes ? nframes - 1 : f;
    last = gb.frameCount & 0xFF;
    counter = 0;
}

/* ---- Display -------------------------------------------------------- */

static void gmvClose();

void Display::init(uint16_t width, uint16_t height, ColorMode m)
{
    gmvClose();
    frames = 1;
    w = width;
    h = height;
    colorMode = m;
    fb = (uint8_t *)gb_game.fb;
    _buffer = (uint16_t *)fb;
    palette = defaultColorPalette;
    fontSize = 1;
    fontWidth = 4;
    fontHeight = 6;
    textWrap = true;
    cursorX = cursorY = 0;
    setColor(Color::white);
    clear();
}

uint16_t Display::native(Color c)
{
    uint16_t v = (uint16_t)c;
    if (colorMode == ColorMode::rgb565)
        return v;
    int best = 0, bd = 1 << 30;
    for (int i = 0; i < 16; i++) {
        uint16_t p = palette[i];
        if (p == v)
            return i;
        int dr = ((p >> 11) & 31) - ((v >> 11) & 31);
        int dg = ((p >> 5) & 63) - ((v >> 5) & 63);
        int db = (p & 31) - (v & 31);
        int d = dr * dr * 4 + dg * dg + db * db * 4;
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

uint16_t Display::native(ColorIndex c)
{
    return colorMode == ColorMode::rgb565 ? palette[(int)c & 15] : (int)c & 15;
}

void Display::setColor(Color c) { pen = native(c); }
void Display::setColor(ColorIndex c) { pen = native(c); }

void Display::clear()
{
    if (!fb)
        return;
    if (colorMode == ColorMode::rgb565)
        memset(fb, 0, (size_t)w * h * 2);
    else
        memset(fb, 0, ((size_t)w * h + 1) / 2);
}

void Display::clear(Color c)
{
    if (!fb)
        return;
    uint16_t v = native(c);
    if (colorMode == ColorMode::rgb565) {
        uint16_t *p = (uint16_t *)fb;
        for (int i = 0; i < w * h; i++)
            p[i] = v;
    } else {
        memset(fb, v * 0x11, ((size_t)w * h + 1) / 2);
    }
}

void Display::clear(ColorIndex c) { clear((Color)palette[(int)c & 15]); }

Color Display::getPixel(int16_t x, int16_t y)
{
    return (Color)palette[getPixelIndex(x, y) & 15];
}

uint8_t Display::getPixelIndex(int16_t x, int16_t y)
{
    if ((unsigned)x >= (unsigned)w || (unsigned)y >= (unsigned)h)
        return 0;
    int i = y * w + x;
    if (colorMode == ColorMode::rgb565)
        return native((Color)((uint16_t *)fb)[i]);
    return (i & 1) ? fb[i >> 1] & 15 : fb[i >> 1] >> 4;
}

void Display::drawPixel(int16_t x, int16_t y) { put(x, y, pen); }

void Display::drawFastHLine(int16_t x, int16_t y, int16_t len)
{
    if (len < 0) {
        x += len + 1;
        len = -len;
    }
    for (int i = 0; i < len; i++)
        put(x + i, y, pen);
}

void Display::drawFastVLine(int16_t x, int16_t y, int16_t len)
{
    if (len < 0) {
        y += len + 1;
        len = -len;
    }
    for (int i = 0; i < len; i++)
        put(x, y + i, pen);
}

void Display::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    int dx = iabs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -iabs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        put(x0, y0, pen);
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

void Display::drawRect(int16_t x, int16_t y, int16_t rw, int16_t rh)
{
    if (rw <= 0 || rh <= 0)
        return;
    drawFastHLine(x, y, rw);
    drawFastHLine(x, y + rh - 1, rw);
    drawFastVLine(x, y, rh);
    drawFastVLine(x + rw - 1, y, rh);
}

/* Rectangle with the corner pixels left out (r > 0). */
void Display::drawRoundRect(int16_t x, int16_t y, int16_t rw, int16_t rh, int16_t r)
{
    if (rw <= 0 || rh <= 0)
        return;
    int c = r > 0 ? r : 0;
    drawFastHLine(x + c, y, rw - 2 * c);
    drawFastHLine(x + c, y + rh - 1, rw - 2 * c);
    drawFastVLine(x, y + c, rh - 2 * c);
    drawFastVLine(x + rw - 1, y + c, rh - 2 * c);
    if (c > 1) {
        drawPixel(x + 1, y + 1);
        drawPixel(x + rw - 2, y + 1);
        drawPixel(x + 1, y + rh - 2);
        drawPixel(x + rw - 2, y + rh - 2);
    }
}

void Display::fillRect(int16_t x, int16_t y, int16_t rw, int16_t rh)
{
    int x0 = imax(x, 0), x1 = imin(x + rw, w);
    int y0 = imax(y, 0), y1 = imin(y + rh, h);
    for (int j = y0; j < y1; j++) {
        if (colorMode == ColorMode::rgb565) {
            uint16_t *p = (uint16_t *)fb + j * w;
            for (int i = x0; i < x1; i++)
                p[i] = pen;
        } else {
            for (int i = x0; i < x1; i++)
                put(i, j, pen);
        }
    }
}

void Display::drawCircle(int16_t cx0, int16_t cy0, int16_t r)
{
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        put(cx0 + x, cy0 + y, pen);
        put(cx0 - x, cy0 + y, pen);
        put(cx0 + x, cy0 - y, pen);
        put(cx0 - x, cy0 - y, pen);
        put(cx0 + y, cy0 + x, pen);
        put(cx0 - y, cy0 + x, pen);
        put(cx0 + y, cy0 - x, pen);
        put(cx0 - y, cy0 - x, pen);
        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

void Display::fillCircle(int16_t cx0, int16_t cy0, int16_t r)
{
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        drawFastHLine(cx0 - x, cy0 + y, 2 * x + 1);
        drawFastHLine(cx0 - x, cy0 - y, 2 * x + 1);
        drawFastHLine(cx0 - y, cy0 + x, 2 * y + 1);
        drawFastHLine(cx0 - y, cy0 - x, 2 * y + 1);
        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

void Display::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                           int16_t x2, int16_t y2)
{
    int ymin = imin(y0, imin(y1, y2)), ymax = imax(y0, imax(y1, y2));
    int xs[3] = {x0, x1, x2}, ys[3] = {y0, y1, y2};
    for (int y = imax(ymin, 0); y <= imin(ymax, h - 1); y++) {
        int lo = 1 << 20, hi = -(1 << 20);
        for (int e = 0; e < 3; e++) {
            int ax = xs[e], ay = ys[e], bx = xs[(e + 1) % 3], by = ys[(e + 1) % 3];
            if (ay == by) {
                if (ay == y) {
                    lo = imin(lo, imin(ax, bx));
                    hi = imax(hi, imax(ax, bx));
                }
                continue;
            }
            if ((y < ay && y < by) || (y > ay && y > by))
                continue;
            int x = ax + (bx - ax) * (y - ay) / (by - ay);
            lo = imin(lo, x);
            hi = imax(hi, x);
        }
        if (hi >= lo)
            drawFastHLine(lo, y, hi - lo + 1);
    }
}

/* One source pixel of `img` (current frame) as a native pixel of this
 * display; returns false when transparent. */
void Display::blit(int16_t x, int16_t y, int16_t dw, int16_t dh, Image &img,
                   int16_t sx, int16_t sy, int16_t sw, int16_t sh)
{
    img.setup();
    if (!dw || !dh || sw <= 0 || sh <= 0)
        return;
    bool flipx = dw < 0, flipy = dh < 0;
    int adw = iabs(dw), adh = iabs(dh);
    int i0 = imax(0, -x), i1 = imin(adw, w - x);
    int j0 = imax(0, -y), j1 = imin(adh, h - y);
    const uint8_t *frame = img.pixels + (uint32_t)img.frame * img.frameBytes;
    int stride = img.mode == ColorMode::rgb565 ? img.w : (img.w + 1) / 2;
    bool rgbOut = colorMode == ColorMode::rgb565;
    const uint16_t *pal = img.mode == ColorMode::pal4 ? img.palette
                                                       : palette;
    int tr = img.opaque ? -1 : img.transparent;
    bool exact = adw == sw && adh == sh;
    uint16_t lastRgb = 0, lastNative = 0;
    bool haveLast = false;

    for (int j = j0; j < j1; j++) {
        int srow = exact ? j : j * sh / adh;
        if (flipy)
            srow = sh - 1 - srow;
        srow += sy;
        if (srow < 0 || srow >= img.h)
            continue;
        const uint8_t *row = frame + (uint32_t)srow * stride * (img.mode == ColorMode::rgb565 ? 2 : 1);
        for (int i = i0; i < i1; i++) {
            int scol = exact ? i : i * sw / adw;
            if (flipx)
                scol = sw - 1 - scol;
            scol += sx;
            if (scol < 0 || scol >= img.w)
                continue;
            uint16_t v;       /* RGB565 or palette index of the source */
            bool srcIsRgb;
            if (img.mode == ColorMode::rgb565) {
                v = row[2 * scol] | (row[2 * scol + 1] << 8);
                if (v == tr)
                    continue;
                srcIsRgb = true;
            } else {
                uint8_t b = row[scol >> 1];
                v = (scol & 1) ? (b & 15) : (b >> 4);
                if (v == tr)
                    continue;
                srcIsRgb = false;
            }
            uint16_t out;
            if (rgbOut) {
                out = srcIsRgb ? v : pal[v];
            } else if (!srcIsRgb && img.mode == ColorMode::index) {
                out = v;
            } else {
                uint16_t rgb = srcIsRgb ? v : pal[v];
                if (haveLast && rgb == lastRgb) {
                    out = lastNative;
                } else {
                    out = native((Color)rgb);
                    lastRgb = rgb;
                    lastNative = out;
                    haveLast = true;
                }
            }
            put(x + i, y + j, out);
        }
    }
}

void Image::drawImage(int16_t x, int16_t y, Image &src)
{
    setup();
    if (!pixels || mode == ColorMode::pal4)
        return;
    Display d;
    d.w = w;
    d.h = h;
    d.colorMode = mode;
    d.fb = (uint8_t *)pixels + (uint32_t)frame * frameBytes;
    d.palette = gb.display.palette;
    src.nextFrame();
    d.blit(x, y, src.w, src.h, src, 0, 0, src.w, src.h);
}

void Display::drawImage(int16_t x, int16_t y, Image &img)
{
    img.nextFrame();
    blit(x, y, img.w, img.h, img, 0, 0, img.w, img.h);
}

void Display::drawImage(int16_t x, int16_t y, Image &img, int16_t w2, int16_t h2)
{
    img.nextFrame();
    blit(x, y, w2, h2, img, 0, 0, img.w, img.h);
}

void Display::drawImage(int16_t x, int16_t y, Image &img, int16_t sx, int16_t sy,
                        int16_t sw, int16_t sh)
{
    img.nextFrame();
    if (sx + sw > img.w)
        sw = img.w - sx;
    if (sy + sh > img.h)
        sh = img.h - sy;
    blit(x, y, sw, sh, img, sx, sy, sw, sh);
}

void Display::drawChar(int16_t x, int16_t y, char c)
{
    uint16_t g;
    if (c == 4)
        g = 0x2FFA;                          /* diamond */
    else if (c == 0x1A)
        g = 0x4DF4;                          /* right arrow */
    else if (c < 32 || c > 126)
        return;
    else
        g = gb_font3x5[c - 32];
    int s = fontSize;
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 3; col++) {
            if (g & (1 << (14 - row * 3 - col))) {
                if (s == 1)
                    put(x + col, y + row, pen);
                else
                    fillRect(x + col * s, y + row * s, s, s);
            }
        }
    }
}

void Display::printf(const char *fmt, ...)
{
    char buf[64];
    va_list ap;
    va_start(ap, fmt);
    gb_vformat(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    print(buf);
}

void Display::printf(int16_t x, int16_t y, const char *fmt, ...)
{
    char buf[64];
    va_list ap;
    va_start(ap, fmt);
    gb_vformat(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    setCursor(x, y);
    print(buf);
}

void Display::print(char c)
{
    if (c == '\n') {
        cursorY += fontSize * 6;
        cursorX = 0;
    } else if (c != '\r') {
        if (textWrap && cursorX + fontSize * 4 > w) {
            cursorX = 0;
            cursorY += fontSize * 6;
        }
        drawChar(cursorX, cursorY, c);
        cursorX += fontSize * 4;
    }
}

void Display::print(const MultiLang *l)
{
    print(gb.language.get(l));
}

void Display::drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap)
{
    int bw = bitmap[0], bh = bitmap[1], row = (bw + 7) / 8;
    bitmap += 2;
    for (int j = 0; j < bh; j++)
        for (int i = 0; i < bw; i++)
            if (bitmap[j * row + i / 8] & (128 >> (i & 7)))
                put(x + i, y + j, pen);
}

void Display::drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t, uint8_t flip)
{
    int bw = bitmap[0], bh = bitmap[1], row = (bw + 7) / 8;
    bitmap += 2;
    for (int j = 0; j < bh; j++)
        for (int i = 0; i < bw; i++)
            if (bitmap[j * row + i / 8] & (128 >> (i & 7)))
                put(x + ((flip & 1) ? bw - 1 - i : i), y + ((flip & 2) ? bh - 1 - j : j), pen);
}

void Display::print(const char *s)
{
    while (*s)
        print(*s++);
}

void Display::print(unsigned long n)
{
    char buf[12];
    int i = 0;
    do {
        buf[i++] = '0' + n % 10;
        n /= 10;
    } while (n);
    while (i)
        print(buf[--i]);
}

void Display::print(long n)
{
    if (n < 0) {
        print('-');
        print((unsigned long)-n);
    } else {
        print((unsigned long)n);
    }
}

void Display::print(float v)
{
    if (v < 0) {
        print('-');
        v = -v;
    }
    long n = (long)(v * 100 + 0.5f);
    print(n / 100);
    print('.');
    print((char)('0' + n / 10 % 10));
    print((char)('0' + n % 10));
}

/* ---- GMV animations ------------------------------------------------- */

/* File: "GV", header size, version, width, height, frames (16 bit little
 * endian), flags (bit 0: indexed), transparent colour. Frames follow, all
 * complete. Indexed: (count, byte) pairs of two pixels each. RGB565 (16
 * colours of the default palette): 0x80 + literal colour, 0x01..0x7F one
 * palette entry, 0x81..0xFF a run of the low 7 bits of the next code. */
static struct {
    int fd;
    uint16_t frames, cur;
    uint32_t start;
    uint8_t buf[256];
    uint16_t pos, len;
} gmv = {-1};

static void gmvClose()
{
    if (gmv.fd >= 0)
        gb_file_close(gmv.fd);
    gmv.fd = -1;
}

static int gmvByte()
{
    if (gmv.pos >= gmv.len) {
        int n = gb_file_read(gmv.fd, gmv.buf, sizeof(gmv.buf));
        gmv.pos = gmv.len = 0;
        if (n <= 0)
            return 0;
        gmv.len = n;
    }
    return gmv.buf[gmv.pos++];
}

static void gmvReadFrame(Display &d)
{
    if (d.colorMode == ColorMode::rgb565) {
        uint16_t *out = (uint16_t *)d.fb;
        uint32_t total = (uint32_t)d.w * d.h, pos = 0;
        while (pos < total) {
            int c = gmvByte(), n = 1, sel = c;
            if (c > 0x80) {
                n = c & 0x7F;
                sel = gmvByte();
            }
            uint16_t col;
            if (sel == 0x80) {
                col = gmvByte();
                col |= gmvByte() << 8;
            } else {
                col = defaultColorPalette[sel & 15];
            }
            while (n-- && pos < total)
                out[pos++] = col;
        }
    } else {
        uint32_t total = ((uint32_t)d.w * d.h + 1) / 2, pos = 0;
        while (pos < total) {
            int n = gmvByte();
            uint8_t v = gmvByte();
            if (!n)
                break;
            while (n-- && pos < total)
                d.fb[pos++] = v;
        }
    }
}

void Display::init(const char *name)
{
    gmvClose();
    int fd = gb_file_open(name);
    uint8_t hd[14];
    if (fd < 0 || gb_file_read(fd, hd, 14) != 14 || hd[0] != 'G' || hd[1] != 'V') {
        gb_file_close(fd);
        return;
    }
    /* the file must match the game's screen size */
    Display::init(gb_game.width, gb_game.height,
                  (hd[11] & 1) ? ColorMode::index : ColorMode::rgb565);
    gmv.fd = fd;
    gmv.start = hd[2] | hd[3] << 8;
    gb_file_seek(fd, gmv.start, 0);
    gmv.pos = gmv.len = 0;
    gmv.cur = 0;
    gmv.frames = frames = (hd[9] | hd[10] << 8) ? hd[9] | hd[10] << 8 : 1;
    gmvReadFrame(*this);
}

void Display::nextFrame()
{
    if (gmv.fd < 0)
        return;
    if (++gmv.cur >= gmv.frames) {
        gmv.cur = 0;
        gb_file_seek(gmv.fd, gmv.start, 0);
        gmv.pos = gmv.len = 0;
    }
    gmvReadFrame(*this);
}

/* ---- Buttons -------------------------------------------------------- */

void Buttons::update()
{
    uint8_t bits = gb_buttons();
    for (int i = 0; i < 8; i++) {
        uint16_t &s = states[i];
        if (bits & (1 << i))
            s = (s == 0 || s == 0xFFFF) ? 1 : (s < 0xFFFE ? s + 1 : s);
        else
            s = (s == 0 || s == 0xFFFF) ? 0 : 0xFFFF;
    }
}

bool Buttons::held(Button b, uint16_t time)
{
    uint16_t s = states[(int)b];
    return s != 0xFFFF && s == (uint16_t)(time + 1);
}

bool Buttons::repeat(Button b, uint16_t period)
{
    uint16_t s = states[(int)b];
    if (s == 0 || s == 0xFFFF)
        return false;
    return period <= 1 || s % period == 1;
}

uint16_t Buttons::timeHeld(Button b)
{
    uint16_t s = states[(int)b];
    return s == 0xFFFF ? 0 : s;
}

/* ---- Sound ---------------------------------------------------------- */

namespace {

/* The FX engine runs at the Gamebuino's 44.1 kHz, four samples per
 * "word" like the original, and is resampled to the mixer rate. */
struct Fx {
    Sound_FX cur;
    const Sound_FX *pattern;
    uint8_t patternLen, patternPos;
    bool active;
    uint32_t time, length;
    int32_t volume, volumeSweep, period, periodSweep;
    int32_t noisePeriod, squarePeriod;
    int squarePolarity;
    uint16_t lfsr;
    int8_t word[4];
    uint8_t wordPos;
    uint32_t acc;
    int8_t last;
} fxs;

volatile uint8_t fxPending;

void fxStart(const Sound_FX &f)
{
    fxs.cur = f;
    fxs.time = 0;
    fxs.volume = (int32_t)f.volume_start << 15;
    fxs.volumeSweep = (int32_t)f.volume_sweep * 256;
    fxs.period = (int32_t)f.period_start << 16;
    fxs.periodSweep = (int32_t)f.period_sweep * 128;
    fxs.length = (uint32_t)f.length * 882;
    fxs.noisePeriod = fxs.squarePeriod = 0;
    fxs.squarePolarity = 1;
    fxs.active = true;
}

int8_t fxVolume()
{
    fxs.volume += fxs.volumeSweep;
    int32_t v = fxs.volume >> 16;
    return v < 0 ? 0 : v > 127 ? 127 : v;
}

int32_t fxFrequency()
{
    fxs.period += fxs.periodSweep;
    return fxs.period;
}

/* Produce the next four samples; false when the FX has ended. */
bool fxWord()
{
    if (fxs.active && fxs.time >= fxs.length) {
        if ((fxs.patternLen && fxs.patternPos + 1 < fxs.patternLen) ||
            (!fxs.patternLen && fxs.pattern[fxs.patternPos].continue_flag)) {
            fxs.patternPos++;
            fxStart(fxs.pattern[fxs.patternPos]);
        } else {
            fxs.active = false;
        }
    }
    if (!fxs.active)
        return false;
    fxs.time += 4;
    if (fxs.cur.type == Sound_FX_Wave::NOISE) {
        int8_t v = fxVolume();
        if (--fxs.noisePeriod <= 0) {
            int32_t p = fxFrequency() >> 20;
            fxs.noisePeriod = p < 0 ? 0 : p;
            bool bit = (fxs.lfsr ^ (fxs.lfsr >> 1)) & 1;
            fxs.lfsr = (fxs.lfsr >> 1) ^ (bit << 14);
        }
        int8_t s = (fxs.lfsr & 1) ? v : -v;
        for (int i = 0; i < 4; i++)
            fxs.word[i] = s;
    } else {
        int8_t v = fxVolume();
        if (fxs.squarePeriod <= (4 << 16)) {
            for (int i = 0; i < 4; i++) {
                fxs.squarePeriod -= 1 << 16;
                if (fxs.squarePeriod <= 0) {
                    int32_t p = fxFrequency();
                    fxs.squarePeriod += p < 0 ? 0 : p;
                    fxs.squarePolarity = !fxs.squarePolarity;
                }
                fxs.word[i] = fxs.squarePolarity ? v : -v;
            }
        } else {
            fxs.squarePeriod -= 4 << 16;
            int8_t s = fxs.squarePolarity ? v : -v;
            for (int i = 0; i < 4; i++)
                fxs.word[i] = s;
        }
    }
    return true;
}

int soundFill(int16_t *buf, int frames, int rate)
{
    for (int i = 0; i < frames; i++) {
        fxs.acc += 44100;
        while (fxs.acc >= (uint32_t)rate) {
            fxs.acc -= rate;
            if (fxs.wordPos >= 4) {
                if (!fxWord()) {
                    fxs.last = 0;
                    fxs.wordPos = 4;
                    break;
                }
                fxs.wordPos = 0;
            }
            fxs.last = fxs.word[fxs.wordPos++];
        }
        buf[i] = fxs.last * 40;
    }
    return fxs.active || fxs.wordPos < 4;
}

} // namespace

void Sound::play(const Sound_FX *pattern, uint8_t length)
{
    if (!gb_audio_enabled())
        return;
    gb_audio_lock(1);
    fxs.pattern = pattern;
    fxs.patternLen = length;
    fxs.patternPos = 0;
    if (!fxs.lfsr)
        fxs.lfsr = 1;
    fxs.wordPos = 4;
    fxStart(pattern[0]);
    gb_audio_lock(0);
    gb_audio_start();
}

int8_t Sound::play(const char *file, bool loop)
{
    return gb_music_play(file, loop) < 0 ? -1 : 0;
}

namespace {
/* note track player (Gamebuino META "tracker" words) */
const uint16_t *trkStart, *trkPos;
uint8_t trkWait;
bool trkLoop;

void trackStep(uint8_t frameMs)
{
    if (!trkPos || (trkWait && --trkWait))
        return;
    for (;;) {
        uint16_t w = *trkPos;
        if (!w) {
            if (!trkLoop) {
                trkPos = 0;
                return;
            }
            trkPos = trkStart;
            w = *trkPos;
            if (!w) {
                trkPos = 0;
                return;
            }
        }
        trkPos++;
        if (w & 3)
            continue;   /* instrument/volume commands: not supported */
        static const uint16_t c0[12] = {1635, 1732, 1835, 1945, 2060, 2183,
                                        2312, 2450, 2596, 2750, 2914, 3087};
        uint8_t n = (w >> 2) & 63;
        trkWait = w >> 8;
        if (!trkWait)
            trkWait = 1;
        if (n != 63) {
            int k = n - 2 + 48;
            gb.sound.tone(((uint32_t)c0[k % 12] << (k / 12)) / 100,
                          (uint32_t)trkWait * frameMs > 40 ? (uint32_t)trkWait * frameMs - 20 : 20);
        }
        return;
    }
}
} // namespace

int8_t Sound::play(const uint16_t *track, bool loop)
{
    trkStart = trkPos = track;
    trkLoop = loop;
    trkWait = 0;
    return 1;
}

void Sound::stop(int8_t id)
{
    if (id > 0) {
        trkPos = 0;
        stopFX();
    } else if (id == 0)
        gb_music_stop();
}

void Sound::stopFX()
{
    gb_audio_lock(1);
    fxs.active = false;
    gb_audio_lock(0);
}

/* ---- Save ----------------------------------------------------------- */

void Save::reset()
{
    memset(gb_save_data, 0, sizeof(gb_save_data));
    for (int d = 0; d < count; d++) {
        uint8_t *p = slot(d);
        if (defaults[d].type == SAVETYPE_INT) {
            int32_t v = defaults[d].val.ival;
            memcpy(p, &v, 4);
        } else if (defaults[d].val.ptr) {
            memcpy(p, defaults[d].val.ptr, defaults[d].length);
        }
    }
}

static uint32_t slotSize(const SaveDefault &d)
{
    return d.type == SAVETYPE_INT ? 4 : d.length;
}

uint8_t *Save::slot(int d)
{
    uint32_t off = 0;
    for (int k = 0; k < d; k++)
        off += slotSize(defaults[k]);
    return gb_save_data + off;
}

int Save::find(uint16_t i)
{
    for (int d = 0; d < count; d++)
        if (defaults[d].i == i)
            return d;
    return -1;
}

void Save::config(const SaveDefault *d, uint16_t n)
{
    defaults = d;
    count = n;
    uint32_t total = 0;
    for (int k = 0; k < n; k++)
        total += slotSize(d[k]);
    if (total > GB_SAVE_MAX)
        total = GB_SAVE_MAX;
    gb_save_load(total);
    if (!gb_save_loaded())
        reset();
}

int32_t Save::get(uint16_t i)
{
    int d = find(i);
    if (d < 0 || defaults[d].type != SAVETYPE_INT)
        return 0;
    int32_t v;
    memcpy(&v, slot(d), 4);
    return v;
}

bool Save::get(uint16_t i, void *buf, uint32_t size)
{
    int d = find(i);
    memset(buf, 0, size);
    if (d < 0 || defaults[d].type != SAVETYPE_BLOB)
        return false;
    memcpy(buf, slot(d), imin(size, defaults[d].length));
    return true;
}

bool Save::set(uint16_t i, int32_t num)
{
    int d = find(i);
    if (d < 0 || defaults[d].type != SAVETYPE_INT)
        return false;
    memcpy(slot(d), &num, 4);
    gb_save_dirty();
    return true;
}

bool Save::set(uint16_t i, const char *buf)
{
    return set(i, (const void *)buf, gb_strlen(buf) + 1);
}

bool Save::set(uint16_t i, const void *buf, uint32_t size)
{
    int d = find(i);
    if (d < 0 || defaults[d].type != SAVETYPE_BLOB)
        return false;
    memcpy(slot(d), buf, imin(size, defaults[d].length));
    gb_save_dirty();
    return true;
}

void Save::del(uint16_t i)
{
    int d = find(i);
    if (d < 0)
        return;
    uint8_t *p = slot(d);
    if (defaults[d].type == SAVETYPE_INT) {
        int32_t v = defaults[d].val.ival;
        memcpy(p, &v, 4);
    } else {
        memset(p, 0, defaults[d].length);
        if (defaults[d].val.ptr)
            memcpy(p, defaults[d].val.ptr, defaults[d].length);
    }
    gb_save_dirty();
}

/* ---- Gui ------------------------------------------------------------ */

void Gui::popup(const char *t, uint8_t duration)
{
    text = t;
    left = duration;
}

int8_t Gui::menu(const char *title, const char *const *items, uint8_t length)
{
    int8_t sel = 0;
    for (;;) {
        while (!gb.update()) {}
        if (gb.buttons.pressed(Button::down))
            sel = (sel + 1) % length;
        if (gb.buttons.pressed(Button::up))
            sel = (sel + length - 1) % length;
        if (gb.buttons.pressed(Button::a))
            return sel;
        if (gb.buttons.pressed(Button::b))
            return -1;
        Display &d = gb.display;
        d.clear();
        d.setColor(Color::white);
        d.setCursor(2, 1);
        d.print(title);
        for (uint8_t i = 0; i < length; i++) {
            d.setCursor(2, 14 + i * 7);
            d.print(i == sel ? '>' : ' ');
            d.print(items[i]);
        }
    }
}

void Gui::keyboard(const char *title, char *text)
{
    static const char set[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const int nset = sizeof(set) - 1;
    int len = gb_strlen(text), pos = 0;
    for (;;) {
        while (!gb.update()) {}
        int k = 0;
        while (k < nset && set[k] != text[pos])
            k++;
        if (gb.buttons.pressed(Button::down))
            k = (k + 1) % nset;
        if (gb.buttons.pressed(Button::up))
            k = (k + nset - 1) % nset;
        text[pos] = set[k % nset];
        if (gb.buttons.pressed(Button::left) && pos > 0)
            pos--;
        if (gb.buttons.pressed(Button::right) && pos < len - 1)
            pos++;
        if (gb.buttons.pressed(Button::a)) {
            if (pos == len - 1)
                return;
            pos++;
        }
        Display &d = gb.display;
        d.clear();
        d.setColor(Color::white);
        d.setCursor(2, 1);
        d.print(title);
        d.setCursor(2, 30);
        d.print(text);
        d.setCursor(2 + pos * 4, 36);
        d.print('^');
    }
}

void Gui::draw()
{
    if (!left)
        return;
    left--;
    Display &d = gb.display;
    int n = gb_strlen(text);
    int bw = imin(n * 4 + 3, d.w), bh = 9;
    int bx = (d.w - bw) / 2, by = d.h - bh;
    uint16_t pen = d.pen;
    int16_t cx0 = d.cursorX, cy0 = d.cursorY;
    uint8_t fs = d.fontSize;
    d.fontSize = 1;
    d.setColor(Color::darkgray);
    d.fillRect(bx, by, bw, bh);
    d.setColor(Color::white);
    d.drawRect(bx, by, bw, bh);
    bool w0 = d.textWrap;
    d.textWrap = false;
    d.setCursor(bx + 2, by + 2);
    d.print(text);
    d.textWrap = w0;
    d.fontSize = fs;
    d.cursorX = cx0;
    d.cursorY = cy0;
    d.pen = pen;
}

/* ---- Gamebuino ------------------------------------------------------ */

void Gamebuino::pickRandomSeed() { gb_srand(gb_millis()); }

void Gamebuino::begin()
{
    display.init(gb_game.width, gb_game.height,
                 gb_game.pixels == GB_INDEXED ? ColorMode::index
                                              : ColorMode::rgb565);
    frameCount = 0;
    nextFrameAt = 0;
    memset(buttons.states, 0, sizeof(buttons.states));
    gui.left = 0;
}

bool Gamebuino::update()
{
    unsigned long now = gb_millis();
    if ((long)(now - nextFrameAt) < 0) {
        gb_idle();
        return false;
    }
    unsigned long ms = frameMs ? frameMs : 40;
    nextFrameAt = (long)(now - nextFrameAt) > (long)ms ? now + ms : nextFrameAt + ms;
    gui.draw();
    if (display.fb)
        gb_present(display.fb, display.palette, display.colorMode != ColorMode::rgb565);
    else
        gb_pump();
    buttons.update();
    frameCount++;
    trackStep((uint8_t)ms);
    return true;
}

} // namespace Gamebuino_Meta

extern "C" int gb_sound_fill(int16_t *buf, int frames, int rate)
{
    using namespace Gamebuino_Meta;
    return soundFill(buf, frames, rate);
}

/* ---- heap ------------------------------------------------------------ */

/* A first-fit allocator over the free part of the plugin arena, for the
 * String class and games that allocate. Blocks carry an 8 byte header. */
namespace {
struct Blk {
    uint32_t size;      /* payload bytes, multiple of 8 */
    uint32_t used;
};
Blk *heapStart;
uint8_t *heapEnd;

void heapInit()
{
    size_t n;
    uint8_t *p = (uint8_t *)gb_heap_area(&n);
    uintptr_t a = ((uintptr_t)p + 7) & ~(size_t)7;
    n -= a - (uintptr_t)p;
    if (n > GB_HEAP_MAX)
        n = GB_HEAP_MAX;
    heapStart = (Blk *)a;
    heapStart->size = (n & ~(size_t)7) - sizeof(Blk);
    heapStart->used = 0;
    heapEnd = (uint8_t *)a + (n & ~(size_t)7);
}

Blk *nextBlk(Blk *b)
{
    Blk *n = (Blk *)((uint8_t *)(b + 1) + b->size);
    return (uint8_t *)n < heapEnd ? n : 0;
}
} // namespace

void *gb_alloc(size_t n)
{
    if (!heapStart)
        heapInit();
    n = (n + 7) & ~(size_t)7;
    if (!n)
        n = 8;
    for (Blk *b = heapStart; b; b = nextBlk(b)) {
        if (b->used || b->size < n)
            continue;
        if (b->size >= n + sizeof(Blk) + 8) {
            Blk *r = (Blk *)((uint8_t *)(b + 1) + n);
            r->size = b->size - n - sizeof(Blk);
            r->used = 0;
            b->size = n;
        }
        b->used = 1;
        return b + 1;
    }
    return 0;
}

void gb_release(void *p)
{
    if (!p)
        return;
    ((Blk *)p - 1)->used = 0;
    for (Blk *b = heapStart; b; b = nextBlk(b)) {   /* coalesce */
        Blk *n;
        while (!b->used && (n = nextBlk(b)) && !n->used)
            b->size += sizeof(Blk) + n->size;
    }
}

void *operator new(size_t n) { return gb_alloc(n); }
void *operator new[](size_t n) { return gb_alloc(n); }
void operator delete(void *p) { gb_release(p); }
void operator delete[](void *p) { gb_release(p); }
void operator delete(void *p, size_t) { gb_release(p); }
void operator delete[](void *p, size_t) { gb_release(p); }

/* ---- String ---------------------------------------------------------- */

void String::assign(const char *s, unsigned n)
{
    char *nb = (char *)gb_alloc(n + 1);
    if (!nb) {
        n = 0;
        nb = buf;
        if (!nb)
            return;
    } else {
        if (n)
            memcpy(nb, s, n);
        gb_release(buf);
    }
    nb[n] = 0;
    buf = nb;
    len = n;
}

void String::append(const char *s, unsigned n)
{
    if (!n)
        return;
    char *nb = (char *)gb_alloc(len + n + 1);
    if (!nb)
        return;
    if (len)
        memcpy(nb, buf, len);
    memcpy(nb + len, s, n);
    nb[len + n] = 0;
    gb_release(buf);
    buf = nb;
    len += n;
}

void String::number(long n, int base)
{
    char tmp[34];
    int i = 0;
    bool neg = n < 0 && base == 10;
    unsigned long v = neg ? -(unsigned long)n : (unsigned long)n;
    do {
        int d = v % base;
        tmp[i++] = d < 10 ? '0' + d : 'a' + d - 10;
        v /= base;
    } while (v);
    if (neg)
        tmp[i++] = '-';
    char out[34];
    for (int k = 0; k < i; k++)
        out[k] = tmp[i - 1 - k];
    assign(out, i);
}

SPIClass SPI;
SerialClass SerialUSB;

int snprintf(char *buf, size_t len, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = gb_vformat(buf, len, fmt, ap);
    va_end(ap);
    return n;
}

extern "C" void __cxa_pure_virtual() { gb_fatal("pure virtual call"); }

unsigned long millis() { return gb_millis(); }
long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void delay(unsigned long ms)
{
    unsigned long end = gb_millis() + ms;
    while ((long)(end - gb_millis()) > 0)
        gb_idle();
}

long random(long max) { return max > 0 ? gb_rand() % max : 0; }
long random(long min, long max) { return min + random(max - min); }
void randomSeed(unsigned long seed) { gb_srand(seed); }

#ifndef GB_HOST_MATH
/* libm is not linked: the few functions the games call. */
extern "C" double floor(double x)
{
    long i = (long)x;
    return (double)(i > x ? i - 1 : i);
}

extern "C" double round(double x)
{
    return x < 0 ? -floor(-x + 0.5) : floor(x + 0.5);
}

/* rounds half to even, like the C library */
extern "C" double nearbyint(double x)
{
    if (x < 0)
        return -nearbyint(-x);
    long f = (long)x;
    double d = x - f;
    if (d > 0.5 || (d == 0.5 && (f & 1)))
        f++;
    return f;
}
#endif
