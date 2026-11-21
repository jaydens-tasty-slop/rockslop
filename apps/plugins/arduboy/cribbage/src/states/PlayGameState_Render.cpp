#include "PlayGameState.h"
#include "../images/Images.h"
#include "../utils/CardUtils.h"


constexpr const static uint8_t DEALER_COMMENT_LENGTH = 64;
constexpr const static uint8_t DEALER_BLINK_IMAGE = 3;
constexpr const static uint8_t DEALER_COMMENT_YPOS_TOP = 6;
constexpr const static uint8_t DEALER_COMMENT_YPOS_MID = 19;
constexpr const static uint8_t DEALER_COMMENT_YPOS_BOT = 33;

constexpr const static uint8_t CARD_LARGE_SPACING = 10;
constexpr const static uint8_t CARD_LARGE_SPACING_DEALER = 10;
constexpr const static uint8_t CARD_LARGE_SPACING_FULL = 22;
constexpr const static uint8_t CARD_LARGE_SPACING_ROTATED = 9;
constexpr const static uint8_t CARD_HAND_SPACING = 12;
constexpr const static uint8_t CARD_DEALER_CENTER = 54;
constexpr const static uint8_t CARD_PLAYER_CENTER = 52;
constexpr const static uint8_t CARD_LARGE_TOP_PLAYER = 41;
constexpr const static uint8_t CARD_SMALL_TOP_PLAYER = 37;
constexpr const static uint8_t CARD_LARGE_TOP_DEALER = 0;
constexpr const static uint8_t PLAY_CENTER = 62;
  

