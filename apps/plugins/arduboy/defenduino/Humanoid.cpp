#include "Humanoid.h"
#include "Globals.h"

#define FLOOR 290

#define CAUGHT_SCORE 500
#define DROPPED_SCORE 500
#define LANDING_SCORE 250

#define FLAG_CLIMBER 0x4
#define FLAG_CAPTURED 0x8
#define FLAG_FALLING 0x10
#define FLAG_CAUGHT 0x20

const uint16_t capturedSound[] PROGMEM = {
	1500,25, 250,25, 1500,25, 250,25,1500,25, 250,25, 1500,25, 250,25,1500,25, 250,25,1500,25, 250,25,1500,25, 250,25,1500,25, 250,25,
	TONES_END };

const uint16_t fallingSound[] PROGMEM = {
	1500,175, 1250,150, 1000,125, 750,100, 500, 75, 250, 50, 100, 25,
	TONES_END };

Humanoid::Humanoid() : MovingGameObject()
{
}

void Humanoid::update(Landscape* pLandscape, PlayerShip* pPlayerShip)
{
	MovingGameObject::update();

	if (isFlagSet(flags, FLAG_FALLING))
	{
		fallingUpdate(pLandscape);
	}
	else if (isFlagSet(flags, FLAG_CAUGHT))
	{
		caughtUpdate(pLandscape, pPlayerShip);
	}
	else if (!isFlagSet(flags, FLAG_CAPTURED))
	{
		byte landscapeHeight = pLandscape->getHeight(worldPos.getPixelX()) - 29;

		if (isFlagSet(flags, FLAG_CLIMBER))
		{
			worldPos.y = landscapeHeight * 10;
			if (worldPos.y > FLOOR)
			{
				worldPos.y = FLOOR;
			}
		}
		else
		{
			worldPos.y = FLOOR;
		}
	}
}


void Humanoid::fallingUpdate(Landscape* pLandscape)
{
	if (arduboy.frameCount % 8 == 0)
	{
		velocity.y++;
	}
	byte landscapeHeight = pLandscape->getHeight(worldPos.getPixelX()) - 29;
	if (worldPos.getPixelY() >= landscapeHeight)
	{
		// Hit the ground
		if (velocity.y >= 8)
		{
			destroy();
		}
		else
		{
			pGameState->addToScore(LANDING_SCORE);
			startWalking();
		}
	}
}

void Humanoid::caughtUpdate(Landscape* pLandscape, PlayerShip* pPlayerShip)
{
	worldPos.x = pPlayerShip->worldPos.x + 30;
	worldPos.y = pPlayerShip->worldPos.y + 40;

	byte landscapeHeight = pLandscape->getHeight(worldPos.getPixelX()) - 29;
	if (worldPos.getPixelY() >= landscapeHeight)
	{
		// Dropped off
		pGameState->addToScore(DROPPED_SCORE);
		worldPos.y = landscapeHeight * 10;

		// Make sure we're not in the same place as another one.
		while (sameXAsAnotherHuman())
		{
			worldPos.x -= 20;
		}

		startWalking();
	}
}

bool Humanoid::sameXAsAnotherHuman()
{
	for (uint8_t i = 0; i < TOTAL_HUMANOIDS; i++)
	{
		Humanoid* pHumanoid = pGameState->getHumanoid(i);
		if (pHumanoid != this && pHumanoid->isActive() && pHumanoid->worldPos.getPixelX() == worldPos.getPixelX())
		{
			return true;
		}
	}
	return false;
}

bool Humanoid::render(Vector2Int screenPos)
{
	bool isVisible;
	if (screenPos.x >= 0 and screenPos.y < SCREEN_WIDTH)
	{
		isVisible = true;
		setFlag(&flags, FLAG_VISIBLE);
		arduboy.drawLine(screenPos.x, screenPos.y, screenPos.x, screenPos.y + 2, WHITE);
	}
	else
	{
		isVisible = false;
		unsetFlag(&flags, FLAG_VISIBLE);
	}
	return isVisible;
}

void Humanoid::setActive(bool active)
{
	GameObject::setActive(active);
	if (rand() % 2 == 1)
	{
		setFlag(&flags, FLAG_CLIMBER);
	}
	else
	{
		unsetFlag(&flags, FLAG_CLIMBER);
	}
	startWalking();
}

void Humanoid::startWalking()
{
	if (rand() % 2 == 0)
	{
		velocity.x = 1;
	}
	else
	{
		velocity.x = -1;
	}
	velocity.y = 0;

	unsetFlag(&flags, FLAG_CAUGHT);
	unsetFlag(&flags, FLAG_CAPTURED);
	unsetFlag(&flags, FLAG_FALLING);
}

void Humanoid::collisionCheck(PlayerShot* pPlayerShots, PlayerShip* pPlayerShip)
{
	if (!isFlagSet(flags, FLAG_CAUGHT))
	{
		Rect thisRect = getCollisionRect();
		for (int i = 0; i < TOTAL_PLAYER_SHOTS; i++)
		{
			if (pPlayerShots[i].isActive() && pPlayerShots[i].tipOnScreen() && arduboy.collide(pPlayerShots[i].getCollisionRect(), thisRect))
			{
				pPlayerShots[i].setActive(false);
				destroy();
				return;
			}
		}

		if (isFlagSet(flags, FLAG_FALLING) && arduboy.collide(pPlayerShip->getCollisionRect(), thisRect))
		{
			// Caught
			pGameState->addToScore(CAUGHT_SCORE);
			unsetFlag(&flags, FLAG_FALLING);
			setFlag(&flags, FLAG_CAUGHT);
			velocity.y = 0;
		}
	}
}

Rect Humanoid::getCollisionRect()
{
	return Rect(worldPos.getPixelX(), worldPos.getPixelY(), 1, 3);
}

void Humanoid::destroy()
{
	explodeObject(&flags, worldPos, PARTICLES_EXPLOSION);
	pGameState->onHumanoidDestroyed();
}

bool Humanoid::isCapturable()
{
	return isActive() && !isFlagSet(flags, FLAG_CAPTURED) && !isFlagSet(flags, FLAG_FALLING) && !isFlagSet(flags, FLAG_CAUGHT);
}

void Humanoid::setCaptured(bool captured)
{
	setFlag(&flags, FLAG_CAPTURED, captured);
	velocity.x = 0;
	velocity.y = 0;

	if (captured)
	{
		sound.tones(capturedSound);
	}
}

void Humanoid::startFalling()
{
	sound.tones(fallingSound);
	velocity.y = 1;
	unsetFlag(&flags, FLAG_CAPTURED);
	setFlag(&flags, FLAG_FALLING);
}

bool Humanoid::isCarried()
{
	return isFlagSet(flags, FLAG_CAUGHT);
}