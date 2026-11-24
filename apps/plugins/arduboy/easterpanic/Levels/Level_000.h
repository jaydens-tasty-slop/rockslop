#pragma once
#include "../Direction.h"
#include "../Point2.h"
#include "../LevelData.h"

namespace EasterPanic
{
	namespace Level_000
	{
		constexpr const uint8_t MaxActions = 4;
		constexpr const uint8_t PlayerX = 4;
		constexpr const uint8_t PlayerY = 4;
		constexpr const Direction PlayerDirection = Direction::North;

		const uint8_t TileData[] PROGMEM =
		{
			0x00, // 00000000
			0x00, // 00000000
			0x3C, // 00111100
			0x2C, // 00101100
			0x24, // 00100100
			0x3C, // 00111100
			0x00, // 00000000
			0x00, // 00000000
		};

		const PointU8 Collectables[] PROGMEM =
		{
			PointU8(3, 3),
		};

		constexpr const LevelData Data = LevelData(MaxActions, Entity(PlayerX, PlayerY, PlayerDirection), TileData, LevelCollectableData(Collectables));
	}
}