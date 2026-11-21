#include "TitleScreenState.h"

#include "Images.h"
#include "Utils.h"
#include "Strings.h"

const TitleScreenState::Option TitleScreenState::options[] =
{
	Option(StateId::LevelSelect, StringPlay),
	//Option(StateId::Gameplay, StringCredits),
	//Option(StateId::Gameplay, StringStats),
};

void TitleScreenState::update(StateMachine & machine)
{
	constexpr const static uint8_t MinOption = 0;
	constexpr const static uint8_t OptionCount = ArrayLength(options);
	constexpr const static uint8_t MaxOption = (MinOption + OptionCount) - 1;

	auto & arduboy = machine.getContext().arduboy;
	
	if (arduboy.justPressed(Arduboy::ButtonUp))
	{
		if (this->option > MinOption)
			--this->option;
	}

	if (arduboy.justPressed(Arduboy::ButtonDown))
	{
		// This line causes a warning while there's only one menu option.
		// The warning will go away when more options are added.
		if (this->option < MaxOption)
			++this->option;
	}

	if(arduboy.justPressed(Arduboy::ButtonA))
	{
		auto state = ((options[this->option].state));
		machine.changeState(state);
	}
}

void TitleScreenState::render(StateMachine & machine)
{
	constexpr const static uint8_t MinOption = 0;
	constexpr const static uint8_t OptionCount = ArrayLength(options);
	//constexpr const static uint8_t MaxOption = (MinOption + OptionCount) - 1;

	auto & arduboy = machine.getContext().arduboy;
	Sprites::drawOverwrite(0, 0, TitleImage, 0);

	constexpr const uint8_t maxWidth =
		MaxValue<uint8_t,
		StringWidth(StringPlay),
		StringWidth(StringCredits),
		StringWidth(StringStats)
		>();

	constexpr const uint8_t step = FontCharHeight + 2;

	constexpr const uint8_t x = CalculateCentreX(maxWidth);

	constexpr const uint8_t yBase = 24 + 4;

	uint8_t y = yBase;
	
	for (uint8_t i = MinOption; i < OptionCount; ++i, y += step)
	{
		arduboy.setCursor(x, y);
		arduboy.print(FlashString((options[i].text)));
		if (this->option == i)
		{
			Sprites::drawOverwrite(x - (2 * FontCharWidth), y, SmallArrowImages, 1);
		}
	}
}