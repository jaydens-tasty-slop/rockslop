#include "Player.h"

#include "../utils/Constants.h"
#include "../utils/CardUtils.h"
#include "../utils/Utils.h"
#include "../utils/Enums.h"

Player::Player() {

  for (uint8_t x = 0; x < 13; x++) {

    this->hand[x] = Constants::NoCard;
    this->prev[x] = Constants::NoCard;
    
  }

  this->handIdx = 0;
  this->prevIdx = 0;

}

uint8_t Player::getScore() {

  return this->score;

}

uint8_t Player::getOverallScore() {

  return this->overallScore;

}

void Player::setScore(uint8_t score) {

  this->score = score;

}

void Player::setOverallScore(uint8_t score) {

  this->overallScore = score;

}

void Player::incScore() {

  this->score++;

}

void Player::decScore() {

  this->score--;

}

void Player::incOverallScore() {

  this->overallScore++;

}

uint8_t Player::getCard(uint8_t index) {

  return this->hand[index];

}

void Player::setCard(uint8_t index, uint8_t card) {

  this->hand[index] = card;

}

uint8_t Player::getCardCount() {

  return this->handIdx;

}

uint8_t Player::getCardIndex(uint8_t card) {

  for (uint8_t x = 0; x < 13; x++) {

    if (this->hand[x] == card) return x;
  
  }

 return Constants::NoCard;

}


void Player::addToHand(uint8_t card) {
    
  this->hand[this->handIdx] = card;
  this->handIdx++;

  CardUtils::sort(this->hand, this->handIdx);

}


void Player::addToPrev(uint8_t card) {
    
  this->prev[this->prevIdx] = card;
  this->prevIdx++;

  CardUtils::sort(this->prev, this->prevIdx);

}


uint8_t Player::removeFromHand(uint8_t index) {

  uint8_t card = this->hand[index];

  for (uint8_t x = index; x < 12; x++) {

    this->hand[x] = this->hand[x + 1];

  }

  this->hand[12] = Constants::NoCard;
  this->handIdx--;

  return card;

}


void Player::resetHand(bool clearScores) {

  this->handIdx = 0;
  this->prevIdx = 0;
  this->score = 0;
  
  memset(this->hand, Constants::NoCard, (sizeof(this->hand) / sizeof(this->hand[0])));
  memset(this->prev, Constants::NoCard, (sizeof(this->hand) / sizeof(this->hand[0])));
  memset(this->rank, Constants::NoCard, (sizeof(this->hand) / sizeof(this->hand[0])));
  memset(this->suitsOpponent, 0, (sizeof(this->suitsOpponent) / sizeof(this->suitsOpponent[0])));

  if (clearScores) {
    this->overallScore = 0;
  }

}


void Player::setSuitOpponent(Suit suit) {

  this->suitsOpponent[static_cast<uint8_t>(suit)] = true;

}


void Player::printHand(uint8_t playerNo) {

  #ifdef DEBUG_PRINT_HANDS

  Serial.print("Player ");
  Serial.print(playerNo);
  Serial.print(": ");

  for (uint8_t x=0; x < this->handIdx; x++) {
    
    Serial.print("(");
    Serial.print(this->hand[x]);
    Serial.print(") ");
    CardUtils::printCard(this->hand[x]);
    Serial.print(" ");

  }

  Serial.println("");
  #endif
  
}


