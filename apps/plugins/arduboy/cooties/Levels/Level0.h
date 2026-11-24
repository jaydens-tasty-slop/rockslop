#pragma once

// For PROGMEM
#include <avr/pgmspace.h>

#include "../Tile.h"

constexpr Tile room0[8][16] PROGMEM
{
	{ Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, },
	{ Tile::Wall, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Wall, },
	{ Tile::Wall, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Wall, },
	{ Tile::Door, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Wall, },
	{ Tile::Wall, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Spikes, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Wall, },
	{ Tile::Wall, Tile::Floor, Tile::Spikes, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Door, },
	{ Tile::Wall, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Floor, Tile::Wall, },
	{ Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, Tile::Wall, },
};