#pragma once

#include "ArrayUtils.h"
#include <avr/pgmspace.h>

constexpr const static uint8_t FontCharWidth = 5;
constexpr const static uint8_t FontCharHeight = 7;
constexpr const static uint8_t FontSpaceWidth = 1;
constexpr const static uint8_t FontSpaceHeight = 1;
constexpr const static uint8_t FontLineHeight = FontCharHeight + FontSpaceHeight;

constexpr inline uint8_t StringWidth(const uint8_t & charCount)
{
	return (charCount * FontCharWidth) + ((charCount - 1) * FontSpaceWidth);
}

constexpr inline size_t StringWidth(const size_t & charCount)
{
	return (charCount * FontCharWidth) + ((charCount - 1) * FontSpaceWidth);
}

constexpr inline size_t StringWidth(const int & charCount)
{
	return StringWidth(static_cast<size_t>(charCount));
}

constexpr inline uint8_t StringHeight(const uint8_t & lineCount)
{
	return (lineCount * FontCharHeight) + ((lineCount - 1) * FontSpaceHeight);
}

constexpr inline size_t StringHeight(const size_t & lineCount)
{
	return (lineCount * FontCharHeight) + ((lineCount - 1) * FontSpaceHeight);
}

constexpr inline size_t StringHeight(const int & lineCount)
{
	return StringHeight(static_cast<size_t>(lineCount));
}

template< typename T, size_t N > constexpr size_t StringWidth(T (&string)[N])
{
	return StringWidth(StringLength(string));
}

inline size_t ProgmemStringWidth(const char * string)
{
	return StringWidth(strlen_P(string));
}

inline size_t RuntimeStringWidth(const char * string)
{
	return StringWidth(strlen(string));
}