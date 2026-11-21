#include "PlayGameState.h"
#include "../images/Images.h"


const uint8_t dicePositionX[] PROGMEM = { 0, 20, 40, 0, 20, 40, 0, 30 };
const uint8_t dicePositionY[] PROGMEM = { 0, 0, 0, 20, 20, 20, 42, 42 };


// ----------------------------------------------------------------------------
//  Initialise state ..
//
void PlayGameState::activate(StateMachine & machine) {

	auto & arduboy = machine.getContext().arduboy;
	auto & gameStats = machine.getContext().gameStats;

	this->viewState = ViewState::SwapPlayer;
	this->currentPlayer = gameStats.numberOfPlayers;
	this->round = 0;

}


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void PlayGameState::update(StateMachine & machine) {

	auto & arduboy = machine.getContext().arduboy;
	auto & gameStats = machine.getContext().gameStats;
  auto justPressed = arduboy.justPressedButtons();
  auto pressed = arduboy.pressedButtons();

	switch (this->viewState) {

	 	case ViewState::SwapPlayer:

			this->currentPlayer++;
			this->currentRoll = 0;
			this->currentHand = 0;
			this->possibleHand = 0;
			this->count = 0;
			this->viewState = ViewState::RollDice;

			if (this->currentPlayer >= gameStats.numberOfPlayers) {
				
				this->currentPlayer = 0;
				this->round++;
			
			}

			clearHand();

		case ViewState::RollDice:

			if (this->count < 4) {

				if (arduboy.everyXFrames(3)) {

					this->count++;

					if (this->count == 4) {

							for (uint8_t i = 0; i < 6; i++) {

								if (this->dice[i] > 0) this->dice[i] = random(1, 7);

							} 

					}

				}

			}
			else {

				this->possibleHand = calculateScore(true);	

				if (this->possibleHand > 0) {

					this->selectedDice = firstSelection();
					this->viewState = ViewState::SelectDice;

				}
				else {


					// Take 500 points from the player if they roll a Farkle ..

					int16_t score = gameStats.players[this->currentPlayer]->score;
					gameStats.players[this->currentPlayer]->score = score - FARKLE_PENALTY;
					this->viewState = ViewState::Farkle;
					this->count = 0;

				}

			}

			break;

		case ViewState::SelectDice:

			if (justPressed & LEFT_BUTTON) {

				switch (this->selectedDice) {

					case 1 ... 6:
						this->selectedDice--;
						break;

					case 7:
						if (this->currentRoll > 0) {
							this->selectedDice--;
						}
						break;

					default: break;
				}

			}
			
			if (justPressed & RIGHT_BUTTON) {

				switch (this->selectedDice) {

					case 0 ... 4:
						this->selectedDice++;
						break;

					case 5:
						if (this->currentRoll > 0) {
							this->selectedDice++;
						}
						break;

					case 6:
						if (this->currentHand + this->currentRoll >= MINIMUM_HAND_SCORE) {
							this->selectedDice++;
						}
						break;

					default: break;
				}
				
			}

			if (justPressed & DOWN_BUTTON) {

				switch (this->selectedDice) {

					case 0 ... 2:
						this->selectedDice = this->selectedDice + 3;
						break;

					case 3:
						if (this->currentRoll > 0) {
							this->selectedDice = 6;
						}
						break;

					case 4:
						if (this->currentRoll > 0) {
							this->selectedDice = 6;
						}
						else if (this->currentHand + this->currentRoll >= MINIMUM_HAND_SCORE) {
							this->selectedDice = 7;
						}
						break;

					case 5:
						if (this->currentHand + this->currentRoll >= MINIMUM_HAND_SCORE) {
							this->selectedDice = 7;
						}
						else if (this->currentRoll > 0) {
							this->selectedDice = 6;
						}
						break;

					default: break;
				}

			}

			if (justPressed & UP_BUTTON) {

				switch (this->selectedDice) {

					case 3 ... 5:
						this->selectedDice = this->selectedDice - 3;
						break;

					case 6:
						this->selectedDice = 3;
						break;

					case 7:
						this->selectedDice = 5;
						break;

					default: break;
				}

			}

			if (justPressed & A_BUTTON) {
				
				switch (this->selectedDice) {
		
					case 0 ... 5:
						
						if (this->dice[this->selectedDice] > 0) {
							this->retainDice[this->selectedDice] = !this->retainDice[this->selectedDice];
						}

						this->currentRoll = calculateScore(false);	
						break;

					case 6: // Roll Again.

						for (uint8_t i = 0; i < 6; i++) {

							if (this->retainDice[i]) {
								this->dice[i] = 0;
							}

						} 

						// If all the dice have been played, roll them all again ..

						if (allDicePlayed()) {

							this->viewState = ViewState::HotDice;	
							this->count = 0;			

							for (uint8_t x = 0; x < NUMBER_OF_FIREWORKS; x++) {

								this->fireWorks_X[x] = 200;	
								this->fireWorks_Y[x] = 0;	
								this->fireWorks_count[x] = (x * 2);	

							}

						}
						else {

							this->count = 0;
							this->viewState = ViewState::RollDice;					

						}

						this->currentHand = this->currentHand + this->currentRoll;
						this->currentRoll = 0;

						break;

					case 7:

						int16_t score = this->currentHand + this->currentRoll;
						gameStats.players[this->currentPlayer]->score = gameStats.players[this->currentPlayer]->score + score;

						viewState = ViewState::TakeScore;
						this->count = 0;
						break;

				}

			}

			if (justPressed & B_BUTTON) { viewState = ViewState::ScoreSheet; }

			break;

		case ViewState::TakeScore:
			
			if (justPressed > 0) { this->count = (FLASH_FRAME_COUNT * 2); }

			if (this->count < (FLASH_FRAME_COUNT * 2)) {
				
				this->count++;

			}
			else {

				this->currentHand = 0;
				this->currentRoll = 0;

				if ((this->currentPlayer == gameStats.numberOfPlayers - 1) && (this->round == NUMBER_OF_ROUNDS_PER_GAME)) {

					machine.changeState(GameStateType::GameOver); 

				} 
				else {

					if (gameStats.numberOfPlayers > 1) {
						viewState = ViewState::NextUp;
					}
					else {
						viewState = ViewState::SwapPlayer;
					}

				}

			}

			break;

		case ViewState::NextUp:
			
			if (justPressed & A_BUTTON) {
				viewState = ViewState::SwapPlayer;
			}

			break;

		case ViewState::Farkle:

			if (this->count < FARKLE_DELAY) this->count++;

			if (justPressed & A_BUTTON) {

				if ((this->currentPlayer == gameStats.numberOfPlayers - 1) && (this->round == NUMBER_OF_ROUNDS_PER_GAME)) {

					machine.changeState(GameStateType::GameOver); 

				} 
				else {

					if (gameStats.numberOfPlayers > 1) {
						viewState = ViewState::NextUp;
					}
					else {
						viewState = ViewState::SwapPlayer;
					}

				}

			}
			break;

		case ViewState::HotDice:
		
			if (justPressed == 0) { 

				for (uint8_t x = 0; x < NUMBER_OF_FIREWORKS; x++) {

					this->fireWorks_count[x]++;

					if (this->fireWorks_count[x] >= 10) {
						this->fireWorks_count[x] = 0;
						this->fireWorks_X[x] = random(0, 128);	
						this->fireWorks_Y[x] = random(0, 64);	
					}

				}

			}
			else {

				clearHand();
				this->count = 0;
				this->viewState = ViewState::RollDice;

			}
			break;

		case ViewState::ScoreSheet:
		
			if (justPressed & B_BUTTON) { this->viewState = ViewState::SelectDice; }

			if (pressed & UP_BUTTON && this->scroll > 0) 			{ this->scroll--; }
			if (pressed & DOWN_BUTTON && this->scroll < 120) 	{ this->scroll++; }

			break;

	}

}


