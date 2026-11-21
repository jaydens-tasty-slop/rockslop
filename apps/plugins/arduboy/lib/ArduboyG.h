/* SPDX-License-Identifier: GPL-2.0-or-later */
// ArduboyG greyscale shim for YP3 (API subset of Mr.Blinky's/Pharap's
// ArduboyG, as used by Onebit Productions' games). The game draws one 1-bit
// plane per loop() into Arduboy2Base::sBuffer; waitForNextPlane() accumulates
// each finished plane into a 2-bit-per-pixel counter and, once per full frame,
// draws it in grey levels with ab_present_gray(). Header-only.
#pragma once
#include <Arduboy2.h>

#ifndef DARK_GRAY
#define DARK_GRAY 1
#define LIGHT_GRAY 2
#endif
#ifndef ABG_FRAME_MS
#define ABG_FRAME_MS 33
#endif

enum class ABG_Mode : uint8_t { L3, L4_Contrast, L4_Triplane };

template<ABG_Mode MODE>
class ArduboyGBase_Config : public Arduboy2Base
{
public:
  static constexpr uint8_t PLANES = (MODE == ABG_Mode::L4_Triplane) ? 3 : 2;

  static void begin() { Arduboy2Base::begin(); }
  static void startGray() { plane_ = 0; started_ = false; pending_ = true; last_ = ab_millis(); }
  static uint8_t currentPlane() { return plane_; }
  // Shape drawing with a grey colour: lit only on planes below the grey level.
  static void drawRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t c)
  { Arduboy2Base::drawRect(x, y, w, h, c > plane_ ? WHITE : BLACK); }
  static void fillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t c)
  { Arduboy2Base::fillRect(x, y, w, h, c > plane_ ? WHITE : BLACK); }
  static void drawPixel(int16_t x, int16_t y, uint8_t c)
  { Arduboy2Base::drawPixel(x, y, c > plane_ ? WHITE : BLACK); }

  static bool needsUpdate() { bool r = pending_; pending_ = false; return r; }

  static void waitForNextPlane(uint8_t clear = BLACK)
  {
    if (started_) {
      for (uint16_t i = 0; i < 1024; i++) {
        uint8_t b = sBuffer[i], c = lo_[i];
        lo_[i] = c ^ b;
        hi_[i] ^= c & b;
      }
      if (++plane_ >= PLANES) {
        plane_ = 0;
        present();
        pending_ = true;
      }
    }
    started_ = true;
    uint8_t fill = (clear > plane_) ? 0xFF : 0x00;
    for (uint16_t i = 0; i < 1024; i++) sBuffer[i] = fill;
    if (plane_ == 0) for (uint16_t i = 0; i < 1024; i++) lo_[i] = hi_[i] = 0;
  }

private:
  static void present()
  {
    ab_present_gray(lo_, hi_, PLANES);
    unsigned long now = ab_millis();
    if (now - last_ < ABG_FRAME_MS) ab_delay(ABG_FRAME_MS - (now - last_));
    last_ = ab_millis();
  }

  static uint8_t plane_;
  static bool started_, pending_;
  static unsigned long last_;
  static uint8_t lo_[1024], hi_[1024];
};
template<ABG_Mode M> uint8_t ArduboyGBase_Config<M>::plane_;
template<ABG_Mode M> bool ArduboyGBase_Config<M>::started_;
template<ABG_Mode M> bool ArduboyGBase_Config<M>::pending_;
template<ABG_Mode M> unsigned long ArduboyGBase_Config<M>::last_;
template<ABG_Mode M> uint8_t ArduboyGBase_Config<M>::lo_[1024];
template<ABG_Mode M> uint8_t ArduboyGBase_Config<M>::hi_[1024];
