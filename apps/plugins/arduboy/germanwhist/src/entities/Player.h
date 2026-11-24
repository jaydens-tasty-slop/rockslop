#pragma once

#include <Arduboy2.h>
#include "../utils/Constants.h"
#include "../utils/Enums.h"

class Player { 

  public:

    Player();

    uint8_t getScore();
    uint8_t getOverallScore();
    uint8_t getCard(uint8_t index);
    uint8_t getCardCount();
    uint8_t getCardIndex(uint8_t card);

    void setScore(uint8_t score);
    void setOverallScore(uint8_t score);
    void setSuitOpponent(Suit suit);

    void incScore();
    void decScore();
    void incOverallScore();


    void setCard(uint8_t index, uint8_t card);
    void addToHand(uint8_t card);
    void addToPrev(uint8_t card);

    uint8_t removeFromHand(uint8_t index);
    void resetHand(bool clearScores);
    void printHand(uint8_t playerNo);

    uint8_t playCard_PhaseOne(Suit trumps, uint8_t exposedCard, uint8_t opponentsCard);
    uint8_t playCard_PhaseTwo(Suit trumps, uint8_t opponentsCard);

  private:

    uint8_t getCardByRank_LowestInRange(uint8_t low, uint8_t high);
    uint8_t getCardByRank_HighestInRange(uint8_t low, uint8_t high);

    void rankCards(Suit trumps, uint8_t exposedCard, bool includeSuitsOpponent);


  private:

    uint8_t score;
    uint8_t overallScore;
    uint8_t handIdx = 0;
    uint8_t prevIdx = 0;
    uint8_t hand[13];
    uint8_t prev[13];
    uint8_t rank[13];
    bool suitsOpponent[4];
    
};
