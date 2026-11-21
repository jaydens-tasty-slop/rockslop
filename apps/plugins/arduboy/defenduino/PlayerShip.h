#pragma once
#include "MovingGameObject.h"

class PlayerShip : public MovingGameObject
{
public:
	PlayerShip();
	void update();
	void render(Vector2Int cameraPos);
	int getCameraTarget();
	void destroy();
	Rect getCollisionRect();
	byte explosionTimer = 0;
	void setActive(bool active, bool forceFaceRight);
	bool isExploding();
	bool facingRight();
	void cancelExplosion();

private:
	void fire();
	void activeUpdate();
	void explodingUpdate();
	void explode();
	void hyperspace();
	void renderThruster(Vector2Int shipScreenPos);
};