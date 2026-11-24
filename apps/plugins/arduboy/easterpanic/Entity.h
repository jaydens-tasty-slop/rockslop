#pragma once

#include "Direction.h"

class Entity
{
public:
	uint8_t x = 0;
	uint8_t y = 0;
	Direction direction = Direction::North;

public:
	constexpr Entity(void) = default;
	
	constexpr Entity(const Entity &) = default;
	
	constexpr Entity(const uint8_t & x, const uint8_t & y)
		: x(x), y(y), direction()
	{
	}
	
	constexpr Entity(const uint8_t & x, const uint8_t & y, const Direction & direction)
		: x(x), y(y), direction(direction)
	{
	}
	
};