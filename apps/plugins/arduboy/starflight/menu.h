#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include "globals.h"
#include "levels.h"


byte slideCount01 = 0;
byte slideCount02 = 0;

void drawTitleScreen()
{
  for (byte i = 0; i < 4; i++) sprites.drawSelfMasked(i * 32, 16, titleScreenSFL, i);
  sprites.drawSelfMasked(2, 8, gameTitleSFL, 0);
}

void setSlidersToZero()
{
  slideCount02 = 0;
  slideCount01 = 0;
}

void makeItSlide()
{
  slideCount01++;
  if (slideCount01 > 6)
  {
    slideCount02++;
    slideCount01 = 6;
  }

  if (slideCount02 > 3)
  {
    slideCount02 = 3;
  }
}

void stateMenuIntro()
{
  globalCounter++;
  for (byte i = 0; i < 4; i++) sprites.drawSelfMasked(32 * i, 10, TEAMarg, i);
  sprites.drawSelfMasked(43, 50, TEAM_argPart5, 0);
  if (globalCounter > 180)
  {
    globalCounter = 0;
    gameState = STATE_MENU_MAIN;
  }
}

void stateMenuMain()
{
  drawTitleScreen();
  for (byte i = 0; i < 4; i++)
  {
    {
      if (((2 + i) - menuSelection) != 0)
      {
        sprites.drawSelfMasked(9 + (28 * i), -9 + slideCount01, menuText, i);
      }
      if (((2 + i) - menuSelection) == 0) sprites.drawSelfMasked(9 + (28 * i), -9 + slideCount01 + slideCount02  , menuText, i);
    }
  }

  if (arduboy.justPressed(RIGHT_BUTTON) && (menuSelection < 5))
  {
    menuSelection++;
    slideCount02 = 0;
  }
  if (arduboy.justPressed(LEFT_BUTTON) && (menuSelection > 2))
  {
    menuSelection--;
    slideCount02 = 0;
  }
  if (arduboy.justPressed(A_BUTTON | B_BUTTON))
  {
    setSlidersToZero();
    gameState = menuSelection;
  }
  if (arduboy.everyXFrames(6)) makeItSlide();
}

void stateMenuHelp()
{
  if (arduboy.justPressed(A_BUTTON | B_BUTTON)) gameState = STATE_MENU_MAIN;
}

void stateMenuInfo()
{
  drawStarField();
  sprites.drawSelfMasked(2, 8, gameTitleSFL, 0);
  sprites.drawSelfMasked(16, 32, info_bitmap, 0);
  if (arduboy.justPressed(A_BUTTON | B_BUTTON)) gameState = STATE_MENU_MAIN;
}

void stateMenuSoundfx()
{
  drawTitleScreen();

  sprites.drawSelfMasked(36, -6 + slideCount01, menuText, 4);
  sprites.drawSelfMasked(57, -9 + slideCount01 + (slideCount02 * (1 - arduboy.audio.enabled())), menuText, 5);
  sprites.drawSelfMasked(78, -9 + slideCount01 + (slideCount02 * (arduboy.audio.enabled())) , menuText, 6);


  if (arduboy.justPressed(LEFT_BUTTON))
  {
    arduboy.audio.off();
    slideCount02 = 0;
  }
  if (arduboy.justPressed(RIGHT_BUTTON))
  {
    arduboy.audio.on();
    slideCount02 = 0;
  }
  if (arduboy.justPressed(A_BUTTON | B_BUTTON))
  {
    setSlidersToZero();
    arduboy.audio.saveOnOff();
    gameState = STATE_MENU_MAIN;
  }
  if (arduboy.everyXFrames(6)) makeItSlide();
}


#endif
