#pragma once

#include "GameStateId.h"
#include "GameState.h"
#include "GameStateMachine.h"
#include "GameContext.h"

class Game : public GameStateMachine<GameContext, GameStateId>
{
private:
	Context context;
	State * currentState;
	StateId currentStateId;
	StateId nextStateId;
	
public:
	void setup(void);
	void loop(void);

	Context & getContext(void) override;
	const Context & getContext(void) const override;

	void changeState(const StateId & stateId) override;

private:
	State * createState(const StateId & stateId) const;

};