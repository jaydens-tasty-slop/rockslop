#include "PlayGameState.h"
#include "../images/Images.h"
#include "../utils/CardUtils.h"


constexpr const static uint8_t DEALER_COMMENT_LENGTH = 64;
constexpr const static uint8_t DEALER_BLINK_IMAGE = 3;
constexpr const static uint8_t DEALER_COMMENT_YPOS_TOP = 6;
constexpr const static uint8_t DEALER_COMMENT_YPOS_MID = 19;
constexpr const static uint8_t DEALER_COMMENT_YPOS_BOT = 33;

constexpr const static uint8_t CARD_LARGE_SPACING = 10;
constexpr const static uint8_t CARD_LARGE_SPACING_DEALER = 6;
constexpr const static uint8_t CARD_LARGE_SPACING_FULL = 15;
constexpr const static uint8_t CARD_LARGE_SPACING_ROTATED = 9;
constexpr const static uint8_t CARD_HAND_SPACING = 12;
constexpr const static uint8_t CARD_DEALER_CENTER = 74;
constexpr const static uint8_t CARD_PLAYER_CENTER = 65;
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

	switch (this->viewState) {

    case ViewState::PhaseOne_Start:
      drawDealer(machine, 88, 12, this->message.dealerFace, this->message);
      drawTableTop(machine);
      break;

    case ViewState::DealCards:
      drawDealer(machine, 88, 12, this->message.dealerFace, this->message);
      drawPlayerHands(machine);
      drawTableTop(machine);
      break;

    case ViewState::TurnUp:
      if (this->counter < 12) drawPhaseOne_Scores(machine);
      drawDealer(machine, 88, 12, this->message.dealerFace, this->message);
      drawPlayerHands(machine);
      drawTableTop(machine);
      drawTurnUp(0, 13, this->turnUp);
      break;

    case ViewState::PhaseOne_PlayersTurn:
    case ViewState::PhaseOne_ComputersTurn:
      drawPhaseOne_Scores(machine);
      drawDealer(machine, 88, 12, this->message.dealerFace, this->message);
      drawPlayerHands(machine);
      drawTableTop(machine);
      drawTurnUp(0, 13, this->turnUp);
      if (this->viewState == ViewState::PhaseOne_PlayersTurn) drawHighlight(machine, this->highlightCard);
      drawTrick(machine);
      break;

    case ViewState::PhaseOne_EndOfTrick:
      if (this->counter < 82) drawPhaseOne_Scores(machine);
      drawDealer(machine, 88, 12, this->message.dealerFace, this->message);
      drawPlayerHands(machine);
      drawTableTop(machine);
      if (this->counter < 82) drawTrick(machine);
      break;

    case ViewState::PhaseTwo_Start:
      drawDealer(machine, 88, 12, this->message.dealerFace, this->message);
      drawTableTop(machine);
      break;

    case ViewState::PhaseTwo_PlayersTurn:
    case ViewState::PhaseTwo_ComputersTurn:
      drawPhaseTwo_Scores(machine);
      drawDealer(machine, 88, 12, this->message.dealerFace, this->message);
      drawPlayerHands(machine);
      drawTableTop(machine);
      if (this->viewState == ViewState::PhaseTwo_PlayersTurn) drawHighlight(machine, this->highlightCard);
      drawTrick(machine);
      break;

    case ViewState::PhaseTwo_EndOfTrick:
      drawPhaseTwo_Scores(machine);
      drawDealer(machine, 88, 12, this->message.dealerFace, this->message);
      drawPlayerHands(machine);
      drawTableTop(machine);
      drawTrick(machine);
      break;

    case ViewState::EndOfGame:
      drawDealer(machine, 88, 12, this->message.dealerFace, this->message);
      drawTableTop(machine);

      arduboy.fillRect(Constants::OverallScore_X - 2, Constants::OverallScore_Y - 11, 32, 7, WHITE);
      font3x5.setCursor(Constants::OverallScore_X + 11, Constants::OverallScore_Y - 11);
      font3x5.print("Me");

      arduboy.fillRect(Constants::OverallScore_X + 34, Constants::OverallScore_Y - 11, 32, 7, WHITE);
      font3x5.setCursor(Constants::OverallScore_X + 44, Constants::OverallScore_Y - 11);
      font3x5.print("You");

      drawNumber(machine, Constants::OverallScore_X + 0, Constants::OverallScore_Y + 0, player2.getOverallScore() / 10);
      drawNumber(machine, Constants::OverallScore_X + 15, Constants::OverallScore_Y + 0, player2.getOverallScore() % 10);

      SpritesB::drawSelfMasked(Constants::OverallScore_X + 31, Constants::OverallScore_Y + 8, Images::Bullet, 0);
      SpritesB::drawSelfMasked(Constants::OverallScore_X + 31, Constants::OverallScore_Y + 13, Images::Bullet, 0);

      drawNumber(machine, Constants::OverallScore_X + 36, Constants::OverallScore_Y + 0, player1.getOverallScore() / 10);
      drawNumber(machine, Constants::OverallScore_X + 51, Constants::OverallScore_Y + 0, player1.getOverallScore() % 10);
      break;

  }

  if (this->message.renderRequired) {
    drawMessageBox(machine, this->message.message, this->message.lines, this->message.width, this->message.alignment);
    this->message.renderRequired = false;
  }

}


