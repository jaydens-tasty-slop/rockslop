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


  void resetGame() {

    player1.resetHand(true);
    player2.resetHand(true);

  }

};