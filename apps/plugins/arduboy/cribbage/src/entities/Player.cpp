#include "Player.h"

#include "../utils/Constants.h"
#include "../utils/CardUtils.h"
#include "../utils/Utils.h"

Player::Player() {}

uint8_t Player::getScore() {

  return this->score;

}

uint8_t Player::getPrevScore() {

  return this->prevScore;

}

bool Player::getGo() {

  return this->go;

}

void Player::setScore(uint8_t score) {

  this->score = score;

}

void Player::setPrevScore(uint8_t score) {

  this->prevScore = score;

}

void Player::setGo(bool value) {

  this->go = value;

}

void Player::addScore(uint8_t score) {

  this->score += score;
  if (this->score > 121) this->score = 121;

}

void Player::addPrevScore(uint8_t score) {

  this->prevScore += score;

}

uint8_t Player::getHandCard(uint8_t index) {

  return this->hand[index];

}

void Player::setHandCard(uint8_t index, uint8_t card) {

  this->hand[index] = card;
  this->orig[index] = card; //SJH < testing only

}

uint8_t Player::getHandCardCount() {

  return this->handIdx;

}

uint8_t Player::getHandCardIndex(uint8_t card) {

  for (uint8_t x = 0; x < 6; x++) {

    if (this->hand[x] == card) return x;
  
  }

 return Constants::NoCard;

}

uint8_t Player::getCribCard(uint8_t index) {

  return this->crib[index];

}

void Player::setCribCard(uint8_t index, uint8_t card) {

  this->crib[index] = card;

}

uint8_t Player::getCribCardCount() {

  return this->cribIdx;

}

uint8_t Player::getOrigCard(uint8_t index) {

  return this->orig[index];

}

// ---------------------------------------------------------------------------------------------------------------------------
//  Calculate the score in the player's hand ..
// ---------------------------------------------------------------------------------------------------------------------------
// 
void Player::calculateHandScore(Score scores[], uint8_t turnUp) {

  uint8_t calcScore[5];
  memcpy(calcScore, this->orig, 4);
  calcScore[4] = turnUp;
  CardUtils::sort(calcScore, 5);
  calculateScores(scores, calcScore, turnUp);

}


// ---------------------------------------------------------------------------------------------------------------------------
//  Calculate the score in the player's crib ..
// ---------------------------------------------------------------------------------------------------------------------------
// 
void Player::calculateCribScore(Score scores[], uint8_t turnUp) {

  uint8_t calcScore[5];
  memcpy(calcScore, this->crib, 4);
  calcScore[4] = turnUp;
  CardUtils::sort(calcScore, 5);
  calculateScores(scores, calcScore, turnUp);

}


