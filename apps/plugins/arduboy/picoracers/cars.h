#ifndef PLAYER_H
#define PLAYER_H

#include <Arduino.h>
#include "globals.h"

#define CAR_TYPE_ONE                 0
#define CAR_TYPE_TWO                 3
#define CAR_TYPE_THREE               6
#define CAR_TYPE_FOUR                9
#define CAR_TYPE_FIVE                12
#define CAR_TYPE_SIX                 15

#define CAR_WIDTH                    16
#define CAR_HEIGHT                   8

byte carTypes[] = {CAR_TYPE_ONE, CAR_TYPE_TWO, CAR_TYPE_THREE, CAR_TYPE_FOUR, CAR_TYPE_FIVE, CAR_TYPE_SIX};

byte playerFrame = 0;

struct Players
{
  public:
    int x;
    int y;
    byte type;
    byte frame;
    byte life;
    byte speed;
    byte speed_cnt;
    boolean isMoveUp;
    boolean isMoveDown;
    boolean isSlow;
    boolean isSlowest;
    boolean isClash;

    void set()
    {
      x = 16;
      y = 28;
      type = 0;
      frame = 0;
      life = 3;
      speed = 0;
      speed_cnt = 0;
      isMoveUp = true;
      isMoveDown = true;
      isSlow = false;
      isSlowest = false;
      isClash = false;
    }
};

Players player;

void drawPlayer()
{
  sprites.drawPlusMask(player.x, player.y, cars_plus_mask, player.frame);
}

struct Enemys
{
  public:
    int x;
    int y;
    byte move_cnt;
    boolean isMoveUp;
    boolean isMoveDown;

    void set()
    {
      x = 128;
      y = 28;
      isMoveUp = true;
      isMoveDown = false;
      move_cnt = 0;
    }
};

Enemys enemy;

void drawEnemy()
{
  //test
  sprites.drawPlusMask(enemy.x, enemy.y, cars_plus_mask, carTypes[ CAR_TYPE_TWO ] + 1);

  if (enemy.y < 20)
  {
    enemy.isMoveUp = false;
    enemy.isMoveDown = true;
  }
  if (enemy.y > 44)
  {
    enemy.isMoveUp = true;
    enemy.isMoveDown = false;
  }

  if (enemy.isMoveUp && enemy.move_cnt == 0)
  {
    enemy.y--;
    enemy.move_cnt = 3;
  }
  if (enemy.isMoveDown && enemy.move_cnt == 0)
  {
    enemy.y++;
    enemy.move_cnt = 3;
  }

  if (enemy.move_cnt > 0)
  {
    enemy.move_cnt--;
  }
}

#endif
