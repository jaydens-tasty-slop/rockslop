#pragma once

#include "../utils/GameContext.h"
#include "../utils/GameState.h"
#include "../utils/Enums.h"
#include "../images/Images.h"
#include "BaseState.h"

class PlayGameState : public BaseState {

  private:

    enum class ViewState : uint8_t {
      PhaseOne_Start,
      DealCards,
      TurnUp,
      PhaseOne_PlayersTurn,
      PhaseOne_ComputersTurn,
      PhaseOne_EndOfTrick,
      PhaseTwo_Start,
      PhaseTwo_PlayersTurn,
      PhaseTwo_ComputersTurn,
      PhaseTwo_EndOfTrick,
      EndOfGame
    };

    ViewState viewState = ViewState::PhaseOne_Start;
    Message message;

  public:	
  
    void activate(StateMachine & machine) override;
    void update(StateMachine & machine) override;
    void render(StateMachine & machine) override;

  private:

    void drawPlayerHands(StateMachine & machine);
    void drawCard(int8_t xPos, uint8_t yPos, uint8_t card, bool leftAlign);
    void drawTurnUp(int8_t xPos, uint8_t yPos, uint8_t card);
    void drawComputerCard(uint8_t xPos, uint8_t yPos, bool fullSizeCard);
    void drawHighlight(StateMachine & machine, uint8_t hghlightCard);
    void drawTableTop(StateMachine & machine);
    void drawTrick(StateMachine & machine);
    void drawPhaseOne_Scores(StateMachine & machine);
    void drawPhaseTwo_Scores(StateMachine & machine);
    void drawNumber(StateMachine & machine, uint8_t x, uint8_t y, uint8_t number);

    void resetHand(StateMachine & machine);
    void resetPlay(StateMachine & machine);
    bool isEndOfGame(StateMachine & machine);
    bool isValidPlay(StateMachine & machine);
    WhichPlayer whoWonTheTrick(StateMachine & machine);

    void saveMessage(String message, uint8_t lines, DealerFace dealerFace, BubbleAlignment alignment);
    void saveMessage(String message, uint8_t lines, uint8_t width, DealerFace dealerFace, BubbleAlignment alignment);
    void skipSequence(StateMachine & machine, uint8_t counter);

  private:

    uint8_t handNo = 0;
    uint8_t counter = 0;
    uint8_t highlightCard = 0;
    uint8_t player1HighlightCard = Constants::NoCard;
    uint8_t turnUp = Constants::NoCard;
    uint8_t player1Card = Constants::NoCard;
    uint8_t player2Card = Constants::NoCard;
    uint8_t player1Scored = 0;
    uint8_t player2Scored = 0;
    Suit trumps = Suit::Nothing;
    bool firstHand = true;

};




