/* ZArduboy: the drawing helpers of Ardu-Whack's own zarduboy.h (GPL-3),
 * running on the Arduboy2 framebuffer. Input, sound and EEPROM use the
 * Arduboy2/ArduboyTones/EEPROM shims instead of the AVR registers. */
#pragma once
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <Arduboy2.h>
#include <ArduboyTones.h>
#include <EEPROM.h>

#define finline __attribute__((always_inline)) inline

template<class... T>
finline void unused(T&&...) {}

#undef min
#undef max

template<typename T>
finline T const& max(T const& a, T const& b) { return (a < b)? b : a; }

template<typename T>
finline T const& min(T const& a, T const& b) { return (a < b)? a : b; }

template<typename T>
finline void swap(T& a, T& b)
{
    auto t = a;
    a = b;
    b = t;
}

template<typename T>
finline T clamp(T a, T amin, T amax)
{
    return min<T>(max<T>(a, amin), amax);
}

enum eColor : uint8_t
{
    Black = 0x00,
    White = 0xff
};

class ZArduboy
{
public:
    static constexpr auto const InputUp    = uint8_t{_BV(7)};
    static constexpr auto const InputRight = uint8_t{_BV(6)};
    static constexpr auto const InputDown  = uint8_t{_BV(4)};
    static constexpr auto const InputLeft  = uint8_t{_BV(5)};
    static constexpr auto const InputA     = uint8_t{_BV(3)};
    static constexpr auto const InputB     = uint8_t{_BV(2)};
    static constexpr auto const LCDWidth   = int16_t{128};
    static constexpr auto const LCDHeight  = int16_t{64};
    static constexpr auto const LCDRAMSize = int16_t{1024}; // 128*64/8
    static constexpr auto const EEPROMSize = uint16_t{1024};

public:
    ZArduboy() : _tones(Arduboy2::audio.enabled) {}

    void begin()
    {
        _a.begin();
        _a.setFrameRate(60);
        _frameBuffer = _a.getBuffer();
        _a.clear();
        _tones.volumeMode(VOLUME_ALWAYS_NORMAL);
    }

    uint8_t buttonState() const
    {
        uint8_t b = _a.buttonsState();
        uint8_t state = 0;
        if(b & UP_BUTTON) state |= InputUp;
        if(b & RIGHT_BUTTON) state |= InputRight;
        if(b & LEFT_BUTTON) state |= InputLeft;
        if(b & DOWN_BUTTON) state |= InputDown;
        if(b & A_BUTTON) state |= InputA;
        if(b & B_BUTTON) state |= InputB;
        return state;
    }

    void idle() { _a.idle(); }

    void display() { _a.display(true); }

    void initRandomSeed() { randomSeed(millis()); }

    void drawBitmap(int16_t x, int16_t y, uint8_t const* bitmap, uint8_t w, uint8_t h)
    {
        if(((x+w) <= 0) || (x > (LCDWidth-1)) || ((y+h) <= 0) || (y > (LCDHeight-1)))
            return;

        auto yOffset = int16_t(abs(y) % 8);
        auto sRow = y / 8;
        if(y < 0)
        {
            --sRow;
            yOffset = 8 - yOffset;
        }
        auto rows = int8_t(h/8);
        if((h%8) != 0)
            ++rows;
        for(auto a = int8_t{0}; a < rows; ++a)
        {
            auto bRow = sRow + a;
            if(bRow > ((LCDHeight/8)-1))
                break;
            if(bRow <= -2)
                continue;
            for(auto iCol = int8_t{0}; iCol < w; ++iCol)
            {
                if((iCol + x) > (LCDWidth-1))
                    break;
                if((iCol + x) < 0)
                    continue;
                auto color = pgm_read_byte(bitmap + (a*w) + iCol);
                if(bRow >= 0)
                    _frameBuffer[ (bRow*LCDWidth) + x + iCol ] |= (color << yOffset);
                if((yOffset != 0) && (bRow < ((LCDHeight/8)-1)))
                    _frameBuffer[ ((bRow+1)*LCDWidth) + x + iCol ] |= (color >> (8-yOffset));
            }
        }
    }

