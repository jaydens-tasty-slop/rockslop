#pragma once

#include "../utils/GameContext.h"
#include "../utils/GameState.h"
#include "../utils/Enums.h"
#include "../images/Images.h"
#include "BaseState.h"

class GameOverState : public BaseState {

  private:

    uint8_t winner = 0;
    uint8_t r = 0;
    uint8_t g = 12;
    uint8_t b = 24;
    
  public:	
  
    void activate(StateMachine & machine) override;
    void update(StateMachine & machine) override;
    void render(StateMachine & machine) override;

};