uint8_t Player::playCard_PhaseOne(Suit trumps, uint8_t exposedCard, uint8_t opponentsCard) {
 
  uint8_t exposedCardVal = CardUtils::getCardValue(exposedCard);
  Suit exposedCardSuit = CardUtils::getCardSuit(exposedCard);
 
  uint8_t opponentsCardVal = Constants::NoCard;
  Suit opponentsCardSuit = Suit::Clubs;
 
  if (opponentsCard != Constants::NoCard) {
 
    opponentsCardVal = CardUtils::getCardValue(opponentsCard);
    opponentsCardSuit = CardUtils::getCardSuit(opponentsCard);
 
  }
 

  // How desirable is this card ?
 
  uint8_t desirability = exposedCardVal + (exposedCardSuit == trumps ? 6 : 0);
 
 
  // If we have played a higher card previously then the score should also be raised ..

  for (uint8_t x = 0; x < this->prevIdx; x++) {
 
    if ((CardUtils::getCardValue(this->prev[x]) > exposedCardVal) &&
        (CardUtils::getCardSuit(this->prev[x]) == exposedCardSuit)) {
 
      desirability++;
       
    }
       
  }


  uint8_t losingCard = Constants::NoCard;
  uint8_t winningCard = Constants::NoCard;
 

  // Are we leading?
 
  if (opponentsCard == Constants::NoCard) {


    // Select an appropriate card ..
 
    switch (desirability) {
 
      case 0 ... 7:
        return getCardByRank_LowestInRange(0, 255);
 
      case 8 ... 13:
        {
          uint8_t selectedCard = getCardByRank_HighestInRange(4, 9);

          // If we couldn't find an appropriate card, then look for a losing card ..
 
          if (selectedCard == Constants::NoCard)    selectedCard = getCardByRank_HighestInRange(0, 4);
 

          // If we couldn't find an appropriate card, then look for a losing card ..
 
          if (selectedCard == Constants::NoCard)    selectedCard = getCardByRank_LowestInRange(9, 255);
          return selectedCard;
 
        }
        break;
 
      case 14 ... 255:
        return getCardByRank_HighestInRange(0, 255);
 
    }
 
  }
  else {
 

    // Can we play this suit?  Work out the highest losing card and the lowest winning card we can play ..
 
    for (uint8_t x = 0; x < this->handIdx; x++) {
 
      if ((CardUtils::getCardValue(hand[x]) < opponentsCardVal) &&
          (CardUtils::getCardSuit(hand[x]) == opponentsCardSuit)) {
 
        if (losingCard == Constants::NoCard || (CardUtils::getCardValue(losingCard) > CardUtils::getCardValue(hand[x]))) {
          losingCard = hand[x];
        }
         
      }
 
      if ((CardUtils::getCardValue(hand[x]) > opponentsCardVal) &&
          (CardUtils::getCardSuit(hand[x]) == opponentsCardSuit)) {
 
        if (winningCard == Constants::NoCard || (CardUtils::getCardValue(winningCard) > CardUtils::getCardValue(hand[x]))) {
          winningCard = hand[x];
        }
         
      }
         
    }
 

    // If we can play the suit then return a card ..

    if (desirability > 16 && winningCard != Constants::NoCard)    return winningCard;
    if (desirability <= 6 && losingCard  != Constants::NoCard)    return losingCard;
 
    if (desirability > 14 && winningCard != Constants::NoCard)    return winningCard;
    if (desirability <= 8 && losingCard  != Constants::NoCard)    return losingCard;
 
    if (desirability > 12 && winningCard != Constants::NoCard)    return winningCard;
    if (desirability < 10 && losingCard  != Constants::NoCard)    return losingCard;
 
    if (losingCard  != Constants::NoCard)                         return losingCard;
    if (winningCard != Constants::NoCard)                         return winningCard;



    // If we could not follow suit, should we trump it?
 
    if (trumps != opponentsCardSuit && desirability > 7) {
     
      uint8_t cardToPlay = Constants::NoCard;
 
      for (uint8_t x = 0; x < this->handIdx; x++) {
 
        if (CardUtils::getCardSuit(hand[x]) == trumps) {
 
          if (cardToPlay == Constants::NoCard || (CardUtils::getCardValue(cardToPlay) > CardUtils::getCardValue(hand[x]))) {
            cardToPlay = hand[x];
          }
           
        }
           
      }
 
      if (cardToPlay != Constants::NoCard) {
        return cardToPlay;
      }
 
    }
 

    // If we haven't played an 'on-suit' card or trumped the card then play a random card ..
 
    {
      uint8_t lowestCard = Constants::NoCard;
 
      for (uint8_t x = 0; x < this->handIdx; x++) {
 
        if (lowestCard == Constants::NoCard || (CardUtils::getCardValue(hand[x]) < CardUtils::getCardValue(lowestCard))) {
 
          lowestCard = hand[x];
 
        }
 
      }
 
      return lowestCard;
 
    }
 
  }

  return Constants::NoCard;
 
}
 