// ---------------------------------------------------------------------------------------------------------------------------
//  Render the state .. 
// ---------------------------------------------------------------------------------------------------------------------------
// 
void PlayGameState::render(StateMachine & machine) {

	auto & arduboy = machine.getContext().arduboy;
  auto & gameStats = machine.getContext().gameStats;
	auto & player1 = gameStats.player1;
	auto & player2 = gameStats.player2;

  bool flashScore = arduboy.getFrameCountHalf(32);

	switch (this->viewState) {

    case ViewState::DisplayScore_Board:
    case ViewState::DisplayScore_Other:
    case ViewState::DisplayScore_Dealer:
    case ViewState::DisplayScore_Crib:
    case ViewState::EndOfGame:
      break;

    default:
      drawDealer(machine, 88, 18, this->message.dealerFace, this->message);
      drawPlayerHands(machine);
      arduboy.drawFastHLine(0, 61, 128);
      arduboy.drawFastHLine(0, 60, 128, BLACK);
      arduboy.drawHorizontalDottedLine(0, 127, 63);
      break;

  }

	switch (this->viewState) {

		case ViewState::DealCards:
		case ViewState::DiscardCribComputer:
      drawCrib(machine, this->cribState);
			break;

		case ViewState::DiscardCribPlayer:
      drawHighlight(machine, this->highlightCard);
      drawCrib(machine, this->cribState);
			break;

    case ViewState::TurnUp:
      drawTurnUp((this->turnUp != Constants::NoCard ? TurnUpState::Visible : TurnUpState::Hidden));
			break;

    case ViewState::PlayersTurn:
    case ViewState::PlayersTurn_Go:
    case ViewState::PlayersTurn_Normal:
      drawTurnUp(TurnUpState::Visible);
      if (this->counter == 0) drawHighlight(machine, this->highlightCard);
      drawPlay();
			break;

    case ViewState::ComputersTurn:
      drawTurnUp(TurnUpState::Visible);
      drawPlay();
			break;

    case ViewState::DisplayScore_Board:
    case ViewState::EndOfGame:
      {
        drawDealer(machine, -1, 22, player1.getScore() < player2.getScore() ? DealerFace::Happy : DealerFace::Sad, this->message);
        SpritesB::drawSelfMasked(43, 0, Images::Divider, 0);
        SpritesB::drawSelfMasked(51, 7, Images::Board, 0);

        bool playerFlash1 = (player1.getPrevScore() != player1.getScore()) && this->highlight;
        bool playerFlash2 = (player2.getPrevScore() != player2.getScore()) && this->highlight;
        this->drawPlayer_Upper(player2.getPrevScore(), this->player2Counter, playerFlash2);
        this->drawPlayer_Lower(player1.getPrevScore(), this->player1Counter, playerFlash1);
        this->drawScores_TopLeft(machine, true, true);

      }
      break;

    case ViewState::DisplayScore_Other:
    case ViewState::DisplayScore_Dealer:
    case ViewState::DisplayScore_Crib:
      {

        bool happy = (this->viewState == ViewState::DisplayScore_Other && gameStats.playerDealer == WhichPlayer::Player1) ||
                     (this->viewState == ViewState::DisplayScore_Dealer && gameStats.playerDealer == WhichPlayer::Player2) ||
                     (this->viewState == ViewState::DisplayScore_Crib && gameStats.playerDealer == WhichPlayer::Player2);

        drawDealer(machine, -1, 22, happy ? (gameStats.getNumberOfScores() != 0 ? DealerFace::Happy : DealerFace::Sad) : DealerFace::Sad, this->message);
        SpritesB::drawSelfMasked(43, 0, Images::Divider, 0);


        // Only show scores if there is no message to be shown ..

        if (!this->message.renderRequired) {


          // Flash the appropriate player's score ..

          switch (this->viewState) {

            case ViewState::DisplayScore_Other:
              if (gameStats.playerDealer == WhichPlayer::Player1) {
                this->drawScores_TopLeft(machine, true, flashScore | (this->counter > 184));
              }
              else {
                this->drawScores_TopLeft(machine, flashScore | (this->counter > 184), true);
              }
              break;

            case ViewState::DisplayScore_Dealer:
              if (gameStats.playerDealer == WhichPlayer::Player1) {
                this->drawScores_TopLeft(machine, flashScore | (this->counter > 184), true);
              }
              else {
                this->drawScores_TopLeft(machine, true, flashScore | (this->counter > 184));
              }
              break;

            case ViewState::DisplayScore_Crib: 
              if (gameStats.playerDealer == WhichPlayer::Player1) {
                this->drawScores_TopLeft(machine, flashScore | (this->counter > 184), true);
              }
              else {
                this->drawScores_TopLeft(machine, true, flashScore | (this->counter > 184));
              }
              break;

            default: break;

          }

        }

        switch (this->counter) {

          case 78:
            drawHandScores(machine);
            break;

          default:

            SpritesB::drawSelfMasked(51, 7, Images::Board, 0);
            bool playerFlash1 = (player1.getPrevScore() != player1.getScore()) && this->highlight;
            bool playerFlash2 = (player2.getPrevScore() != player2.getScore()) && this->highlight;
            this->drawPlayer_Upper(player2.getPrevScore(), this->player2Counter, playerFlash2);
            this->drawPlayer_Lower(player1.getPrevScore(), this->player1Counter, playerFlash1);
            break;

        }

      }
      break;

	}

  switch (this->viewState) {
    
    case ViewState::DisplayScore_Board:
    case ViewState::DisplayScore_Other:
    case ViewState::DisplayScore_Dealer:
    case ViewState::DisplayScore_Crib:
    case ViewState::EndOfGame:
      break;

    default:

      // Only show top score if there is no dealer message to be shown ..
      
      if (!this->message.renderRequired || (this->message.renderRequired && (this->message.alignment == BubbleAlignment::Normal_Player|| this->message.alignment == BubbleAlignment::Score_Computer))) {
        drawScore(machine, 114, -1, player2.getScore(), true);
      }
      drawScore(machine, 114, 52, player1.getScore(), true);
      break;

  }

  if (this->message.renderRequired) {
    drawMessageBox(machine, this->message.message, this->message.lines, this->message.width, this->message.xPos, this->message.alignment);
    this->message.renderRequired = false;
  }

  if (this->showTotal) {

    arduboy.fillRect(42, 26, 42, 11, BLACK);
    arduboy.fillRect(43, 27, 40, 9, WHITE);
    font3x5.setCursor(45, 28);
    font3x5.print(F(" Score: "));
    font3x5.print(this->getBoardValue()< 10 ? "0" : "");
    font3x5.print(this->getBoardValue());

  }

}


