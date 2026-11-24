#pragma once

#include <Arduboy2.h>
#include "../utils/Enums.h"

class Score { 
    
  public:

    Score();

    uint8_t getCard(uint8_t index);
    uint8_t getScore();
    ScoreType getType();

    void setHand(uint8_t index, uint8_t card);
    void setScore(uint8_t score);
    void setType(ScoreType score);

    void reset();

  private:

    uint8_t hand[5];
    uint8_t score;
    ScoreType type;

};
