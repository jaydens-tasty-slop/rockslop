#pragma once

//
// countDigitsConst
//

constexpr inline uint8_t countDigitsConst(const uint8_t & value)
{
	return (value < 10) ? 1 : (value < 100) ? 2 : 3;
}

constexpr inline uint8_t countDigitsConst(const uint16_t & value)
{
	return (value < 10) ? 1 : (value < 100) ? 2 : (value < 1000) ? 3 :  (value < 10000) ? 4 : 5;
}

//
// countDigits
//

inline uint8_t countDigits(const uint8_t & value)
{
	constexpr const uint8_t base = 10;
	constexpr const uint8_t minDigits = 1;
	constexpr const uint8_t maxDigits = 3;
	{
		uint8_t limit = base;
		for(uint8_t i = minDigits; i <= maxDigits; ++i, limit *= base)
			if(value < limit)
				return i;
	}
	return maxDigits;
}

inline uint8_t countDigits(const uint16_t & value)
{
	constexpr const uint8_t base = 10;
	constexpr const uint8_t minDigits = 1;
	constexpr const uint8_t maxDigits = 5;
	{
		uint16_t limit = base;
		for(uint8_t i = minDigits; i <= maxDigits; ++i, limit *= base)
			if(value < limit)
				return i;
	}
	return maxDigits;
}

inline uint8_t countDigits(const uint32_t & value)
{
	constexpr const uint8_t base = 10;
	constexpr const uint8_t minDigits = 1;
	constexpr const uint8_t maxDigits = 10;
	{
		uint32_t limit = base;
		for(uint8_t i = minDigits; i <= maxDigits; ++i, limit *= base)
			if(value < limit)
				return i;
	}
	return maxDigits;
}

//
// extractDigits
//

template< size_t size >
void extractDigits(uint8_t (&buffer)[size], uint8_t value)
{
	static_assert(size <= 255, "extractDigits only supports arrays up to 255 bytes in length");
	for(uint8_t i = 0; i < size; ++i)
	{
		buffer[i] = value % 10;
		value /= 10;
	}
}

template< size_t size >
void extractDigits(uint8_t (&buffer)[size], uint16_t value)
{
	static_assert(size <= 255, "extractDigits only supports arrays up to 255 bytes in length");
	for(uint8_t i = 0; i < size; ++i)
	{
		buffer[i] = value % 10;
		value /= 10;
	}
}

template< size_t size >
void extractDigits(uint8_t (&buffer)[size], uint32_t value)
{
	static_assert(size <= 255, "extractDigits only supports arrays up to 255 bytes in length");
	for(uint8_t i = 0; i < size; ++i)
	{
		buffer[i] = value % 10;
		value /= 10;
	}
}

template< size_t size >
void extractDigits(uint8_t (&buffer)[size], uint64_t value)
{
	static_assert(size <= 255, "extractDigits only supports arrays up to 255 bytes in length");
	for(uint8_t i = 0; i < size; ++i)
	{
		buffer[i] = value % 10;
		value /= 10;
	}
}