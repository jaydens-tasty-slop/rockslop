#include "PlayerShip.h"
#include "Globals.h"

const unsigned char spriteRight[] PROGMEM = { 0x8, 0x8, 0x2, 0x7, 0x7, 0x6, 0x6, 0x2, 0x2, 0x2, };
const unsigned char thrusterSprite[] PROGMEM = { 0x2, 0x8, 0x2, 0x7, };

const uint16_t fireSound[] PROGMEM = {
	500,50, 250,50, 175,50, 75,50,
	TONES_END };

#define SHIP_HORIZ_ACCELERATION 2
#define SHIP_MAX_SPEED 20
#define SHIP_HORIZ_DECELERATION 1

#define FLAG_FACING_RIGHT 0x4
#define FLAG_EXPLODING 0x8
#define FLAG_HIDDEN 0x10

PlayerShip::PlayerShip() : MovingGameObject()
{
	worldPos.x = worldPos.y = 0;
	setFlag(&flags, FLAG_FACING_RIGHT);
}

void PlayerShip::update()
{
	if(isFlagSet(flags, FLAG_EXPLODING))
	{ 
		explodingUpdate();
	}
	else
	{
		activeUpdate();
	}
	
}

void PlayerShip::activeUpdate()
{
	MovingGameObject::update();

	int pixelY = worldPos.getPixelY();
	if (arduboy.pressed(UP_BUTTON) && pixelY > -HALF_SCREEN_HEIGHT)
	{
		worldPos.y -= 10;
		if (worldPos.y < -HALF_SCREEN_HEIGHT_UNITS)
		{
			worldPos.y = -HALF_SCREEN_HEIGHT_UNITS;
		}
	}
	else if (arduboy.pressed(DOWN_BUTTON) && pixelY < HALF_SCREEN_HEIGHT - 3)
	{
		worldPos.y += 10;
		if (worldPos.y > HALF_SCREEN_HEIGHT_UNITS - 3)
		{
			worldPos.y = HALF_SCREEN_HEIGHT_UNITS - 3;
		}
	}

	if (arduboy.pressed(RIGHT_BUTTON))
	{
		if (!isFlagSet(flags, FLAG_FACING_RIGHT))
		{
			setFlag(&flags, FLAG_FACING_RIGHT);
		}
		velocity.x += SHIP_HORIZ_ACCELERATION;
		if (velocity.x > SHIP_MAX_SPEED)
		{
			velocity.x = SHIP_MAX_SPEED;
		}
	}
	else if (arduboy.pressed(LEFT_BUTTON))
	{
		if (isFlagSet(flags, FLAG_FACING_RIGHT))
		{
			unsetFlag(&flags, FLAG_FACING_RIGHT);
		}
		velocity.x -= SHIP_HORIZ_ACCELERATION;
		if (velocity.x < -SHIP_MAX_SPEED)
		{
			velocity.x = -SHIP_MAX_SPEED;
		}
	}
	else
	{
		if (velocity.x > 0)
		{
			velocity.x -= SHIP_HORIZ_DECELERATION;
			if (velocity.x < 0)
			{
				velocity.x = 0;
			}
		}
		else if (velocity.x < 0)
		{
			velocity.x += SHIP_HORIZ_DECELERATION;
			if (velocity.x > 0)
			{
				velocity.x = 0;
			}
		}
	}

	if (arduboy.justPressed(B_BUTTON))
	{
		fire();
	}

	if (arduboy.justPressed(A_BUTTON))
	{
		if (!arduboy.pressed(B_BUTTON))
		{
			pGameState->onSmartBomb();
		}
		else
		{
			hyperspace();
		}
	}
}

void PlayerShip::explodingUpdate() 
{
	explosionTimer--;
	if (explosionTimer == 0)
	{
		explode();
	}
	else if(explosionTimer % 4 == 0)
	{
		setFlag(&flags, FLAG_HIDDEN, !isFlagSet(flags, FLAG_HIDDEN));
	}
}

int PlayerShip::getCameraTarget()
{
	int cameraTarget;
	if (isFlagSet(flags, FLAG_FACING_RIGHT))
	{
		cameraTarget = (worldPos.x + 320) - (5 * (velocity.x));
		if (cameraTarget >= WORLD_WIDTH_UNITS)
		{
			cameraTarget -= WORLD_WIDTH_UNITS;
		}
	}
	else
	{
		cameraTarget = (worldPos.x - 320) - (5 * (velocity.x));

		if (cameraTarget < 0)
		{
			cameraTarget += WORLD_WIDTH_UNITS;
		}
	}

	return cameraTarget;
}

