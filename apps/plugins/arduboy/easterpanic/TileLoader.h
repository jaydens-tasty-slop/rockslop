#pragma once

#include "Grid.h"
#include "Tile.h"
#include "Utils.h"

template< uint8_t Width, uint8_t Height >
class TileLoader
{
public:
	using TileGrid = Grid<Tile, Width, Height>;
	
public:
	// ToDo: swap to RLE or RL-hybrid encoding to pack more levels
	static void LoadProgmem(TileGrid & grid, uint8_t const * data)
	{
		for(uint8_t y = 0; y < 8; ++y)
		{
			auto value = ProgmemRead(&data[y]);
			for(uint8_t x = 0; x < 8; ++x)
			{
				auto bit = value & 0x80;
				value <<= 1;
				grid.getItem(x, y) = Tile(bit != 0);
			}
		}
	}
};