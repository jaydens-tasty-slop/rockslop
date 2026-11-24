/*
 Star Flight: http://www.team-arg.org/SFL-manual.html

 Arduboy version 0.3:  http://www.team-arg.org/SFL-downloads.html

 MADE by TEAM a.r.g. : http://www.team-arg.org/About.html

 2016 - JO3RI - PIQUESEL

 Game License: MIT : https://opensource.org/licenses/MIT
*/

#include "Arglib.h"
#include "globals.h"
#include "menu.h"
#include "game.h"
#include "inputs.h"
#include "player.h"
#include "enemies.h"
#include "elements.h"
#include "levels.h"


//determine the game
#define GAME_ID 26

typedef void (*FunctionPointer) ();
const FunctionPointer PROGMEM  mainGameLoop[] = {
  stateMenuIntro,
  stateMenuMain,
  stateMenuHelp,
  stateMenuPlay,
  stateMenuInfo,
  stateMenuSoundfx,
  stateGameNextLevel,
  stateGamePlaying,
  stateGamePause,
  stateGameOver,
};

void setup()
{
  arduboy.start();
  arduboy.setFrameRate(60);
  Serial.begin(9600);
}

void loop() {
  if (!(arduboy.nextFrame())) return;
  arduboy.poll();
  arduboy.clearDisplay();
  ((FunctionPointer) pgm_read_ptr(&mainGameLoop[gameState]))();
  arduboy.display();
};