    void drawBitmapEx(int16_t x, int16_t y, uint8_t const* bitmap, uint8_t w, uint8_t h, bool flip)
    {
        if(((x+w) <= 0) || (x > (LCDWidth-1)) || ((y+h) <= 0) || (y > (LCDHeight-1)))
            return;

        auto yOffset = int16_t(abs(y) % 8);
        auto sRow = y / 8;
        if(y < 0)
        {
            --sRow;
            yOffset = 8 - yOffset;
        }
        auto rows = int8_t(h/8);
        if((h%8) != 0)
            ++rows;
        for(auto a = int8_t{0}; a < rows; ++a)
        {
            auto bRow = sRow + a;
            if(bRow > ((LCDHeight/8)-1))
                break;
            if(bRow <= -2)
                continue;

            for(auto col = int8_t{0}; col < w; ++col)
            {
                if((col + x) > (LCDWidth-1))
                    break;
                if((col + x) < 0)
                    continue;
                auto color = pgm_read_byte(bitmap + (a*w) + ((flip==true)? w-col-1 : col));
                if(bRow >= 0)
                    _frameBuffer[ (bRow*LCDWidth) + x + col ] |= (color << yOffset);
                if((yOffset != 0) && (bRow < ((LCDHeight/8)-1)))
                    _frameBuffer[ ((bRow+1)*LCDWidth) + x + col ] |= (color >> (8-yOffset));
            }
        }
    }

    void drawBitmapOver(int16_t x, int16_t y, uint8_t const* bitmap, uint8_t const* mask, uint8_t w, uint8_t h)
    {
        if(((x + w) <= 0) || (x > (LCDWidth - 1)) || ((y + h) <= 0) || (y > (LCDHeight - 1)))
            return;
        // xOffset technically doesn't need to be 16 bit but the math operations
        // are measurably faster if it is
        auto yOffset = static_cast<int8_t>(abs(y) % 8);
        auto sRow = static_cast<int8_t>(y / 8);
        if((y < 0) && (yOffset > 0))
        {
            --sRow;
            yOffset = 8 - yOffset;
        }
        // if the left side of the render is offscreen skip those loops
        auto xOffset = static_cast<uint16_t>((x<0)? abs(x) : 0);
        // if the right side of the render is offscreen skip those loops
        auto rendered_width = static_cast<uint8_t>(((x + w) > (LCDWidth - 1))? ((LCDWidth - x) - xOffset) : (w - xOffset));
        // if the top side of the render is offscreen skip those loops
        auto start_h = static_cast<uint8_t>((sRow < -1)? abs(sRow) - 1 : 0);
        auto loop_h = static_cast<uint8_t>(h / 8 + (h % 8 > 0 ? 1 : 0)); // divide, then round up
        // if (sRow + loop_h - 1 > (HEIGHT/8)-1)
        if((sRow + loop_h) > (LCDHeight / 8))
            loop_h = static_cast<uint8_t>((LCDHeight / 8) - sRow);
        // prepare variables for loops later so we can compare with 0
        // instead of comparing two variables
        loop_h -= start_h;
        sRow += start_h;
        auto ofs = static_cast<uint16_t>((sRow * LCDWidth) + x + xOffset);
        auto bofs = bitmap + (start_h * w) + xOffset;
        auto mul_amt = static_cast<uint8_t>(1 << yOffset);

        if(mask == nullptr)
        {
            // we only want to mask the 8 bits of our own sprite, so we can
            // calculate the mask before the start of the loop
            auto mask_data = static_cast<uint16_t>(~(0xFF * mul_amt));
            // really if yOffset = 0 you have a faster case here that could be
            // optimized
            for(auto a = uint8_t{0}; a < loop_h; ++a)
            {
                for(auto iCol = uint8_t{0}; iCol < rendered_width; ++iCol)
                {
                    auto bitmap_data = uint16_t(pgm_read_byte(bofs) * mul_amt);
                    if(sRow >= 0)
                    {
                        auto data = _frameBuffer[ofs];
                        data &= static_cast<uint8_t>(mask_data);
                        data |= static_cast<uint8_t>(bitmap_data);
                        _frameBuffer[ofs] = data;
                    }
                    if((yOffset != 0) && (sRow < 7))
                    {
                        auto tmp = static_cast<uint16_t>(ofs + LCDWidth);
                        auto data = _frameBuffer[tmp];
                        data &= *( reinterpret_cast<uint8_t*>(&mask_data) + 1);
                        data |= *( reinterpret_cast<uint8_t*>(&bitmap_data) + 1);
                        _frameBuffer[tmp] = data;
                    }
                    ++ofs;
                    ++bofs;
                }
                ++sRow;
                bofs += w - rendered_width;
                ofs += LCDWidth - rendered_width;
            }
        }
        else
        {
            auto mask_ofs = const_cast<uint8_t*>(mask) + (start_h * w) + xOffset;
            for(auto a = uint8_t{0}; a < loop_h; ++a)
            {
                for(auto iCol = uint8_t{0}; iCol < rendered_width; ++iCol)
                {
                    // NOTE: you might think in the yOffset==0 case that this results
                    // in more effort, but in all my testing the compiler was forcing
                    // 16-bit math to happen here anyways, so this isn't actually
                    // compiling to more code than it otherwise would. If the offset
                    // is 0 the high part of the word will just never be used.

                    // load data and bit shift
                    // mask needs to be bit flipped
                    uint16_t mask_data = ~(pgm_read_byte(mask_ofs) * mul_amt);
                    uint16_t bitmap_data = pgm_read_byte(bofs) * mul_amt;
                    if(sRow >= 0)
                    {
                        uint8_t data = _frameBuffer[ofs];
                        data &= static_cast<uint8_t>(mask_data);
                        data |= static_cast<uint8_t>(bitmap_data);
                        _frameBuffer[ofs] = data;
                    }
                    if((yOffset != 0) && (sRow < 7))
                    {
                        auto tmp = uint16_t(ofs + LCDWidth);
                        uint8_t data = _frameBuffer[tmp];
                        data &= (*(reinterpret_cast<unsigned char *>(&mask_data) + 1));
                        data |= (*(reinterpret_cast<unsigned char *>(&bitmap_data) + 1));
                        _frameBuffer[tmp] = data;
                    }
                    ofs++;
                    mask_ofs++;
                    bofs++;
                }
                sRow++;
                bofs += w - rendered_width;
                mask_ofs += w - rendered_width;
                ofs += LCDWidth - rendered_width;
            }
        }
    }

