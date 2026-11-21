#pragma once

#include "Enemy.h"
#include "PlayerShip.h"

class Baiter : public Enemy
{
public:
	void update(PlayerShip* pPlayerShip);
	bool render(Vector2Int cameraPos);
	void onSpawn(Vector2Int position, PlayerShip* pPlayerShip);
	void collisionCheck(PlayerShot* playerShots, PlayerShip* pPlayerShip);
	void destroy();

private:
	void setVelocity(PlayerShip* pPlayerShip);
	Vector2Int baseVelocity;
};