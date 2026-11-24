#ifndef OCTOPUS_HOMESTATE
#define OCTOPUS_HOMESTATE

#include "Global.h"
#include "Sprites.h"

// -------------------------------------------------------------------------
// Home state
// -------------------------------------------------------------------------

class HomeState {

  public:

    void update()
    {
      if ((gb.buttons.released(BUTTON_A)) || (gb.buttons.released(BUTTON_B))) {
        HomeState::initGame();
      } else {
        HomeState::draw();
      }
    }

    // -------------------------------------------------------------------------
    // Initialization of the game
    // -------------------------------------------------------------------------

    void initGame() {

      gb.lights.fill(BLACK);
      gameState = GameState::run;
      gb.sound.playTick();
      animFrame = 0;
      catchGold = false;
      inGold = false;
      score = 0;
      getPoint = 0;
      life = 3;
      bool hited = false;
      subGold = false;
      subMove = 0;
      barcaState = 0;
      moveSubButton = true;
      dieFrame = 0;
      endGame = false;
      hited = false;
    }

    // -------------------------------------------------------------------------
    // Draw Titlescreen
    // -------------------------------------------------------------------------

    void draw()    {

      drawBackground(PIC_SPLASH, spriteGameOver, 0, 0, false);

    }
};

HomeState homeState;

#endif
