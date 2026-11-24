#ifndef maus_h
#define maus_h

#include <Arduboy2.h>

class Maus
{
  public:
    Maus(){};
    byte getX();
    byte getY();
    byte getSpeed();
    byte getInvCount();
    void updateMaus(byte newX, byte newY);
    void setInvincible(bool isInv);
    bool getInvincible();
  private:
    byte x;
    byte y;
    byte moveSpeed = 4;
    byte invincibleCount = 0;
    bool isInvincible = false;
};

#endif
