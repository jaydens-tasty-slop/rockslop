#pragma once

#include <Arduboy2.h>

class GameOverState
{
public:
	GameOverState(unsigned long score);
	void update();

private:
	unsigned long score;
	unsigned long highScore;
};