// ----------------------------------------------------------------------------
//  Render the state .. 
//
void PlayGameState::render(StateMachine & machine) {

	auto & arduboy = machine.getContext().arduboy;
	auto & gameStats = machine.getContext().gameStats;
	
	const bool flash = arduboy.getFrameCountHalf(FLASH_FRAME_COUNT);

	if (this->viewState != ViewState::ScoreSheet && this->viewState != ViewState::HotDice) {

		arduboy.drawHorizontalDottedLine(60, 64, 0);
		arduboy.drawVerticalDottedLine(0, 64, 62);
		arduboy.drawHorizontalDottedLine(60, 64, 62);

		for (uint8_t i = 0; i < 8; i++) {

			uint8_t x = pgm_read_byte(&dicePositionX[i]);
			uint8_t y = pgm_read_byte(&dicePositionY[i]);

			switch (i) {

				case 0 ... 5:
							
						if (this->dice[i] > 0) {

							if (this->retainDice[i]) {
						
								arduboy.drawCompressed(x, y, Images::Dice_Solid, WHITE);
								arduboy.drawCompressed(x + 4, y + 4, Images::Dice_Faces[this->dice[i] - 1], BLACK);

							}
							else {

								if ((this->viewState == ViewState::RollDice && this->count == 4) || this->viewState != ViewState::RollDice) { 

									arduboy.drawCompressed(x, y, Images::Dice_Hollow, WHITE);
									arduboy.drawCompressed(x + 4, y + 4, Images::Dice_Faces[this->dice[i] - 1], WHITE);

								}
								else {

									arduboy.drawCompressed(x, y, Images::Dice_Rotate[this->count + (i % 2 == 0 ? 0 : 4)], WHITE);

								}

							}

							if (i == this->selectedDice && flash) {

								arduboy.drawCompressed(x, y, Images::Dice_Highlight, BLACK);

							}  

						}
						else {

							arduboy.drawCompressed(x, y, Images::Dice_None, WHITE);

							if (i == this->selectedDice && flash) {

								arduboy.drawCompressed(x, y, Images::Dice_None_Highlight, WHITE);

							}  

						}

					break;

				case 6:

					if (this->currentRoll > 0) {

						arduboy.drawCompressed(x, y, Images::Roll, WHITE);

						if (i == this->selectedDice && flash) {

							arduboy.drawCompressed(x, y, Images::Button_Highlight, BLACK);

						}  

					}
					else {

						arduboy.drawCompressed(x, y, Images::Button_Disabled, WHITE);

					}

					break;

				case 7:

					if (this->currentRoll > 0 && this->currentRoll + this->currentHand >= MINIMUM_HAND_SCORE) {

						arduboy.drawCompressed(x, y, Images::Take, WHITE);

						if (i == this->selectedDice && flash) {

							arduboy.drawCompressed(x, y, Images::Button_Highlight, BLACK);

						}  

					}
					else {

						arduboy.drawCompressed(x, y, Images::Button_Disabled, WHITE);

					}

					break;

			}

		}


		// Render player statistics ..

		arduboy.drawCompressed(67, 0, Images::Border_With_Shadow, WHITE);
		arduboy.drawCompressed(68, 1, Images::Icons[gameStats.players[this->currentPlayer]->getIcon()], WHITE);

		font4x6.setCursor(89, -1);
		BaseState::printName4x6(machine, this->currentPlayer);

		arduboy.drawHorizontalDottedLine(89, 128, 8);

		font3x5.setCursor(89, 10);
		font3x5.print(F("Roll"));
		BaseState::renderScore(machine, this->currentRoll, 113, 10);				
						
		font3x5.setCursor(89, 17);
		font3x5.print(F("Turn"));
		BaseState::renderScore(machine, this->currentHand, 113, 17);				

		if (this->viewState != ViewState::TakeScore || flash) {

			font3x5.setCursor(89, 24);
			font3x5.print(F("Total"));
			BaseState::renderScore(machine, gameStats.players[this->currentPlayer]->score, 113, 24);

		}


		// Render round ..

		font3x5.setCursor(8, 57);
		font3x5.print(F("Turn~"));
		font3x5.print(this->round);
		font3x5.print(F("~of~"));
		font3x5.print(NUMBER_OF_ROUNDS_PER_GAME);


		// Render other players ..

		{
			uint8_t x = 67;

			for (uint8_t i = 0; i < 4; i++) {
			
				if (i != this->currentPlayer) {

					arduboy.drawCompressed(x, 36, Images::Border_With_Shadow, WHITE);

					if (i < gameStats.numberOfPlayers) {
						arduboy.drawCompressed(x + 1, 37, Images::Icons[gameStats.players[i]->getIcon()]);
					}
					else {
						arduboy.drawCompressed(x + 1, 37, Images::Icons[0]);
					}

					BaseState::renderScore(machine, gameStats.players[i]->score, x + 2, 57);
					x = x + 21;

				}

			}

		}


		// Farkle?

		if (this->viewState == ViewState::Farkle && this->count == FARKLE_DELAY) {

			SpritesB::drawExternalMask(12, 8, Images::Farkle, Images::Farkle_Mask, 0, 0);
			arduboy.setRGBled(RED_LED, (flash ? 32 : 0));

		}


		// // Hot Dice?

		// else if (this->viewState == ViewState::HotDice) {

		// 	arduboy.drawCompressed(7, 11, Images::HotDice_Mask, BLACK);
		// 	arduboy.drawCompressed(7, 11, Images::HotDice, WHITE);

		// 	arduboy.setRGBled(GREEN_LED, (flash ? 32 : 0));

		// }
		
		else {

		 	arduboy.setRGBled(0, 0, 0);

		}


		// Next Up?

		if (this->viewState == ViewState::NextUp) {

			uint8_t nextPlayer = this->currentPlayer + 1;

			if (nextPlayer >= gameStats.numberOfPlayers) {
					
				nextPlayer = 0;
				
			}

			arduboy.drawCompressed(30, 16, Images::NextUp_Mask, BLACK);
			arduboy.drawCompressed(30, 16, Images::NextUp, WHITE);
			arduboy.drawCompressed(74, 24, Images::Icons[gameStats.players[nextPlayer]->getIcon()], WHITE);

		}

	}

	else if (this->viewState == ViewState::HotDice) { // Hot Dice?

		for (uint8_t x = 0; x < NUMBER_OF_FIREWORKS; x++) {
	
			arduboy.drawCompressed(this->fireWorks_X[x], this->fireWorks_Y[x], Images::FireWorks[this->fireWorks_count[x]], WHITE);

		}

		arduboy.drawCompressed(7, 15, Images::HotDice_Mask, BLACK);
		arduboy.drawCompressed(7, 15, Images::HotDice, WHITE);
		arduboy.setRGBled(GREEN_LED, (flash ? 32 : 0));

	}

	else if (this->viewState == ViewState::ScoreSheet) {
		
		SpritesB::drawOverwrite(12, -scroll, Images::Farkle, 0);
		arduboy.drawCompressed(6, -scroll + 50, Images::ScoreSheet, WHITE);

	}

}


