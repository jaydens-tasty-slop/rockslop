#include "GameOverState.h"
#include "../images/Images.h"


// ----------------------------------------------------------------------------
//  Initialise state ..
//
void GameOverState::activate(StateMachine & machine) {
	
	(void)machine;
	this->index = 2;

}


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void GameOverState::update(StateMachine & machine) {

	auto & arduboy = machine.getContext().arduboy;
  auto justPressed = arduboy.justPressedButtons();
  auto pressed = arduboy.pressedButtons();


	// Update ticker ..

	if (arduboy.everyXFrames(8)) {
		this->counter++;
		if (this->counter == 28) {
			this->counter = 0;
		}
		if (this->counter == 10 && this->index == 2) {
			this->index = 0;
		}
	}

	
	// Handle other input ..

	if ((justPressed & LEFT_BUTTON) && this->index == 1) this->index = 0;
	if ((justPressed & RIGHT_BUTTON) && this->index == 0) this->index = 1;

	if (justPressed & A_BUTTON) {
		if (this->index == 0) {
			machine.changeState(GameStateType::PlayGame); 
		}
		if (this->index == 1) {
			machine.changeState(GameStateType::TitleScreen); 
		}
	}

}


// ----------------------------------------------------------------------------
//  Render the state .. 
//
void GameOverState::render(StateMachine & machine) {

	auto & arduboy = machine.getContext().arduboy;

	Message message;

	if (this->index < 2) {
		SpritesB::drawExternalMask((this->index == 0 ? 9 : 40), 31, Images::Hand, Images::Hand_Mask, 0, 0);
	}

	DealerFace dealerFace = DealerFace::Normal;
	if (this->index == 0) dealerFace = DealerFace::Happy;
	if (this->index == 1) dealerFace = DealerFace::Sad;

	this->drawDealer(machine, 78, 12, dealerFace, message);
	arduboy.fillRect(0, 60, 128, 63, BLACK);
	arduboy.drawFastHLine(0, 61, 128);
	arduboy.drawHorizontalDottedLine(0, 127, 63);

}

