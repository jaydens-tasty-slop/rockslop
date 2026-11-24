#pragma once

#include "../utils/GameContext.h"
#include "../utils/GameState.h"
#include "../utils/Enums.h"
#include "../fonts/Font3x5.h"

class BaseState : public GameState<GameContext, GameStateType> {

  protected:

    Font3x5 font3x5 = Font3x5();

    void drawScore(StateMachine & machine, uint8_t x, int8_t y, uint8_t score, bool renderText);
    void drawScores_TopLeft(StateMachine & machine, bool renderText_Player1, bool renderText_Player2);

    void drawMessageBox(StateMachine & machine, String message, uint8_t lines, uint8_t width, uint8_t xPos, BubbleAlignment alignment);
    void drawDealer(StateMachine & machine, int8_t xPos, uint8_t yPos, DealerFace dealerFace, Message message);

	private:
   
};