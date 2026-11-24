#include "Game.h"

#include "SplashScreenState.h"
#include "TitleScreenState.h"
#include "LevelSelectState.h"
#include "GameplayState.h"

void Game::setup(void)
{
	auto & arduboy = this->context.arduboy;

	this->currentStateId = GameStateId::SplashScreen;
	this->currentState = this->createState(this->currentStateId);
	this->nextStateId = this->currentStateId;

	arduboy.boot();
	arduboy.safeMode();
	arduboy.initRandomSeed();

	this->currentState->activate(*this);
}

void Game::loop(void)
{
	auto & arduboy = this->context.arduboy;

	if(!arduboy.nextFrame())
		return;

	arduboy.pollButtons();
	
	if (this->currentState != nullptr)
		this->currentState->update(*this);
		
	arduboy.clear();
	
	if (this->currentState != nullptr)
		this->currentState->render(*this);
		
	arduboy.display();
	
	if(this->currentStateId != this->nextStateId)
	{
		if (this->currentState != nullptr)
		{
			this->currentState->deactivate(*this);
			delete this->currentState;
		}

		this->currentState = this->createState(this->nextStateId);
		this->currentStateId = this->nextStateId;
		this->currentState->activate(*this);
	}
}

Game::Context & Game::getContext(void)
{
	return this->context;
}

const Game::Context & Game::getContext(void) const
{
	return this->context;
}

void Game::changeState(const StateId & stateId)
{
	this->nextStateId = stateId;
}

Game::State * Game::createState(const StateId & stateId) const
{
	switch (stateId)
	{
		case GameStateId::SplashScreen: return new SplashScreenState();
		case GameStateId::TitleScreen: return new TitleScreenState();
		case GameStateId::LevelSelect: return new LevelSelectState();
		case GameStateId::Gameplay: return new GameplayState();
		default: return nullptr;
	}
}