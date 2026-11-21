#pragma once

#include "Entity.h"
#include "LevelCollectableData.h"

class LevelData
{
public:
	uint8_t maxActions = 0;
	Entity playerStart = Entity();
	uint8_t const * tileData = nullptr;
	LevelCollectableData collectableData = LevelCollectableData();

public:
	constexpr LevelData(void) = default;
	
	constexpr LevelData(uint8_t maxActions, Entity playerStart, uint8_t const * tileData, LevelCollectableData collectableData)
		: maxActions(maxActions), playerStart(playerStart), tileData(tileData), collectableData(collectableData)
	{
	}
};