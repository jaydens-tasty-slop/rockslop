#include "Lander.h"
#include "Globals.h"

const unsigned char landerSprite[] PROGMEM = { 0x7, 0x8, 0x24, 0x16, 0xd, 0x3f, 0xd, 0x16, 0x24, };
const unsigned char mutantSprite[] PROGMEM = { 0x7, 0x8, 0x24, 0x16, 0x9, 0x31, 0x9, 0x16, 0x24, };

#define FLAG_SEEKING 0x4
#define FLAG_LANDING 0x8
#define FLAG_ESCAPING 0x10
#define FLAG_MUTANT 0x20

#define LANDER_HORIZ_SPEED 3
#define CRUISING_ALTITUDE 10

#define LANDER_SCORE 150

Lander::Lander()
{

}

void Lander::update(Landscape* pLandscape, PlayerShip* pPlayerShip)
{
	MovingGameObject::update();

	uint8_t fireChance;
	if (isFlagSet(flags, FLAG_SEEKING))
	{
		seekingUpdate(pLandscape, pPlayerShip);
		fireChance = 128;	// 1/128
	} 
	else if (isFlagSet(flags, FLAG_LANDING))
	{
		landingUpdate(pPlayerShip);
		fireChance = 100;	// 1/100
	}
	else if (isFlagSet(flags, FLAG_ESCAPING))
	{
		escapingUpdate(pPlayerShip);
		fireChance = 100;  // 1/100
	}
	else if (isFlagSet(flags, FLAG_MUTANT))
	{
		mutantUpdate(pPlayerShip);
		fireChance = 32;	// 1/32
	}

	if (isFlagSet(flags, FLAG_VISIBLE) && rand() % fireChance == 0)
	{
		fireAtPlayer(pPlayerShip, worldPos);
	}
}

void Lander::seekingUpdate(Landscape* pLandscape, PlayerShip* pPlayerShip)
{
	uint8_t landscapeHeight = pLandscape->getHeight(worldPos.getPixelX());
	int preferredHeight;
	// Convert to camera space and sub 11  so we keep above it
	int convertedLandscapeHeight = landscapeHeight - 43;

	if (convertedLandscapeHeight < CRUISING_ALTITUDE)
	{
		preferredHeight = convertedLandscapeHeight;
	}
	else
	{
		preferredHeight = CRUISING_ALTITUDE;
	}

	int pixelY = worldPos.getPixelY();
	if (pixelY < preferredHeight)
	{
		worldPos.y += 5;
		if (worldPos.getPixelY() > preferredHeight)
		{
			worldPos.y = preferredHeight * 10;
		}
	}
	else if(pixelY > preferredHeight)
	{
		worldPos.y -= 10;
		if (worldPos.getPixelY() < preferredHeight)
		{
			worldPos.y = preferredHeight * 10;
		}
	}

	// 1/16 chance we'll check for humanoids below
	if (arduboy.frameCount % 16 == 0)
	{
		humanoid = pGameState->getCapturableHumanoidAtPosition(worldPos.getPixelX()+3);

		if (humanoid != NO_HUMANOID_FOUND)
		{
			unsetFlag(&flags, FLAG_SEEKING);
			setFlag(&flags, FLAG_LANDING);

			velocity.x = 0;
			velocity.y = 5;
		}
	}
}

void Lander::landingUpdate(PlayerShip* pPlayerShip)
{
	Humanoid* pHumanoid = pGameState->getHumanoid(humanoid);

	if (!pHumanoid->isCapturable())
	{
		startSeeking();
	}
	else
	{
		worldPos.x = pHumanoid->worldPos.x - 30;
		if (worldPos.y >= pHumanoid->worldPos.y - 60)
		{
			pHumanoid->setCaptured(true);
			unsetFlag(&flags, FLAG_LANDING);
			setFlag(&flags, FLAG_ESCAPING);

			velocity.y = -5;
		}
	}
}

void Lander::escapingUpdate(PlayerShip* pPlayerShip)
{
	Humanoid* pHumanoid = pGameState->getHumanoid(humanoid);
	pHumanoid->worldPos.y = worldPos.y + 40;
	
	if (worldPos.getPixelY() <= -38)
	{
		// Escaped off the top of the screen with no humanoid
		setActive(false);
		pGameState->onCountedEnemyDeath();
	} 
	else if (worldPos.getPixelY() <= -32 && pHumanoid->isActive())
	{
		// Mutant time!
		pHumanoid->destroy();
		becomeMutant();
	}
}

void Lander::becomeMutant()
{
	unsetFlag(&flags, FLAG_LANDING);
	unsetFlag(&flags, FLAG_SEEKING);
	unsetFlag(&flags, FLAG_ESCAPING);
	setFlag(&flags, FLAG_MUTANT);
}

void Lander::mutantUpdate(PlayerShip* pPlayerShip)
{
	if (pPlayerShip->worldPos.x < worldPos.x)
	{
		velocity.x = -LANDER_HORIZ_SPEED;
	}
	else
	{
		velocity.x = LANDER_HORIZ_SPEED;
	}

	if (pPlayerShip->worldPos.y < worldPos.y)
	{
		velocity.y = -10;
	}
	else
	{
		velocity.y = 10;
	}

	// Chance to add some "wiggle"
	if (rand() % 3 == 1)
	{
		velocity.y *= -1;
	}
}

bool Lander::render(Vector2Int screenPos)
{
	return renderSpriteIfVisible(isFlagSet(flags, FLAG_MUTANT) ? mutantSprite : landerSprite, &flags, screenPos);
}

void Lander::setActive(bool active)
{
	GameObject::setActive(active);

	if (active)
	{
		if (pGameState->remainingHumanoids > 0)
		{
			startSeeking();
		}
		else
		{
			// No humans left, so start as a mutant.
			becomeMutant();
		}
	}
}

void Lander::startSeeking()
{
	unsetFlag(&flags, FLAG_LANDING);
	unsetFlag(&flags, FLAG_MUTANT);
	unsetFlag(&flags, FLAG_ESCAPING);
	setFlag(&flags, FLAG_SEEKING);

	velocity.y = 0;
	if (rand() % 3 == 0)
	{
		velocity.x = -LANDER_HORIZ_SPEED;
	}
	else
	{
		velocity.x = LANDER_HORIZ_SPEED;
	}
}

void Lander::collisionCheck(PlayerShot* pPlayerShots, PlayerShip* pPlayerShip)
{
	if (Enemy::collisionCheck(7, 6, pPlayerShots, pPlayerShip)) {
		destroy();
	}
}

void Lander::destroy()
{
	Enemy::destroy(LANDER_SCORE, true);
	if (isFlagSet(flags, FLAG_ESCAPING) && humanoid != NO_HUMANOID_FOUND)
	{
		Humanoid* pHumanoid = pGameState->getHumanoid(humanoid);
		humanoid = NO_HUMANOID_FOUND;

		if (pHumanoid->isActive())
		{
			pHumanoid->startFalling();
		}
	}

}

bool Lander::isMutant()
{
	return isFlagSet(flags, FLAG_MUTANT);
}


