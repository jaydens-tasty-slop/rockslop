#include "LevelSelectState.h"

#include "Utils.h"
#include "Levels.h"
#include "Strings.h"

void LevelSelectState::update(StateMachine & machine)
{
	auto arduboy = machine.getContext().arduboy;
	
	if(arduboy.justPressed(Arduboy::ButtonUp))
	{
		if(this->index > ArrayFirstIndex(Levels))
			--this->index;
	}
	
	if(arduboy.justPressed(Arduboy::ButtonDown))
	{
		if(this->index < ArrayLastIndex(Levels))
			++this->index;	
	}
	
	if(arduboy.justPressed(Arduboy::ButtonA))
	{
		machine.getContext().selectedLevel = this->index;
		machine.changeState(StateId::Gameplay);
		return;
	}
	
	if(arduboy.justPressed(Arduboy::ButtonB))
	{
		machine.changeState(StateId::TitleScreen);
		return;
	}
}

void LevelSelectState::render(StateMachine & machine)
{
	// Layout constants
	const uint8_t centreY = CalculateCentreY(FontLineHeight);
	const uint8_t singleMargin = 2;
	const uint8_t doubleMargin = singleMargin * 2;
	const uint8_t lineHeight = FontLineHeight + doubleMargin;
	const int8_t minOffset = -2;
	const int8_t maxOffset = 2;
	
	auto arduboy = machine.getContext().arduboy;
	
	// Draw level names, including previous two and next two
	for(int8_t i = minOffset; i <= maxOffset; ++i)
	{
		const int8_t index = this->index + i; // int8_t is cheaper than integer promotion
		const uint8_t levelCount = ArrayLength(Levels); // Caching saves progmem
		if(index >= 0 && static_cast<uint8_t>(index) < levelCount)
		{
			arduboy.setCursor(doubleMargin, centreY + (lineHeight * i));
			arduboy.print(FlashString(StringLevelHeading));
			arduboy.print(index);
		}
	}
	
	// Draw selector
	arduboy.drawRect(0 + singleMargin, centreY - singleMargin, Arduboy::ScreenWidth - doubleMargin, lineHeight, Arduboy::ColourWhite);
}