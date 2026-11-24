#include "Globals.h"
#include "Baiter.h"

const unsigned char spriteData[] PROGMEM = { 0x08, 0x08, 0x2, 0x5, 0x7, 0x5, 0x5, 0x7, 0x5, 0x2, };

#define HORIZONTAL_VELOCITY 6
#define VERTICAL_VELOCITY 6

#define BAITER_SCORE 200

#define PAUSE_DISTANCE 300

void Baiter::update(PlayerShip* pPlayerShip)
{
	MovingGameObject::update();

	verticalWrap(&worldPos);

	if (arduboy.frameCount % 10 == 0)
	{
		if (arduboy.frameCount % 30 == 0 && abs(pPlayerShip->worldPos.x - worldPos.x) > PAUSE_DISTANCE && rand() % 4 != 0)
		{
			setVelocity(pPlayerShip);
		}
		else
		{
			// 1/2 chance we'll fire
			if (isFlagSet(flags, FLAG_VISIBLE) && rand() % 2 == 0)
			{
				int xDiff = pPlayerShip->worldPos.x - worldPos.x;
				if ((xDiff > 0 && velocity.x > 0) || (xDiff < 0 && velocity.x < 0))
				{
					fireAtPlayer(pPlayerShip, worldPos);
				}
			}
		}
	}

	velocity = baseVelocity;
	velocity.x += pPlayerShip->velocity.x;
}

bool Baiter::render(Vector2Int screenPos)
{
	return renderSpriteIfVisible(spriteData, &flags, screenPos);
}

void Baiter::onSpawn(Vector2Int position, PlayerShip* pPlayerShip)
{
	worldPos = position;

	setVelocity(pPlayerShip);
}

void Baiter::collisionCheck(PlayerShot* pPlayerShots, PlayerShip* pPlayerShip)
{
	if (Enemy::collisionCheck(8, 3, pPlayerShots, pPlayerShip))
	{
		destroy();
	}
}

void Baiter::destroy()
{
	Enemy::destroy(BAITER_SCORE, false);
}

void Baiter::setVelocity(PlayerShip* pPlayerShip)
{
	if (pPlayerShip->worldPos.x > worldPos.x)
	{
		baseVelocity.x = HORIZONTAL_VELOCITY;
	}
	else
	{
		baseVelocity.x = -HORIZONTAL_VELOCITY;
	}
	


	if (pPlayerShip->worldPos.y > worldPos.y)
	{
		baseVelocity.y = VERTICAL_VELOCITY;
	}
	else
	{
		baseVelocity.y = -VERTICAL_VELOCITY;
	}
}
