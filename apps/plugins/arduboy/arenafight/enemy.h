#ifndef enemy_h
#define enemy_h
#include "actor.h"

class enemy: public actor{
  
public:
  enemy(const char * nm, equpment wpn, baseStats seed, const uint8_t * sprite, int lvl);


};


#endif // enemy_h

