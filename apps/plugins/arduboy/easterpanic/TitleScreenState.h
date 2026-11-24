#pragma once

#include "Game.h"
#include "ListOption.h"

class TitleScreenState : public Game::State
{
private:
	using Option = ListOption<StateId>;

	static const Option options[];

private:
	uint8_t option = 0;
	
public:
	void update(StateMachine & machine) override;
	void render(StateMachine & machine) override;
};