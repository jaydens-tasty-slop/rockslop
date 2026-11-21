#pragma once

#include "GameObject.h"

#define PARTICLES_EXPLOSION 0
#define PARTICLES_SPAWN 1
#define PARTICLES_SPAWN_BAITER 2
#define PARTICLES_PLAYER 3
#define PARTICLES_SPAWN_PLAYER 4


class Particles : public GameObject
{
public:
	void show(byte type);
	void update();
	void render(Vector2Int cameraPos);

private:
	byte distance;
};