// ---------------------------------------------------------------------------------------------------------------------------
//  Draw phase one scores ..
// ---------------------------------------------------------------------------------------------------------------------------
// 
void PlayGameState::drawPhaseOne_Scores(StateMachine & machine) {

	auto & arduboy = machine.getContext().arduboy;

  if (this->trumps != Suit::Nothing) {
    arduboy.fillRect(0, 0, 34, 8);
    font3x5.setCursor(1, 0);
    font3x5.print("Trumps");
    SpritesB::drawErase(27, 1, Images::Suits, static_cast<uint8_t>(this->trumps));
  }

  arduboy.fillRect(93, 0, 34, 8);
  font3x5.setCursor(96, 0);
  if (this->handNo < 10 ) font3x5.print("0");
  font3x5.print(this->handNo);
  font3x5.print(" of 13");

}


// ---------------------------------------------------------------------------------------------------------------------------
//  Draw phase two scores ..
// ---------------------------------------------------------------------------------------------------------------------------
// 
void PlayGameState::drawPhaseTwo_Scores(StateMachine & machine) {

	auto & arduboy = machine.getContext().arduboy;
  auto & gameStats = machine.getContext().gameStats;
	auto & player1 = gameStats.player1;
	auto & player2 = gameStats.player2;

  bool flashScore = arduboy.getFrameCountHalf(32);

  if (this->trumps != Suit::Nothing) {
    arduboy.fillRect(0, 0, 34, 8);
    font3x5.setCursor(1, 0);
    font3x5.print("Trumps");
    SpritesB::drawErase(27, 1, Images::Suits, static_cast<uint8_t>(this->trumps));
  }

  if (!this->message.renderRequired || this->message.alignment != BubbleAlignment::Right) {

    arduboy.fillRect(93, 0, 34, 8);

    if (this->viewState == ViewState::PhaseTwo_EndOfTrick && flashScore && this->player2Scored > 0) {
      font3x5.setCursor(107, 0);
      this->player2Scored--;
    }
    else {
      font3x5.setCursor(99, 0);
      if (player2.getScore() < 10 ) font3x5.print("0");
      font3x5.print(player2.getScore());
    }
    font3x5.print(" - ");
    if (this->viewState == ViewState::PhaseTwo_EndOfTrick && flashScore && this->player1Scored > 0) {
      this->player1Scored--;
    }
    else {
      if (player1.getScore() < 10 ) font3x5.print("0");
      font3x5.print(player1.getScore());
    }

  }

}


// ---------------------------------------------------------------------------------------------------------------------------
//  Draw trick in the correct order played ..
// ---------------------------------------------------------------------------------------------------------------------------
// 
void PlayGameState::drawTrick(StateMachine & machine) {

  auto & gameStats = machine.getContext().gameStats;

  if (gameStats.playerDealer == WhichPlayer::Player1) {
    if (this->player2Card != Constants::NoCard) drawCard(59, 15, player2Card, false);
    if (this->player1Card != Constants::NoCard) drawCard(48, 17, player1Card, false);
  }
  else {
    if (this->player1Card != Constants::NoCard) drawCard(48, 17, player1Card, false);
    if (this->player2Card != Constants::NoCard) drawCard(59, 15, player2Card, false);
  }

}


// ---------------------------------------------------------------------------------------------------------------------------
//  Draw table top ..
// ---------------------------------------------------------------------------------------------------------------------------
// 
void PlayGameState::drawTableTop(StateMachine & machine) {

  auto & arduboy = machine.getContext().arduboy;
  arduboy.fillRect(0, 60, 128, 64, BLACK);
  arduboy.drawFastHLine(0, 61, 128);
  arduboy.drawHorizontalDottedLine(0, 127, 63);

}


