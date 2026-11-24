/*
 Trench Run
 Copyright (C) 2019 Lucas Cardinali
 All rights reserved.
*/

#include "global.h"
#include "assets.h"
#include "intro.h"
#include "input.h"
#include "menu.h"
#include "game.h"

typedef void (*FunctionPointer) ();

const FunctionPointer PROGMEM mainGameLoop[] = {
  stateIntroText,
  stateIntroCrawl,
  stateMenu,
  stateWarmup,
  stateGame,
  statePause,
  stateDeath,
  stateWin,
  stateLose,
};

void setup() {
  ab.begin();
  ab.initRandomSeed();
  ab.setFrameRate(FPS);
}

void loop() {
  if (!(ab.nextFrame())) return;
  ab.pollButtons();
  ((FunctionPointer) pgm_read_ptr(&mainGameLoop[gameState]))();
  checkInput();
  ab.display(true);
}
