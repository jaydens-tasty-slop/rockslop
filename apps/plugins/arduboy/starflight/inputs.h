#ifndef INPUTS_H
#define INPUTS_H

#include <Arduino.h>
#include "globals.h"
#include "player.h"


void checkInputs()
{

  if (arduboy.pressed(RIGHT_BUTTON) && !spaceShip.crashing)
  {
    if (spaceShip.x < GAME_RIGHT) spaceShip.x++;
  }
  if (arduboy.pressed(LEFT_BUTTON) && !spaceShip.crashing)
  {
    if (spaceShip.x > GAME_LEFT) spaceShip.x--;
  }
  if (arduboy.pressed(UP_BUTTON) && !spaceShip.crashing)
  {
    if (spaceShip.y > GAME_TOP) spaceShip.y--;
    if (arduboy.everyXFrames(9)) spaceShip.frame--;
    if (spaceShip.frame < 0)spaceShip.frame = 0;
  }
  if (arduboy.pressed(DOWN_BUTTON) && !spaceShip.crashing)
  {
    if (spaceShip.y < GAME_BOTTOM) spaceShip.y++;
    if (arduboy.everyXFrames(9)) spaceShip.frame++;
    if (spaceShip.frame  > 4)spaceShip.frame = 4;
  }
  if (arduboy.justPressed(B_BUTTON) && !spaceShip.crashing)
  {
    shoot();
    //if (arduboy.everyXFrames(1) && (bombCharge > 1))bombCharge--;
  }
  /*
    if (arduboy.pressed(B_BUTTON) && !spaceShip.crashing && (bombCharge < 120) && !playerBomb.isActive && (bombs > 0))
    {
      if (arduboy.everyXFrames(1))bombCharge++;
    }

    if (arduboy.notPressed(B_BUTTON) && (bombCharge > 1))
    {
      if (arduboy.everyXFrames(1))bombCharge--;
    }

    if (bombCharge > 119)
    {
      bombCharge = 1;
      playerBomb.isActive = true;
      bombs--;
      if (bombs < 0) bombs = 0;
    }
  */
  if (arduboy.justPressed(A_BUTTON) && !spaceShip.crashing)
  {
    spaceShip.weaponType++;
    if (spaceShip.weaponType > 2) spaceShip.weaponType = 0;
    //gameState = STATE_GAME_PAUSE;
  }

  if (arduboy.notPressed(UP_BUTTON) && arduboy.notPressed(DOWN_BUTTON) && !spaceShip.crashing)
  {
    if (arduboy.everyXFrames(12))
    {
      if (spaceShip.frame > 2)
        spaceShip.frame--;
      if (spaceShip.frame < 2)
        spaceShip.frame++;
    }
  }
  if (spaceShip.crashing) spaceShip.frame++;
}


#endif
