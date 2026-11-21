#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include "globals.h"

byte moveOnY[] = {0, 1, 2, 1};
byte moveOnYFrame = 0;
byte lineFrame = 0;

void titleScreen()
{
  sprites.drawSelfMasked(43, 4, racer, 0);
  sprites.drawSelfMasked(8, 9, pico, 0);
}

void titleRoad()
{
  if (arduboy.everyXFrames(10)) moveOnYFrame = (++moveOnYFrame) % 4;
  if (arduboy.everyXFrames(1)) lineFrame = (++lineFrame) % 32;
  for (byte i = 0; i < 5; i++) sprites.drawSelfMasked(8 + (33 * i) - lineFrame , 45, roadLine, 0);
  sprites.drawSelfMasked(23, 34, bigCarSmokeOne, moveOnYFrame);
  sprites.drawSelfMasked(23, 48, bigCarSmokeTwo, moveOnYFrame);
  for (byte i = 0; i < 6; i++)sprites.drawOverwrite(35 + (7 * i), 42, roadMask, 0);
  sprites.drawSelfMasked(36, 33 + moveOnY[moveOnYFrame], bigCar, 0);
  for (byte i = 0; i < 3; i++)sprites.drawOverwrite(104 + (8 * i), 42, roadMask, 0);
}

void stateMenuIntro()
{
  globalCounter++;
  sprites.drawSelfMasked(34, 4, T_arg, 0);
  if (globalCounter > 180)
  {
    globalCounter = 0;
    gameState = STATE_MENU_MAIN;
  }
}

void stateMenuMain()
{
  titleScreen();
  titleRoad();

  for (byte i = 0; i < 4; i++)
  {
    if (((2 + i) - menuSelection) != 0) sprites.drawSelfMasked(108, 16 + (9 * i), menuText, i);
    if (((2 + i) - menuSelection) == 0)
    {
      for (byte k = 0; k < 3; k++)sprites.drawOverwrite(108 + (8 * k), 13 + (9*i), roadMask, 0);
      sprites.drawSelfMasked(105, 15 + (9 * i), menuText, i);
      sprites.drawPlusMask(105, 15 + (9 * i), menuSmallOverlay_plus_mask, 0);
    }
  }

  if (arduboy.justPressed(DOWN_BUTTON) && (menuSelection < 5)) menuSelection++;
  if (arduboy.justPressed(UP_BUTTON) && (menuSelection > 2)) menuSelection--;
  if (arduboy.justPressed(B_BUTTON)) gameState = menuSelection;
}

void stateMenuHelp()
{
  if (arduboy.justPressed(A_BUTTON | B_BUTTON)) gameState = STATE_MENU_MAIN;
}


void stateMenuInfo()
{
  titleScreen();
  if (arduboy.justPressed(A_BUTTON | B_BUTTON)) gameState = STATE_MENU_MAIN;
}

void stateMenuSoundfx()
{
  titleScreen();
  titleRoad();

  sprites.drawSelfMasked(108, 25, menuText, 4);
  
  if (arduboy.audio.enabled())
  {
    sprites.drawSelfMasked(108, 34, menuText, 5);
    for (byte k = 0; k < 3; k++)sprites.drawOverwrite(108 + (8 * k), 40, roadMask, 0);
    sprites.drawSelfMasked(105, 42, menuText, 6);
    sprites.drawPlusMask(105, 42, menuSmallOverlay_plus_mask, 0);
  }
  else
  {
    sprites.drawSelfMasked(108, 43, menuText, 6);
    for (byte k = 0; k < 3; k++)sprites.drawOverwrite(108 + (8 * k), 31, roadMask, 0);
    sprites.drawSelfMasked(105, 33, menuText, 5);
    sprites.drawPlusMask(105, 33, menuSmallOverlay_plus_mask, 0);
  }
  
  if (arduboy.justPressed(DOWN_BUTTON)) arduboy.audio.on();
  if (arduboy.justPressed(UP_BUTTON)) arduboy.audio.off();
  if (arduboy.justPressed(A_BUTTON | B_BUTTON))
  {
    arduboy.audio.saveOnOff();
    gameState = STATE_MENU_MAIN;
  }
}

void stateMenuPlay()
{
  scorePlayer = 0;
  gameState = STATE_GAME_PREPARE_LEVEL;
}

#endif
