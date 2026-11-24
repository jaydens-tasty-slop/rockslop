#include "Food.h"

/*** Public ***/

Food::Food(byte x, byte y, byte value, byte frame)
{
  this->x = x;
  this->y = y;
  this->value = value;
  this->frame = frame;
}

byte Food::getX()
{
  return this->x;
}

byte Food::getY()
{
  return this->y;
}

byte Food::getValue()
{
  return this->value;
}

byte Food::getFrame()
{
  return this->frame;
}

