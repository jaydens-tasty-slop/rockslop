#include <stdlib.h>
#include <math.h>
/*
  PICO RACER: http://www.team-arg.org/race-manual.html

  Arduboy version 0.1:  http://www.team-arg.org/pirc-downloads.html

  MADE by TEAM a.r.g. : http://www.team-arg.org/more-about.html

  2016 - emuty - PixelArtM - JO3RI

  License: MIT : https://opensource.org/licenses/MIT

*/

//determine the game
#define GAME_ID 43

#include "Arglib.h"
#include "../lib/ab_math.h"
#include "globals.h"
#include "menu.h"
#include "game.h"
#include "inputs.h"
#include "cars.h"
#include "elements.h"
#include "tracks.h"

typedef void (*FunctionPointer) ();

const FunctionPointer PROGMEM mainGameLoop[] = {
  stateMenuIntro,
  stateMenuMain,
  stateMenuHelp,
  stateMenuPlay,
  stateMenuInfo,
  stateMenuSoundfx,
  stateGamePrepareLevel,
  stateGameNextLevel,
  stateGamePlaying,
  stateGamePause,
  stateGameOver,
};


void setup() {
  arduboy.start();
  arduboy.setFrameRate(60);                                 // set the frame rate of the game at 60 fps
}


void loop() {
  if (!(arduboy.nextFrame())) return;
  arduboy.poll();
  arduboy.clearDisplay();
  ((FunctionPointer) pgm_read_ptr (&mainGameLoop[gameState]))();
  arduboy.display();
}

