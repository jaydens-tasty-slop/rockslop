#pragma once
#include "../Direction.h"
#include "../Point2.h"
#include "../LevelData.h"

namespace EasterPanic
{
	namespace Level_002
	{
		constexpr const uint8_t MaxActions = 8;
		constexpr const uint8_t PlayerX = 3;
		constexpr const uint8_t PlayerY = 3;
		constexpr const Direction PlayerDirection = Direction::North;

		const uint8_t TileData[] PROGMEM =
		{
			0xFE, // 11111110
			0x82, // 10000010
			0xAA, // 10101010
			0x82, // 10000010
			0xAA, // 10101010
			0x82, // 10000010
			0xFE, // 11111110
			0x00, // 00000000
		};

		const PointU8 Collectables[] PROGMEM =
		{
			PointU8(1, 1),
			PointU8(3, 1),
			PointU8(5, 1),
			PointU8(1, 3),
			PointU8(5, 3),
			PointU8(1, 5),
			PointU8(3, 5),
			PointU8(5, 5),
		};

		constexpr const LevelData Data = LevelData(MaxActions, Entity(PlayerX, PlayerY, PlayerDirection), TileData, LevelCollectableData(Collectables));
	}
}