void PlayerShip::fire()
{
	PlayerShot* shot = pGameState->getPlayerShot();
	if (shot != NULL)
	{
		sound.tones(fireSound);

		shot->worldPos.y = worldPos.y + 10;

		float shotVelocity;
		if (isFlagSet(flags, FLAG_FACING_RIGHT))
		{
			shot->worldPos.x = worldPos.x + 80;
			shotVelocity = velocity.x + 30;
		}
		else
		{
			shot->worldPos.x = worldPos.x - 10;
			shotVelocity = velocity.x - 30;

		}

		shot->setActive(true);
		shot->fire(shotVelocity);
	}
}

void PlayerShip::render(Vector2Int screenPos)
{
	if (!isFlagSet(flags, FLAG_HIDDEN))
	{
		renderSprite(spriteRight, screenPos, isFlagSet(flags, FLAG_FACING_RIGHT) ? MIRROR_NONE : MIRROR_HORIZONTAL);

		if ((arduboy.pressed(RIGHT_BUTTON) || arduboy.pressed(LEFT_BUTTON)) && arduboy.frameCount % 2 == 0)
		{
			renderThruster(screenPos);
		}
	}
}

void PlayerShip::renderThruster(Vector2Int shipScreenPos)
{
	uint8_t col1 = rand() % 256;
	uint8_t col2 = rand() % 256;

	bool right = isFlagSet(flags, FLAG_FACING_RIGHT);

	renderSprite(thrusterSprite, Vector2Int(shipScreenPos.x + (right ? -3 : 9), shipScreenPos.y), right ? MIRROR_NONE : MIRROR_HORIZONTAL);
}

void PlayerShip::destroy()
{
	if (!isFlagSet(flags, FLAG_EXPLODING))
	{
		pGameState->freezeActors();
		setFlag(&flags, FLAG_EXPLODING);
		explosionTimer = 30;
	}
}

Rect PlayerShip::getCollisionRect()
{
	return Rect(worldPos.getPixelX(), worldPos.getPixelY(), 8, 3);
}


void PlayerShip::setActive(bool active, bool forceFaceRight)
{
	GameObject::setActive(active);

	if (active)
	{
		unsetFlag(&flags, FLAG_EXPLODING);
		unsetFlag(&flags, FLAG_HIDDEN);

		if (forceFaceRight)
		{
			setFlag(&flags, FLAG_FACING_RIGHT);
		}
		velocity.x = velocity.y = 0;
	}
}

void PlayerShip::explode()
{
	explodeObject(&flags, worldPos, PARTICLES_PLAYER);
	pGameState->addToScore(25);

	// Destroy any carried humans
	for (uint8_t i = 0; i < TOTAL_HUMANOIDS; i++)
	{
		Humanoid* pHumanoid = pGameState->getHumanoid(i);
		if (pHumanoid->isActive() && pHumanoid->isCarried())
		{
			pHumanoid->destroy();
		}
	}
}

bool PlayerShip::isExploding()
{
	return isFlagSet(flags, FLAG_EXPLODING);
}

bool PlayerShip::facingRight()
{
	return isFlagSet(flags, FLAG_FACING_RIGHT);
}

void PlayerShip::cancelExplosion()
{
	unsetFlag(&flags, FLAG_EXPLODING);
}

void PlayerShip::hyperspace()
{
	velocity.x = velocity.y = 0;
	worldPos.x = rand() % WORLD_WIDTH_UNITS-1;
	worldPos.y = (rand() % (SCREEN_HEIGHT_UNITS-50)) - (HALF_SCREEN_HEIGHT_UNITS-25);
	setFlag(&flags, FLAG_FACING_RIGHT, (rand() % 2 == 0) ? true : false);
	pGameState->getCamera()->worldPos.x = getCameraTarget();

	Particles* pParticles = pGameState->getParticles();

	if (pParticles != NULL)
	{
		setActive(false, false);
		pParticles->worldPos.x = worldPos.x;
		pParticles->worldPos.y = worldPos.y;
		pParticles->show(PARTICLES_SPAWN_PLAYER);
	}
}
