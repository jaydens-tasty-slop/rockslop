#ifndef player_h
#define player_h
#include "actor.h"

class playerCharacter: public actor{
  
public:
  playerCharacter();
  int wallet;
  equpment weapon;
  equpment inventory[5];
  int ex;
  int arenalvl;
  int potion;

  void printInv();
  void printStats();
  void equipt(uint16_t inventoryLocation);
  void pickClass(uint16_t x);
  void checkLvlUp();
  void usePotion();
  
};



#endif // player_h

