#include "BaseState.h"

#include "../images/Images.h"
#include "../utils/Utils.h"
#include "../utils/Enums.h"
#include "../fonts/Font3x5.h"



void BaseState::renderScore(StateMachine & machine, int16_t score, uint8_t x, uint8_t y) {

	auto & arduboy = machine.getContext().arduboy;

	if (score < 0) { arduboy.drawFastHLine(x - 3, y + 3, 2); }

	if (score > 10000) {

		for (uint8_t j = 5, x2 = x - 4; j > 0; --j, x2 += 4) {
			
			uint8_t digits[5] = {};
			extractDigits(digits, static_cast<uint16_t>(absT(score)));

			font3x5.setCursor(x2, y);
			font3x5.print(digits[j - 1]);

		}

	}
	else {

		for (uint8_t j = 4, x2 = x; j > 0; --j, x2 += 4) {
			
			uint8_t digits[4] = {};
			extractDigits(digits, static_cast<uint16_t>(absT(score)));

			font3x5.setCursor(x2, y);
			font3x5.print(digits[j - 1]);
		}

	}

}


void BaseState::renderMovingDice(StateMachine & machine, uint8_t y) {

	auto & arduboy = machine.getContext().arduboy;


	// Adjust the dice positions ..

	if (arduboy.everyXFrames(2)) {

		this->xPos++;
		if (this->xPos == 0) xPos = -96;

	}

	SpritesB::drawExternalMask(xPos, y, Images::Title_Dice, Images::Title_Dice_Mask, 0, 0);
	SpritesB::drawExternalMask(xPos + 96, y, Images::Title_Dice, Images::Title_Dice_Mask, 0, 0);
	SpritesB::drawExternalMask(xPos + 192, y, Images::Title_Dice, Images::Title_Dice_Mask, 0, 0);

}


uint8_t BaseState::getWinner(StateMachine & machine) {

	auto & gameStats = machine.getContext().gameStats;

	uint8_t winnerIdx = 0;

	for (uint8_t i = 1; i < gameStats.numberOfPlayers; i++) {

		if (gameStats.players[i]->score > gameStats.players[winnerIdx]->score) {
			winnerIdx = i;
		}

	}

	return winnerIdx;

}


void BaseState::printName3x5(StateMachine & machine, uint8_t idx) {
	
  auto & gameStats = machine.getContext().gameStats;

  for (uint8_t i = 1; i < NAME_LENGTH; i++) {

    this->font3x5.print(gameStats.players[idx]->name[i]);

  }

}


void BaseState::printName4x6(StateMachine & machine, uint8_t idx) {
	
  auto & gameStats = machine.getContext().gameStats;

  for (uint8_t i = 1; i < NAME_LENGTH; i++) {

    this->font4x6.print(gameStats.players[idx]->name[i]);

  }

}