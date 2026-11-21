#include "MovingGameObject.h"
#include "Globals.h"

void MovingGameObject::update()
{
	if (velocity.x != 0)
	{
		worldPos.x += velocity.x;
	}

	if (velocity.y != 0)
	{
		worldPos.y += velocity.y;
	}

	if (worldPos.x >= WORLD_WIDTH_UNITS)
	{
		worldPos.x -= WORLD_WIDTH_UNITS;
	}
	else if (worldPos.x < 0)
	{
		worldPos.x += WORLD_WIDTH_UNITS-1;
	}
}