    void drawCompressed(int16_t sx, int16_t sy, uint8_t const* bitmap, uint8_t w, uint8_t h)
    {
        // no need to draw at all if we're offscreen
        if((sx + w < 0) || (sx >= LCDWidth) || (sy + h < 0) || (sy >= LCDHeight))
            return;
        // set up decompress state
        _cs.src = bitmap;
        _cs.bit = 0x100;
        _cs.byte = 0;
        _cs.src_pos = 0;
        // sy = sy - (frame*h);
        auto yOffset = abs(sy) % 8;
        auto sRow = static_cast<int8_t>(sy / 8);
        if(sy < 0)
        {
            --sRow;
            yOffset = 8 - yOffset;
        }
        auto rows = h / 8;
        if((h%8) != 0)
            ++rows;
        auto a = int8_t{0}; // +(frame*rows);
        auto iCol = int16_t{0};
        auto byte = uint8_t{0};
        auto bit = uint16_t{1};
        auto col = _getval(1); // starting colour
        while(a < rows) // + (frame*rows))
        {
            auto bl = uint8_t{1};
            while(!_getval(1))
                bl += 2;
            auto len = _getval(bl) + 1; // span length
            // draw the span
            for(auto i = 0; i < len; ++i)
            {
                if(col)
                    byte |= bit;
                bit <<= 1;
                if(bit == 0x100) // reached end of byte
                {
                    // draw
                    auto bRow = static_cast<int8_t>(sRow + a);
                    //if (byte) // possible optimisation
                    if((bRow <= (LCDHeight / 8) - 1)
                       && (bRow > -2)
                       && (iCol + sx <= (LCDWidth - 1))
                       && (iCol + sx >= 0))
                    {
                        if(bRow >= 0)
                            _frameBuffer[(bRow * LCDWidth) + sx + iCol] |= byte << yOffset;
                        //if((yOffset) && (bRow < (LCDHeight / 8) - 1) && (bRow > -2))
                        if(yOffset)
                            _frameBuffer[((bRow + 1)*LCDWidth) + sx + iCol] |= byte >> (8 - yOffset);
                    }
                    // iterate
                    ++iCol;
                    if(iCol >= w)
                    {
                        iCol = 0;
                        ++a;
                    }
                    // reset byte
                    byte = 0;
                    bit = 1;
                }
            }
            col = 1 - col; // toggle colour for next span
        }
    }

