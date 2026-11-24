#pragma once

#include "GameStateMachine.h"

template< typename TContext, typename TStateId >
class GameState
{
public:
	using Context = TContext;
	using StateId = TStateId;
	using StateMachine = GameStateMachine<Context, StateId>;

public:
	virtual ~GameState(void) {};

	virtual void activate(StateMachine & machine)
	{
		// Get rid of 'unused parameter' warnings
		(void)machine;
	}
	virtual void deactivate(StateMachine & machine)
	{
		// Get rid of 'unused parameter' warnings
		(void)machine;
	}
	
	virtual void update(StateMachine & machine) = 0;
	virtual void render(StateMachine & machine) = 0;
};