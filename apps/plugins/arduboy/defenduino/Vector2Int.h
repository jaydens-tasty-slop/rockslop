#pragma once

struct Vector2Int
{
public:
	int x = 0;
	int y = 0;

	Vector2Int();
	Vector2Int(int x, int y);
	Vector2Int operator-(const Vector2Int& in);

	Vector2Int normalize();

	int getPixelX();
	int getPixelY();
};
