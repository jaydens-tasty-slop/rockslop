#ifndef INPUTS_H
#define INPUTS_H

#include <Arduino.h>
#include "globals.h"
#include "cars.h"
#include "road.h"
#include "elements.h"

void checkInputs()
{
  player.frame = carTypes[player.type] + 1;

  if (player.speed > 0)
  {
    if (arduboy.pressed(UP_BUTTON) && (player.y > GAME_TOP))
    {
      if (player.isMoveUp) {
        player.frame = carTypes[player.type];
        player.y = player.y - 1;
      }
    }
    if (arduboy.pressed(DOWN_BUTTON) && (player.y < GAME_BOTTOM))
    {
      if (player.isMoveDown) {
        player.frame = carTypes[player.type] + 2;
        player.y = player.y + 1;
      }
    }
  }

  if (arduboy.pressed(A_BUTTON))
  {
    if (player.speed < 6 && player.speed_cnt == 0 && !player.isClash)
    {
      player.speed_cnt = 20;

      if (!player.isSlow && !player.isSlowest)
      {
        player.speed++;
      }
      else
      {
        if (player.isSlowest)
        {
          player.speed = 1;
        }
        else if (player.isSlow)
        {
          player.speed = 2;
        }
      }
    }
  }

  if (arduboy.notPressed(A_BUTTON))
  {
    if (player.speed > 0 && player.speed_cnt == 0)
    {
      player.speed--;
      player.speed_cnt = 15;
    }
  }
/*
  if (arduboy.justPressed(B_BUTTON))
  {
    powerUp.type++;
    if (powerUp.type > AMOUNT_OF_POWER_UPS - 1) powerUp.type = 0;
  }
*/
  if (player.isSlowest)
  {
    if (player.speed > 1 && player.speed_cnt == 0)
    {
      player.speed--;
      player.speed_cnt = 40;
    }
  }
  else if (player.isSlow)
  {
    if (player.speed > 2 && player.speed_cnt == 0)
    {
      player.speed--;
      player.speed_cnt = 40;
    }
  }

  // Clash -> Effect
  if (player.isClash)
  {
    if (player.speed > 0)
    {
      player.speed = 0;
      player.speed_cnt = 25;
    }

    if (player.speed_cnt > 0)
    {
      if (player.speed_cnt % 2)
      {
        player.y = player.y + 2;
      }
      else
      {
        player.y = player.y - 2;
      }
    }
    else
    {
      player.isClash = false;
    }
  }

  if (player.speed_cnt > 0)
  {
    player.speed_cnt--;
  }

}

#endif