void PlayGameState::removeElement(byte* array, uint8_t elementNo, uint8_t arrayLength) {

	for (uint8_t x = elementNo; x < arrayLength - 1; x++) {
	
		array[x] = array[x + 1];

	}
	
	array[arrayLength - 1] = 0;

}


void PlayGameState::isort(uint8_t *a, uint8_t n) {

	for (uint8_t i = 1; i < n; ++i) {
		
		uint8_t j = a[i];
		int8_t k;

		for (k = i - 1; (k >= 0) && (j < a[k]); k--) {
			a[k + 1] = a[k];
		}

		a[k + 1] = j;

	}

}


uint16_t PlayGameState::calculateScore(boolean checkForFarkle) {
		
	uint8_t dice[6] = {0};
	uint8_t diceCount = 0;
	uint16_t score = 0;
		
	if (checkForFarkle) {
		
		for (uint8_t i = 0; i < 6; i++) {

			if (this->dice[i] > 0) {

					dice[diceCount] = this->dice[i];
					diceCount++;

			}

		}

	}
	else {

		for (uint8_t i = 0; i < 6; i++) {

			if (this->dice[i] > 0 && this->retainDice[i]) {

					dice[diceCount] = this->dice[i];
					diceCount++;

			}

		}

	}

	isort(dice, diceCount);
	
	if (diceCount > 0) {
			
		
		// Look for threes, fours, five, sixes ..
		
		for (uint8_t i = 1; i <= 6; i++) {
				
			int thisScore = 0;
			int counter = 0;
			
			for (uint8_t d = 0; d <  diceCount; d++) {
			
				if (dice[d] == i) {
				
					counter++;
				
				}
			
			}
			
			if (counter > 2) {
			
				switch (counter) {
				
					case 3:
					
						if (i == 1) { thisScore = thisScore + 1000; }
						if (i != 1) { thisScore = thisScore + (i * 100); }
						break;
					
					case 4:
					
						if (i == 1) { thisScore = thisScore + (1000 * 2); }
						if (i != 1) { thisScore = thisScore + (i * 100 * 2); }
						break;
					
					case 5:
					
						if (i == 1) { thisScore = thisScore + (1000 * 4); }
						if (i != 1) { thisScore = thisScore + (i * 100 * 4); }
						break;
					
					case 6:
					
						if (i == 1) { thisScore = thisScore + (1000 * 8); }
						if (i != 1) { thisScore = thisScore + (i * 100 * 8); }
						break;
				
				}
				
				
				// Remove items from collection if they have been added to score ..

				for (int8_t iRemove = diceCount; iRemove >= 0; iRemove--) {
				
					if (dice[iRemove] == i) {
					
						removeElement(dice, iRemove, 6);
						diceCount--;

					}
				
				}
				
				counter = 0;
				score = score + thisScore;
			
			}
				
		}
		
		
		// Look for hands of six (straights or three pairs) ..
		
		if (diceCount == 6) {
				
			if (dice[0] == 1 && dice[1] == 2 && dice[2] == 3 && dice[3] == 4 && dice[4] == 5 && dice[5] == 6) {
					
				score = score + 1500;
				diceCount=0;
				
			}
			else {
			
				if (dice[0] == dice[1] && dice[2] == dice[3] && dice[4] == dice[5]) {
					
					score = score + 750;
					diceCount=0;
					
				}
					
			}
				
		}
		
		
		// What about stray ones and fives?
		
		if (diceCount > 0) {
				
			for (int8_t iRemove = diceCount - 1; iRemove >= 0; iRemove--) {

				if (dice[iRemove] == 5) {

					removeElement(dice, iRemove, 6);
					diceCount--;
					score = score + 50;

				}
				else if (dice[iRemove] == 1) {
					
					removeElement(dice, iRemove, 6);
					diceCount--;
					score = score + 100;

				}
					
			}
				
		}
		
		
		// Any left overs ?

		if (!checkForFarkle && diceCount > 0) {

				score = 0;
				
		}
			
	}

	return score;

}


uint8_t PlayGameState::firstSelection() {

	for (int8_t i = 0; i < 6; i++) {

		//if (this->currentRoll > 0) return i;
		if (this->dice[i] > 0) return i;

	}

	return 0;

}

bool PlayGameState::allDicePlayed() {

	for (uint8_t i = 0; i < 6; i++) {

		if (this->dice[i] > 0) return false;

	}

	return true;

}

void PlayGameState::clearHand() {

	for (uint8_t i = 0; i < 6; i++) {

		this->dice[i] = 1;
		this->retainDice[i] = false;

	} 

}