// ---------------------------------------------------------------------------------------------------------------------------
//  Calculate the score in the player's nominated hand ..
// ---------------------------------------------------------------------------------------------------------------------------
// 
void Player::calculateScores(Score scores[], uint8_t calcScore[], uint8_t turnUp) {

  // Clear scores collection ..

  for (uint8_t k = 0; k < Constants::PlayerHandScores; k++) {
    scores[k].reset();

  }

  uint8_t scoreIdx = 0;


  // One for his nob ..

  for (uint8_t k = 0; k < 5; k++) {

    if (calcScore[k] != turnUp && CardUtils::getCardValue(calcScore[k], false) == 11 && CardUtils::getCardSuit(calcScore[k]) == CardUtils::getCardSuit(turnUp)) {

      scores[scoreIdx].setHand(0, calcScore[k]);
      scores[scoreIdx].setScore(1);
      scores[scoreIdx].setType(ScoreType::OneForNob);
      scoreIdx++;

    }

  }



  for (uint8_t i = 31; i > 0; i--) {

    uint8_t possibility[5] = { Constants::NoCard, Constants::NoCard, Constants::NoCard, Constants::NoCard, Constants::NoCard };
    uint8_t possIdx = 0;

    for (uint8_t j = 0; j < 5; j++) {

      if ((1<<j) & i) {
        possibility[possIdx] = calcScore[j];
        possIdx++;

      }
      else {

      }

    }


	// Check for pairs, three of a kind, four of a kind ..

	if (possIdx == 2) {

		uint8_t card1Value = CardUtils::getCardValue(possibility[0], false);
		uint8_t card2Value = CardUtils::getCardValue(possibility[1], false);

		if (card1Value == card2Value) {
			// Count how many cards of this value exist in the full hand
			uint8_t matchCount = 0;
			for (uint8_t k = 0; k < 5; k++) {
				if (CardUtils::getCardValue(calcScore[k], false) == card1Value) {
					matchCount++;
				}
			}

			// Only record this score once - when we first encounter this value
			bool alreadyScored = false;
			for (uint8_t k = 0; k < scoreIdx; k++) {
				if ((scores[k].getType() == ScoreType::Pair) &&
					CardUtils::getCardValue(scores[k].getCard(0), false) == card1Value) {
					alreadyScored = true;
					break;
				}
			}

			if (!alreadyScored) {
				// Collect all cards of this value
				uint8_t cardIdx = 0;
				for (uint8_t k = 0; k < 5; k++) {
					if (CardUtils::getCardValue(calcScore[k], false) == card1Value) {
						scores[scoreIdx].setHand(cardIdx, calcScore[k]);
						cardIdx++;
					}
				}

				// Set score and type based on count
				const uint8_t pairScores[] = { 0, 0, 2, 6, 12 };
				scores[scoreIdx].setScore(pairScores[matchCount]);

				if (matchCount == 2 || matchCount == 3 || matchCount == 4) scores[scoreIdx].setType(ScoreType::Pair);

				scoreIdx++;
			}
		}
	}


    // Check for 15 ..

    {
      uint8_t total = 0;

      for (uint8_t k = 0; k < possIdx; k++) {

        total = total + CardUtils::getCardValue(possibility[k], true);

      }

      if (total == 15) {

        for (uint8_t k = 0; k < 5; k++) {
          scores[scoreIdx].setHand(k, possibility[k]);
        }

        scores[scoreIdx].setScore(2);
        scores[scoreIdx].setType(ScoreType::Fifteen);
        scoreIdx++;

      }

    }


    // Check for run of 3 ..

    if (possIdx == 3) {

      if (CardUtils::getCardValue(possibility[0], false) + 1 == CardUtils::getCardValue(possibility[1], false) && 
          CardUtils::getCardValue(possibility[1], false) + 1 == CardUtils::getCardValue(possibility[2], false)) {

        bool found = false;

        for (uint8_t k = 0; k < Constants::PlayerHandScores; k++) {

          if ((scores[k].getType() == ScoreType::RunOf4 || scores[k].getType() == ScoreType::RunOf5) &&
          
              ((scores[k].getCard(0) == possibility[0] &&
                scores[k].getCard(1) == possibility[1] &&
                scores[k].getCard(2) == possibility[2]) ||

               (scores[k].getCard(1) == possibility[0] &&
                scores[k].getCard(2) == possibility[1] &&
                scores[k].getCard(3) == possibility[2]) ||

               (scores[k].getCard(2) == possibility[0] &&
                scores[k].getCard(3) == possibility[1] &&
                scores[k].getCard(4) == possibility[2]))) {

            found = true;

          }

        }

        if (!found){

          for (uint8_t k = 0; k < 5; k++) {
            scores[scoreIdx].setHand(k, possibility[k]);

          }

          scores[scoreIdx].setScore(3);
          scores[scoreIdx].setType(ScoreType::RunOf3);
          scoreIdx++;

        }

      }

    }


    // Check for run of 4 ..

    if (possIdx == 4) {

      if (CardUtils::getCardValue(possibility[0], false) + 1 == CardUtils::getCardValue(possibility[1], false) && 
          CardUtils::getCardValue(possibility[1], false) + 1 == CardUtils::getCardValue(possibility[2], false) && 
          CardUtils::getCardValue(possibility[2], false) + 1 == CardUtils::getCardValue(possibility[3], false)) {

        bool found = false;

        for (uint8_t k = 0; k < Constants::PlayerHandScores; k++) {

          if (scores[k].getType() == ScoreType::RunOf5 &&
          
              ((scores[k].getCard(0) == possibility[0] &&
                scores[k].getCard(1) == possibility[1] &&
                scores[k].getCard(2) == possibility[2] &&
                scores[k].getCard(3) == possibility[3]) ||

               (scores[k].getCard(1) == possibility[0] &&
                scores[k].getCard(2) == possibility[1] &&
                scores[k].getCard(3) == possibility[2] &&
                scores[k].getCard(4) == possibility[3]))) {

            found = true;

          }

        }

        if (!found) {

          for (uint8_t k = 0; k < 5; k++) {
            scores[scoreIdx].setHand(k, possibility[k]);

          }

          scores[scoreIdx].setScore(4);
          scores[scoreIdx].setType(ScoreType::RunOf4);
          scoreIdx++;

        }

      }

    }


    // Check for run of 5 ..

    if (possIdx == 5) {

      if (CardUtils::getCardValue(possibility[0], false) + 1 == CardUtils::getCardValue(possibility[1], false) && 
          CardUtils::getCardValue(possibility[1], false) + 1 == CardUtils::getCardValue(possibility[2], false) && 
          CardUtils::getCardValue(possibility[2], false) + 1 == CardUtils::getCardValue(possibility[3], false) && 
          CardUtils::getCardValue(possibility[3], false) + 1 == CardUtils::getCardValue(possibility[4], false)) {

        for (uint8_t k = 0; k < 5; k++) {

          scores[scoreIdx].setHand(k, possibility[k]);

        }

        scores[scoreIdx].setScore(5);
        scores[scoreIdx].setType(ScoreType::RunOf5);
        scoreIdx++;

      }

    }

  }

}