// ---------------------------------------------------------------------------------------------------------------------------
//  Render the scoring sequences from hand or crib in RHS ..
// ---------------------------------------------------------------------------------------------------------------------------
// 
void PlayGameState::drawHandScores(StateMachine & machine) {

#define SCORE_LINE_HEIGHT 8
#define SCORE_ROW_TOP 27
#define SCORE_CARD_TOP 1

	auto & arduboy = machine.getContext().arduboy;
  auto & gameStats = machine.getContext().gameStats;
	auto & player1 = gameStats.player1;
	auto & player2 = gameStats.player2;

  // Render hand details ..

  font3x5.setCursor(4, 8);
  arduboy.fillRect(1, 8, 39, 7, WHITE);

  switch (this->viewState) {

    case ViewState::DisplayScore_Other:
    case ViewState::DisplayScore_Dealer:

      if ((gameStats.playerDealer == WhichPlayer::Player1 && this->viewState == ViewState::DisplayScore_Other) ||
          (gameStats.playerDealer == WhichPlayer::Player2 && this->viewState == ViewState::DisplayScore_Dealer)) {
        font3x5.print(F(" My Hand"));
      }
      else {
        font3x5.print(F("Your Hand"));
      }

      break;

    case ViewState::DisplayScore_Crib: 

      if (gameStats.playerDealer == WhichPlayer::Player1) {
        font3x5.print(F("Your Crib"));
      }
      else {
        font3x5.print(F(" My Crib"));
      }

      break;

    default: break;

  }

  font3x5.setTextColor(WHITE);



  uint8_t numberOfScores = gameStats.getNumberOfScores();
  uint8_t renderLine = 0;


  // Render hand ..

  this->drawSmallCard(49, SCORE_CARD_TOP, this->turnUp, false);

  for (uint8_t x = 0; x < 4; x++) {
        
    switch (this->viewState) {

      case ViewState::DisplayScore_Other:
      case ViewState::DisplayScore_Dealer:
        {
          uint8_t card = 0;
          if ((gameStats.playerDealer == WhichPlayer::Player1 && this->viewState == ViewState::DisplayScore_Other) ||
              (gameStats.playerDealer == WhichPlayer::Player2 && this->viewState == ViewState::DisplayScore_Dealer)) {
            card = player2.getOrigCard(x);
          }
          else {
            card = player1.getOrigCard(x);
          }
        
          this->drawSmallCard(68 + (x * 10), SCORE_CARD_TOP, card, x != 3);

        }

        break;

      case ViewState::DisplayScore_Crib: 
        {
          uint8_t card = 0;
          if (gameStats.playerDealer == WhichPlayer::Player1) {
            card = player1.getCribCard(x);
          }
          else {
            card = player2.getCribCard(x);
          }
          this->drawSmallCard(68 + (x * 10), SCORE_CARD_TOP, card, x != 3);
        }

        break;

      default: break;

    }

  }


  // Render upper arrow if needed ..

  if (numberOfScores > 5) {
    SpritesB::drawSelfMasked(120, 19, Images::Arrow_Up, !this->scoreUpperRow);
  }

  if (gameStats.getNumberOfScores() == 0) {

    font3x5.setCursor(72, 38);
    font3x5.print(F("Nothing !"));

  }
  else {
    
    for (uint8_t i = 0; i < numberOfScores; i++) {

      if (i >= this->scoreUpperRow && i <= this->scoreUpperRow + 3) {

        uint8_t handWidth = 0;

        for (uint8_t j = 0; j < 5; j++) {

          if (gameStats.scores[i].getCard(j) != Constants::NoCard) {

            handWidth++;
            uint8_t cardVal = CardUtils::getCardValue(gameStats.scores[i].getCard(j), false);
            SpritesB::drawSelfMasked(50 + (j * 14) - (cardVal == 10 ? 1 : 0), SCORE_ROW_TOP + (renderLine * SCORE_LINE_HEIGHT), Images::Pips[cardVal - 1], 0);
            SpritesB::drawSelfMasked(55 + (j * 14), SCORE_ROW_TOP + (renderLine * SCORE_LINE_HEIGHT), Images::Suits, static_cast<uint8_t>(CardUtils::getCardSuit(gameStats.scores[i].getCard(j))));

          }

        } 

        if (handWidth < 5) {
          arduboy.drawHorizontalDottedLine(50 + (handWidth * 14), 114, SCORE_ROW_TOP + 5 + (renderLine * SCORE_LINE_HEIGHT));
        }

        uint8_t score = gameStats.scores[i].getScore();
        font3x5.setCursor(120, SCORE_ROW_TOP + (renderLine * SCORE_LINE_HEIGHT) - 1);
        if (score < 10) font3x5.print("  ");
        font3x5.print(score);
        renderLine++;

      }

    }


    // Render lower arrow ..

    if (numberOfScores > 3) {
      SpritesB::drawSelfMasked(120, 60, Images::Arrow_Down, !(this->scoreUpperRow + 3 < numberOfScores));
    }

    if (this->scoreUpperRow + 3 >= numberOfScores) {
      font3x5.setCursor(93, SCORE_ROW_TOP + (renderLine * SCORE_LINE_HEIGHT));
      font3x5.print(F("Total:"));
      font3x5.setCursor(120, SCORE_ROW_TOP + (renderLine * SCORE_LINE_HEIGHT));
      if (gameStats.getScoresTotal() < 10) font3x5.print("  ");
      font3x5.print(gameStats.getScoresTotal());
    }

  }
      
  font3x5.setTextColor(BLACK);

}


