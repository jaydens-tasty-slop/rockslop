#pragma once
#include "../Direction.h"
#include "../Point2.h"
#include "../LevelData.h"

namespace EasterPanic
{
	namespace Level_001
	{
		constexpr const uint8_t MaxActions = 8;
		constexpr const uint8_t PlayerX = 2;
		constexpr const uint8_t PlayerY = 2;
		constexpr const Direction PlayerDirection = Direction::East;

		const uint8_t TileData[] PROGMEM =
		{
			0x00, // 00000000
			0x7E, // 01111110
			0x42, // 01000010
			0x5A, // 01011010
			0x5A, // 01011010
			0x42, // 01000010
			0x7E, // 01111110
			0x00, // 00000000
		};

		const PointU8 Collectables[] PROGMEM =
		{
			PointU8(5, 2),
			PointU8(2, 5),
			PointU8(5, 5),
		};

		constexpr const LevelData Data = LevelData(MaxActions, Entity(PlayerX, PlayerY, PlayerDirection), TileData, LevelCollectableData(Collectables));
	}
}