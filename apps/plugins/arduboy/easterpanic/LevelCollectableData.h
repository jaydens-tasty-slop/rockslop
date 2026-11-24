#pragma once

#include "Point2.h"

class LevelCollectableData
{
public:
	uint8_t collectableCount = 0;
	const PointU8 * collectables = nullptr;
	
public:
	constexpr LevelCollectableData(void) = default;
	
	constexpr LevelCollectableData(uint8_t collectableCount, const PointU8 * collectables)
		: collectableCount(collectableCount), collectables(collectables)
	{
	}

	constexpr LevelCollectableData(const PointU8 (&)[0])
		: collectableCount(0), collectables(nullptr)
	{
	}
		
	template< size_t size >
	constexpr LevelCollectableData(const PointU8 (&collectables)[size])
		: collectableCount(size), collectables(collectables)
	{
	}
};