uint8_t Player::playCard_PhaseTwo(Suit trumps, uint8_t opponentsCard) {
 
  uint8_t opponentsCardVal = Constants::NoCard;
  Suit opponentsCardSuit;
 
  if (opponentsCard != Constants::NoCard) {
 
    opponentsCardVal = CardUtils::getCardValue(opponentsCard);
    opponentsCardSuit = CardUtils::getCardSuit(opponentsCard);
 
  }
  

  uint8_t losingCard = Constants::NoCard;
  uint8_t winningCard = Constants::NoCard;


  // Are we leading?  If so, simply lead the best cards first.
 
  if (opponentsCard == Constants::NoCard) {
 
    uint8_t card = Constants::NoCard;
    uint8_t ranking = 0;

    this->rankCards(trumps, Constants::NoCard, true);
 
    for (uint8_t x = 0; x < this->handIdx; x++) {
 
      if (this->rank[x] >= ranking) {
        card = hand[x];
        ranking = rank[x];
      }

    }

    return card;
 
  }
  else {
   
 
    // Can we play this suit?  Work out the lowest losing card and the lowest winning card we can play ..
 
    for (uint8_t x = 0; x < this->handIdx; x++) {
 
      if ((CardUtils::getCardValue(hand[x]) < opponentsCardVal) &&
          (CardUtils::getCardSuit(hand[x]) == opponentsCardSuit)) {
 
        if (losingCard == Constants::NoCard || (CardUtils::getCardValue(losingCard) > CardUtils::getCardValue(hand[x]))) {
          losingCard = hand[x];
        }
         
      }
 
      if ((CardUtils::getCardValue(hand[x]) > opponentsCardVal) &&
          (CardUtils::getCardSuit(hand[x]) == opponentsCardSuit)) {
 
        if (winningCard == Constants::NoCard || (CardUtils::getCardValue(winningCard) > CardUtils::getCardValue(hand[x]))) {
          winningCard = hand[x];
        }
         
      }
         
    }


    // If we can play the suit then return a card ..
 
    if (winningCard != Constants::NoCard)        return winningCard;
    if (losingCard  != Constants::NoCard)        return losingCard;



    // If we could not follow suit, should we trump it?
 
    if (trumps != opponentsCardSuit) {
       
      uint8_t cardToPlay = Constants::NoCard;
 
      for (uint8_t x = 0; x < this->handIdx; x++) {
 
        if (CardUtils::getCardSuit(hand[x]) == trumps) {
 
          if (cardToPlay == Constants::NoCard || (CardUtils::getCardValue(cardToPlay) < CardUtils::getCardValue(hand[x]))) {
            cardToPlay = hand[x];
          }
           
        }
           
      }
 
      if (cardToPlay != Constants::NoCard) {
        return cardToPlay;
      }
 
    }
 

    // If we haven't played an 'on-suit' card or trumped the card then play a random card ..
    {
      uint8_t randomCard = Constants::NoCard;
 
      for (uint8_t x = 0; x < this->handIdx; x++) {
 
        if (randomCard == Constants::NoCard || (CardUtils::getCardValue(hand[x]) < CardUtils::getCardValue(randomCard))) {
 
          randomCard = hand[x];
 
        }
 
      }
 
      return randomCard;
 
    }
 
  }
 
  return Constants::NoCard;

}
 

uint8_t Player::getCardByRank_LowestInRange(uint8_t low, uint8_t high) {
 
  uint8_t selectedRank = 255;
  uint8_t selectedCard = Constants::NoCard;
 
  for (uint8_t x = 0; x < this->handIdx; x++) {
 
    if (this->rank[x] >= low && this->rank[x] <= high && this->rank[x] <= selectedRank) {
      selectedRank = this->rank[x];
      selectedCard = this->hand[x];
    }
 
  }
 
  return selectedCard;
 
}
 
 
uint8_t Player::getCardByRank_HighestInRange(uint8_t low, uint8_t high) {
 
  uint8_t selectedRank = 0;
  uint8_t selectedCard = Constants::NoCard;
 
  for (uint8_t x = 0; x < this->handIdx; x++) {
 
    if (this->rank[x] >= low && this->rank[x] <= high && this->rank[x] >= selectedRank) {
      selectedRank = this->rank[x];
      selectedCard = this->hand[x];
    }
 
  }
 
  return selectedCard;
 
}
 

void Player::rankCards(Suit trumps, uint8_t exposedCard, bool includeSuitsOpponent) {


  // Rank the cards ..
 
  for (uint8_t x = 0; x < this->handIdx; x++) {

    uint8_t rankVal = CardUtils::getCardValue(hand[x]);


    // Automatically bump aces up ..

    if (rankVal == 14) { 

      rankVal = rankVal + 14; 

    }
    else {


      // If all higher cards of the same suit have been played then the card should be bumped up ..

      bool upScore = true;

      for (uint8_t y = rankVal + 1; y < 15; y++) {

        bool found = false;

        for (uint8_t z = 0; z < this->prevIdx; z++) {
        
          if (CardUtils::getCardValue(this->prev[z]) == y && 
              CardUtils::getCardSuit(this->prev[z]) == CardUtils::getCardSuit(this->hand[x])) {
          
            found = true;

          }

        }

        if (!found) {

          upScore = false;
          break;
        
        }

      }

      if (upScore) rankVal = rankVal + (14 - rankVal);

    }


    // If the exposed card is a trump card then downplay other trumps ..

    if (exposedCard != Constants::NoCard) {

      if (CardUtils::getCardSuit(exposedCard) == trumps && CardUtils::getCardSuit(this->hand[x]) == trumps) {

        if (rankVal > 4) {
          rankVal = rankVal - 4;
        }
        else {
          rankVal = 0;
        }

      }

    }


    // Check to see if an opponent is short suited in this suit ..

    if (includeSuitsOpponent && !this->suitsOpponent[static_cast<uint8_t>(trumps)]) {

      Suit cardSuit = CardUtils::getCardSuit(this->hand[x]);

      if (this->suitsOpponent[static_cast<uint8_t>(cardSuit)]) {
        rankVal = 0;
      }

    }


    // Update array with final result ..

    this->rank[x] = rankVal;

  }
 
}

