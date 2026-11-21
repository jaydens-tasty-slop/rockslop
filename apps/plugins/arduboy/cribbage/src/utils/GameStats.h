#pragma once

#include <stdint.h>
#include "Utils.h"
#include "../entities/Entities.h"

struct Message {
  String message;
  uint8_t lines;
  uint8_t width;
  uint8_t xPos;
  DealerFace dealerFace = DealerFace::Normal;
  BubbleAlignment alignment = BubbleAlignment::None;
  bool renderRequired = false;
};


class GameStats {

  public: 

    GameStats() { };

    Player player1;
    Player player2;
    Deck deck;

    WhichPlayer playerDealer = WhichPlayer::Player2;
    WhichPlayer playersTurn = WhichPlayer::Player1;
    Score scores[Constants::PlayerHandScores];


  uint8_t getNumberOfScores() {

    uint8_t count = 0;

    for (uint8_t i = 0; i < Constants::PlayerHandScores; i++) {
      if (scores[i].getCard(0) != Constants::NoCard) count++;
    }

    return count;
    
  }

  uint8_t getScoresTotal() {

    uint8_t total = 0;

    for (uint8_t i = 0; i < Constants::PlayerHandScores; i++) {
      if (scores[i].getCard(0) != Constants::NoCard) total = total + scores[i].getScore();
    }

    return total;
    
  }

  void resetGame() {

    player1.resetHand(true);
    player2.resetHand(true);

  }

};