void Player::discardToCrib(uint8_t &card1, uint8_t &card2) {
  
  card1 = Constants::NoCard;
  card2 = Constants::NoCard;
  
  uint8_t discards[6] = {255, 255, 255, 255, 255, 255 };


  // Never discard 5's ..

  for (uint8_t x = 0; x < 6; x++) {

    if (CardUtils::getCardValue(this->hand[x], false) == 5) discards[x] = 1; // Never

  }


  // Avoid discarding pairs ..

  for (uint8_t x = 0; x < 5; x++) {

    if (CardUtils::getCardValue(this->hand[x], false) == CardUtils::getCardValue(this->hand[x + 1], false)) {
      if (discards[x] == 255) discards[x] = 2; // Avoid
      if (discards[x + 1] == 255) discards[x + 1] = 2; // Avoid
    }

  }


  // Avoid discarding seq ..

  for (uint8_t x = 0; x < 5; x++) {

    for (uint8_t y = x + 1; y < 6; y++) {

      if (CardUtils::getCardValue(this->hand[x], false) + 1 == CardUtils::getCardValue(this->hand[y], false)) {

        if (discards[x] == 255) discards[x] = 3; //avoid
        if (discards[y] == 255) discards[y] = 3; //avoid

      }

    }

  }


  // Avoid small cards ..

  for (uint8_t x = 0; x < 6; x++) {

    if (discards[x] == 255) {

      uint8_t cardValue = CardUtils::getCardValue(hand[x], false);

      switch (cardValue) {

        case 1 ... 4:
          discards[x] = cardValue + 4;
          break;

        case 7 ... 8:
          discards[x] = 6;
          break;

        case 10 ... 13:
          discards[x] = 8;
          break;
          
      }

    }

  }


  // Return two cards ..

  for (uint8_t j = 255; j > 0; j--) {

    for (uint8_t i = 0; i < 5; i++) {

      if (discards[i] == j) {

        if (card1 == Constants::NoCard) { card1 = this->hand[i]; }
        else 
        if (card1 != discards[i] && card2 == Constants::NoCard) { card2 = this->hand[i]; }

      }

    }

  }


  // No cards have been chosen so randomly select some!

  if (card1 == Constants::NoCard) { card1 = random(0, 6); }

  if (card2 == Constants::NoCard) { 

    while (card1 != card2) {
      card2 = random(0, 6); 
    }

  }

  #ifdef DEBUG_PRINT_COMPUTER_DISCARDS
    Serial.print("Comp Discards: ");
    for (uint8_t x=0; x < 6; x++) {      
      Serial.print(discards[x]);
      Serial.print(" ");
    }
    Serial.println(" > ");
    Serial.print(card1);
    Serial.print(", ");
    Serial.println(card2);
  #endif

}

