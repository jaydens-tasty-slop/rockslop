#pragma once

#include "Enemy.h"
#include "PlayerShip.h"

class Bomber : public Enemy
{
public:
	void update(PlayerShip* pPlayerShip);
	bool render(Vector2Int cameraPos);
	void onSpawn(Vector2Int position, bool right);
	void collisionCheck(PlayerShot* playerShots, PlayerShip* pPlayerShip);
	void destroy();


private:

};