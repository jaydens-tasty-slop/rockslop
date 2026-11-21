#include "TitleScreenState.h"
#include "../images/Images.h"
#include "../utils/EEPROM_Utils.h"

constexpr const static uint8_t UPLOAD_DELAY = 16;


// ----------------------------------------------------------------------------
//  Initialise state ..
//
void TitleScreenState::activate(StateMachine & machine) {

	auto & gameStats = machine.getContext().gameStats;

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


	// Update ticker ..

	if (arduboy.everyXFrames(8)) {
		this->counter++;
		if (this->counter == 28) this->counter = 0;
	}


	// Handle other input ..

	if (justPressed & A_BUTTON) {
		machine.changeState(GameStateType::PlayGame); 
	}

}


// ----------------------------------------------------------------------------
//  Render the state .. 
//
void TitleScreenState::render(StateMachine & machine) {

	SpritesB::drawOverwrite(75, 0, Images::TitleScreen_Board, 0);
	SpritesB::drawOverwrite(7, 16, Images::TitleScreen_Words, 0);

	for (int16_t x = -20; x < 154; x = x + 28) {
		SpritesB::drawOverwrite(x - this->counter, 44, Images::TitleScreen_Cards, 0);
	}

}
