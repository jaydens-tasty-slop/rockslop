#include "Globals.h"
#include "Swarmer.h"

const unsigned char spriteData[] PROGMEM = { 0x05, 0x08,0xc, 0xa, 0xf, 0xa, 0xc, };

#define HORIZONTAL_VELOCITY 7
#define VERTICAL_VELOCITY 2

#define SWARMER_SCORE 150

void Swarmer::update(PlayerShip* pPlayerShip)
{
	MovingGameObject::update();
	verticalWrap(&worldPos);

	if (arduboy.frameCount % 10 == 0) 
	{
		if (abs(pPlayerShip->worldPos.getPixelX() - worldPos.getPixelX()) > HALF_SCREEN_WIDTH)
		{
			setHorizontalVelocity(pPlayerShip);
		}

		if (arduboy.frameCount % 30 == 0)
		{
			setVerticalVelocity(pPlayerShip);
		}
		else
		{
			// 1/10 chance we'll fire
			if (isFlagSet(flags, FLAG_VISIBLE) && rand() % 10 == 0)
			{
				uint16_t xDiff = pPlayerShip->worldPos.x - worldPos.x;
				if ((xDiff > 0 && velocity.x > 0) || (xDiff < 0 && velocity.x < 0))
				{
					fireAtPlayer(pPlayerShip, worldPos);
				}
			}
		}
	}
}

bool Swarmer::render(Vector2Int screenPos)
{
	return renderSpriteIfVisible(spriteData, &flags, screenPos);
}


void Swarmer::collisionCheck(PlayerShot* pPlayerShots, PlayerShip* pPlayerShip)
{
	if (Enemy::collisionCheck(5, 4, pPlayerShots, pPlayerShip))
	{
		destroy(false);
	}
}


void Swarmer::destroy(bool smartBomb)
{
	Enemy::destroy(SWARMER_SCORE, true);
}

void Swarmer::onSpawn(Vector2Int position, PlayerShip* pPlayerShip)
{
	worldPos = position;
	velocity.y = rand() % 2 == 0 ? VERTICAL_VELOCITY : -VERTICAL_VELOCITY;
	setHorizontalVelocity(pPlayerShip);
}

void Swarmer::setHorizontalVelocity(PlayerShip* pPlayerShip)
{
	if (pPlayerShip->worldPos.x > worldPos.x)
	{
		velocity.x = HORIZONTAL_VELOCITY;
	}
	else
	{
		velocity.x = -HORIZONTAL_VELOCITY;
	}
}

void Swarmer::setVerticalVelocity(PlayerShip* pPlayerShip)
{
	if (pPlayerShip->worldPos.y > worldPos.y)
	{
		velocity.y = VERTICAL_VELOCITY;
	}
	else
	{
		velocity.y = -VERTICAL_VELOCITY;
	}
}