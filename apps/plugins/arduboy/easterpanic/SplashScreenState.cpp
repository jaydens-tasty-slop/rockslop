#include "SplashScreenState.h"

#include "Images.h"
#include "Utils.h"

void SplashScreenState::update(StateMachine & machine)
{
	auto & arduboy = machine.getContext().arduboy;

	if (arduboy.justPressed(Arduboy::ButtonA) || arduboy.justPressed(Arduboy::ButtonB))
	{
		machine.changeState(StateId::TitleScreen);
		return;
	}

	// TODO: fix this god-awful mess
	if (blinkDelay > 0)
	{
		if (arduboy.everyXFrames(60))
		{
			--this->blinkDelay;
			if (blinkDelay == 0)
				this->blinkInvert = !this->blinkInvert;
		}
	}
	else if (arduboy.everyXFrames(2))
	{
		if (this->blinkInvert)
		{
			if (this->blinkTick > BlinkTickMin)
				--this->blinkTick;
			else
				this->blinkDelay = BlinkDelayDefault + random(BlinkDelayVariance + 1);
		}
		else
		{
			if (this->blinkTick < BlinkTickMax)
				++this->blinkTick;
			else
				this->blinkInvert = !this->blinkInvert;
		}
	}
}

void SplashScreenState::render(StateMachine & machine)
{
	auto & arduboy = machine.getContext().arduboy;
	
	{
		constexpr const uint8_t x = CalculateCentreX(PharapLogoWidth);
		constexpr const uint8_t y = CalculateCentreY(PharapLogoHeight);

		Sprites::drawOverwrite(x, y, PharapLogo, 0);
		arduboy.fillRect(x + 14, y + 15, 34 - 14, this->blinkTick, Arduboy::ColourWhite);
		arduboy.fillRect(x + 14, (y + 27) - this->blinkTick, 34 - 14, this->blinkTick, Arduboy::ColourWhite);
	}

	/*{
		constexpr const uint8_t width = StringWidth(StringProgrammer0);
		constexpr const uint8_t height = FontCharHeight;

		constexpr const uint8_t x = CalculateCentreX(width);

		auto & arduboy = machine.getContext().Arduboy;

		arduboy.setCursor(x, y + PharapLogoHeight + 4);
		arduboy.print(FlashString(StringPharap));
	}*/
}