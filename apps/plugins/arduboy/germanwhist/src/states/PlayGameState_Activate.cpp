#include "PlayGameState.h"
#include "../images/Images.h"
#include "../utils/CardUtils.h"


// ----------------------------------------------------------------------------
//  Initialise state ..
//
void PlayGameState::activate(StateMachine & machine) {

	auto & arduboy = machine.getContext().arduboy;
	auto & gameStats = machine.getContext().gameStats;
	auto & player1 = gameStats.player1;
	auto & player2 = gameStats.player2;
	auto & deck = gameStats.deck;

	deck.shuffle();
	resetHand(machine);
	player1.resetHand(true);
	player2.resetHand(true);

	this->viewState = ViewState::PhaseOne_Start;
	this->firstHand = true;
	this->handNo = 1;
	gameStats.playerDealer = WhichPlayer::Player2;

}