// ---------------------------------------------------------------------------------------------------------------------------
//  Render the player's and computer's hand ..
// ---------------------------------------------------------------------------------------------------------------------------
// 
void PlayGameState::drawPlayerHands(StateMachine & machine) {

  auto & gameStats = machine.getContext().gameStats;
  auto & player1 = gameStats.player1;

  // Player ..

  {
    uint8_t cardCount = player1.getCardCount();
    uint8_t leftHand = (player1.getCardCount() >= 12 ? -2 : -1);// tCARD_PLAYER_CENTER - (widthTot / 2);

    for (uint8_t x = 0; x < cardCount; x++) {
      
      uint8_t card = player1.getCard(x);

      if (x < cardCount - 1 || gameStats.player1.getCardCount() == 13) {
        drawCard(leftHand + (x * CARD_LARGE_SPACING), CARD_LARGE_TOP_PLAYER - (this->player1HighlightCard == x ? 4 : 0), card, true);   
      }
      else {
        drawCard(leftHand + (x * CARD_LARGE_SPACING) + 1, CARD_LARGE_TOP_PLAYER - (this->player1HighlightCard == x ? 4 : 0), card, false);   
      }
      
    }

  }

}


void PlayGameState::drawTurnUp(int8_t xPos, uint8_t yPos, uint8_t card) {

  if (card == Constants::NoCard) return;

	uint8_t cardNumber = CardUtils::getCardValue(card);
	Suit suit = CardUtils::getCardSuit(card);

  SpritesB::drawSelfMasked(xPos, yPos, Images::Card_Back, 0);
  SpritesB::drawSelfMasked(xPos + 7, yPos, Images::Card_Blank, 0);
  SpritesB::drawErase(xPos + 13, yPos + 12, Images::Suits, static_cast<uint8_t>(suit));
  SpritesB::drawErase(xPos + 13 + (cardNumber == 10 ? 0 : 1), yPos + 5, Images::Pips[cardNumber - 2], 0);

}


void PlayGameState::drawCard(int8_t xPos, uint8_t yPos, uint8_t card, bool leftAlign) {

	uint8_t cardNumber = CardUtils::getCardValue(card);
	Suit suit = CardUtils::getCardSuit(card);

  uint8_t xOffset = (leftAlign ? 4 : 6);

  SpritesB::drawExternalMask(xPos, yPos, Images::Card_Blank, Images::Card_Mask, 0, 0);
  SpritesB::drawErase(xPos + xOffset, yPos + 12, Images::Suits, static_cast<uint8_t>(suit));
  SpritesB::drawErase(xPos + xOffset + (cardNumber == 10 ? 0 : 1), yPos + 5, Images::Pips[cardNumber - 2], 0);

}


void PlayGameState::drawHighlight(StateMachine & machine, uint8_t hghlightCard) {

  auto & gameStats = machine.getContext().gameStats;
	auto & player1 = gameStats.player1;

  int8_t leftHand = (player1.getCardCount() >= 12 ? -2 : -1);
  SpritesB::drawExternalMask(leftHand + (hghlightCard * CARD_LARGE_SPACING) + 1, 51, Images::Hand, Images::Hand_Mask, 0, 0);

}


void PlayGameState::drawComputerCard(uint8_t xPos, uint8_t yPos, bool fullSizeCard) {

	if (fullSizeCard) {

		SpritesB::drawSelfMasked(xPos - CARD_LARGE_SPACING_FULL, yPos, Images::Computer_Full, 0);

	}
	else {

	  SpritesB::drawSelfMasked(xPos - CARD_LARGE_SPACING, yPos, Images::Computer_Half, 0);

	}

}

	const uint8_t PROGMEM Segments[] = {
		63, 6, 219, 207, 102, 109, 253, 7, 255, 231
  };

void PlayGameState::drawNumber(StateMachine & machine, uint8_t x, uint8_t y, uint8_t number) {

	uint8_t segmentData = pgm_read_byte(&Segments[number]);

	if (segmentData & 1)	SpritesB::drawSelfMasked(x + 3, y, Images::Number_Horizontal, 0);
	if (segmentData & 2)	SpritesB::drawSelfMasked(x + 10, y + 2, Images::Number_Vert, 0);
	if (segmentData & 4)	SpritesB::drawSelfMasked(x + 10, y + 12, Images::Number_Vert, 0);
	if (segmentData & 8)	SpritesB::drawSelfMasked(x + 3, y + 20, Images::Number_Horizontal, 0);
	if (segmentData & 16)	SpritesB::drawSelfMasked(x, y + 12, Images::Number_Vert, 0);
	if (segmentData & 32)	SpritesB::drawSelfMasked(x, y + 2, Images::Number_Vert, 0);
	if (segmentData & 64)	SpritesB::drawSelfMasked(x + 3, y + 10, Images::Number_Horizontal, 0);

}