void Player::addToHand(uint8_t card) {
    
  this->hand[this->handIdx] = card;
  this->handIdx++;

  CardUtils::sort(this->hand, this->handIdx);

}

void Player::addToCrib(uint8_t card) {

  this->crib[this->cribIdx] = card;
  this->cribIdx++;

  CardUtils::sort(this->crib, this->cribIdx);

}

uint8_t Player::removeFromHand(uint8_t index) {

  uint8_t card = this->hand[index];

  for (uint8_t x = index; x < 6; x++) {

    this->hand[x] = this->hand[x + 1];

  }

  this->hand[5] = Constants::NoCard;
  this->handIdx--;

  if (this->handIdx == 4) {
    memcpy(this->orig, this->hand, 4);
  }

  return card;

}

void Player::resetHand(bool clearScores) {

  this->handIdx = 0;
  this->cribIdx = 0;
  
  memset(this->hand, Constants::NoCard, (sizeof(this->hand) / sizeof(this->hand[0])));
  memset(this->crib, Constants::NoCard, (sizeof(this->crib) / sizeof(this->crib[0])));

  this->go = false;

  if (clearScores) {
    this->score = 0;
    this->prevScore = 0;
  }

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


void Player::printCrib(uint8_t playerNo) {

  #ifdef DEBUG_PRINT_HAND

  Serial.print("Player ");
  Serial.print(playerNo);
  Serial.print(": ");

  for (uint8_t x=0; x < this->cribIdx; x++) {
    
    Serial.print("(");
    Serial.print(this->crib[x]);
    Serial.print(") ");
    CardUtils::printCard(this->crib[x]);
    Serial.print(" ");

  }

  Serial.println("");
  #endif
  
}

void Player::playCard(uint8_t playedCards[], bool canOpponentPlay, uint8_t &card, uint8_t &points) {

  uint8_t discards[6] = {255, 255, 255, 255, 255, 255 };

  uint8_t pointsToDate = 0;
  uint8_t cardsPlayed = 0;


  // Calculate the number of points scored so far ..

  for (uint8_t x = 0; x < 8; x++) {
  
    if (playedCards[x] != Constants::NoCard) {
      cardsPlayed++;
      pointsToDate = pointsToDate + CardUtils::getCardValue(playedCards[x], true);
    }
  
  }


  // Convert a pair to a Double Royal pair ..

  if (cardsPlayed >= 3) {
  
    uint8_t card1 = CardUtils::getCardValue(playedCards[cardsPlayed - 3], false);
    uint8_t card2 = CardUtils::getCardValue(playedCards[cardsPlayed - 2], false);
    uint8_t card3 = CardUtils::getCardValue(playedCards[cardsPlayed - 1], false);
    uint8_t cardVal = CardUtils::getCardValue(playedCards[cardsPlayed - 3], true);
    uint8_t total = pointsToDate + cardVal;

    if (card1 == card2 && card1 == card3 && (total <= 31)) {
  
      for (uint8_t y = 0; y < this->handIdx; y++) {

        if (card1 == CardUtils::getCardValue(this->hand[y], false)) {

          card = removeFromHand(y);
          points = 12 + (total == 15 ? 2 : 0) + (total == 31 ? 2 : 0);
          return;

        }

      }

    }

  }


  // Convert a pair to a Royal pair ..

  if (cardsPlayed >= 2) {
  
    uint8_t card1 = CardUtils::getCardValue(playedCards[cardsPlayed - 2], false);
    uint8_t card2 = CardUtils::getCardValue(playedCards[cardsPlayed - 1], false);
    uint8_t cardVal = CardUtils::getCardValue(playedCards[cardsPlayed - 2], true);
    uint8_t total = pointsToDate + cardVal;

    if ((card1 == card2) && (total <= 31)) {
  
      for (uint8_t y = 0; y < this->handIdx; y++) {

        if (card1 == CardUtils::getCardValue(this->hand[y], false)) {

          card = removeFromHand(y);
          points = 6 + (total == 15 ? 2 : 0) + (total == 31 ? 2 : 0);
          return;

        }

      }

    }

  }


  // Check for run of four ..

  if (cardsPlayed >= 3) {

    uint8_t card1 = CardUtils::getCardValue(playedCards[cardsPlayed - 3], false);
    uint8_t card2 = CardUtils::getCardValue(playedCards[cardsPlayed - 2], false);
    uint8_t card3 = CardUtils::getCardValue(playedCards[cardsPlayed - 1], false);
        
    for (uint8_t y = 0; y < this->handIdx; y++) {

      uint8_t card4 = CardUtils::getCardValue(this->hand[y], false);
      uint8_t cardVal = CardUtils::getCardValue(this->hand[y], true);
      uint8_t total = pointsToDate + cardVal;

      if (total <= 31 &&
        (card1 != card2 && card1 != card3 && card1 != card4 && card2 != card3 && card2 != card4 && card3 != card4) &&
        diffT(card1, card2) <= 3 && diffT(card1, card3) <=3 && diffT(card1, card4) <=3 && 
        diffT(card2, card3) <=3 && diffT(card2, card4) <=3 && 
        diffT(card3, card4) <=3) {

        card = removeFromHand(y);
        points = 4 + (total == 15 ? 2 : 0) + (total == 31 ? 2 : 0);
        return;

      }

    }

  }           


  // Check for run of three ..

  if (cardsPlayed >= 2) {

    uint8_t card1 = CardUtils::getCardValue(playedCards[cardsPlayed - 2], false);
    uint8_t card2 = CardUtils::getCardValue(playedCards[cardsPlayed - 1], false);
    
    for (uint8_t y = 0; y < this->handIdx; y++) {
    
      uint8_t card3 = CardUtils::getCardValue(this->hand[y], false);
      uint8_t cardVal = CardUtils::getCardValue(this->hand[y], true);
      uint8_t total = pointsToDate + cardVal;

      if (total <= 31 &&
          (card1 != card2 && card1 != card3 && card2 != card3) &&
          diffT(card1, card2) <= 2 && diffT(card1, card3) <=2 && diffT(card2, card3) <=2) {

        card = removeFromHand(y);

        points = 3 + (total == 15 ? 2 : 0) + (total == 31 ? 2 : 0);
        return;

      }

    }

  }


  // Check for a pair ..

  if ((!canOpponentPlay || random(0, 3) == 1) && cardsPlayed >= 1) {

    uint8_t card1 = CardUtils::getCardValue(playedCards[cardsPlayed - 1], false);
    
    for (uint8_t y = 0; y < this->handIdx; y++) {
    
      uint8_t card2 = CardUtils::getCardValue(this->hand[y], false);
      uint8_t cardVal = CardUtils::getCardValue(this->hand[y], true);
      uint8_t total = pointsToDate + cardVal;

      if (total <= 31 && (card1 == card2)) {

        card = removeFromHand(y);

        points = 2 + (total == 15 ? 2 : 0) + (total == 31 ? 2 : 0);
        return;

      }

    }

  }


  // Check for 15 or 31 ..

  for (uint8_t y = 0; y < this->handIdx; y++) {
    
    uint8_t total = pointsToDate + CardUtils::getCardValue(this->hand[y], true);

    if (total == 15 || total == 31) {

      card = removeFromHand(y);
      points = 2;
      return;

    }

  }


  // Make an 11 if you have another 10 in the hand ..
  
  for (uint8_t x = 0; x < this->handIdx; x++) {

    if (pointsToDate + CardUtils::getCardValue(hand[x], true) == 11) {

      for (uint8_t y = 0; y < this->handIdx; y++) {

        if (x != y && CardUtils::getCardValue(hand[y], true) == 10) {

          card = removeFromHand(x);
          points = 0;
          return;
          
        }

      }

    }

  }


  // --------------------------------------------------------------------------------

  // Lead a 4 ..

  if (playedCards[0] == Constants::NoCard) {

    for (uint8_t x = 0; x < this->handIdx; x++) {

      if (CardUtils::getCardValue(hand[x], false) == 4) {
          
        card = removeFromHand(x);
        points = 0;
        return;

      }

    }

  }


  // Do not play a card that has just been played unless you have two or more of them!

  uint8_t lastCard = CardUtils::getCardValue(playedCards[cardsPlayed - 1], false);
  uint8_t sameVal = 0;

  for (uint8_t x = 0; x < this->handIdx; x++) {

    if (CardUtils::getCardValue(hand[x], false) == lastCard) sameVal++;

  }

  if (sameVal == 1) {

    for (uint8_t x = 0; x < this->handIdx; x++) {

      if ((CardUtils::getCardValue(hand[x], false) == lastCard) && (pointsToDate + CardUtils::getCardValue(this->hand[x], true) <= 31)) discards[x] = 1;  // Never

    }

  }


  // Play high cards first ..

  for (uint8_t x = 0; x < this->handIdx; x++) {

    if ((discards[x] == Constants::NoCard) && (pointsToDate + CardUtils::getCardValue(this->hand[x], true) <= 31)) {
      discards[x] = 20 + CardUtils::getCardValue(this->hand[x], false);
    }

  }


#ifdef DEBUG_PRINT_DISCARDS
  Serial.println("-------------");
  Serial.print("Hand :");
  for (uint8_t j = 0; j < 4; j++) {
    Serial.print(CardUtils::getCardValue(this->hand[j], true));
    Serial.print(" ");
  }
  Serial.println("");
  Serial.print("Discards :");
  for (uint8_t j = 0; j < 4; j++) {
    Serial.print(discards[j]);
    Serial.print(" ");
  }
  Serial.println("");
#endif


  // Return a card ..

  for (uint8_t j = 255; j > 0; j--) {

    for (uint8_t i = 0; i < this->handIdx; i++) {

      if (discards[i] == j && (pointsToDate + CardUtils::getCardValue(this->hand[i], true) <= 31)) {

        card = removeFromHand(i);

        points = (cardsPlayed >= 1 && (CardUtils::getCardValue(playedCards[cardsPlayed - 1], false) == CardUtils::getCardValue(card, false) ? 2 : 0));
        return;

      }

    }

  }

  card = Constants::NoCard;
  this->go = true;
  return;
  

}

bool Player::canPlay(uint8_t playedCards[]) {

  uint8_t pointsToDate = 0;
  uint8_t cardsPlayed = 0;

  for (uint8_t x = 0; x < 8; x++) {

    if (playedCards[x] != Constants::NoCard) {
      cardsPlayed = x + 1;
      pointsToDate = pointsToDate + CardUtils::getCardValue(playedCards[x], true);
    }

  }

  for (uint8_t i = 0; i < this->handIdx; i++) {

    if (pointsToDate + CardUtils::getCardValue(this->hand[i], true) <= 31) {

      return true;

    }

  }

  return false;

}
