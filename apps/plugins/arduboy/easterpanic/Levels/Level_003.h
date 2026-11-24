#pragma once
#include "../Direction.h"
#include "../Point2.h"
#include "../LevelData.h"

namespace EasterPanic
{
	namespace Level_003
	{
		constexpr const uint8_t MaxActions = 12;
		constexpr const uint8_t PlayerX = 2;
		constexpr const uint8_t PlayerY = 2;
		constexpr const Direction PlayerDirection = Direction::West;

		const uint8_t TileData[] PROGMEM =
		{
			0x7E, // 01111110
			0xC3, // 11000011
			0x81, // 10000001
			0x81, // 10000001
			0x81, // 10000001
			0x81, // 10000001
			0xC3, // 11000011
			0x7E, // 01111110
		};

		const PointU8 Collectables[] PROGMEM =
		{
			PointU8(1, 2),
			PointU8(1, 5),
			PointU8(6, 2),
			PointU8(6, 5),
			PointU8(2, 1),
			PointU8(5, 1),
			PointU8(2, 6),
			PointU8(5, 6),
		};

		constexpr const LevelData Data = LevelData(MaxActions, Entity(PlayerX, PlayerY, PlayerDirection), TileData, LevelCollectableData(Collectables));
	}
}