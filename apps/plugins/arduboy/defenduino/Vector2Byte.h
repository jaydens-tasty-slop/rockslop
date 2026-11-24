#pragma once

#include <Arduboy2.h>

struct Vector2Byte
{
public:
	byte x = 0;
	byte y = 0;
	Vector2Byte();
	Vector2Byte(byte newX, byte newY);
};
