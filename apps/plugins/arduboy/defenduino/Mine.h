#pragma once

#include "GameObject.h"
#include "PlayerShip.h"

class Mine : public GameObject
{
public:
	bool render(Vector2Int cameraPos);
	void collisionCheck(PlayerShip* pPlayerShip);

private:
	Rect getCollisionRect();
};