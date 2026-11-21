#include <stdlib.h>
#include <math.h>
#include "enemy.h"
#include "images.h"

enemy::enemy(const char * nm, equpment wpn, baseStats seed, const uint8_t * sprite, int lvl){
  name = nm;
  hp = getStat(seed.totalHP);
  bmp = sprite;
  equpment weapon = wpn;
  level = lvl;
  baseStats statSeed = seed;
}




