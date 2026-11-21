#pragma once

#include "MovingGameObject.h"
#include "PlayerShip.h"
#include "PlayerShot.h"

class Enemy : public MovingGameObject
{

protected:
	void destroy(uint16_t score, boolean isCounted);
	bool collisionCheck(uint8_t width, uint8_t height, PlayerShot* pPlayerShots, PlayerShip* pPlayerShip);
};