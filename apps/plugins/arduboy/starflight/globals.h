#ifndef GLOBAL_H
#define GLOBAL_H

#include <Arduino.h>
#include "Arglib.h"
#include "bitmaps.h"

//define menu states (on main menu)
#define STATE_MENU_INTRO             0
#define STATE_MENU_MAIN              1
#define STATE_MENU_HELP              2
#define STATE_MENU_PLAY              3
#define STATE_MENU_INFO              4
#define STATE_MENU_SOUNDFX           5

//define game states (on main menu)
#define STATE_GAME_NEXT_LEVEL        6
#define STATE_GAME_PLAYING           7
#define STATE_GAME_PAUSE             8
#define STATE_GAME_OVER              9

#define GAME_TOP                     -6
#define GAME_BOTTOM                  55
#define GAME_LEFT                    0
#define GAME_RIGHT                   112

#define LEVEL_TO_START_WITH          1

#define SCORE_SMALL_FONT             0
#define SCORE_BIG_FONT               1

Arduboy arduboy;
Sprites sprites(arduboy);

byte gameState = STATE_MENU_INTRO;    // start the game with the TEAM a.r.g. logo
byte menuSelection = STATE_MENU_PLAY; // PLAY menu item is pre-selected
byte globalCounter = 0;
byte level;
unsigned long scorePlayer; 

#endif
