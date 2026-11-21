// Minimal FX-only replacement for SpritesU (Mr.Blinky/Pharap), written for
// YP3: plain C++ drawing of FX plus-mask / overwrite images into
// Arduboy2Base::sBuffer. Image = w, h, then frames of pages*(w) (data[,mask]).
#pragma once
#include <Arduboy2.h>
#include <ArduboyFX.h>

namespace SpritesU_impl {
static void draw(int16_t x, int16_t y, uint24_t image, uint16_t frame, bool mask)
{
  FX::seekData(image);
  uint8_t w = FX::readPendingUInt8();
  uint8_t h = FX::readEnd();
  uint8_t pages = (h + 7) >> 3;
  uint32_t stride = (uint32_t)w * pages * (mask ? 2 : 1);
  uint32_t base = (uint32_t)image + 2 + stride * frame;
  for (uint8_t p = 0; p < pages; p++) {
    int16_t ys = y + 8 * p;
    if (ys >= 64 || ys <= -8) continue;
    uint8_t lim = (p == pages - 1 && (h & 7)) ? (uint8_t)(0xFF >> (8 - (h & 7))) : 0xFF;
    int16_t row = ys >> 3;
    uint8_t sh = ys & 7;
    FX::seekData(base + (uint32_t)p * w * (mask ? 2 : 1));
    for (uint8_t i = 0; i < w; i++) {
      uint8_t d = FX::readPendingUInt8();
      uint8_t m = mask ? FX::readPendingUInt8() : lim;
      if (!mask) m = lim;
      int16_t cx = x + i;
      if ((uint16_t)cx >= 128) continue;
      uint16_t D = (uint16_t)d << sh, M = (uint16_t)m << sh;
      if (row >= 0 && row < 8) {
        uint8_t *b = &Arduboy2Base::sBuffer[row * 128 + cx];
        *b = (*b & ~(uint8_t)M) | ((uint8_t)D & (uint8_t)M);
      }
      if (row + 1 >= 0 && row + 1 < 8 && (M >> 8)) {
        uint8_t *b = &Arduboy2Base::sBuffer[(row + 1) * 128 + cx];
        *b = (*b & ~(uint8_t)(M >> 8)) | ((uint8_t)(D >> 8) & (uint8_t)(M >> 8));
      }
    }
    FX::readEnd();
  }
}
}
namespace SpritesU {
inline void drawPlusMaskFX(int16_t x, int16_t y, uint24_t image, uint16_t frame) { SpritesU_impl::draw(x, y, image, frame, true); }
inline void drawOverwriteFX(int16_t x, int16_t y, uint24_t image, uint16_t frame) { SpritesU_impl::draw(x, y, image, frame, false); }
}
