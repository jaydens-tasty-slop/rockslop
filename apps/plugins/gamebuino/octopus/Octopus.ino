// -------------------------------------------------------------
// Octopus Game & Watch for META
// version make by Jean-Charles LEBEAU
// 01/03/2020 
// -------------------------------------------------------------
//
// Special thanks:
//
// To Roby for his Arduboy version: 
// https://github.com/rferrari81/Arduboy-Octopus
// To Steph for the structure of the META HD G&W games
// To Pharap for all the help that he done for all c++ 
// learners from Arduboy forum (https://community.arduboy.com/)
// ------------------------------------------------------------


// includes the official library
#include <Gamebuino-Meta.h>

// includes other parts of pogram

#include "Global.h"
#include "HD_Tools.h"
#include "HomeState.h"
#include "RunState.h"
#include "PauseState.h"
#include "GameOverState.h"
#include "pictures.h"
#include "Sprites.h"


// -------------------------------------------------------------------------
// Initialization of the META
// -------------------------------------------------------------------------

void setup() {
  // initializes the META
  gb.begin();
  if (debug == true) SerialUSB.begin(9600);
  // default screen buffer won't be used
  // so it must be set to 0x0 pixels
  gb.display.init(0, 0, ColorMode::rgb565);
  gameState = GameState::home;
}


// -------------------------------------------------------------------------
// Main control loop
// -------------------------------------------------------------------------

void loop() {

 
  gb.waitForUpdate();

  switch (gameState) {

    case GameState::home: // Start screen
      homeState.update();
      homeState.draw();
      break;

    case GameState::run: // Game running
      runState.update();
      runState.draw();
      break;

    case GameState::gameOver:  // GameOver screen
      gameOverState.update();
      gameOverState.draw();
      break;

    case GameState::pauseScreen: // pauseScreen Game
      pauseState.update();
      pauseState.draw();
      break;

  }
}
