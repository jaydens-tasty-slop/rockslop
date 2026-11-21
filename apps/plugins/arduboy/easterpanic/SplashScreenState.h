#pragma once

#include "Game.h"

class SplashScreenState : public Game::State
{
private:
	constexpr static const uint8_t BlinkDelayDefault = 2;
	constexpr static const uint8_t BlinkDelayVariance = 5;
	constexpr static const uint8_t BlinkTickMin = 0;
	constexpr static const uint8_t BlinkTickMax = 7;

private:
	bool blinkInvert;
	uint8_t blinkTick;
	uint8_t blinkDelay = BlinkDelayDefault;
	
public:
	void update(StateMachine & machine) override;
	void render(StateMachine & machine) override;
};