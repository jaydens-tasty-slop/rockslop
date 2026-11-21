#pragma once

#include "utils/Utils.h"
#include "utils/GameCookie.h"
#include "states/States.h"

class Game {

  private:

    TitleScreenState titleScreenState;
    SplashScreenState splashScreenState;
    PlayGameState playGameState;
    HighScoreState highScoreState;
    
    GameCookie *cookie;
    GameStateType currentState;

  public:

    bool inPlay() const { return currentState == GameStateType::PlayGame; }

    void setup(GameCookie *cookie);
    void loop(void);

};