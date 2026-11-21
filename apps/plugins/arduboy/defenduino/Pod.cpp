#include "Globals.h"
#include "Pod.h"

const unsigned char spriteData[] PROGMEM = { 0x07, 0x08, 0x8, 0x2a, 0x1c, 0x7f, 0x1c, 0x2a, 0x8, };

#define HORIZONTAL_VELOCITY 3
#define DEFAULT_VERTICAL_VELOCITY 1

#define POD_SCORE 1000

void Pod::update(PlayerShip* pPlayerShip)
{
	MovingGameObject::update();

	verticalWrap(&worldPos);

}

bool Pod::render(Vector2Int screenPos)
{
	return renderSpriteIfVisible(spriteData, &flags, screenPos);
}


void Pod::onSpawn(Vector2Int position, bool right)
{
	worldPos = position;

	velocity.x = right ? HORIZONTAL_VELOCITY : -HORIZONTAL_VELOCITY;
	velocity.y = rand() % 2 == 0 ? DEFAULT_VERTICAL_VELOCITY : -DEFAULT_VERTICAL_VELOCITY;

}

void Pod::collisionCheck(PlayerShot* pPlayerShots, PlayerShip* pPlayerShip)
{
	if (Enemy::collisionCheck(6, 6, pPlayerShots, pPlayerShip))
	{
		destroy(false, pPlayerShip);
	}
}


void Pod::destroy(bool smartBomb, PlayerShip* pPlayerShip)
{
	Enemy::destroy(POD_SCORE, true);

	for (int i = 0; i < 4; i++)
	{
		Swarmer* pSwarmer = pGameState->getSwarmer();
		if (pSwarmer != NULL)
		{
			pSwarmer->setActive(true);
			Vector2Int spawnPos(worldPos.x - (40 * i) + 80, worldPos.y - (40 * i) + 80);
			pSwarmer->onSpawn(spawnPos, pPlayerShip);
		}
	}
}
