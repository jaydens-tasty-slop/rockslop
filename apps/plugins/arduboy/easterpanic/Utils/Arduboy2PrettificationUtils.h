#pragma once

#include <Arduboy2.h>

//
// Const Globals
//

namespace Arduboy
{
	constexpr const uint8_t ScreenWidth = WIDTH;
	constexpr const uint8_t ScreenHeight = HEIGHT;
	
	constexpr const uint8_t ColourBlack = BLACK;
	constexpr const uint8_t ColourWhite = WHITE;
	constexpr const uint8_t ColourInvert = INVERT;

	constexpr const uint8_t ButtonA = A_BUTTON;
	constexpr const uint8_t ButtonB = B_BUTTON;
	constexpr const uint8_t ButtonLeft = LEFT_BUTTON;
	constexpr const uint8_t ButtonRight = RIGHT_BUTTON;
	constexpr const uint8_t ButtonUp = UP_BUTTON;
	constexpr const uint8_t ButtonDown = DOWN_BUTTON;

	constexpr const uint8_t EepromStorageSpaceStart = EEPROM_STORAGE_SPACE_START;
}

// Don't have time to do the rest, I'll add them later

//
// Undefs
//

// Probably don't everywhere work yet, I'll fix this later
#undef WIDTH
#undef HEIGHT

#undef BLACK
#undef WHITE
#undef INVERT

#undef A_BUTTON
#undef B_BUTTON
#undef LEFT_BUTTON
#undef RIGHT_BUTTON
#undef UP_BUTTON
#undef DOWN_BUTTON

#undef EEPROM_STORAGE_SPACE_START