#pragma once

#include <Arduino.h>

#if defined( __AVR_ATtiny85__)
  // use Daniel C's fast display driver for ATtiny85 and SSD1306
  #define _USE_FAST_TINY_DRIVER_

  // ATtiny85 is heavily flash limited, so we can't have it all ;)
  // uncomment the following two lines to enable dungeon floor at the cost of sound effects
  #define _ENABLE_DUNGEON_FLOOR_
  #define NO_SOUND
#else
  // option to prevent usage of Arduboy2 by preprocessor directive
  #ifndef _NO_ARDUBOY2_
    // use Arduboy2 for everything that's not an ATtiny85
    #define _USE_ARDUBOY2_
    // always enable dungeon floor on Arduboy
    // draw dungeon floor (additional 192 bytes for graphics + some bytes for code)
    #define _ENABLE_DUNGEON_FLOOR_
  #endif
#endif
