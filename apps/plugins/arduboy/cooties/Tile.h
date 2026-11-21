#pragma once

// For uint8_t
#include <stdint.h>

enum class Tile : uint8_t
{
	Floor = 0x00,
	Wall = 0x01,
	Door = 0x02,
	Spikes = 0x03,
};