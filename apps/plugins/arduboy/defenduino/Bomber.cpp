#include "Globals.h"
#include "Bomber.h"

const unsigned char spriteData[] PROGMEM = { 0x08, 0x08, 0xf8, 0xfe, 0xda, 0xfa, 0xfa, 0x42, 0x7e, 0x00, };

#define HORIZONTAL_VELOCITY 5
#define DEFAULT_VERTICAL_VELOCITY 1

#define BOMBER_SCORE 250

void Bomber::update(PlayerShip* pPlayerShip)
{
	MovingGameObject::update();

	verticalWrap(&worldPos);

	if (isFlagSet(flags, FLAG_VISIBLE) && rand() % 64 == 0)
	{
		Mine* pMine = pGameState->getMine();
		if (pMine != NULL)
		{
			pMine->setActive(true);
			pMine->worldPos = worldPos;
		}
	}
}

bool Bomber::render(Vector2Int screenPos)
{
	uint8_t mirror;
	if (velocity.x > 0)
	{
		if (velocity.y > 0)
		{
			mirror = MIRROR_HORIZONTAL;
		}
		else
		{
			mirror = MIRROR_HOR_VER;
		}
	}
	else
	{
		if (velocity.y > 0)
		{
			mirror = MIRROR_NONE;
		}
		else
		{
			mirror = MIRROR_VERTICAL;
		}
	}

	return renderSpriteIfVisible(spriteData, &flags, screenPos, mirror);
}

void Bomber::onSpawn(Vector2Int position, bool right)
{
	worldPos = position;

	velocity.x = right ? HORIZONTAL_VELOCITY : -HORIZONTAL_VELOCITY;
	velocity.y = rand() % 2 == 0 ? DEFAULT_VERTICAL_VELOCITY : -DEFAULT_VERTICAL_VELOCITY;

}

void Bomber::collisionCheck(PlayerShot* pPlayerShots, PlayerShip* pPlayerShip)
{
	if (Enemy::collisionCheck(8, 8, pPlayerShots, pPlayerShip))
	{
		destroy();
	}
}

void Bomber::destroy()
{
	Enemy::destroy(BOMBER_SCORE, true);
}
