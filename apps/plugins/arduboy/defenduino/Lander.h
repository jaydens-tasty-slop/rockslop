#pragma once

#include "Enemy.h"
#include "Landscape.h"
#include "PlayerShot.h"
#include "PlayerShip.h"

class Lander : public Enemy
{
public:
	Lander();
	void update(Landscape* pLandscape, PlayerShip* pPlayerShip);
	bool render(Vector2Int cameraPos);
	void setActive(bool active);
	void collisionCheck(PlayerShot* playerShots, PlayerShip* pPlayerShip);
	void startSeeking();
	bool isMutant();
	void destroy();
	void becomeMutant();

private:
	void seekingUpdate(Landscape* pLandscape, PlayerShip* pPlayerShip);
	void landingUpdate(PlayerShip* pPlayerShip);
	void escapingUpdate(PlayerShip* pPlayerShip);
	void mutantUpdate(PlayerShip* pPlayerShip);

	uint8_t humanoid;
};