    void drawPixel(int16_t x, int16_t y, uint8_t color)
    {
        if((x < 0) || (y < 0) || (x >= LCDWidth) || (y >= LCDHeight))
            return;
        auto pos = (y / 8) * LCDWidth + x;
        if(color == eColor::White)
            _frameBuffer[pos] |= _BV(y % 8);
        else
            _frameBuffer[pos] &= ~_BV(y % 8);
    }

    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color)
    {
        auto steep = abs(y1 - y0) > abs(x1 - x0);
        if(steep)
        {
            swap(x0, y0);
            swap(x1, y1);
        }
        if(x0 > x1)
        {
            swap(x0, x1);
            swap(y0, y1);
        }

        auto dx = x1 - x0;
        auto dy = abs(y1 - y0);
        auto err = dx >> 1;
        auto ystep = (y0 < y1) ? 1 : -1;
        for(; x0 <= x1; ++x0)
        {
            auto x = (steep) ? y0 : x0;
            auto y = (steep) ? x0 : y0;
            drawPixel(x, y, color);
            err -= dy;
            if(err < 0)
            {
                y0 += ystep;
                err += dx;
            }
        }
    }

    void drawHLine(int16_t x0, int16_t y, int16_t x1, uint8_t color)
    {
        auto start = max(x0, int16_t{0});
        auto end = min(x1, int16_t{LCDWidth});
        for(auto xpos = start; xpos <= end; ++xpos)
            drawPixel(xpos, y, color);
    }

    void drawVLine(int16_t x, int16_t y0, int16_t y1, uint8_t color)
    {
        auto start = max(y0, int16_t{0});
        auto end = min(y1, int16_t{LCDHeight});
        for(auto ypos = start; ypos <= end; ++ypos)
            drawPixel(x, ypos, color);
    }

    void drawRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color)
    {
        drawHLine(x0, y0, x1, color);
        drawHLine(x0, y1, x1, color);
        drawVLine(x0, y0+1, y1-1, color);
        drawVLine(x1, y0+1, y1-1, color);
    }

    void drawCircle(int16_t centerX, int16_t centerY, uint8_t radius, eColor color)
    {
        drawPixel(centerX, centerY + radius, color);
        drawPixel(centerX, centerY - radius, color);
        drawPixel(centerX + radius, centerY, color);
        drawPixel(centerX - radius, centerY, color);

        auto f = static_cast<int16_t>(1 - radius);
        auto x = int16_t{0};
        auto y = int16_t{radius};
        auto ddF_x = int16_t{1};
        auto ddF_y = static_cast<int16_t>(-2 * radius);
        while(x < y)
        {
            if(f >= 0)
            {
                --y;
                ddF_y += 2;
                f += ddF_y;
            }
            ++x;
            ddF_x += 2;
            f += ddF_x;

            drawPixel(centerX + x, centerY + y, color);
            drawPixel(centerX - x, centerY + y, color);
            drawPixel(centerX + x, centerY - y, color);
            drawPixel(centerX - x, centerY - y, color);
            drawPixel(centerX + y, centerY + x, color);
            drawPixel(centerX - y, centerY + x, color);
            drawPixel(centerX + y, centerY - x, color);
            drawPixel(centerX - y, centerY - x, color);
        }
    }


    void enableSound(bool value)
    {
        _isSoundEnabled = value;
        if(value)
            Arduboy2::audio.on();
        else
            Arduboy2::audio.off();
    }

    bool isSoundEnabled() { return _isSoundEnabled; }

    void tones(uint16_t const* data)
    {
        if(_isSoundEnabled)
            _tones.tones(data);
    }

    void writeToEEPROM(uint16_t pos, uint8_t value) { EEPROM.update(pos, value); }
    uint8_t readFromEEPROM(uint16_t pos) { return EEPROM.read(pos); }
    void updateToEEPROM(uint16_t pos, uint8_t value) { EEPROM.update(pos, value); }

private:
    Arduboy2 _a;
    ArduboyTones _tones;
    uint8_t* _frameBuffer = nullptr;
    bool _isSoundEnabled = false;

    // for compressed bitmaps
    struct _CSESSION
    {
        int byte;
        int bit;
        uint8_t const* src;
        int src_pos;
    };
    _CSESSION _cs;

    int16_t _getval(uint8_t bits)
    {
        auto val = int16_t{0};
        for(auto i = uint8_t{0}; i < bits; ++i)
        {
            if(_cs.bit == 0x100)
            {
                _cs.bit = 0x1;
                _cs.byte = pgm_read_byte(&_cs.src[_cs.src_pos]);
                ++_cs.src_pos;
            }
            if(_cs.byte & _cs.bit)
                val += (1 << i);
            _cs.bit <<= 1;
        }
        return val;
    }

};

