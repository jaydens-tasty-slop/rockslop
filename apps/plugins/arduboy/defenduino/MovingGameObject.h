#pragma once

#include "GameObject.h"

class MovingGameObject : public GameObject
{
public:
	void update();
	Vector2Int velocity;
};