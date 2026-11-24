#pragma once

#include "Game.h"

class LevelSelectState : public Game::State
{
private:
	uint8_t index;
	uint8_t pageIndex;

public:
	void update(StateMachine & machine) override;
	void render(StateMachine & machine) override;	
};