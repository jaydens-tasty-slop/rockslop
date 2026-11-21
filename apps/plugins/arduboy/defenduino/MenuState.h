#pragma once

#include <Arduboy2.h>

class MenuState
{
public:
	MenuState();
	void update();

private:
	uint8_t startSelected = true;
	void drawMenu(uint8_t x, uint8_t y, bool selected, const __FlashStringHelper* text);
	int16_t scrollTextPos = 128;
	void drawScrollText();
};

