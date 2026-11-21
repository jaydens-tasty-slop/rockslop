#include "Particles.h"
#include "Globals.h"

#define FLAG_SPAWNING 0x4
#define FLAG_SPAWNING_BAITER 0x8
#define FLAG_INVERTED 0x10
#define FLAG_PLAYER 0x20
#define FLAG_SPAWNING_PLAYER 0x40

#define MAX_DISTANCE_SMALL 32
#define MAX_DISTANCE_LARGE 128

const uint16_t explosionSound[] PROGMEM = {
	50,10, 25,10, 75,10, 22,10,200,10, 100,10, 66,10, 33,10, 175, 10, 200, 10, 72, 10,
	50,10, 25,10, 75,10, 22,10,200,10, 100,10, 66,10, 33,10, 175, 10, 200, 10, 72, 10,
	TONES_END };

const uint16_t playerExplosionSound[] PROGMEM = {
	150,10, 125,10, 175,10, 122,10,300,10, 200,10, 166,10, 133,10, 275, 10, 300, 10, 172, 10,
	50,10, 25,10, 75,10, 22,10,200,10, 100,10, 66,10, 33,10, 175, 10, 200, 10, 72, 10,
	50,10, 25,10, 75,10, 22,10,200,10, 100,10, 66,10, 33,10, 175, 10, 200, 10, 72, 10,
	TONES_END };

const uint16_t spawnSound[] PROGMEM = {
	// 22-50
	50,10, 25,10, 45,10, 22,10,
	// 50-100
	70,10, 100,10, 66,10, 50,10,
	// 75-150
	75, 10, 150, 10, 120, 10, 80, 10,
	// 100-200
	175,10, 200,10, 100,10, 122,10,
	// 125-250
	250,10, 125,10, 166,10, 233,10,
	// 150-300
	175, 10, 300, 10, 172, 10, 201, 10,
	// 175-350
	300, 10, 200, 10, 180, 10, 320, 10,
	TONES_END };

void Particles::show(uint8_t type)
{
	GameObject::setActive(true);

	unsetFlag(&flags, FLAG_PLAYER);
	unsetFlag(&flags, FLAG_SPAWNING);
	unsetFlag(&flags, FLAG_SPAWNING_BAITER);
	unsetFlag(&flags, FLAG_SPAWNING_PLAYER);

	distance = 0;

	if (type == PARTICLES_SPAWN || type == PARTICLES_SPAWN_BAITER || type == PARTICLES_SPAWN_PLAYER)
	{
		sound.tones(spawnSound);
		setFlag(&flags, FLAG_SPAWNING);

		if (type == PARTICLES_SPAWN_BAITER)
		{
			setFlag(&flags, FLAG_SPAWNING_BAITER);
		}
		else if (type == PARTICLES_SPAWN_PLAYER)
		{
			setFlag(&flags, FLAG_SPAWNING_PLAYER);
		}

		distance = MAX_DISTANCE_SMALL;
	}
	else if(type == PARTICLES_PLAYER)
	{
		setFlag(&flags, FLAG_PLAYER);
		arduboy.invert(true);
		setFlag(&flags, FLAG_INVERTED);
		sound.tones(playerExplosionSound);
	}
	else
	{
		sound.tones(explosionSound);
	}
}

void Particles::update()
{
	if (isFlagSet(flags, FLAG_SPAWNING))
	{
		distance--;

		if (distance <= 0)
		{
			unsetFlag(&flags, FLAG_ACTIVE);
			if (isFlagSet(flags, FLAG_SPAWNING_BAITER))
			{
				pGameState->completeSpawningBaiter(worldPos.x, worldPos.y);
			}
			else if (isFlagSet(flags, FLAG_SPAWNING_PLAYER))
			{
				pGameState->playerShip.setActive(true, false);
			}
			else
			{
				pGameState->completeSpawningLander(worldPos.x, worldPos.y);
			}
		}
	}
	else
	{
		distance+=2;
		byte maxDistance;
		if (isFlagSet(flags, FLAG_PLAYER))
		{
			maxDistance = MAX_DISTANCE_LARGE;
		}
		else
		{
			maxDistance = MAX_DISTANCE_SMALL;
		}

		if (distance >= maxDistance)
		{
			unsetFlag(&flags, FLAG_ACTIVE);

			if (isFlagSet(flags, FLAG_PLAYER))
			{
				pGameState->lostLife();
			}
		}
	}

	if (isFlagSet(flags, FLAG_INVERTED))
	{
		arduboy.invert(false);
		unsetFlag(&flags, FLAG_INVERTED);
	}
}

void Particles::render(Vector2Int screenPos)
{
	if (distance > 0)
	{
		int halfDist = distance / 2;
		int leftEdge = screenPos.x - halfDist;
		int rightEdge = screenPos.x + halfDist;

		if ((leftEdge < SCREEN_WIDTH && leftEdge >= 0) ||
			(rightEdge < SCREEN_WIDTH && rightEdge >= 0))
		{
			for (int x = -distance; x <= distance; x += distance)
			{
				for (int y = -distance; y <= distance; y += distance)
				{
					if (x != 0 || y != 0)
					{
						arduboy.drawPixel(screenPos.x + x, screenPos.y + y);
					}
				}
			}
		}
	}
}