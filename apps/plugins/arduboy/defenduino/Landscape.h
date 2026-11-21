#pragma once

#include <Arduboy2.h>


class Landscape
{
public:
	void render(int cameraXPos);
	byte getHeight(int xPos);
};