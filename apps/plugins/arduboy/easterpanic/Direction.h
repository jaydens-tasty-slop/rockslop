#pragma once

enum class Direction : uint8_t
{
	North = 0,
	East = 1,
	South = 2,
	West = 3,
};

// inline linkage mandatory
inline Direction & operator++(Direction & direction)
{
	direction = static_cast<Direction>((static_cast<uint8_t>(direction) + 1) & 0x03);
	return direction;
}

// inline linkage mandatory
inline Direction & operator--(Direction & direction)
{
	direction = static_cast<Direction>((static_cast<uint8_t>(direction) - 1) & 0x03);
	return direction;
}