#ifndef ENEMIES_H
#define ENEMIES_H

#include "Arglib.h"
#include "globals.h"

#define ENEMY_AQY_039               0
#define ENEMY_AQY_127               1
#define ENEMY_AQY_308               2
#define ENEMY_AQY_506               3
#define ENEMY_ZPV_039               4
#define ENEMY_ZPV_127               5
#define ENEMY_ZPV_308               6
#define ENEMY_ZPV_506               7
#define ENEMY_HAL_039               8
#define ENEMY_HAL_127               9
#define ENEMY_HAL_308               10
#define ENEMY_HAL_506               11
#define ENEMY_LQP_039               12
#define ENEMY_LQP_127               13
#define ENEMY_LQP_308               14
#define ENEMY_LQP_506               15

#define MAX_ONSCREEN_ENEMIES        8

struct Enemies
{
  public:
    int x;
    int y;
    int HP;
    boolean isActive = false;
    byte frame = 0;
    byte type = 0;
    byte front = 0;
    byte back = 0;
};

struct EndBoss
{
  public:
    int x = 64;
    byte y = 12;
    int HP;
    boolean isActive = false;

};

Enemies enemy[MAX_ONSCREEN_ENEMIES];
EndBoss endboss;


void resetEnemies()
{
  for (byte i = 0; i < MAX_ONSCREEN_ENEMIES; i++)
  {
    enemy[i].frame = i;
  }
}

void updateEnemies()
{
  if (arduboy.everyXFrames(6))
  {
    for (byte i = 0; i < MAX_ONSCREEN_ENEMIES; i++)
    {
      enemy[i].front = enemy[i].type / 4;
      enemy[i].back  = enemy[i].type % 4;
      enemy[i].frame++;
      if (enemy[i].frame > 7) enemy[i].frame = 0;
      if (enemy[i].x < -48) enemy[i].isActive = false;
      if (enemy[i].y < -48) enemy[i].isActive = false;
      if (enemy[i].y > 112) enemy[i].isActive = false;
    }
  }
}

void drawEnemies()
{
  for ( byte i = 0; i < MAX_ONSCREEN_ENEMIES; i++)
  {
    if (enemy[i].isActive)
    {
      sprites.drawSelfMasked(enemy[i].x, enemy[i].y, enemyFronts, (8*enemy[i].front) + enemy[i].frame);
      sprites.drawSelfMasked(enemy[i].x + 8, enemy[i].y, enemyBacks, (8*enemy[i].back) + enemy[i].frame);
    }
  }
}


#endif
