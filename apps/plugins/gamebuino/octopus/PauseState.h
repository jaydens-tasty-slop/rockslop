#ifndef OCTOPUS_PAUSESTATE
#define OCTOPUS_PAUSESTATE


// -------------------------------------------------------------------------
// Pause state
// -------------------------------------------------------------------------

class PauseState {

  public:

    void update()
    {
      if ((gb.buttons.released(BUTTON_A)) || (gb.buttons.released(BUTTON_B))) {
        gameState = GameState::run;
        gb.sound.play("pauseScreen.wav");
      } else {
        PauseState::draw();
      }
    }


    // -------------------------------------------------------------------------
    // Draw Game Over screen
    // -------------------------------------------------------------------------

    void draw()    {

      drawBackground(PIC_BACKGROUND, spritePauseScreen, 66, 60, true);

    }
};

PauseState pauseState;

#endif
