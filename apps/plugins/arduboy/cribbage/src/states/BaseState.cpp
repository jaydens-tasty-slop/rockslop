#include "BaseState.h"

#include "../images/Images.h"
#include "../utils/Utils.h"
#include "../utils/Enums.h"
#include "../fonts/Font3x5.h"

void BaseState::drawMessageBox(StateMachine &machine, String message, uint8_t lines, uint8_t width, uint8_t xPos, BubbleAlignment alignment) {

	auto & arduboy = machine.getContext().arduboy;

	uint8_t x = (alignment == BubbleAlignment::Normal_Player || alignment == BubbleAlignment::Score_Computer ? 0 : 128 - width);
	if (xPos != 255) x = xPos;
	int8_t y = -1;
	uint8_t yBottom = y + (lines * 8) + 4;

	arduboy.fillRect(x, y + 4, width, yBottom - y, BLACK);	
	arduboy.fillRect(x + 4, y, width - 8, yBottom - y + 8, BLACK);	
	arduboy.fillRect(x + 1, y + 4, width - 2, yBottom - y + 1, WHITE);	
	arduboy.fillRect(x + 4, y + 1, width - 8, yBottom - y + 6, WHITE);	
	arduboy.drawRect(x + 2, y + 2, width - 4, yBottom - y + 4, BLACK);	


	SpritesB::drawExternalMask(x, y, Images::Talk_Top_Left, Images::Talk_Top_Left_Mask, 0, 0);
	SpritesB::drawExternalMask(x + width - 8, y, Images::Talk_Top_Right, Images::Talk_Top_Right_Mask, 0, 0);

	if (alignment == BubbleAlignment::Normal_Player || alignment == BubbleAlignment::Score_Computer) {

		SpritesB::drawExternalMask(x, yBottom - 1, Images::Talk_Bubble_Left, Images::Talk_Bubble_Left_Mask, 0, 0);
		SpritesB::drawExternalMask(x + width - 8, yBottom, Images::Talk_Bottom_Right, Images::Talk_Bottom_Right_Mask, 0, 0);

	}
	else {

		SpritesB::drawExternalMask(x, yBottom, Images::Talk_Bottom_Left, Images::Talk_Bottom_Left_Mask, 0, 0);
		SpritesB::drawExternalMask(x + width - 26, yBottom - 1, Images::Talk_Bubble_Right, Images::Talk_Bubble_Right_Mask, 0, 0);

	}

  font3x5.setCursor(x + 6, y + 6);
  font3x5.print(message);

}


void BaseState::drawScore(StateMachine & machine, uint8_t x, int8_t y, uint8_t score, bool renderText) {

	auto & arduboy = machine.getContext().arduboy;

  uint8_t xPos = x + 6; 
  if (score >= 10)  xPos = x + 4; 
  if (score >= 100) xPos = x + 2; 

  arduboy.fillRect(x, y, 14, 8, BLACK);
  arduboy.fillRect(x + 1, y + 1, 13, 7);
  font3x5.setCursor(xPos, y + 1);
  if (renderText) font3x5.print(score);

}


// Draw comp then player scores in top left..

void BaseState::drawScores_TopLeft(StateMachine & machine, bool renderText_Player1, bool renderText_Player2) {

	auto & arduboy = machine.getContext().arduboy;
  auto & gameStats = machine.getContext().gameStats;
	auto & player1 = gameStats.player1;
	auto & player2 = gameStats.player2;

  drawScore(machine, 5, -1, player2.getScore(), renderText_Player2);
  drawScore(machine, 26, -1, player1.getScore(), renderText_Player1);
  arduboy.fillRect(1, 0, 5, 7);
  arduboy.fillRect(22, 0, 5, 7);
  SpritesB::drawErase(1, 2, Images::Peg, 0);
  SpritesB::drawErase(22, 2, Images::Peg, 1);            

}


// ---------------------------------------------------------------------------------------------------------------------------
//  Render the dealer in the nominated position .. 
// ---------------------------------------------------------------------------------------------------------------------------
// 
void BaseState::drawDealer(StateMachine & machine, int8_t xPos, uint8_t yPos, DealerFace dealerFace, Message message) {

	auto & arduboy = machine.getContext().arduboy;

  bool blink = (arduboy.getFrameCount(128) < 4);
  uint8_t talking = arduboy.getFrameCount(48) / 12;

  SpritesB::drawExternalMask(xPos, yPos, Images::Dealer, Images::Dealer_Mask, 0, 0);
  SpritesB::drawSelfMasked(xPos + 12, yPos + 17, Images::Dealer_Eyes, blink);

  if (message.renderRequired && (message.alignment == BubbleAlignment::Normal_Computer || message.alignment == BubbleAlignment::Score_Computer)) {

    uint8_t index = (talking == 0 ? talking : talking + 2);
    SpritesB::drawSelfMasked(xPos + 17, yPos + 27, Images::Dealer_Mouth, index);

  }
  else {

    SpritesB::drawSelfMasked(xPos + 17, yPos + 27, Images::Dealer_Mouth, static_cast<uint8_t>(dealerFace));

  }

}