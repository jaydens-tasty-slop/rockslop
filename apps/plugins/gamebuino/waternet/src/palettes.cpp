#include <Gamebuino-Meta.h>
#include "palettedefines.h"
#include "palettes.h"

uint8_t palIndex = 0;
uint8_t inverseColors = 0;
Color pal[16];

uint8_t  getMaxPalettes()
{
    return maxColorSelections;
}

void setPalIndex(uint8_t index)
{
  palIndex = index;
  //only possible to change on title
  setPaletteTitle();
}

void setPalInverse(uint8_t value)
{
    inverseColors = value;
    //only possible to change on title
    setPaletteTitle();
}

uint8_t getPalInverse()
{
    return inverseColors;
}

uint8_t getPalIndex()
{
  return palIndex;
}


void setPaletteTitle()
{

    switch(palIndex)
    {
        case 0: memcpy(pal, GB_PALETTE, 16 * sizeof(Color)); break;
        case 1: memcpy(pal, BLACK_WHITE_PALETTE_TITLE, 16 * sizeof(Color)); break;
        case 2: memcpy(pal, GBC_PALETTE_TITLE, 16 * sizeof(Color)); break;
        case 3: memcpy(pal, RED_PALETTE_TITLE, 16 * sizeof(Color)); break;
        case 4: memcpy(pal, WHITE_BLUE_PALETTE_TITLE, 16 * sizeof(Color)); break;
        case 5: memcpy(pal, TROPICAL_PALETTE_TITLE, 16 * sizeof(Color)); break;   
    }
    
    if(inverseColors)
    {
        Color tmp0 = pal[0];
        Color tmp1 = pal[1];
        Color tmp2 = pal[2];
        Color tmp3 = pal[3];

        pal[3] = tmp0;
        pal[2] = tmp1;
        pal[1] = tmp2;
        pal[0] = tmp3;
    }
    
    gb.display.setPalette(pal);
}

void setPaletteGame()
{

    switch(palIndex)
    {
        case 0: memcpy(pal, GB_PALETTE, 16 * sizeof(Color)); break;
        case 1: memcpy(pal, BLACK_WHITE_PALETTE_GAME, 16 * sizeof(Color)); break;
        case 2: memcpy(pal, GBC_PALETTE_GAME, 16 * sizeof(Color)); break;
        case 3: memcpy(pal, RED_PALETTE_GAME, 16 * sizeof(Color)); break;
        case 4: memcpy(pal, WHITE_BLUE_PALETTE_GAME, 16 * sizeof(Color)); break;
        case 5: memcpy(pal, TROPICAL_PALETTE_GAME, 16 * sizeof(Color)); break;
    }
    
    if(inverseColors)
    {
        Color tmp0 = pal[0];
        Color tmp1 = pal[1];
        Color tmp2 = pal[2];
        Color tmp3 = pal[3];

        pal[3] = tmp0;
        pal[2] = tmp1;
        pal[1] = tmp2;
        pal[0] = tmp3;
    }
    
    gb.display.setPalette(pal);
}

const __FlashStringHelper* getPaletteName()
{
  switch(palIndex)
  {
        case 0: return F("COLOR  GB");
        case 1: return F("COLOR  BW");
        case 2: return F("COLOR  GBC");
        case 3: return F("COLOR  RED");
        case 4: return F("COLOR  BLUE");  
        case 5: return F("COLOR  FIRE");
        default: return F("");
  }   
}