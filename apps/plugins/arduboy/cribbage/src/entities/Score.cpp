#include "Score.h"
#include "../utils/Constants.h"

Score::Score() {}

uint8_t Score::getCard(uint8_t index) {

  return this->hand[index];

}

uint8_t Score::getScore() {

  return this->score;

}

ScoreType Score::getType() {

  return this->type;

}

void Score::setHand(uint8_t index, uint8_t card) {

  this->hand[index] = card;

}

void Score::setScore(uint8_t score) {

  this->score = score;

}

void Score::setType(ScoreType type) {

  this->type = type;

}

void Score::reset() {

  memset(this->hand, Constants::NoCard, (sizeof(this->hand) / sizeof(this->hand[0])));
  this->score = 0;

}