/* structs.h */

#pragma once
#include <stdint.h>

/* Structs */
struct enem
{
  int16_t type[7];
  float x[7];
  float y[7];
  int16_t direction[7];
  int16_t tilex[7];
  int16_t tiley[7];
  int16_t animation[7];
  int16_t limleft[7];
  int16_t limright[7];
  int16_t speed[7];
  int16_t fire[7];
  int16_t adjustx1[7];
  int16_t adjustx2[7];
  int16_t adjusty1[7];
  int16_t adjusty2[7];
};

struct checkpoint_t {
    int16_t room, previous;
    float x,y;
};

struct hero
{
  float x;
  float y;
  int16_t direction;    /* 0: facing left, 1: facing right */
  int16_t jump;         /* jump state:
                           0: not jumping
                           1: going up
                           2: going down */
  float height;         /* Limit of jump */
  int16_t animation;    /* animation frame */
  float gravity;        /* downwards speed, constant */
  int16_t points[8];        /* Points of collision */
  int16_t ground;       /* Pixel where is ground */
  int16_t collision[4]; /* Collisions, in 4 directions */
  int16_t ducking;      /* 1 or 0 */

  // int checkpoint[4];
  checkpoint_t checkpoint;
  int16_t state[2];     /* lives and crosses */
  int16_t flags[7];     /* various flags:
                           0: castle doors closed
                           1: bell touched
                           2: lever, room 10
                           3: lever, room 9
                           4: lever, room 20
                           5:
                           6: progress flag (?):
                                0: start
                                1: checkpoint taken (probably?)
                                2:
                                3: parchment touched
                                4:
                             >= 5: player control disabled
                                6:
                                7:
                              > 7: ending sequence counter
                           */
  int16_t death;        /* 0: alive, otherwise a death animation frame */
  int16_t push[4];      /* keys being held (0 or 1):
                           0: ?
                           1: ?
                           2: left
                           3: right */
  int16_t temp;         /* 0: initial, nothing
                           1: set on death, disables control
                           ... used as a counter sometimes
                           */
};

struct roomnums {
    int32_t current, previous;
};

typedef struct
{
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
} rect;
