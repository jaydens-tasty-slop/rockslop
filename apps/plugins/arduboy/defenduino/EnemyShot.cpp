#include "EnemyShot.h"
#include "Globals.h"

const unsigned char shotSprite[] PROGMEM = { 0x3, 0x8, 0x2, 0x7, 0x2, };

#define SHOT_VELOCITY 1

#define FLAG_CHASER 0x4

#define COLLISION_HEIGHT 3


EnemyShot::EnemyShot() : MovingGameObject()
{
}

void EnemyShot::update(PlayerShip* pPlayerShip)
{
	MovingGameObject::update();
	
	if (isFlagSet (flags, FLAG_CHASER))
	{
		velocity.x = baseVelX + pPlayerShip->velocity.x;
	}
}

bool EnemyShot::render(Vector2Int screenPos)
{
	bool isVisible = renderSpriteIfVisible(shotSprite, &flags, screenPos);
	
	if(!isVisible)
	{
		// Turn it off if it goes off the screen
		unsetFlag(&flags, FLAG_ACTIVE);
	}
	return isVisible;
}

void EnemyShot::fire(PlayerShip* pPlayerShip, Vector2Int startPos)
{
	worldPos = startPos;

	Vector2Int dir = pPlayerShip->worldPos - startPos;
	Vector2Int normalized = dir.normalize();

	velocity = Vector2Int(SHOT_VELOCITY * (normalized.x), SHOT_VELOCITY * (normalized.y));

	// Now add some randomness
	int8_t miss = (rand() % 20) - 10;

	velocity.x += miss;
	velocity.y -= miss;

	setFlag(&flags, FLAG_ACTIVE);

	baseVelX = velocity.x;

	// 20% chance this will be a chaser
	setFlag(&flags, FLAG_CHASER, (rand() % 5) == 0);
	
}

void EnemyShot::collisionCheck(PlayerShip* pPlayerShip)
{
	Rect thisRect = getCollisionRect();
	if (pPlayerShip->isActive() and !pPlayerShip->isExploding() && arduboy.collide(pPlayerShip->getCollisionRect(), thisRect))
	{
		unsetFlag(&flags, FLAG_ACTIVE);
		pPlayerShip->destroy();
	}
}

Rect EnemyShot::getCollisionRect()
{
	return Rect(worldPos.getPixelX(), worldPos.getPixelY(), 3, 3);
}