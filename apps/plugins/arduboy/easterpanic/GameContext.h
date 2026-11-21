#pragma once

#include <Arduboy2.h>

#include "Tile.h"
#include "Grid.h"

class GameContext
{
public:	
	constexpr const static uint8_t Width = 8;
	constexpr const static uint8_t Height = 8;
	constexpr const static uint16_t TileCount = Width * Height;
	
	using TileGrid = Grid<Tile, Width, Height>;

	constexpr const static uint16_t SaveStart = EEPROM_STORAGE_SPACE_START;
	/*constexpr const static uint16_t SaveSize = sizeof(GameStats);
	constexpr const static uint16_t SaveEnd = (SaveStart + SaveSize) - 1;*/

public:
	Arduboy2 arduboy;
	
	uint8_t selectedLevel;
};