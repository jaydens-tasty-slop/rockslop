#include "PlayGameState.h"
#include "../images/Images.h"
#include "../utils/CardUtils.h"
#include "../utils/Utils.h"
#include "../utils/Enums.h"


void PlayGameState::resetPlay(StateMachine & machine) {

	auto & gameStats = machine.getContext().gameStats;
	auto & player1 = gameStats.player1;
	auto & player2 = gameStats.player2;

}


void PlayGameState::resetHand(StateMachine & machine) {

	auto & gameStats = machine.getContext().gameStats;
	auto & player1 = gameStats.player1;
	auto & player2 = gameStats.player2;
	auto & deck = gameStats.deck;

	player1.resetHand(false);
	player2.resetHand(false);
	deck.shuffle();
	
}


void PlayGameState::saveMessage(String message, uint8_t lines, DealerFace dealerFace, BubbleAlignment alignment) {

	saveMessage(message, lines, 72, dealerFace, alignment);

}


void PlayGameState::saveMessage(String message, uint8_t lines, uint8_t width, DealerFace dealerFace, BubbleAlignment alignment) {

	this->message.message = message;
	this->message.lines= lines;
	this->message.width= width;
	this->message.dealerFace = dealerFace;
	this->message.alignment = alignment;
	this->message.renderRequired = true;

}


void PlayGameState::skipSequence(StateMachine & machine, uint8_t counter) {

	auto & arduboy = machine.getContext().arduboy;
	auto justPressed = arduboy.justPressedButtons();

	if (justPressed & A_BUTTON) {

		if (isEndOfGame(machine)) {
			this->counter = 0;
			this->viewState = ViewState::EndOfGame;
		}
		else {
			this->counter = counter;
		}

	}

}

bool PlayGameState::isEndOfGame(StateMachine & machine) {

	return false;

}


WhichPlayer PlayGameState::whoWonTheTrick(StateMachine & machine) {

	auto & gameStats = machine.getContext().gameStats;

	Suit player1CardSuit = CardUtils::getCardSuit(this->player1Card);
	Suit player2CardSuit = CardUtils::getCardSuit(this->player2Card);
	uint8_t player1CardVal = CardUtils::getCardValue(this->player1Card);
	uint8_t player2CardVal = CardUtils::getCardValue(this->player2Card);
	
	if (player1CardSuit == player2CardSuit) {

		return (player1CardVal > player2CardVal ? WhichPlayer::Player1 : WhichPlayer::Player2);

	}

	if (player1CardSuit == this->trumps) return WhichPlayer::Player1;

	if (player2CardSuit == this->trumps) return WhichPlayer::Player2;

	return (gameStats.playerDealer == WhichPlayer::Player1 ? WhichPlayer::Player2 : WhichPlayer::Player1);

}

bool PlayGameState::isValidPlay(StateMachine & machine) {

  auto & gameStats = machine.getContext().gameStats;
	auto & player1 = gameStats.player1;

	bool haveSuit = false;
	if (this->player2Card != Constants::NoCard) {

		Suit suitToFollow = CardUtils::getCardSuit(this->player2Card);

		for (uint8_t x = 0; x < player1.getCardCount(); x++) {

			if (CardUtils::getCardSuit(player1.getCard(x)) == suitToFollow) {
				haveSuit = true;
				break;
			}

		}

		Suit suitToCheck = CardUtils::getCardSuit(player1.getCard(this->highlightCard));

		if (haveSuit && (suitToCheck!= suitToFollow)) {
			return false;
		}

	}

	return true;

}
