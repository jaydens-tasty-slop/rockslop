#include "GameOverState.h"
#include "../images/Images.h"


// ----------------------------------------------------------------------------
//  Initialise state ..
//
void GameOverState::activate(StateMachine & machine) {
	
	this->winner = getWinner(machine);

}


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void GameOverState::update(StateMachine & machine) {

	auto & arduboy = machine.getContext().arduboy;
  auto justPressed = arduboy.justPressedButtons();
  auto pressed = arduboy.pressedButtons();


	this->r++; if (this->r > 48) this->r = 0;
	this->g++; if (this->g > 48) this->g = 0;
	this->b++; if (this->b > 48) this->b = 0;

	
	// Handle other input ..

	if (justPressed & A_BUTTON) {
		arduboy.setRGBled(0, 0, 0);
		machine.changeState(GameStateType::HighScore); 
	}

}


// ----------------------------------------------------------------------------
//  Render the state .. 
//
void GameOverState::render(StateMachine & machine) {

	auto & arduboy = machine.getContext().arduboy;
	auto & gameStats = machine.getContext().gameStats;
	
	SpritesB::drawOverwrite(8, 0, Images::Winner, 0);
	arduboy.drawCompressed(32, 29, Images::Border_No_Shadow, WHITE);
	arduboy.drawCompressed(33, 30, Images::Icons[gameStats.players[this->winner]->getIcon()], WHITE);

	renderMovingDice(machine, 47);

	font4x6.setCursor(54, 30);
	BaseState::printName4x6(machine, this->winner);
	arduboy.drawHorizontalDottedLine(54, 92, 38);
	font3x5.setCursor(54, 39);
  font3x5.print(F("Total"));
	BaseState::renderScore(machine, gameStats.players[this->winner]->score, 78, 39);


	arduboy.setRGBled(absT(this->r - 32), absT(this->g - 32), absT(this->b - 32));

}

