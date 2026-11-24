#include "Maus.h"

/*** Public ***/

byte Maus::getX()
{
  return this->x;
}

byte Maus::getY()
{
  return this->y;
}

byte Maus::getSpeed()
{
  return this->moveSpeed;
}

byte Maus::getInvCount()
{
  return this->invincibleCount;
}

void Maus::updateMaus(byte newX, byte newY)
{
  this->x = newX;
  this->y = newY;
  if(this->isInvincible) this->invincibleCount++;
}

bool Maus::getInvincible()
{
  return this->isInvincible;
}

void Maus::setInvincible(bool isInv)
{
  this->isInvincible = isInv;
  if(!isInv) this->invincibleCount = 0;
}
/*** Private ***/
