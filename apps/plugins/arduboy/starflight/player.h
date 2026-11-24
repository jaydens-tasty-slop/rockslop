#ifndef PLAYER_H
#define PLAYER_H

#define WEAPON_CANON             0
#define WEAPON_ROCKETS           1
#define WEAPON_LASER             2

#define MAX_WEAPON_TYPES         3
#define MAX_AMOUNT_BULLETS       8

#define DAMAGE_BULLET            1
#define DAMAGE_ROCKETS           2
#define DAMAGE_WAVE              4

#define FUSE_BULLET              25
#define FUSE_ROCKETS             50
#define FUSE_WAVE                50

#define SPEEDY_BULLET            3
#define SPEEDY_ROCKETS           1
#define SPEEDY_WAVE              2

#define START_FRAME_BULLET       0
#define START_FRAME_ROCKETS      1
#define START_FRAME_WAVE         5

#define MAX_FRAME_BULLET         0
#define MAX_FRAME_ROCKETS        3
#define MAX_FRAME_WAVE           7




#include <Arduino.h>
#include "globals.h"

const unsigned char PROGMEM bulletCharacteristics[] = {
  DAMAGE_BULLET, DAMAGE_ROCKETS, DAMAGE_WAVE,
  FUSE_BULLET, FUSE_ROCKETS, FUSE_WAVE,
  SPEEDY_BULLET, SPEEDY_ROCKETS, SPEEDY_WAVE,
  START_FRAME_BULLET, START_FRAME_ROCKETS, START_FRAME_WAVE,
  MAX_FRAME_BULLET, MAX_FRAME_ROCKETS, MAX_FRAME_WAVE
};

struct Players
{
  public:
    int x;
    int y;
    byte weaponType;
    boolean isActive;
    boolean isImune;
    boolean crashing;
    boolean hud;
    byte imuneTimer;
    byte life;
    int frame;
    byte bombs;
    byte currentBullet;


    void set()
    {
      x = 8;
      y = 24;
      frame = 2;
      isImune = true;
      isActive = true;
      imuneTimer = 60;
      crashing = false;
      hud = true;
      currentBullet = 0;
    }

    void reset()
    {
      weaponType = WEAPON_CANON;
      life = 3;
      bombs = 3;
      hud = true;
    }
};

struct PlayerShot
{
  public:
    byte x;
    byte y;
    byte damage;
    boolean isVisible;
    byte type;
    byte frame;
    byte maxFrame;
    int speedX;
    byte fuse;

    void set(byte weaponChoice)
    {
      type = weaponChoice;
      damage = pgm_read_word (&bulletCharacteristics[weaponChoice]);
      fuse = pgm_read_word (&bulletCharacteristics[weaponChoice + MAX_WEAPON_TYPES]);
      speedX = pgm_read_word (&bulletCharacteristics[weaponChoice + (2 * MAX_WEAPON_TYPES)]);
      maxFrame = pgm_read_word (&bulletCharacteristics[weaponChoice + (4 * MAX_WEAPON_TYPES)]);
      frame = 0;
    }

    void update()
    {
      if ((type == WEAPON_ROCKETS) && arduboy.everyXFrames(2))
      {
        frame++;
        if (fuse == 40) speedX += 2;
        if (fuse == 35) speedX += 2;
        if (fuse == 30) speedX += 1;
      }
      if ((type == WEAPON_LASER)) frame++;
      //else frame++;
      if (frame > maxFrame) frame = 0;
      fuse--;
      if ((fuse == 0) || (x > 126) || (damage < 1))
      {
        isVisible = false;
      }
      x += speedX;
    }
};

Players spaceShip;
PlayerShot bullets[MAX_AMOUNT_BULLETS];


void resetBullets()
{
  for (byte i = 0; i < MAX_AMOUNT_BULLETS; i++) bullets[i].isVisible = false;
}

void shoot()
{

  bullets[spaceShip.currentBullet].set(spaceShip.weaponType);
  bullets[spaceShip.currentBullet].isVisible = true;
  bullets[spaceShip.currentBullet].x = spaceShip.x;
  bullets[spaceShip.currentBullet].y = spaceShip.y + 4;
  spaceShip.currentBullet++;
  if (spaceShip.currentBullet > MAX_AMOUNT_BULLETS - 1) spaceShip.currentBullet = 0;
}

void checkIfShipIsImune()
{
  if (spaceShip.isImune)
  {
    if (arduboy.everyXFrames(3)) spaceShip.isActive = !spaceShip.isActive;
    spaceShip.imuneTimer--;
    if (spaceShip.imuneTimer < 1)
    {
      spaceShip.isImune = false;
      spaceShip.isActive = true;
    }
  }
}

void updateBullets()
{
  for (byte i = 0; i < MAX_AMOUNT_BULLETS; i++)
  {
    if (bullets[i].isVisible)
    {
      bullets[i].update();
    }
  }
}

void drawSpaceShip()
{
  if (spaceShip.isActive) sprites.drawPlusMask(spaceShip.x, spaceShip.y, spaceShip_plus_mask, spaceShip.frame);
}


void drawBullets()
{
  for (byte i = 0; i < MAX_AMOUNT_BULLETS; i++)
  {
    if (bullets[i].isVisible)
    {
      sprites.drawPlusMask(bullets[i].x, bullets[i].y, bullets_plus_mask, bullets[i].frame + pgm_read_word (&bulletCharacteristics[spaceShip.weaponType + (3 * MAX_WEAPON_TYPES)]));
    }
  }
}



#endif
