#ifndef palettes_h
#define palettes_h

#include <Gamebuino-Meta.h>
#include <stdint.h>

const __FlashStringHelper* getPaletteName();
void setPalIndex(uint8_t index);
void setPaletteGame();
void setPaletteTitle();
void setPalInverse(uint8_t value);
uint8_t  getMaxPalettes();
uint8_t getPalIndex();
uint8_t getPalInverse();

#endif