// ---------------------------------------------------------------------------------------------------------------------------
//  Render the player's anc domuter's hand ..
// ---------------------------------------------------------------------------------------------------------------------------
// 
void PlayGameState::drawPlayerHands(StateMachine & machine) {

  auto & gameStats = machine.getContext().gameStats;


  // Player ..

  {
    uint8_t cardCount = gameStats.player1.getHandCardCount();
    uint8_t widthTot = (cardCount * CARD_LARGE_SPACING) + (CARD_LARGE_SPACING_FULL - CARD_LARGE_SPACING);
    uint8_t leftHand = CARD_PLAYER_CENTER - (widthTot / 2);

    for (uint8_t x = 0; x < cardCount; x++) {
      
      uint8_t card = gameStats.player1.getHandCard(x);

      if (x < cardCount - 1) {
        drawCard(leftHand + (x * CARD_LARGE_SPACING), CARD_LARGE_TOP_PLAYER, card, false);   
      }
      else {
        drawCard(leftHand + (x * CARD_LARGE_SPACING), CARD_LARGE_TOP_PLAYER, card, true);   
      }
      
    }

  }



  // Computer ..

  {
    uint8_t cardCount = gameStats.player2.getHandCardCount();
    uint8_t width = (cardCount * CARD_LARGE_SPACING_DEALER) + (CARD_LARGE_SPACING_FULL - CARD_LARGE_SPACING_DEALER);
    uint8_t rightHandSide = CARD_DEALER_CENTER + (width / 2);

    for (uint8_t x = 0; x < cardCount; x++) {
        
      if (x < cardCount - 1) {

        drawComputerCard(rightHandSide - (x * CARD_LARGE_SPACING_DEALER), CARD_LARGE_TOP_DEALER, false);   

      }
      else {

        drawComputerCard(rightHandSide - (x * CARD_LARGE_SPACING_DEALER) + 2, CARD_LARGE_TOP_DEALER, true);   

      }
      
    }

  }
      
}


// ---------------------------------------------------------------------------------------------------------------------------
//  Draw a large card in the nominated top position.  If the card is the end card, draw full size otherwise partial.
// ---------------------------------------------------------------------------------------------------------------------------
// 
void PlayGameState::drawComputerCard(uint8_t xPos, uint8_t yPos, bool fullSizeCard) {

	if (fullSizeCard) {

		SpritesB::drawSelfMasked(xPos - CARD_LARGE_SPACING_FULL, yPos - 8, Images::Computer_Full, 0);

	}
	else {

	  SpritesB::drawSelfMasked(xPos - CARD_LARGE_SPACING, yPos - 8, Images::Computer_Half, 0);

	}

}


