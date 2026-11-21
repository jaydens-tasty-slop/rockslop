#pragma once

#include "MovingGameObject.h"
#include "PlayerShip.h"

class GameCamera : public MovingGameObject
{
public:
	GameCamera();
	Vector2Int worldToScreenPos(Vector2Int worldPos);
	void update(PlayerShip* pPlayerShip);

private:
};