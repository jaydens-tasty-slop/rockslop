#include "PlayGameState.h"
#include "../images/Images.h"
#include "../utils/CardUtils.h"


// ----------------------------------------------------------------------------
//  Handle state updates .. 
//
void PlayGameState::update(StateMachine & machine) {

	auto & arduboy = machine.getContext().arduboy;
	auto & gameStats = machine.getContext().gameStats;
	auto & player1 = gameStats.player1;
	auto & player2 = gameStats.player2;
	auto & deck = gameStats.deck;

  auto justPressed = arduboy.justPressedButtons();
  auto pressed = arduboy.pressedButtons();

	switch (this->viewState) {

		case ViewState::PhaseOne_Start:

			saveMessage(F("In this round we play\nto win the up-turned\ncard and build a great\nhand for phase two!"), 4, 92, DealerFace::Normal, BubbleAlignment::Left);

			if (justPressed & A_BUTTON) {

				this->counter = 0;
				this->player1HighlightCard = Constants::NoCard;
				this->viewState = ViewState::DealCards;	

			}
			break;

		case ViewState::DealCards:

			if (arduboy.everyXFrames(16) && player1.getCardCount() == 0) {
				
				resetHand(machine);
				gameStats.playerDealer = (gameStats.playerDealer == WhichPlayer::Player2 ? WhichPlayer::Player1 : WhichPlayer::Player2);
				
			}

			if (player1.getCardCount() < 13) {

				if (arduboy.everyXFrames(8)) {

					player1.addToHand(deck.getCard());
					player2.addToHand(deck.getCard());

				}

			}
			else {

				this->viewState = ViewState::TurnUp;
	
			}

			break;

		case ViewState::TurnUp:

			this->counter++;

			switch (this->counter) {

				case 0 ... 10:
					break;

				case 11:
					this->turnUp = deck.getCard();
					if (this->trumps == Suit::Nothing) {
						this->trumps = CardUtils::getCardSuit(this->turnUp);
					}
					break;

				case 12 ... 82:
					
					skipSequence(machine, 82);
					if (gameStats.playerDealer == WhichPlayer::Player2) {
						saveMessage(F("Your turn to start@"), 1, 81, DealerFace::Normal, BubbleAlignment::Centre);
					}
					else {
						saveMessage(F("My turn to start@"), 1, 71, DealerFace::Normal, BubbleAlignment::Centre);
					}
					
					break;

				case 83:
					
					this->highlightCard = 0;
					this->firstHand = false;
					this->counter = 0;

					if (gameStats.playerDealer == WhichPlayer::Player2) {
						this->viewState = ViewState::PhaseOne_PlayersTurn;
					}
					else {
						this->viewState = ViewState::PhaseOne_ComputersTurn;
					}

					break;

			}

			break;

		case ViewState::PhaseOne_PlayersTurn:

			// Highlight card won in previous round ..

			if (this->counter < 70) {
				this->counter++;
			}
			else {
				this->player1HighlightCard = Constants::NoCard;
			}


			// Handle user actions  ..

			if ((justPressed & LEFT_BUTTON) && (this->highlightCard > 0))													this->highlightCard--;
			if ((justPressed & RIGHT_BUTTON) && this->highlightCard < player1.getCardCount() - 1)	this->highlightCard++;

			if (justPressed & A_BUTTON) {

				if (!isValidPlay(machine)) break;

				this->player1HighlightCard = Constants::NoCard;
				this->player1Card = player1.removeFromHand(this->highlightCard);
				this->counter = 0;

				if (gameStats.playerDealer == WhichPlayer::Player1) {
					this->viewState = ViewState::PhaseOne_EndOfTrick;
				}
				else {
					this->viewState = ViewState::PhaseOne_ComputersTurn;
				}

				if (this->highlightCard == player1.getCardCount()) this->highlightCard--;

			}			

			break;

		case ViewState::PhaseOne_ComputersTurn:

			this->counter++;

			switch (this->counter) {

				case 0 ... 30:
					skipSequence(machine, 70);
					saveMessage(F("I will play@"), 1, 56, DealerFace::Normal, BubbleAlignment::Centre);
					break;

				case 31 ... 35:
					break;

				case 36:
					this->player2Card = player2.playCard_PhaseOne(this->trumps, this->turnUp, this->player1Card);
					player2.removeFromHand(player2.getCardIndex(this->player2Card));
					break;

				case 37 ... 42:
					break;

				case 43 ... 70:
					skipSequence(machine, 70);
					saveMessage(F("@this card."), 1, 54, DealerFace::Normal, BubbleAlignment::Centre);
					break;

				case 71:
					if (this->player2Card == Constants::NoCard) {
						this->player2Card = player2.playCard_PhaseOne(this->trumps, this->turnUp, this->player1Card);
						player2.removeFromHand(player2.getCardIndex(this->player2Card));
					}

					this->counter = 0;

					if (gameStats.playerDealer == WhichPlayer::Player2) {
						this->viewState = ViewState::PhaseOne_EndOfTrick;
					}
					else {
						this->viewState = ViewState::PhaseOne_PlayersTurn;
					}
					break;

			}

			break;

		case ViewState::PhaseOne_EndOfTrick:

			this->counter++;

			switch (this->counter) {

				case 0 ... 10:
					break;

				case 11 ... 80:
					skipSequence(machine, 80);
					if (this->whoWonTheTrick(machine) == WhichPlayer::Player1) {
						saveMessage(F("     Yours."), 1, 56, DealerFace::Normal, BubbleAlignment::Centre);
						this->player1Scored = 128;
					}
					else {
						saveMessage(F("      Mine."), 1, 54, DealerFace::Normal, BubbleAlignment::Centre);
						this->player2Scored = 128;
					}
					break;

				case 81:
					player2.addToPrev(this->player1Card);
					player2.addToPrev(this->player2Card);
					if (this->whoWonTheTrick(machine) == WhichPlayer::Player1) {
						player1.addToHand(this->turnUp);
						this->player1HighlightCard = player1.getCardIndex(this->turnUp);
						player2.addToHand(deck.getCard());
						gameStats.playerDealer = WhichPlayer::Player2;
						
					}
					else {
						uint8_t card = deck.getCard();
						player1.addToHand(card);
						this->player1HighlightCard = player1.getCardIndex(card);
						player2.addToHand(this->turnUp);
						gameStats.playerDealer = WhichPlayer::Player1;
					}

					if (this->handNo < 13) { //sjh
						// Reset hand ..
						this->counter = 0;
						this->player1Card = Constants::NoCard;
						this->player2Card = Constants::NoCard;
						this->turnUp = Constants::NoCard;;
						this->handNo++;
						this->viewState = ViewState::TurnUp;
					}

				case 82 ... 155:
					saveMessage(F(" Thats the end\n of phase one."), 2, 65, DealerFace::Normal, BubbleAlignment::Centre);
					skipSequence(machine, 155);
					break;

				case 156:
					this->viewState = ViewState::PhaseTwo_Start;
					break;

			}

			break;

		case ViewState::PhaseTwo_Start:

			saveMessage(F("In this round we play\nfor tricks to see who\n  wins the game.  Use\n your trumps wisely!"), 4, 88, DealerFace::Normal, BubbleAlignment::Left);

			if (justPressed & A_BUTTON) {

				this->handNo = 1;
				this->counter = 0;
				this->player1HighlightCard = Constants::NoCard;
				this->player1Card = Constants::NoCard;
				this->player2Card = Constants::NoCard;

				if (gameStats.playerDealer == WhichPlayer::Player2) {
					this->viewState = ViewState::PhaseTwo_PlayersTurn;	
				}
				else {
					this->viewState = ViewState::PhaseTwo_ComputersTurn;	
				}

			}
			break;


		case ViewState::PhaseTwo_PlayersTurn:

			if (this->counter < 70) {
				this->counter++;
			}
			else {
				this->player1HighlightCard = Constants::NoCard;
			}

			if ((justPressed & LEFT_BUTTON) && (this->highlightCard > 0))													this->highlightCard--;
			if ((justPressed & RIGHT_BUTTON) && this->highlightCard < player1.getCardCount() - 1)	this->highlightCard++;

			if (justPressed & A_BUTTON) {

				if (!isValidPlay(machine)) break;

				this->player1HighlightCard = Constants::NoCard;
				this->player1Card = player1.removeFromHand(this->highlightCard);
				this->counter = 0;

				if (this->highlightCard == player1.getCardCount()) this->highlightCard--;


				// If the player was unable to follow suit, record the fact ..

				if (player2Card != Constants::NoCard) {

					if (CardUtils::getCardSuit(player1Card) != CardUtils::getCardSuit(player2Card)) {
						player2.setSuitOpponent(CardUtils::getCardSuit(player2Card));
					}

				}


				// Can we play on ?

				if (gameStats.playerDealer == WhichPlayer::Player1) {
					this->viewState = ViewState::PhaseTwo_EndOfTrick;
				}
				else {
					this->viewState = ViewState::PhaseTwo_ComputersTurn;
				}

			}			

			break;

		case ViewState::PhaseTwo_ComputersTurn:

			this->counter++;

			switch (this->counter) {

				case 0 ... 30:
					skipSequence(machine, 70);
					saveMessage(F("I will play@"), 1, 56, DealerFace::Normal, BubbleAlignment::Centre);
					break;

				case 31 ... 35:
					break;

				case 36:
					this->player2Card = player2.playCard_PhaseTwo(this->trumps, this->player1Card);
					player2.removeFromHand(player2.getCardIndex(this->player2Card));
					break;

				case 37 ... 42:
					break;

				case 43 ... 70:
					skipSequence(machine, 70);
					saveMessage(F("@this card."), 1, 54, DealerFace::Normal, BubbleAlignment::Centre);
					break;

				case 71:
					if (this->player2Card == Constants::NoCard) {
						this->player2Card = player2.playCard_PhaseTwo(this->trumps, this->player1Card);
						player2.removeFromHand(player2.getCardIndex(this->player2Card));
					}

					this->counter = 0;

					if (gameStats.playerDealer == WhichPlayer::Player2) {
						this->viewState = ViewState::PhaseTwo_EndOfTrick;
					}
					else {
						this->viewState = ViewState::PhaseTwo_PlayersTurn;
					}
					break;

			}

			break;

		case ViewState::PhaseTwo_EndOfTrick:

			this->counter++;

			switch (this->counter) {

				case 0 ... 10:
					break;

				case 11:
					skipSequence(machine, 80);
					if (this->whoWonTheTrick(machine) == WhichPlayer::Player1) {
						saveMessage(F("     Yours."), 1, 56, DealerFace::Normal, BubbleAlignment::Centre);
						player1.incScore();
						this->player1Scored = 128;
						this->player2Scored = 0;
					}
					else {
						player2.incScore();
						saveMessage(F("      Mine."), 1, 54, DealerFace::Normal, BubbleAlignment::Centre);
						this->player1Scored = 0;
						this->player2Scored = 128;
					}
					break;

				case 12 ... 80:
					this->message.renderRequired = true;
					break;

				case 81:

					this->handNo++;

					if (this->whoWonTheTrick(machine) == WhichPlayer::Player1) {
						gameStats.playerDealer = WhichPlayer::Player2;
						this->viewState = ViewState::PhaseTwo_PlayersTurn;
					}
					else {
						gameStats.playerDealer = WhichPlayer::Player1;
						this->viewState = ViewState::PhaseTwo_ComputersTurn;
					}

					// Reset hand ..
					this->counter = 0;
					this->player1Card = Constants::NoCard;
					this->player2Card = Constants::NoCard;
					this->turnUp = Constants::NoCard;
					this->player1Scored = 0;
					this->player2Scored = 0;

					if (this->handNo > 13) { //SJH
						// player1.setScore(random(0, 14));//sjh
						// player2.setScore(13 - player1.getScore());//sjh
						this->viewState = ViewState::EndOfGame;
					}

			}

			break;

		
		case ViewState::EndOfGame:

			switch (this->counter) {

				case 0 ... 9:
					this->counter++;
					break;

				case 10:

					if (player1.getOverallScore() + player1.getScore() > Constants::GameOverScore || player2.getOverallScore() + player2.getScore() > Constants::GameOverScore) {

						if (player1.getOverallScore() + player1.getScore() > player2.getOverallScore() + player2.getScore()) {
							saveMessage(F("Congratulations@ you won!"), 1, 110, DealerFace::Sad, BubbleAlignment::Centre);
						}
						else {
							saveMessage(F("Close game but I won!"), 1, 100, DealerFace::Sad, BubbleAlignment::Centre);
						}

					}
					else {

						switch (player1.getScore()) {

							case 0 ... 3:
								saveMessage(F("Disappointing round@ for you!"), 1, 120, DealerFace::Happy, BubbleAlignment::Centre);
								break;

							case 4 ... 5:
								saveMessage(F("I just beat you!"), 1, 70, DealerFace::Happy, BubbleAlignment::Centre);
								break;

							case 6 ... 7:
								saveMessage(F("Close game!"), 1, 62, DealerFace::Normal, BubbleAlignment::Centre);
								break;

							case 8 ... 9:
								saveMessage(F(" You beat me!"), 1, 64, DealerFace::Sad, BubbleAlignment::Centre);
								break;

							default:
								saveMessage(F("  Nice work!"), 1, 56, DealerFace::Sad, BubbleAlignment::Centre);
								break;

						}

					}

					this->counter++;
					break;

				case 11 ... 200:

					if (this->counter < 80) this->message.renderRequired = true;
					if (this->counter < 200) this->counter++;

					if (player1.getScore() > 0 || player2.getScore() > 0) {

						if (arduboy.everyXFrames(16)) {

							if (player1.getScore() > 0) {
								player1.decScore();
								player1.incOverallScore();
							}

							if (player2.getScore() > 0) {
								player2.decScore();
								player2.incOverallScore();
							}
							
						}

					}

					if (justPressed & A_BUTTON) {

						player1.setOverallScore(player1.getOverallScore() + player1.getScore());
						player2.setOverallScore(player2.getOverallScore() + player2.getScore());
						player1.setScore(0);
						player2.setScore(0);

						this->counter = 0;
						
						if (player1.getOverallScore() < Constants::GameOverScore && player2.getOverallScore() < Constants::GameOverScore) { 
							deck.shuffle();
							player1.resetHand(false);
							player2.resetHand(false);
							this->trumps = Suit::Nothing;
							this->handNo = 1;
							this->player1HighlightCard = Constants::NoCard;
							this->viewState = ViewState::DealCards;	
						}
						else {
							machine.changeState(GameStateType::TitleScreen);  
						}

					}

					break;

			}
					
			break;

	}

}