// ---------------------------------------------------------------------------------------------------------------------------
//  Draw a large card in the nominated lower position.  If the card is the end card, draw full size otherwise partial.
// ---------------------------------------------------------------------------------------------------------------------------
// 
void PlayGameState::drawCard(uint8_t xPos, uint8_t yPos, uint8_t card, bool fullSizeCard) {

	uint8_t cardNumber = card % 13;

  if (fullSizeCard) {

		SpritesB::drawSelfMasked(xPos, yPos, Images::Card_Outline_Full, 0);
		SpritesB::drawErase(xPos + 8, yPos + 11, Images::Suits, card / 13);
		SpritesB::drawErase(xPos + 3, yPos + 4, Images::Pips[cardNumber], 0);

	}
  else {

		SpritesB::drawSelfMasked(xPos, yPos, Images::Card_Outline_Half, 0);
		SpritesB::drawErase(xPos + 3, yPos + 11, Images::Suits, card / 13);
		SpritesB::drawErase(xPos + 3, yPos + 4, Images::Pips[cardNumber], 0);

  }

}


// ---------------------------------------------------------------------------------------------------------------------------
//  Draw a small card in the nominated position ..
// ---------------------------------------------------------------------------------------------------------------------------
// 
void PlayGameState::drawSmallCard(uint8_t xPos, uint8_t yPos, uint8_t card, bool leftAlign) {

	uint8_t cardNumber = CardUtils::getCardValue(card, false);
	Suit suit = CardUtils::getCardSuit(card);

  uint8_t xOffset = (leftAlign ? 4 : 6);

  SpritesB::drawExternalMask(xPos, yPos, Images::TurnUp, Images::TurnUp_Mask, 2, 0);
  SpritesB::drawErase(xPos + xOffset, yPos + 12, Images::Suits, static_cast<uint8_t>(suit));
  SpritesB::drawErase(xPos + xOffset + (cardNumber == 10 ? 0 : 1), yPos + 5, Images::Pips[cardNumber - 1], 0);

}


// ---------------------------------------------------------------------------------------------------------------------------
//  Draw the selection hand ..
// ---------------------------------------------------------------------------------------------------------------------------
// 
void PlayGameState::drawHighlight(StateMachine & machine, uint8_t hghlightCard) {

  auto & gameStats = machine.getContext().gameStats;

  uint8_t leftHand = 0;
  uint8_t widthTot = (gameStats.player1.getHandCardCount() * CARD_LARGE_SPACING) + (CARD_LARGE_SPACING_FULL - CARD_LARGE_SPACING);


  // Determine left hand side of each hand ..

  leftHand = CARD_PLAYER_CENTER - (widthTot / 2);
  SpritesB::drawExternalMask(leftHand + (hghlightCard * CARD_LARGE_SPACING) + 1, 51, Images::Hand, Images::Hand_Mask, 0, 0);

}


// ---------------------------------------------------------------------------------------------------------------------------
//  Draw the crib card(s) ..
// ---------------------------------------------------------------------------------------------------------------------------
// 
void PlayGameState::drawCrib(StateMachine & machine, CribState cribState) {

  auto & gameStats = machine.getContext().gameStats;
  Player player;

  if (gameStats.playerDealer == WhichPlayer::Player1) {
    player = gameStats.player1;
  }
  else {
    player = gameStats.player2;
  }


  uint8_t yPos = (gameStats.playerDealer == WhichPlayer::Player1 ? 37 : 0);

  switch (cribState) {

    case CribState::Empty:
      SpritesB::drawSelfMasked(0, yPos, Images::Crib_Blank, 0);
      break;

    case CribState::Hidden:

      if (player.getCribCardCount() > 0) {
        
        SpritesB::drawSelfMasked(0, yPos, Images::Crib_First, 0);

        for (uint8_t x = 1; x < player.getCribCardCount(); x++) {
      
          SpritesB::drawSelfMasked(2 + (x * 2), yPos + 2 + (x * 2), Images::Crib_Outline, 0);

        }

      }
      break;

    case CribState::Visible:
      break;

  }

}


