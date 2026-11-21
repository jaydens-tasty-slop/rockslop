#pragma once

#include "MovingGameObject.h"
#include "PlayerShip.h"

class EnemyShot : public MovingGameObject
{
public:
	EnemyShot();
	bool render(Vector2Int screenPos);
	void fire(PlayerShip* pPlayerShip, Vector2Int startPos);
	void collisionCheck(PlayerShip* pPlayerShip);
	void update(PlayerShip* pPlayerShip);

private:
	Rect getCollisionRect();
	uint16_t baseVelX;
};