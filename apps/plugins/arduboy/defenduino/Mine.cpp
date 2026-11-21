#include "Mine.h"
#include "Globals.h"

const unsigned char spriteData[] PROGMEM = { 0x3, 0x8, 0x5, 0x2, 0x5, };

bool Mine::render(Vector2Int screenPos)
{
	bool bIsVisible = renderSpriteIfVisible(spriteData, &flags, screenPos);

	if (!bIsVisible)
	{
		setActive(false);
	}

	return bIsVisible;
}

void Mine::collisionCheck(PlayerShip* pPlayerShip)
{
	Rect thisRect = getCollisionRect();

	if (pPlayerShip->isActive() and !pPlayerShip->isExploding() && arduboy.collide(pPlayerShip->getCollisionRect(), thisRect))
	{
		setActive(false);
		pPlayerShip->destroy();
	}
}

Rect Mine::getCollisionRect()
{
	return Rect(worldPos.getPixelX(), worldPos.getPixelY(), 3, 3);
}