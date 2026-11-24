#pragma once

#include <Arduboy2.h>
#include "../utils/Constants.h"
#include "Score.h"

class Player { 

  public:

    Player();

    uint8_t getScore();
    uint8_t getPrevScore();
    bool getGo();

    void setScore(uint8_t score);
    void setPrevScore(uint8_t score);
    void setGo(bool value);

    void addScore(uint8_t score);
    void addPrevScore(uint8_t score);

    uint8_t getHandCard(uint8_t index);
    uint8_t getHandCardCount();
    uint8_t getHandCardIndex(uint8_t card);
    uint8_t getOrigCard(uint8_t index);

    uint8_t getCribCard(uint8_t index);
    uint8_t getCribCardCount();

    void setHandCard(uint8_t index, uint8_t card);
    void setCribCard(uint8_t index, uint8_t card);

    void calculateHandScore(Score scores[], uint8_t turnUp);
    void calculateCribScore(Score scores[], uint8_t turnUp);

    void discardToCrib(uint8_t &card1, uint8_t &card2);
    void addToHand(uint8_t card);
    void addToCrib(uint8_t card);

    void playCard(uint8_t playedCards[], bool canOpponentPlay, uint8_t &card, uint8_t &points);

    uint8_t removeFromHand(uint8_t index);
    void resetHand(bool clearScores);
    void printHand(uint8_t playerNo);
    void printCrib(uint8_t playerNo);
    bool canPlay(uint8_t playedCards[]);

  private:

    void calculateScores(Score scores[], uint8_t calcScore[], uint8_t turnUp);

  private:

    uint8_t handIdx;
    uint8_t cribIdx;

    uint8_t hand[6];
    uint8_t orig[4];
    uint8_t crib[4];

    uint8_t score = 0;
    uint8_t prevScore = 0;
    bool go = false;
    
};
