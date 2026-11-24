#pragma once
#include "zarduboy.h"

uint16_t constexpr sndLevelIntro[] PROGMEM =
{
    Note_E5,   200,
    Note_Rest, 200,
    Note_E5,   200,
    Note_C5,   200,
    Note_G3,   100,
    Note_Rest, 900,
    Note_G3,   100,
    Note_End
};

uint16_t constexpr sndLevelIntro2[] PROGMEM =
{
    Note_E5,   200, // 3000
    Note_Rest, 100,
    Note_F5,   100,
    Note_Rest, 50,
    Note_E5,   100,
    Note_Rest, 50,
    Note_F5,   100,
    Note_Rest, 100,
    //Note_E5,   100,
    //Note_F5,   50,
    //Note_E5,   50,
    //Note_E5,   50,
    //Note_F5,   50,
    //Note_F5,   100,
    //Note_CS5,  100,

    Note_G3,   100, // 2200
    Note_Rest, 900,
    Note_G3,   100,
    Note_End
};

uint16_t constexpr sndHammerHit[] PROGMEM =
{
    Note_G3, 100,
    Note_G4, 100,
    Note_F3, 100,
    Note_End
};

uint16_t constexpr sndHammerIsHit[] PROGMEM =
{
    Note_G3,  100,
    Note_GS4, 100,
    Note_FS4, 100,
    Note_End
};

uint16_t constexpr sndHammeMiss[] PROGMEM =
{
    Note_G3, 100,
    Note_End
};

uint16_t constexpr sndMenuSelect[] PROGMEM =
{
    Note_G3, 100,
    Note_F3, 300,
    Note_End
};
