#include "Vector2Int.h"
#include "Globals.h"

Vector2Int::Vector2Int() {}

Vector2Int::Vector2Int(int x, int y)
{
	this->x = x;
	this->y = y;
}

Vector2Int Vector2Int::operator-(const Vector2Int& in)
{
	return Vector2Int(x - in.x, y - in.y);
}

Vector2Int Vector2Int::normalize()
{
	float scale;
	int absX = abs(x);
	int absY = abs(y);
	if (absX > absY)
	{
		scale = absX / 10;
	}
	else
	{
		scale = absY / 10;
	}


	if (scale != 0)
	{
		return Vector2Int(x / scale, y / scale);
	}
	else
	{
		return Vector2Int(x, y);
	}
}

int Vector2Int::getPixelX()
{
	return x / 10;
}

int Vector2Int::getPixelY()
{
	return y / 10;
}