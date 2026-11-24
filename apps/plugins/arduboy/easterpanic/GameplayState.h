#pragma once

#include "Game.h"
#include "Entity.h"
#include "Point2.h"
#include "Action.h"
#include "Array.h"
#include "List.h"
#include "Stack.h"
#include "ListOption.h"

class GameplayState : public Game::State
{
public:
	using TileGrid = typename Context::TileGrid;

private:
	enum class StateType
	{
		Options,
		Quit,
		EditingActionList,
		EditingCurrentAction,
		RunningActions,
		Error,
		Success,
	};
	
	class ForLoopState
	{
	private:
		uint8_t value;
		uint8_t limit;
		uint8_t actionIndex;
	public:
		
		ForLoopState(void) = default;
		
		ForLoopState(uint8_t limit, uint8_t actionIndex)
			: value(0), limit(limit), actionIndex(actionIndex)
		{
		}
		
		uint8_t getValue(void) const
		{
			return this->value;
		}
		
		uint8_t getLimit(void) const
		{
			return this->limit;
		}
		
		uint8_t getActionIndex(void) const
		{
			return this->actionIndex;
		}
		
		bool next(void)
		{
			auto nextValue = value + 1;
			if(nextValue >= limit)
				return false;
			value = nextValue;
			return true;
		}
	};
	
	enum class ErrorType : uint8_t
	{
		None,
		UnrecognisedError, // How meta
		UnrecognisedInstruction,
		TooManyForStarts,
		TooManyForEnds,
		TooManyNestedForLoops,
		InvalidPlayerDirection,
	};
	
	const char * getErrorStringLine0(ErrorType type);
	const char * getErrorStringLine1(ErrorType type);

	class MenuOption
	{
	public:
		StateType state;
		uint8_t imageIndex;
		const char * text;
		
	public:
		MenuOption(void) = default;
		constexpr MenuOption(const StateType & state, const uint8_t & imageIndex, const char * text)
			: state(state), imageIndex(imageIndex), text(text)
		{
		}
	};
	
private:
	TileGrid tiles;
	Entity player;
	List<PointU8, 16> collectables;
	
	Stack<ForLoopState, 8> forStack;	
	List<Action, 21> actions;
	uint8_t selectedAction;	
	uint8_t nextAction;
	
	bool editingArgument;
	
	StateType state;
	ErrorType error;
	
	uint8_t selectedOption = 1;	
	static const MenuOption menuOptions[];	

public:
	void activate(StateMachine & machine) override;

	void update(StateMachine & machine) override;
	void render(StateMachine & machine) override;
	
private:
	void loadLevel(uint8_t index);
	void reportError(ErrorType error);
	void checkForCollectableCollision(void);
	
	void renderGrid(StateMachine & machine);
	void renderEntity(StateMachine & machine);
	void renderCollectables(StateMachine & machine);
	void renderPlayfield(StateMachine & machine);
	
public:
	void updateEditingActionList(StateMachine & machine);
	void renderEditingActionList(StateMachine & machine);
	
	void updateEditingCurrentAction(StateMachine & machine);
	void renderEditingCurrentAction(StateMachine & machine);
	
	void updateRunningActions(StateMachine & machine);
	void renderRunningActions(StateMachine & machine);
	
	void updateError(StateMachine & machine);
	void renderError(StateMachine & machine);
	
	void updateOptions(StateMachine & machine);
	void renderOptions(StateMachine & machine);
	
	void updateSuccess(StateMachine & machine);
	void renderSuccess(StateMachine & machine);
};