#ifndef actor_h
#define actor_h

#include "Globals.h"

struct equpment {
  uint16_t atkMod;
  uint16_t defMod;
  uint16_t spcMod;
  const char* name;
  uint16_t cost;
};

struct baseStats {
  
  uint16_t totalHP;
  uint16_t strength;
  uint16_t defense;
  uint16_t speed;
  uint16_t special;
  
};


class actor {
  
  public:
  const char* name;
  int hp;
  const byte* bmp;
  uint16_t level;
  equpment weapon;
  baseStats statSeed;


   explicit actor();
  // pickClass(uint16_t type);
   
   uint16_t damage();
   void takeDamage(actor attacker, uint16_t x);
   void takeSpecial(actor attacker);
   void levelUp();
   uint16_t getStat (uint16_t stat);
   uint16_t expDrop();



};

  //class warrior: public actor {
  //public:
  //equpment weapon;
  
//};

//class tank: public actor {
 // equpment weapon;
  
//};

//class mage: public actor {
  //equpment weapon;
  
//};

#endif // actor_h



