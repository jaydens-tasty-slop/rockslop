#pragma once

#include <Arduboy2.h>
#include "../utils/Constants.h"

class Deck { 
    
  public:

    Deck();

    uint8_t getCard();
    void shuffle();

  private:

    uint8_t index;
    uint8_t deck[Constants::NumberOfCards];

};
