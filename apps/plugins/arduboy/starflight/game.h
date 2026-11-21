#ifndef GAME_H
#define GAME_H

#include <Arduino.h>
#include "globals.h"
#include "inputs.h"
#include "player.h"
#include "enemies.h"
#include "elements.h"
#include "levels.h"


void stateMenuPlay()
{
  level = LEVEL_TO_START_WITH - 1;
  scorePlayer = 0;
  resetEnemies();
  resetBullets();
  spaceShip.reset();
  gameState = STATE_GAME_NEXT_LEVEL;
}



void stateGameNextLevel()
{
  spaceShip.set();
  currentWave = 0;
  previousWave = 255;
  level++;
  if (level > TOTAL_AMOUNT_OF_LEVELS) gameState = STATE_MENU_INFO;
  else gameState = STATE_GAME_PLAYING;
};



void stateGamePlaying()
{
  
  checkInputs();
  checkIfShipIsImune();
  updateEnemies();
  updateBullets();
  if (arduboy.everyXFrames(2))((FunctionPointer) pgm_read_ptr(&Levels[level - 1][currentWave]))();
  if (checkEndLevel()) gameState = STATE_GAME_NEXT_LEVEL;
  
  drawStarField();
  drawEnemies();
  drawBullets();
  drawSpaceShip();

  drawLifesHUD();
  drawBombsHUD();
  drawScore(SCORE_SMALL_FONT);
  
}

void stateGamePause()
{
  if (arduboy.justPressed(A_BUTTON | B_BUTTON)) gameState = STATE_GAME_PLAYING;
}

void stateGameOver()
{
  if (arduboy.justPressed(A_BUTTON | B_BUTTON)) gameState = STATE_MENU_MAIN;
}

#endif
