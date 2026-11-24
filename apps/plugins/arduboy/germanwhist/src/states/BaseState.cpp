#include "BaseState.h"

#include "../images/Images.h"
#include "../utils/Utils.h"
#include "../utils/Enums.h"
#include "../fonts/Font3x5.h"

void BaseState::drawMessageBox(StateMachine &machine, String message, uint8_t lines, uint8_t width, BubbleAlignment alignment) {

	auto & arduboy = machine.getContext().arduboy;

  uint8_t x = 0;

  switch (alignment) {

    case BubbleAlignment::Left:
      x = 0;
      break;

    case BubbleAlignment::Centre:
      x = 64 - (width / 2);
      break;

    case BubbleAlignment::Right:
      x = 128 - width;
      break;

    default: break;

  }

	int8_t y = -1;
	uint8_t yBottom = y + (lines * 8) + 2;

	arduboy.fillRect(x, y + 4, width, yBottom - y, BLACK);	
	arduboy.fillRect(x + 4, y, width - 8, yBottom - y + 8, BLACK);	
	arduboy.fillRect(x + 1, y + 4, width - 2, yBottom - y + 1, WHITE);	
	arduboy.fillRect(x + 4, y + 1, width - 8, yBottom - y + 6, WHITE);	
	arduboy.drawRect(x + 2, y + 2, width - 4, yBottom - y + 4, BLACK);	

	SpritesB::drawExternalMask(x, y, Images::Talk_Top_Left, Images::Talk_Top_Left_Mask, 0, 0);
	SpritesB::drawExternalMask(x + width - 8, y, Images::Talk_Top_Right, Images::Talk_Top_Right_Mask, 0, 0);

  SpritesB::drawExternalMask(x, yBottom, Images::Talk_Bottom_Left, Images::Talk_Bottom_Left_Mask, 0, 0);
  SpritesB::drawExternalMask(x + width - 26, yBottom - 1, Images::Talk_Bubble_Right, Images::Talk_Bubble_Right_Mask, 0, 0);

  font3x5.setCursor(x + 6, y + 5);
  font3x5.print(message);

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

  if (message.renderRequired) {

    uint8_t index = (talking == 0 ? talking : talking + 2);
    SpritesB::drawSelfMasked(xPos + 17, yPos + 27, Images::Dealer_Mouth, index);

  }
  else {

    SpritesB::drawSelfMasked(xPos + 17, yPos + 27, Images::Dealer_Mouth, static_cast<uint8_t>(dealerFace));

  }

  arduboy.fillRect(0, 55, 128, 59, WHITE);

}