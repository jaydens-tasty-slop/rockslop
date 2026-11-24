#pragma once

#include "../utils/GameContext.h"
#include "../utils/GameState.h"
#include "../utils/Enums.h"
#include "../fonts/Font3x5.h"

class BaseState : public GameState<GameContext, GameStateType> {

  protected:

    Font3x5 font3x5 = Font3x5();

    void drawMessageBox(StateMachine & machine, String message, uint8_t lines, uint8_t width, BubbleAlignment alignment);
    void drawDealer(StateMachine & machine, int8_t xPos, uint8_t yPos, DealerFace dealerFace, Message message);

	private:
   
};