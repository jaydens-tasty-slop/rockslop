#pragma once

#include "../utils/Arduboy2Ext.h"
#include "Enums.h"

class CardUtils { 
    
  public:

    CardUtils();

    static uint8_t getCardValue(uint8_t cardNumber, bool pictureCardsAsTens);
    static Suit getCardSuit(uint8_t cardNumber);
    static uint16_t getCardSort(uint8_t cardNumber);
    static void printCard(uint8_t cardNumber);

    static void sort(uint8_t arr[], uint8_t n);

  private:

    static void swap(uint8_t *xp, uint8_t *yp);

};
