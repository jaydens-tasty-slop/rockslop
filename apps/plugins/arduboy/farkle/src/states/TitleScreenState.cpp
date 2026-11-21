#include "TitleScreenState.h"
#include "../images/Images.h"
#include "../utils/EEPROM_Utils.h"

constexpr const static uint8_t UPLOAD_DELAY = 16;


// ----------------------------------------------------------------------------
//  Initialise state ..
//
void TitleScreenState::activate(StateMachine & machine) {

	auto & gameStats = machine.getContext().gameStats;

	for (uint8_t i = 0; i < PLAYER_MAX; i ++) {
		gameStats.players[i]->score = 0;
	}

	gameStats.playerBeingEdited = 1;

  EEPROM_Utils::getName(gameStats.player1.name, EEPROM_PLAYER1);
  EEPROM_Utils::getName(gameStats.player2.name, EEPROM_PLAYER2);
  EEPROM_Utils::getName(gameStats.player3.name, EEPROM_PLAYER3);
  EEPROM_Utils::getName(gameStats.player4.name, EEPROM_PLAYER4);

}


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void TitleScreenState::update(StateMachine & machine) {

	auto & arduboy = machine.getContext().arduboy;
  auto justPressed = arduboy.justPressedButtons();
  auto pressed = arduboy.pressedButtons();

	// Restart ?

	// if (pressed & DOWN_BUTTON) {

	// 	if (restart < UPLOAD_DELAY) {
	// 		restart++;
	// 	}
	// 	else {
	// 		arduboy.exitToBootloader();
	// 	}

	// }
	// else {
	// 	restart = 0;
	// }


	// Handle other input ..

	if (justPressed & A_BUTTON) {
		machine.changeState(GameStateType::PlayerNames); 
	}

}


// ----------------------------------------------------------------------------
//  Render the state .. 
//
void TitleScreenState::render(StateMachine & machine) {

	auto & arduboy = machine.getContext().arduboy;

	SpritesB::drawOverwrite(12, 0, Images::Farkle, 0);
	renderMovingDice(machine, 35);

}
