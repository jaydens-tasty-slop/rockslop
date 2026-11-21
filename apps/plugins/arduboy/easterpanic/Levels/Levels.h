#include "../LevelData.h"

#include "Level_000.h"
#include "Level_001.h"
#include "Level_002.h"
#include "Level_003.h"

const LevelData Levels[] PROGMEM =
{
	EasterPanic::Level_000::Data,
	EasterPanic::Level_001::Data,
	EasterPanic::Level_002::Data,
	EasterPanic::Level_003::Data,
};