// ---------------------------------------------------------------------------------------------------------------------------
//  Draw the turn up card ..
// ---------------------------------------------------------------------------------------------------------------------------
// 
void PlayGameState::drawTurnUp(TurnUpState turnUpState) {

  switch (turnUpState) {

    case TurnUpState::Hidden:
      SpritesB::drawSelfMasked(0, 12, Images::TurnUp, 1);
      SpritesB::drawSelfMasked(5, 16, Images::TurnUp_Deck, 0);
      break;

    case TurnUpState::Visible:
      drawSmallCard(0, 12, this->turnUp, false);
      SpritesB::drawSelfMasked(5, 16, Images::TurnUp_Deck, 0);
      break;

    default: break;

  }

}


// ---------------------------------------------------------------------------------------------------------------------------
//  Render the cards currently in play ..
// ---------------------------------------------------------------------------------------------------------------------------
// 
void PlayGameState::drawPlay() {

  uint8_t xLeft = PLAY_CENTER - ((16 + (playIdx * 10)) / 2);

  for (uint8_t x = 0; x < 8; x++) {

    bool leftAlign = !(x == 8 || x == playIdx - 1);

    if (this->playedCards[x] != Constants::NoCard) {

      drawSmallCard(xLeft + (x * 10), 13, this->playedCards[x], leftAlign);

    }

  }

}


// ---------------------------------------------------------------------------------------------------------------------------
//  Render the upper player's progress on the cribbage board ..
// ---------------------------------------------------------------------------------------------------------------------------
// 
void PlayGameState::drawPlayer_Upper(uint8_t oldPosition, uint8_t newPosition, bool flash) {

  uint8_t oldX = pgm_read_byte(&Board_Positions_Player_1[oldPosition * 2]);
  uint8_t oldY = pgm_read_byte(&Board_Positions_Player_1[oldPosition * 2] + 1) + 3;

  uint8_t newX = pgm_read_byte(&Board_Positions_Player_1[newPosition * 2]);
  uint8_t newY = pgm_read_byte(&Board_Positions_Player_1[newPosition * 2] + 1) + 3;

  uint8_t oldFrame = 0;
  uint8_t newFrame = 0;

  if (oldY >= 128) {
    oldFrame = 1;
    oldY = oldY & 0x7F;
  }

  if (newY >= 128) {
    newFrame = 1;
    newY = newY & 0x7F;
  }

  SpritesB::drawExternalMask(oldX, oldY, Images::Peg, Images::Peg_Mask, 0, oldFrame);
  if (flash) SpritesB::drawExternalMask(newX, newY, Images::Peg, Images::Peg_Mask, 0, newFrame);

}


// ---------------------------------------------------------------------------------------------------------------------------
//  Render the lower player's progress on the cribbage board ..
// ---------------------------------------------------------------------------------------------------------------------------
// 
void PlayGameState::drawPlayer_Lower(uint8_t oldPosition, uint8_t newPosition, bool flash) {

  uint8_t oldX = pgm_read_byte(&Board_Positions_Player_2[oldPosition * 2]);
  uint8_t oldY = pgm_read_byte(&Board_Positions_Player_2[oldPosition * 2] + 1) + 3;

  uint8_t newX = pgm_read_byte(&Board_Positions_Player_2[newPosition * 2]);
  uint8_t newY = pgm_read_byte(&Board_Positions_Player_2[newPosition * 2] + 1) + 3;

  uint8_t oldFrame = 0;
  uint8_t newFrame = 0;

  if (oldY >= 128) {
    oldFrame = 1;
    oldY = oldY & 0x7F;
  }

  if (newY >= 128) {
    newFrame = 1;
    newY = newY & 0x7F;
  }

  SpritesB::drawExternalMask(oldX, oldY, Images::Peg, Images::Peg_Mask, 1, oldFrame);
  if (flash) SpritesB::drawExternalMask(newX, newY, Images::Peg, Images::Peg_Mask, 1, newFrame);

}