enum eNote : uint16_t
{
    Note_Rest = 0,
    Note_C0   = 16,
    Note_CS0  = 17,
    Note_D0   = 18,
    Note_DS0  = 19,
    Note_E0   = 21,
    Note_F0   = 22,
    Note_FS0  = 23,
    Note_G0   = 25,
    Note_GS0  = 26,
    Note_A0   = 28,
    Note_AS0  = 29,
    Note_B0   = 31,
    Note_C1   = 33,
    Note_CS1  = 35,
    Note_D1   = 37,
    Note_DS1  = 39,
    Note_E1   = 41,
    Note_F1   = 44,
    Note_FS1  = 46,
    Note_G1   = 49,
    Note_GS1  = 52,
    Note_A1   = 55,
    Note_AS1  = 58,
    Note_B1   = 62,
    Note_C2   = 65,
    Note_CS2  = 69,
    Note_D2   = 73,
    Note_DS2  = 78,
    Note_E2   = 82,
    Note_F2   = 87,
    Note_FS2  = 93,
    Note_G2   = 98,
    Note_GS2  = 104,
    Note_A2   = 110,
    Note_AS2  = 117,
    Note_B2   = 123,
    Note_C3   = 131,
    Note_CS3  = 139,
    Note_D3   = 147,
    Note_DS3  = 156,
    Note_E3   = 165,
    Note_F3   = 175,
    Note_FS3  = 185,
    Note_G3   = 196,
    Note_GS3  = 208,
    Note_A3   = 220,
    Note_AS3  = 233,
    Note_B3   = 247,
    Note_C4   = 262,
    Note_CS4  = 277,
    Note_D4   = 294,
    Note_DS4  = 311,
    Note_E4   = 330,
    Note_F4   = 349,
    Note_FS4  = 370,
    Note_G4   = 392,
    Note_GS4  = 415,
    Note_A4   = 440,
    Note_AS4  = 466,
    Note_B4   = 494,
    Note_C5   = 523,
    Note_CS5  = 554,
    Note_D5   = 587,
    Note_DS5  = 622,
    Note_E5   = 659,
    Note_F5   = 698,
    Note_FS5  = 740,
    Note_G5   = 784,
    Note_GS5  = 831,
    Note_A5   = 880,
    Note_AS5  = 932,
    Note_B5   = 988,
    Note_C6   = 1047,
    Note_CS6  = 1109,
    Note_D6   = 1175,
    Note_DS6  = 1245,
    Note_E6   = 1319,
    Note_F6   = 1397,
    Note_FS6  = 1480,
    Note_G6   = 1568,
    Note_GS6  = 1661,
    Note_A6   = 1760,
    Note_AS6  = 1865,
    Note_B6   = 1976,
    Note_C7   = 2093,
    Note_CS7  = 2218,
    Note_D7   = 2349,
    Note_DS7  = 2489,
    Note_E7   = 2637,
    Note_F7   = 2794,
    Note_FS7  = 2960,
    Note_G7   = 3136,
    Note_GS7  = 3322,
    Note_A7   = 3520,
    Note_AS7  = 3729,
    Note_B7   = 3951,
    Note_C8   = 4186,
    Note_CS8  = 4435,
    Note_D8   = 4699,
    Note_DS8  = 4978,
    Note_E8   = 5274,
    Note_F8   = 5588,
    Note_FS8  = 5920,
    Note_G8   = 6272,
    Note_GS8  = 6645,
    Note_A8   = 7040,
    Note_AS8  = 7459,
    Note_B8   = 7902,
    Note_C9   = 8372,
    Note_CS9  = 8870,
    Note_D9   = 9397,
    Note_DS9  = 9956,
    Note_E9   = 10548,
    Note_F9   = 11175,
    Note_FS9  = 11840,
    Note_G9   = 12544,
    Note_GS9  = 13290,
    Note_A9   = 14080,
    Note_AS9  = 14917,
    Note_B9   = 15804,
    Note_End  = 0x8000
};
