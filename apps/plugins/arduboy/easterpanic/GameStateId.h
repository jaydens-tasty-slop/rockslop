#pragma once

#include <stdint.h>

enum class GameStateId : uint8_t
{
	None,
	SplashScreen,
	TitleScreen,
	LevelSelect,
	Gameplay
};