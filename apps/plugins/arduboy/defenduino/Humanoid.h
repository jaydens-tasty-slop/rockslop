#pragma once
#include "MovingGameObject.h"
#include "Landscape.h"
#include "PlayerShot.h"
#include "PlayerShip.h"

class Humanoid : public MovingGameObject
{
public:
	Humanoid();
	void update(Landscape* pLandscape, PlayerShip* pPlayerShip);
	bool render(Vector2Int cameraPos);
	void setActive(bool active);
	void collisionCheck(PlayerShot* playerShots, PlayerShip* pPlayerShip);
	bool isCapturable();
	bool isCarried();
	void setCaptured(bool captured);
	void destroy();
	void startFalling();
	void startWalking();


private:
	Rect getCollisionRect();
	void fallingUpdate(Landscape* pLandscape);
	void caughtUpdate(Landscape* pLandscape, PlayerShip* pPlayerShip);
	bool sameXAsAnotherHuman();
};