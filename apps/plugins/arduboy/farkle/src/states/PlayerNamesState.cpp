#include "PlayerNamesState.h"
#include "../images/Images.h"


// ----------------------------------------------------------------------------
//  Initialise state ..
//
void PlayerNamesState::activate(StateMachine & machine) {

	auto & gameStats = machine.getContext().gameStats;

	this->viewState = ViewState::NumberOfPlayers;
	getPlayerName(machine);

	if (this->name.getIcon() == 0) {
		this->name.setChar(0, 1);
	}

}


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void PlayerNamesState::update(StateMachine & machine) {

	auto & arduboy = machine.getContext().arduboy;
	auto & gameStats = machine.getContext().gameStats;
  auto justPressed = arduboy.justPressedButtons();
  auto pressed = arduboy.pressedButtons();


	switch (this->viewState) {

		case ViewState::NumberOfPlayers:

			if (justPressed & B_BUTTON) 				{	machine.changeState(GameStateType::TitleScreen); }
			if (justPressed & A_BUTTON) 				{ viewState = ViewState::EnterName; }

			if ((justPressed & UP_BUTTON) || (justPressed & DOWN_BUTTON) || arduboy.isFrameCount(KEY_REPEAT_DELAY, this->frameCount)) {

					if ((pressed & UP_BUTTON) && gameStats.numberOfPlayers < 4) { 

						setNumberOfPlayers(machine, gameStats.numberOfPlayers + 1);
						this->frameCount = arduboy.getFrameCount(KEY_REPEAT_DELAY);

					}
					else if ((pressed & DOWN_BUTTON) && gameStats.numberOfPlayers > 1) { 

						setNumberOfPlayers(machine, gameStats.numberOfPlayers - 1);
						this->frameCount = arduboy.getFrameCount(KEY_REPEAT_DELAY);

					}

			}
			
			break;

		case ViewState::EnterName:
			{
				uint8_t charIndex = name.getCharIndex();

				if (justPressed & LEFT_BUTTON) 						 	{ name.decCharIndex(); } 
				if (justPressed & RIGHT_BUTTON) 					 	{ name.incCharIndex(); } 

				if ((justPressed & UP_BUTTON) || (justPressed & DOWN_BUTTON) || arduboy.isFrameCount(KEY_REPEAT_DELAY, frameCount)) {

					if (pressed & UP_BUTTON) { 

						name.incChar(charIndex); 
						frameCount = arduboy.getFrameCount(KEY_REPEAT_DELAY);

					}
					else if (pressed & DOWN_BUTTON) { 

						name.decChar(charIndex); 
						frameCount = arduboy.getFrameCount(KEY_REPEAT_DELAY);

					}
					
				}


				// Go back a player or to number of players screen ..

				if (justPressed & B_BUTTON) { 

					if (gameStats.playerBeingEdited > 1) {
					
						updatePlayerName(machine);
						gameStats.playerBeingEdited--;
						getPlayerName(machine);

					}
					else {

						viewState = ViewState::NumberOfPlayers;

					}

				}


				// Go forward to next player or start game ..

				if (justPressed & A_BUTTON) { 
	
					bool allSpaces = true;
					
					for (uint8_t x = 1; x < NAME_LENGTH; x++) {

						if (name.getChar(x) != ' ') allSpaces = false;

					}			

					if (!allSpaces) {

						updatePlayerName(machine);

						if (gameStats.playerBeingEdited < gameStats.numberOfPlayers)  {
							
								gameStats.playerBeingEdited++;
								getPlayerName(machine);
						
						}
						else {

							machine.changeState(GameStateType::PlayGame); 

						}

					}
					
				}

			}

			break;

	}

}


// ----------------------------------------------------------------------------
//  Render the state .. 
//
void PlayerNamesState::render(StateMachine & machine) {

	auto & arduboy = machine.getContext().arduboy;
	auto & gameStats = machine.getContext().gameStats;
	
	const bool flash = arduboy.getFrameCountHalf(FLASH_FRAME_COUNT_2);

	renderMovingDice(machine, 35);

	switch (this->viewState) {

		case ViewState::NumberOfPlayers:
			font4x6.setCursor(0, 8);
			font4x6.print(F("Number~of~players~? "));
			font4x6.print(gameStats.numberOfPlayers);

			if (flash) {
				arduboy.drawFastHLine(94, 16, 4);
			}

			break;

		case ViewState::EnterName:

			font4x6.setCursor(0, 0);
			font4x6.print(F("Player~"));
			font4x6.print(gameStats.playerBeingEdited);

			font4x6.setCursor(0, 16);
			font4x6.print(F("Icon~~Name"));
			arduboy.drawPixel(48, 19);
			arduboy.drawPixel(48, 21);
			arduboy.drawLine(23, 17, 21, 22);

			arduboy.drawCompressed(53, 11, Images::Border_No_Shadow, WHITE);
			arduboy.drawCompressed(54, 12, Images::Icons[static_cast<uint8_t>(name.getChar(0))], WHITE);

			if (flash && name.getCharIndex() == 0) {
				arduboy.drawCompressed(58, 5, Images::ArrowUp, WHITE);
				arduboy.drawCompressed(58, 31, Images::ArrowDown, WHITE);
			}

			arduboy.drawHorizontalDottedLine(77, 124, 24);

			for (uint8_t x = 1; x < NAME_LENGTH; x++) {

				font4x6.setCursor(71 + (x * NAME_CHARACTER_SPACING), 16);

				if (flash && name.getCharIndex() == x) {
					arduboy.drawFastHLine(71 + (x * NAME_CHARACTER_SPACING), 24, 4, WHITE);
				}

				font4x6.print(name.getChar(x));

			}

			break;

	}

}

void PlayerNamesState::getPlayerName(StateMachine & machine) {
			
	auto & gameStats = machine.getContext().gameStats;

	this->name.setChars(gameStats.players[gameStats.playerBeingEdited - 1]->name);
	this->name.setStartLoc(EEPROM_PLAYER1 + (10 * (gameStats.playerBeingEdited - 1)));
	this->name.clearIconsInUse();

	if (gameStats.playerBeingEdited - 1 != 0) this->name.addIconsInUse(gameStats.players[0]->getIcon());
	if (gameStats.playerBeingEdited - 1 != 1) this->name.addIconsInUse(gameStats.players[1]->getIcon());
	if (gameStats.playerBeingEdited - 1 != 2) this->name.addIconsInUse(gameStats.players[2]->getIcon());
	if (gameStats.playerBeingEdited - 1 != 3) this->name.addIconsInUse(gameStats.players[3]->getIcon());

}

void PlayerNamesState::updatePlayerName(StateMachine & machine) {
		
	auto & gameStats = machine.getContext().gameStats;
	strcpy(gameStats.players[gameStats.playerBeingEdited - 1]->name, this->name.getString());

}

void PlayerNamesState::setNumberOfPlayers(StateMachine & machine, uint8_t numberOfPlayers) {
				
	auto & gameStats = machine.getContext().gameStats;

	if (gameStats.numberOfPlayers >= numberOfPlayers) {

		gameStats.players[gameStats.numberOfPlayers]->setIcon(0);

	}

	gameStats.numberOfPlayers = numberOfPlayers;

}