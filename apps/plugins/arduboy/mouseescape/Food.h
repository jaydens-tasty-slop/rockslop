#ifndef food_h
#define food_h

#include <Arduboy2.h>

class Food
{
  public:
    Food(byte x, byte y, byte value, byte frame);
    byte getX();
    byte getY();
    byte getValue();
    byte getFrame();
  private:
    byte x;
    byte y;
    byte value;
    byte frame;
};

#endif
