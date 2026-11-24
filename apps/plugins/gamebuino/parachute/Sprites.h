#ifndef PARACHUTE_SPRITES
#define PARACHUTE_SPRITES

#include "Global.h"

// -------------------------------------------------------------------------
// Sprites
// -------------------------------------------------------------------------
// The surface of the spritesheet is deliberately not optimized here to
// facilitate positioning calculations at the time of rendering.
// -------------------------------------------------------------------------

// Each sprite is characterized by its dimensions and coordinates on Spritesheet
struct Sprite {
  uint8_t spritesheet;
  uint8_t x, y;
  uint8_t w, h;
};

// a static sprite for the Helicopter
constexpr Sprite helicopter = { idSpritesheetB, 125, 5, 30, 25 };

// Sprites of the blades of the Helicopter
constexpr Sprite blades[] = {
  { idSpritesheetA, 120,  6,  9, 1 },  //  0
  { idSpritesheetA, 132,  6,  9, 1 },  //  1
  { idSpritesheetA, 135, 10,  9, 1 },  //  2
  { idSpritesheetA, 147, 10,  9, 1 }   //  3
};

// the different postures and positions of the player's avatar
constexpr Sprite boat[] = {
  { idSpritesheetB, 16, 80,  27, 20 },  //  0
  { idSpritesheetA, 47, 80,  28, 24 },  //  1
  { idSpritesheetB, 79, 80,  31, 19 }   //  2
};

// the misses sprites
constexpr Sprite miss[] = {
  { idSpritesheetA, 120, 86,  22, 7  },  //  0
  { idSpritesheetA, 117, 95,  13, 11 },  //  1
  { idSpritesheetA, 131, 95,  13, 11 },  //  2
  { idSpritesheetA, 145, 95,  13, 11 }   //  3
};

// the sprites of the flooded
constexpr Sprite flooded[] = {
  { idSpritesheetA, 88 , 103,  20, 10 },  //  0
  { idSpritesheetB, 56 , 103,  21, 11 },  //  1
  { idSpritesheetA, 26 , 102,  20, 12 },  //  2
  { idSpritesheetA, 46 , 115,  13,  8 },  //  3
  { idSpritesheetA, 74 , 113,  13,  9 },  //  4
  { idSpritesheetA, 116, 109,  12, 14 }   //  5
};

// the sprites of the shark
constexpr Sprite shark[] = {
  { idSpritesheetA, 79 , 106,  7 ,  6 },  //  0
  { idSpritesheetA, 47 , 106,  7 ,  6 },  //  1
  { idSpritesheetA, 32 , 116,  6 ,  6 },  //  2
  { idSpritesheetA, 60 , 115,  7 ,  7 },  //  3
  { idSpritesheetB, 89 , 107,  25, 16 }   //  4
};

// the sprite pause
constexpr Sprite spritePause = { idSpritesheetA, 1, 9, 28, 7 };

// the sprite Game Over
constexpr Sprite spriteGameOver = { idSpritesheetA, 1, 17, 53, 7 };

// the sprites of the para
constexpr Sprite para[] = {
  // col 1
  { idSpritesheetA, 106,  5,  9, 13 },  //  0
  { idSpritesheetB,  90,  8, 14, 14 },  //  1
  { idSpritesheetA,  76,  8, 15, 21 },  //  2
  { idSpritesheetB,  63, 14, 11, 25 },  //  3
  { idSpritesheetA,  48, 33, 15, 22 },  //  4
  { idSpritesheetB,  36, 52, 18, 22 },  //  5
  { idSpritesheetA,  28, 72, 19, 22 },  //  6
  // col 2
  { idSpritesheetA, 112, 19,  10, 12 },  //  7
  { idSpritesheetB, 100, 17,  15, 14 },  //  8
  { idSpritesheetA,  89, 18,  11, 27 },  //  9
  { idSpritesheetB,  74, 32,  15, 23 },  //  10
  { idSpritesheetA,  64, 48,  17, 24 },  //  11
  { idSpritesheetB,  59, 72,  19, 22 },  //  12
  // col 3
  { idSpritesheetA, 124, 30,  10, 11 },  //  13
  { idSpritesheetB, 114, 28,  11, 18 },  //  14
  { idSpritesheetA, 100, 32,  14, 21 },  //  15
  { idSpritesheetB,  92, 49,  18, 23 },  //  16
  { idSpritesheetA,  93, 72,  18, 22 }   //  17
};

#endif
