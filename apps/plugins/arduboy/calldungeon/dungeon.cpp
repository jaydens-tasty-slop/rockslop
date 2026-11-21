// 
// Call of Dungeon for Arduboy
// 

#include "Arduboy.h"
#include "game.h"

Arduboy mArduboy;
// built in setup(): the constructor needs the plugin API, which a global
// constructor runs too early for
Game *mGame;

void setup() {
  mGame = new Game;
  mArduboy.begin();
  mArduboy.initRandomSeed();
  mArduboy.setFrameRate(60);
}

void loop() {
  if (!(mArduboy.nextFrame())) {
    return;
  }

  mArduboy.clear();
  mGame->check_input();
  mGame->do_state();
  mGame->draw();
  mArduboy.display();
}

