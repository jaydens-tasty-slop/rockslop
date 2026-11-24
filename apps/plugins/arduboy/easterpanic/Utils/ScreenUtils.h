#pragma once

#include "Arduboy2PrettificationUtils.h"
#include "Type.h"

constexpr const static uint8_t ScreenWidth = Arduboy::ScreenWidth;
constexpr const static uint8_t ScreenHeight = Arduboy::ScreenHeight;
constexpr const static uint8_t HalfScreenWidth = (ScreenWidth / 2);
constexpr const static uint8_t HalfScreenHeight = (ScreenHeight / 2);

template< typename Type >
constexpr Type intRound(const Type & value)
{
	return ((value % 2 == 0) ? value : value + 1);
}

template< typename Type >
constexpr Type div2Round(const Type & value)
{
	return intRound(value) / 2;
}

template< typename Type >
constexpr Type CalculateCentre(const Type & maxDimension, const Type & dimension)
{
	return div2Round(maxDimension) - div2Round(dimension);
}

template< typename T, typename U >
constexpr LargerType<T, U> CalculateCentre(const T & maxDimension, const U & dimension)
{
	using Result = LargerType<T, U>;
	return CalculateCentre<Result>(static_cast<Result>(maxDimension), static_cast<Result>(dimension));
}

constexpr int16_t CalculateCentreX(const uint8_t & width)
{
	return div2Round(ScreenWidth) - div2Round(width);
}

constexpr int16_t CalculateCentreY(const uint8_t & height)
{
	return div2Round(ScreenHeight) - div2Round(height);
}

constexpr int16_t CalculateCentreX(const uint8_t & x, const uint8_t & maxWidth, const uint8_t & width)
{
	return (x + div2Round(maxWidth - x)) - div2Round(width);
}

constexpr int16_t CalculateCentreY(const uint8_t & y, const uint8_t & maxHeight, const uint8_t & height)
{
	return (y + div2Round(maxHeight - y)) - div2Round(height);
}