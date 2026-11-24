#pragma once

#include "../utils/GameContext.h"
#include "../utils/GameState.h"
#include "../utils/Enums.h"
#include "../fonts/Font3x5.h"
#include "../fonts/Font4x6.h"

class BaseState : public GameState<GameContext, GameStateType> {

  protected:

    Font3x5 font3x5 = Font3x5();
    Font4x6 font4x6 = Font4x6();

    void renderScore(StateMachine & machine, int16_t score, uint8_t x, uint8_t y);
    void renderMovingDice(StateMachine & machine, uint8_t y);
    uint8_t getWinner(StateMachine & machine);
    void printName3x5(StateMachine & machine, uint8_t idx);
    void printName4x6(StateMachine & machine, uint8_t idx);


	private:

    int8_t xPos = -96;
    
};